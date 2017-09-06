/* nemo-desktop-manager.c */

#include <config.h>

#define DEBUG_FLAG NEMO_DEBUG_DESKTOP
#include <libnemo-private/nemo-debug.h>

#include "nemo-desktop-manager.h"
#include "nemo-blank-desktop-window.h"
#include "nemo-desktop-window.h"
#include "nemo-application.h"
#include "nemo-cinnamon-dbus.h"

#include <gdk/gdkx.h>

#include <libnemo-private/nemo-global-preferences.h>
#include <libnemo-private/nemo-desktop-utils.h>

static gboolean layout_changed (NemoDesktopManager *manager);

#define DESKTOPS_ON_PRIMARY "true::false"
#define DESKTOPS_ON_ALL "true::true"
#define DESKTOPS_ON_NON_PRIMARY "false::true"
#define DESKTOPS_ON_NONE "false::false"
#define DESKTOPS_DEFAULT DESKTOPS_ON_PRIMARY
#define PRIMARY_MONITOR 0

typedef enum {
    RUN_STATE_INIT = 0,
    RUN_STATE_STARTUP,
    RUN_STATE_RUNNING,
    RUN_STATE_FALLBACK
} RunState;

typedef struct {
    GdkScreen *screen;
    GdkWindow *root_window;

    gulong scale_factor_changed_id;
    guint update_layout_idle_id;

    gboolean desktop_on_primary_only;

    NemoActionManager *action_manager;

    GList *desktops;

    NemoCinnamonDBus *proxy;

    guint other_desktop : 1;
    guint proxy_owned : 1;
    RunState current_run_state;

    gulong name_owner_changed_id;
    gulong proxy_signals_id;
} NemoDesktopManagerPrivate;

struct _NemoDesktopManager
{
    GtkWindow parent_object;

    NemoDesktopManagerPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE (NemoDesktopManager, nemo_desktop_manager, G_TYPE_OBJECT);

typedef struct {
    GtkWidget *window;

    gint monitor_num;
    gboolean shows_desktop;
    gboolean is_primary;
} DesktopInfo;

static const gchar *
run_state_str (RunState state)
{
    switch (state) {
        case RUN_STATE_INIT:
            return "RunState.INIT";
        case RUN_STATE_STARTUP:
            return "RunState.STARTUP";
        case RUN_STATE_RUNNING:
            return "RunState.RUNNING";
        case RUN_STATE_FALLBACK:
            return "RunState.DEFAULT";
        default:
            g_assert_not_reached ();
    }
}

static void
free_info (DesktopInfo *info)
{
    g_return_if_fail (info != NULL);

    g_clear_pointer (&info->window, gtk_widget_destroy);
    g_slice_free (DesktopInfo, info);
}

static RunState
get_run_state (NemoDesktopManager *manager)
{
    NemoDesktopManagerPrivate *priv;
    gint ret;

    priv = manager->priv;

    if (priv->other_desktop) {
        ret = RUN_STATE_FALLBACK;
        goto out;
    }

    if (priv->proxy == NULL || !priv->proxy_owned) {
        ret = RUN_STATE_INIT;
        goto out;
    }

    if (!nemo_cinnamon_dbus_call_get_run_state_sync (priv->proxy,
                                                     &ret,
                                                     NULL,
                                                     NULL)) {
        ret = RUN_STATE_FALLBACK;
        goto out;
    }

out:
    DEBUG ("Run state is %s\n", run_state_str (ret));

    return (RunState) ret;
}

static gint
get_n_monitors (NemoDesktopManager *manager)
{
    NemoDesktopManagerPrivate *priv;
    gsize n_monitors;
    gint *indices;
    GVariant *monitors;

    priv = manager->priv;

    if (priv->other_desktop) {
        return nemo_desktop_utils_get_num_monitors ();
    }

    if (!nemo_cinnamon_dbus_call_get_monitors_sync (priv->proxy,
                                                    &monitors,
                                                    NULL,
                                                    NULL)) {
        return nemo_desktop_utils_get_num_monitors ();
    }

    indices = (gint *) g_variant_get_fixed_array (monitors, &n_monitors, sizeof(gint));
    g_variant_unref (monitors);

    return n_monitors;
}

static void
get_window_rect_for_monitor (NemoDesktopManager *manager,
                             gint                monitor,
                             GdkRectangle       *rect)
{
    NemoDesktopManagerPrivate *priv;
    GVariant *out_rect_var;
    GdkRectangle *out_rect;
    gsize n_elem;

    priv = manager->priv;

    if (priv->other_desktop) {
        nemo_desktop_utils_get_monitor_geometry (monitor, rect);
        return;
    }

    if (!nemo_cinnamon_dbus_call_get_monitor_work_rect_sync (priv->proxy,
                                                             monitor,
                                                             &out_rect_var,
                                                             NULL,
                                                             NULL)) {
        nemo_desktop_utils_get_monitor_geometry (monitor, rect);
        return;
    }

    out_rect = (GdkRectangle *) g_variant_get_fixed_array (out_rect_var, &n_elem, sizeof(gint));

    rect->x = out_rect->x;
    rect->y = out_rect->y;
    rect->width = out_rect->width;
    rect->height = out_rect->height;

    g_variant_unref (out_rect_var);
}

static void
close_all_windows (NemoDesktopManager *manager)
{
    g_list_foreach (manager->priv->desktops, (GFunc) free_info, NULL);
    g_clear_pointer (&manager->priv->desktops, g_list_free);
}

static void
queue_update_layout (NemoDesktopManager *manager)
{
    if (manager->priv->update_layout_idle_id > 0) {
        g_source_remove (manager->priv->update_layout_idle_id);
        manager->priv->update_layout_idle_id = 0;
    }

    manager->priv->update_layout_idle_id = g_idle_add ((GSourceFunc) layout_changed, manager);
}

static void
on_window_scale_changed (GtkWidget          *window,
                         GParamSpec         *pspec,
                         NemoDesktopManager *manager)
{
    manager->priv->scale_factor_changed_id = 0;

    queue_update_layout (manager);
}

static void
create_new_desktop_window (NemoDesktopManager *manager,
                                         gint  monitor,
                                     gboolean  primary,
                                     gboolean  show_desktop)
{
    GtkWidget *window;

    DesktopInfo *info = g_slice_new0 (DesktopInfo);

    info->monitor_num = monitor;
    info->shows_desktop = show_desktop;
    info->is_primary = primary;

    if (show_desktop) {
        window = GTK_WIDGET (nemo_desktop_window_new (monitor));
    } else {
        window = GTK_WIDGET (nemo_blank_desktop_window_new (monitor));
    }

    info->window = window;

    /* We realize it immediately so that the NEMO_DESKTOP_WINDOW_ID
       property is set so gnome-settings-daemon doesn't try to set the
       background. And we do a gdk_flush() to be sure X gets it. */

    gtk_widget_realize (GTK_WIDGET (window));
    gdk_flush ();

    if (manager->priv->scale_factor_changed_id == 0) {
        manager->priv->scale_factor_changed_id = g_signal_connect (window,
                                                                   "notify::scale-factor",
                                                                   G_CALLBACK (on_window_scale_changed),
                                                                   manager);
    }

    gtk_application_add_window (GTK_APPLICATION (nemo_application_get_singleton ()),
                                GTK_WINDOW (window));

    manager->priv->desktops = g_list_append (manager->priv->desktops, info);
}

static gboolean
layout_changed (NemoDesktopManager *manager)
{
    gint n_monitors = 0;
    gint x_primary = 0;
    gboolean show_desktop_on_primary = FALSE;
    gboolean show_desktop_on_remaining = FALSE;

    manager->priv->update_layout_idle_id = 0;

    close_all_windows (manager);

    gchar *pref = g_settings_get_string (nemo_desktop_preferences, NEMO_PREFERENCES_DESKTOP_LAYOUT);

    if (g_strcmp0 (pref, "") == 0) {
        g_settings_set_string (nemo_desktop_preferences, NEMO_PREFERENCES_DESKTOP_LAYOUT, DESKTOPS_DEFAULT);
        g_free (pref);
        layout_changed (manager);
        return FALSE;
    }

    gchar **pref_split = g_strsplit (pref, "::", 2);

    if (g_strv_length (pref_split) != 2) {
        g_settings_set_string (nemo_desktop_preferences, NEMO_PREFERENCES_DESKTOP_LAYOUT, DESKTOPS_DEFAULT);
        g_free (pref);
        g_strfreev (pref_split);
        layout_changed (manager);
        return FALSE;;
    }

    n_monitors = get_n_monitors (manager);
    x_primary = 0; /* always */

    show_desktop_on_primary = g_strcmp0 (pref_split[0], "true") == 0;
    show_desktop_on_remaining = g_strcmp0 (pref_split[1], "true") == 0;

    manager->priv->desktop_on_primary_only = show_desktop_on_primary && !show_desktop_on_remaining;

    gint i = 0;
    gboolean primary_set = FALSE;

    for (i = 0; i < n_monitors; i++) {
        if (i == x_primary) {
            create_new_desktop_window (manager, i, show_desktop_on_primary, show_desktop_on_primary);
            primary_set = primary_set || show_desktop_on_primary;
        } else if (!nemo_desktop_utils_get_monitor_cloned (i, x_primary)) {
            gboolean set_layout_primary = !primary_set && !show_desktop_on_primary && show_desktop_on_remaining;
            create_new_desktop_window (manager, i, set_layout_primary, show_desktop_on_remaining);
            primary_set = primary_set || set_layout_primary;
        }
    }

    g_free (pref);
    g_strfreev (pref_split);

    return FALSE;
}


// static GdkFilterReturn
// gdk_filter_func (GdkXEvent *gdk_xevent,
//                  GdkEvent  *event,
//                  gpointer   data)
// {
//     XEvent *xevent = gdk_xevent;
//     NemoDesktopIconGridView *icon_view;

//     icon_view = NEMO_DESKTOP_ICON_GRID_VIEW (data);

//     switch (xevent->type) {
//         case PropertyNotify:
//             if (xevent->xproperty.atom == gdk_x11_get_xatom_by_name ("_NET_WORKAREA")) {
//                 update_margins (icon_view);
//             }
//             break;
//         default:
//             break;
//     }

//     return GDK_FILTER_CONTINUE;
// }

static void
on_bus_name_owner_changed (NemoDesktopManager *manager)
{
    gchar *name_owner;

    g_return_if_fail (manager->priv->proxy != NULL);

    name_owner = g_dbus_proxy_get_name_owner (G_DBUS_PROXY (manager->priv->proxy));

    manager->priv->proxy_owned = name_owner != NULL;

    DEBUG ("New name owner: %s", name_owner ? name_owner : "unowned");

    g_free (name_owner);
}

static void
on_run_state_changed (NemoDesktopManager *manager)
{
    RunState new_state;

    DEBUG ("New run state...");

    new_state = get_run_state (manager);

    if (new_state != manager->priv->current_run_state) {
        manager->priv->current_run_state = new_state;

        if (new_state == RUN_STATE_RUNNING) {
            layout_changed (manager);
            g_application_release (G_APPLICATION (nemo_application_get_singleton ()));
        }
    }

}

static void
on_monitors_changed (NemoDesktopManager *manager)
{

}

static void
on_proxy_signal (GDBusProxy *proxy,
                 gchar      *sender,
                 gchar      *signal_name,
                 GVariant   *params,
                 gpointer   *user_data)
{
    g_printerr ("signal: %s\n", signal_name);
    if (g_strcmp0 (signal_name, "RunStateChanged") == 0) {
        on_run_state_changed (NEMO_DESKTOP_MANAGER (user_data));
    } 
    else
    if (g_strcmp0 (signal_name, "MonitorsChanged") == 0) {
        on_monitors_changed (NEMO_DESKTOP_MANAGER (user_data));
    }
}

static void
on_proxy_created (GObject      *source,
                  GAsyncResult *res,
                  gpointer      user_data)
{
    NemoDesktopManager *manager = NEMO_DESKTOP_MANAGER (user_data);
    NemoDesktopManagerPrivate *priv = manager->priv;

    priv->proxy = nemo_cinnamon_dbus_proxy_new_for_bus_finish (res, NULL);

    if (priv->proxy == NULL) {
        DEBUG ("Cinnamon proxy unsuccessful, applying default behavior");

        /* We should always end up with a proxy, as long as dbus itself is working.. */
        priv->other_desktop = TRUE;
        return;
    }

    DEBUG ("Cinnamon proxy established, getting owner and state");


    priv->name_owner_changed_id = g_signal_connect_swapped (priv->proxy,
                                                            "notify::g-name-owner",
                                                            G_CALLBACK (on_bus_name_owner_changed),
                                                            manager);

    priv->proxy_signals_id = g_signal_connect (priv->proxy,
                                               "g-signal",
                                               G_CALLBACK (on_proxy_signal),
                                               manager);

    on_bus_name_owner_changed (manager);
    on_run_state_changed (manager);
}


static void
nemo_desktop_manager_dispose (GObject *object)
{
    NemoDesktopManager *manager = NEMO_DESKTOP_MANAGER (object);

    DEBUG ("Disposing NemoDesktopManager");

    close_all_windows (manager);

    g_signal_handlers_disconnect_by_func (nemo_desktop_preferences, queue_update_layout, manager);
    g_signal_handlers_disconnect_by_func (nemo_preferences, queue_update_layout, manager);

    G_OBJECT_CLASS (nemo_desktop_manager_parent_class)->dispose (object);
}

static void
nemo_desktop_manager_finalize (GObject *object)
{
    g_object_unref (NEMO_DESKTOP_MANAGER (object)->priv->action_manager);
    g_object_unref (NEMO_DESKTOP_MANAGER (object)->priv->proxy);

    DEBUG ("Finalizing NemoDesktopManager");

    G_OBJECT_CLASS (nemo_desktop_manager_parent_class)->finalize (object);
}

static void
nemo_desktop_manager_class_init (NemoDesktopManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = nemo_desktop_manager_finalize;
  object_class->dispose = nemo_desktop_manager_dispose;
}

static void
nemo_desktop_manager_init (NemoDesktopManager *manager)
{
    NemoDesktopManagerPrivate *priv;

    manager->priv = G_TYPE_INSTANCE_GET_PRIVATE (manager, NEMO_TYPE_DESKTOP_MANAGER, NemoDesktopManagerPrivate);

    DEBUG ("Desktop Manager Initialization");

    priv = manager->priv;

    priv->scale_factor_changed_id = 0;
    priv->desktops = NULL;
    priv->desktop_on_primary_only = FALSE;

    priv->action_manager = nemo_action_manager_new ();

    priv->update_layout_idle_id = 0;

    g_signal_connect_swapped (nemo_desktop_preferences, 
                              "changed::" NEMO_PREFERENCES_SHOW_DESKTOP,
                              G_CALLBACK (queue_update_layout),
                              manager);

    g_signal_connect_swapped (nemo_desktop_preferences,
                              "changed::" NEMO_PREFERENCES_DESKTOP_LAYOUT,
                              G_CALLBACK (queue_update_layout),
                              manager);

    g_signal_connect_swapped (nemo_desktop_preferences,
                              "changed::" NEMO_PREFERENCES_USE_DESKTOP_GRID,
                              G_CALLBACK (queue_update_layout),
                              manager);

    /* Monitor the preference to have the desktop */
    /* point to the Unix home folder */

    g_signal_connect_swapped (nemo_preferences,
                              "changed::" NEMO_PREFERENCES_DESKTOP_IS_HOME_DIR,
                              G_CALLBACK (queue_update_layout),
                              manager);

    g_signal_connect_swapped (nemo_preferences,
                              "changed::" NEMO_PREFERENCES_SHOW_ORPHANED_DESKTOP_ICONS,
                              G_CALLBACK (queue_update_layout),
                              manager);

    if (g_strcmp0 (g_getenv ("XDG_SESSION_DESKTOP"), "cinnamon") == 0) {
        DEBUG ("XDG_SESSION_DESKTOP is cinnamon, establishing proxy");

        g_application_hold (G_APPLICATION (nemo_application_get_singleton ()));

        nemo_cinnamon_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                              G_DBUS_PROXY_FLAGS_NONE,
                                              "org.Cinnamon",
                                              "/org/Cinnamon",
                                              NULL,
                                              (GAsyncReadyCallback) on_proxy_created,
                                              manager);
    } else {
        DEBUG ("XDG_SESSION_DESKTOP is not cinnamon, applying default behavior");

        priv->other_desktop = TRUE;
        layout_changed (manager);
    }
}

static NemoDesktopManager *_manager = NULL;

NemoDesktopManager*
nemo_desktop_manager_get (void)
{
    if (_manager == NULL) {
        _manager = g_object_new (NEMO_TYPE_DESKTOP_MANAGER, NULL);
    }

    return _manager;
}

gboolean
nemo_desktop_manager_has_desktop_windows (NemoDesktopManager *manager)
{
    GList *iter;
    gboolean ret = FALSE;

    g_return_val_if_fail (manager != NULL, FALSE);

    for (iter = manager->priv->desktops; iter != NULL; iter = iter->next) {
        DesktopInfo *info = iter->data;

        if (info->shows_desktop) {
            ret = TRUE;
            break;
        }
    }

    return ret;
}

gboolean
nemo_desktop_manager_get_monitor_is_active (NemoDesktopManager *manager,
                                                          gint  monitor)
{
    GList *iter;
    gboolean ret = FALSE;

    g_return_val_if_fail (manager != NULL, FALSE);

    for (iter = manager->priv->desktops; iter != NULL; iter = iter->next) {
        DesktopInfo *info = iter->data;

        if (info->monitor_num == monitor) {
            ret = info->shows_desktop;
            break;
        }
    }

    return ret;
}

gboolean
nemo_desktop_manager_get_monitor_is_primary (NemoDesktopManager *manager,
                                                           gint  monitor)
{
    GList *iter;
    gboolean ret = FALSE;

    g_return_val_if_fail (manager != NULL, FALSE);

    for (iter = manager->priv->desktops; iter != NULL; iter = iter->next) {
        DesktopInfo *info = iter->data;

        if (info->monitor_num == monitor) {
            ret = info->is_primary;
            break;
        }
    }

    return ret;
}

gboolean
nemo_desktop_manager_get_primary_only (NemoDesktopManager *manager)
{
    return manager->priv->desktop_on_primary_only;
}

NemoActionManager *
nemo_desktop_manager_get_action_manager (void)
{
    g_return_val_if_fail (_manager != NULL, NULL);

    return _manager->priv->action_manager;
}

void
nemo_desktop_manager_get_window_rect_for_monitor (NemoDesktopManager *manager,
                                                  gint                monitor,
                                                  GdkRectangle       *rect)
{
    g_return_if_fail (manager != NULL);

    get_window_rect_for_monitor (manager, monitor, rect);
}
