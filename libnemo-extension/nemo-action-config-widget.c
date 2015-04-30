/* nemo-action-config-widget.h */

/*  A widget that displays a list of actions to enable or disable.
 *  This is usually part of a NemoPluginManagerWidget
 */

#include <config.h>
#include "nemo-action-config-widget.h"
#include <glib.h>

G_DEFINE_TYPE (NemoActionConfigWidget, nemo_action_config_widget, NEMO_TYPE_CONFIG_BASE_WIDGET);

#define BLACKLIST_KEY "disabled-actions"

#define ACTION_FILE_GROUP "Nemo Action"
#define KEY_ACTIVE "Active"
#define KEY_NAME "Name"
#define KEY_ICON_NAME "Icon-Name"
#define KEY_STOCK_ID "Stock-Id"

typedef struct {
    NemoActionConfigWidget *widget;

    gchar *name;
    gchar *stock_id;
    gchar *icon_name;
    gchar *filename;
} ActionProxy;

static void
action_proxy_free (ActionProxy *proxy)
{
    g_clear_pointer (&proxy->name, g_free);
    g_clear_pointer (&proxy->stock_id, g_free);
    g_clear_pointer (&proxy->icon_name, g_free);
    g_clear_pointer (&proxy->filename, g_free);
}

static GtkWidget *
get_button_for_row (GtkWidget *row)
{
    GtkWidget *ret;

    GtkWidget *box = gtk_bin_get_child (GTK_BIN (row));
    GList *clist = gtk_container_get_children (GTK_CONTAINER (box));

    ret = clist->data;

    g_list_free (clist);

    return ret;
}

static gboolean
on_row_activated (GtkWidget *box, GtkWidget *row, GtkWidget *widget)
{
    GtkWidget *button = get_button_for_row (row);

    gtk_button_clicked (GTK_BUTTON (button));
}

static void
on_check_toggled(GtkWidget *button, ActionProxy *proxy)
{
    gboolean enabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

    gchar **blacklist = g_settings_get_strv (proxy->widget->settings, BLACKLIST_KEY);

    GPtrArray *new_list = g_ptr_array_new ();

    int i;

    if (enabled) {
        for (i = 0; i < g_strv_length (blacklist); i++) {
            if (g_strcmp0 (blacklist[i], proxy->filename) == 0)
                continue;
            g_ptr_array_add (new_list, g_strdup (blacklist[i]));
        }
    } else {
        for (i = 0; i < g_strv_length (blacklist); i++) {
            g_ptr_array_add (new_list, g_strdup (blacklist[i]));
        }

        g_ptr_array_add (new_list, g_strdup (proxy->filename));
    }

    g_ptr_array_add (new_list, NULL);

    gchar **new_list_ptr = (char **) g_ptr_array_free (new_list, FALSE);

    g_signal_handler_block (proxy->widget->settings, proxy->widget->bl_handler);
    g_settings_set_strv (proxy->widget->settings, BLACKLIST_KEY, (const gchar * const *) new_list_ptr);
    g_signal_handler_unblock (proxy->widget->settings, proxy->widget->bl_handler);

    g_strfreev (blacklist);
    g_strfreev (new_list_ptr);
}

static ActionProxy *
make_action_proxy (const gchar *filename, const gchar *fullpath)
{
    GKeyFile *key_file = g_key_file_new();

    g_key_file_load_from_file (key_file, fullpath, G_KEY_FILE_NONE, NULL);

    if (g_key_file_has_key (key_file, ACTION_FILE_GROUP, KEY_ACTIVE, NULL)) {
        if (!g_key_file_get_boolean (key_file, ACTION_FILE_GROUP, KEY_ACTIVE, NULL)) {
            g_key_file_free (key_file);
            return NULL;
        }
    }

    ActionProxy *proxy = g_slice_new0 (ActionProxy);

    gchar *name = g_key_file_get_locale_string (key_file,
                                                ACTION_FILE_GROUP,
                                                KEY_NAME,
                                                NULL,
                                                NULL);

    if (name != NULL)
        proxy->name = g_strdup (name);

    gchar *icon_name = g_key_file_get_string (key_file,
                                              ACTION_FILE_GROUP,
                                              KEY_ICON_NAME,
                                              NULL);

    if (icon_name != NULL)
        proxy->icon_name = g_strdup (icon_name);

    gchar *stock_id = g_key_file_get_string (key_file,
                                             ACTION_FILE_GROUP,
                                             KEY_STOCK_ID,
                                             NULL);

    if (stock_id != NULL)
        proxy->stock_id = g_strdup (stock_id);

    proxy->filename = g_strdup (filename);

    g_free (name);
    g_free (icon_name);
    g_free (stock_id);
    g_key_file_free (key_file);

    return proxy;
}

static void
populate_from_directory (NemoActionConfigWidget *widget, const gchar *path)
{
    GDir *dir;

    dir = g_dir_open (path, 0, NULL);

    if (dir) {
        const char *name;

        while ((name = g_dir_read_name (dir))) {
            if (g_str_has_suffix (name, ".nemo_action")) {
                char *filename;

                filename = g_build_filename (path, name, NULL);
                ActionProxy *p = make_action_proxy (name, filename);

                if (p != NULL) {
                    p->widget = widget;
                    widget->actions = g_list_append (widget->actions, p);
                }

                g_free (filename);
            }
        }

        g_dir_close (dir);
    }
}

static gchar *
strip_accel (const gchar *input)
{
    gchar *ret = NULL;

    gchar **split = g_strsplit (input, "_", 2);

    if (g_strv_length (split) == 1)
        ret = g_strdup (split[0]);
    else if (g_strv_length (split) == 2)
        ret = g_strconcat (split[0], split[1], NULL);

    g_strfreev (split);

    return ret;
}

static void
refresh_widget (NemoActionConfigWidget *widget)
{
    if (widget->actions != NULL) {
        g_list_free_full (widget->actions, (GDestroyNotify) action_proxy_free);
        widget->actions = NULL;
    }

    nemo_config_base_widget_clear_list (NEMO_CONFIG_BASE_WIDGET (widget));

    gchar *path = NULL;

    path = g_build_filename ("/", "usr", "share", "nemo", "actions", NULL);
    populate_from_directory (widget, path);
    g_clear_pointer (&path, g_free);

    path = g_build_filename (g_get_user_data_dir (), "nemo", "actions", NULL);
    populate_from_directory (widget, path);
    g_clear_pointer (&path, g_free);

    if (widget->actions == NULL) {
        GtkWidget *empty_label = gtk_label_new (NULL);
        gchar *markup = NULL;

        markup = g_strdup_printf ("<i>%s</i>", _("No actions found"));

        gtk_label_set_markup (GTK_LABEL (empty_label), markup);
        g_free (markup);

        GtkWidget *empty_row = gtk_list_box_row_new ();
        gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (empty_row), FALSE);
        gtk_container_add (GTK_CONTAINER (empty_row), empty_label);

        gtk_widget_show_all (empty_row);
        gtk_container_add (GTK_CONTAINER (NEMO_CONFIG_BASE_WIDGET (widget)->listbox), empty_row);
    } else {
        GList *l;
        gchar **blacklist = g_settings_get_strv (widget->settings, BLACKLIST_KEY);

        for (l = widget->actions; l != NULL; l=l->next) {
            ActionProxy *proxy = l->data;

            gboolean active = TRUE;
            gint i = 0;

            for (i = 0; i < g_strv_length (blacklist); i++) {
                if (g_strcmp0 (blacklist[i], proxy->filename) == 0) {
                    active = FALSE;
                    break;
                }
            }

            GtkWidget *w;
            GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

            GtkWidget *button = gtk_check_button_new ();

            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), active);
            g_signal_connect (button, "toggled", G_CALLBACK (on_check_toggled), proxy);
            gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 2);

            w = gtk_image_new ();
            if (proxy->stock_id != NULL)
                gtk_image_set_from_stock (GTK_IMAGE (w), proxy->stock_id, GTK_ICON_SIZE_MENU);
            else if (proxy->icon_name != NULL)
                gtk_image_set_from_icon_name (GTK_IMAGE (w), proxy->icon_name, GTK_ICON_SIZE_MENU);
            gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 2);

            gchar *display_name = strip_accel (proxy->name);
            w = gtk_label_new (display_name);
            g_free (display_name);

            gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 2);

            GtkWidget *row = gtk_list_box_row_new ();
            gtk_container_add (GTK_CONTAINER (row), box);

            gtk_widget_show_all (row);
            gtk_container_add (GTK_CONTAINER (NEMO_CONFIG_BASE_WIDGET (widget)->listbox), row);
        }

        g_strfreev (blacklist);
    }

    nemo_config_base_widget_set_default_buttons_sensitive (NEMO_CONFIG_BASE_WIDGET (widget), widget->actions != NULL);
}

static void
on_settings_changed (GSettings *settings, gchar *key, gpointer user_data)
{
    NemoActionConfigWidget *w = NEMO_ACTION_CONFIG_WIDGET (user_data);

    refresh_widget (w);
}

static void
on_enable_clicked (GtkWidget *button, NemoActionConfigWidget *widget)
{
    g_settings_set_strv (widget->settings, BLACKLIST_KEY, NULL);
}

static void
on_disable_clicked (GtkWidget *button, NemoActionConfigWidget *widget)
{
    GPtrArray *new_list = g_ptr_array_new ();

    GList *l;

    for (l = widget->actions; l != NULL; l = l->next)
        g_ptr_array_add (new_list, g_strdup (((ActionProxy *) l->data)->filename));

    g_ptr_array_add (new_list, NULL);

    gchar **new_list_ptr = (char **) g_ptr_array_free (new_list, FALSE);
    g_settings_set_strv (widget->settings, BLACKLIST_KEY, (const gchar * const *) new_list_ptr);

    g_strfreev (new_list_ptr);
}

static void
nemo_action_config_widget_class_init (NemoActionConfigWidgetClass *klass)
{
}

static void
nemo_action_config_widget_init (NemoActionConfigWidget *self)
{
    self->actions = NULL;

    self->settings = g_settings_new ("org.nemo.plugins");
    self->bl_handler = g_signal_connect (self->settings, "changed::" BLACKLIST_KEY,
                                         G_CALLBACK (on_settings_changed), self);

    GtkWidget *label = nemo_config_base_widget_get_label (NEMO_CONFIG_BASE_WIDGET (self));

    gchar *title = g_strdup (_("Actions"));
    gchar *markup = g_strdup_printf ("<b>%s</b>", title);

    gtk_label_set_markup (GTK_LABEL (label), markup);

    g_free (title);
    g_free (markup);

    g_signal_connect (nemo_config_base_widget_get_enable_button (NEMO_CONFIG_BASE_WIDGET (self)), "clicked",
                                                                 G_CALLBACK (on_enable_clicked), self);

    g_signal_connect (nemo_config_base_widget_get_disable_button (NEMO_CONFIG_BASE_WIDGET (self)), "clicked",
                                                                  G_CALLBACK (on_disable_clicked), self);

    g_signal_connect (NEMO_CONFIG_BASE_WIDGET (self)->listbox, "row-activated", G_CALLBACK (on_row_activated), self);

    refresh_widget (self);
}

GtkWidget *
nemo_action_config_widget_new (void)
{
  return g_object_new (NEMO_TYPE_ACTION_CONFIG_WIDGET, NULL);
}
