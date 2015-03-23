/* nemo-script-config-widget.h */

/*  A widget that displays a list of scripts to enable or disable.
 *  This is usually part of a NemoPluginManagerWidget
 */

#include <config.h>
#include "nemo-script-config-widget.h"
#include <glib.h>

G_DEFINE_TYPE (NemoScriptConfigWidget, nemo_script_config_widget, NEMO_TYPE_CONFIG_BASE_WIDGET);

#define BLACKLIST_KEY "disabled-scripts"

typedef struct {
    NemoScriptConfigWidget *widget;

    gchar *name;
} ScriptProxy;

static void
script_proxy_free (ScriptProxy *proxy)
{
    g_clear_pointer (&proxy->name, g_free);
}

static gboolean
on_button_release (GtkWidget *evbox, GdkEventButton *event, GtkWidget *button)
{
    if (event->button == GDK_BUTTON_PRIMARY) {
        gtk_button_clicked (GTK_BUTTON (button));
    }
}

static void
on_check_toggled(GtkWidget *button, ScriptProxy *proxy)
{
    gboolean enabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

    gchar **blacklist = g_settings_get_strv (proxy->widget->settings, BLACKLIST_KEY);

    GPtrArray *new_list = g_ptr_array_new ();

    int i;

    if (enabled) {
        for (i = 0; i < g_strv_length (blacklist); i++) {
            if (g_strcmp0 (blacklist[i], proxy->name) == 0)
                continue;
            g_ptr_array_add (new_list, g_strdup (blacklist[i]));
        }
    } else {
        for (i = 0; i < g_strv_length (blacklist); i++) {
            g_ptr_array_add (new_list, g_strdup (blacklist[i]));
        }

        g_ptr_array_add (new_list, g_strdup (proxy->name));
    }

    g_ptr_array_add (new_list, NULL);

    gchar **new_list_ptr = (char **) g_ptr_array_free (new_list, FALSE);

    g_settings_set_strv (proxy->widget->settings, BLACKLIST_KEY, new_list_ptr);

    g_strfreev (blacklist);
    g_strfreev (new_list_ptr);
}

static void
populate_from_directory (NemoScriptConfigWidget *widget, const gchar *path)
{
    GDir *dir;

    dir = g_dir_open (path, 0, NULL);

    if (dir) {
        const char *name;

        while ((name = g_dir_read_name (dir))) {
            char *filename;

            filename = g_build_filename (path, name, NULL);

            if (g_file_test (filename, G_FILE_TEST_IS_DIR)) {
                populate_from_directory (widget, filename);
                g_free (filename);
                continue;
            }

            ScriptProxy *p = g_slice_new0 (ScriptProxy);
            p->name = g_strdup (name);
            p->widget = widget;

            widget->scripts = g_list_append (widget->scripts, p);
            g_free (filename);
        }

        g_dir_close (dir);
    }
}

static void
refresh_widget (NemoScriptConfigWidget *widget)
{
    if (widget->scripts != NULL) {
        g_list_free_full (widget->scripts, (GDestroyNotify) script_proxy_free);
        widget->scripts = NULL;
    }

    nemo_config_base_widget_clear_list (NEMO_CONFIG_BASE_WIDGET (widget));

    gchar *path = NULL;

    path = g_build_filename ("/", "usr", "share", "nemo", "scripts", NULL);
    populate_from_directory (widget, path);
    g_clear_pointer (&path, g_free);

    path = g_build_filename (g_get_user_data_dir (), "nemo", "scripts", NULL);
    populate_from_directory (widget, path);
    g_clear_pointer (&path, g_free);

    GList *l;
    gchar **blacklist = g_settings_get_strv (widget->settings, BLACKLIST_KEY);

    for (l = widget->scripts; l != NULL; l=l->next) {
        ScriptProxy *proxy = l->data;

        gboolean active = TRUE;
        gint i = 0;

        for (i = 0; i < g_strv_length (blacklist); i++) {
            if (g_strcmp0 (blacklist[i], proxy->name) == 0) {
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

        w = gtk_label_new (proxy->name);
        gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 2);

        GtkWidget *ebox = gtk_event_box_new ();
        gtk_event_box_set_above_child (GTK_EVENT_BOX (ebox), TRUE);
        gtk_container_add (GTK_CONTAINER (ebox), box);

        g_signal_connect (ebox, "button-release-event", G_CALLBACK (on_button_release), button);

        gtk_widget_show_all (ebox);
        gtk_container_add (GTK_CONTAINER (NEMO_CONFIG_BASE_WIDGET (widget)->listbox), ebox);
    }

    g_strfreev (blacklist);
}

static void
on_settings_changed (GSettings *settings, gchar *key, gpointer user_data)
{
    NemoScriptConfigWidget *w = NEMO_SCRIPT_CONFIG_WIDGET (user_data);

    refresh_widget (w);
}

static void
nemo_script_config_widget_class_init (NemoScriptConfigWidgetClass *klass)
{
}

static void
nemo_script_config_widget_init (NemoScriptConfigWidget *self)
{
    self->scripts = NULL;

    self->settings = g_settings_new ("org.nemo.plugins");
    g_signal_connect (self->settings, "changed::" BLACKLIST_KEY, G_CALLBACK (on_settings_changed), self);

    GtkWidget *label = nemo_config_base_widget_get_label (NEMO_CONFIG_BASE_WIDGET (self));

    gchar *title = g_strdup (_("Scripts"));
    gchar *markup = g_strdup_printf ("<b>%s</b>", title);

    gtk_label_set_markup (GTK_LABEL (label), markup);

    g_free (title);
    g_free (markup);

    refresh_widget (self);
}

GtkWidget *
nemo_script_config_widget_new (void)
{
  return g_object_new (NEMO_TYPE_SCRIPT_CONFIG_WIDGET, NULL);
}
