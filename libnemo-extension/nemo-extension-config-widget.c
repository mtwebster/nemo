/* nemo-extension-config-widget.h */

/*  A widget that displays a list of extensions to enable or disable.
 *  This is usually part of a NemoPluginManagerWidget
 */

#include <config.h>
#include "nemo-extension-config-widget.h"
#include <gmodule.h>

#include <glib.h>

G_DEFINE_TYPE (NemoExtensionConfigWidget, nemo_extension_config_widget, NEMO_TYPE_CONFIG_BASE_WIDGET);

#define BLACKLIST_KEY "disabled-extensions"

typedef struct {
    NemoExtensionConfigWidget *widget;

    gchar *name;
    gchar *display_name;
    gchar *desc;
} ExtensionProxy;

static void
extension_proxy_free (ExtensionProxy *proxy)
{
    g_clear_pointer (&proxy->name, g_free);
    g_clear_pointer (&proxy->display_name, g_free);
    g_clear_pointer (&proxy->desc, g_free);
}

static gboolean
on_button_release (GtkWidget *evbox, GdkEventButton *event, GtkWidget *button)
{
    if (event->button == GDK_BUTTON_PRIMARY) {
        gtk_button_clicked (GTK_BUTTON (button));
    }
}

static void
on_check_toggled(GtkWidget *button, ExtensionProxy *proxy)
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

#define PREFIX "NEMO_EXTENSION:::"
#define PREFIX_LEN 17

static void
detect_extensions (NemoExtensionConfigWidget *widget)
{
    gchar *out = NULL;

    if (g_spawn_command_line_sync (LIBEXECDIR "/nemo-extensions-list",
                                   &out,
                                   NULL,
                                   NULL,
                                   NULL)) {
        if (out) {
            gchar **lines = g_strsplit (out, "\n", -1);

            g_free (out);

            int i;
            for (i = 0; i < g_strv_length (lines) - 1; i++) {
                if (g_str_has_prefix (lines[i], PREFIX)) {
                    ExtensionProxy *p = g_slice_new0 (ExtensionProxy);

                    gchar **split = g_strsplit (lines[i] + PREFIX_LEN, ":::", -1);
                    gint len = g_strv_length (split);

                    if (len > 0) {
                        p->name = g_strdup (split[0]);
                    }

                    if (len == 3) {
                        p->display_name = g_strdup (split[1]);
                        p->desc = g_strdup (split[2]);
                    } else {
                        p->display_name = NULL;
                        p->desc = NULL;
                    }

                    p->widget = widget;
                    widget->extensions = g_list_append (widget->extensions, p);
                    g_strfreev (split);
                }
            }
            g_strfreev (lines);
        }
    } else {
        g_printerr ("oops could not run nemo-extensions-list\n");
    }
}

static void
refresh_widget (NemoExtensionConfigWidget *widget)
{
    if (widget->extensions != NULL) {
        g_list_free_full (widget->extensions, (GDestroyNotify) extension_proxy_free);
        widget->extensions = NULL;
    }

    nemo_config_base_widget_clear_list (NEMO_CONFIG_BASE_WIDGET (widget));

    detect_extensions (widget);

    GList *l;
    gchar **blacklist = g_settings_get_strv (widget->settings, BLACKLIST_KEY);

    for (l = widget->extensions; l != NULL; l=l->next) {
        ExtensionProxy *proxy = l->data;

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

        if (proxy->display_name == NULL) {
            w = gtk_label_new (proxy->name);
        } else {
            w = gtk_label_new (NULL);
            gchar *markup = g_strdup_printf ("%s - <i>%s</i>", proxy->display_name, proxy->desc);
            gtk_label_set_markup (GTK_LABEL (w), markup);
            g_free (markup);
        }

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
    NemoExtensionConfigWidget *w = NEMO_EXTENSION_CONFIG_WIDGET (user_data);

    refresh_widget (w);
}

static void
nemo_extension_config_widget_class_init (NemoExtensionConfigWidgetClass *klass)
{
}

static void
nemo_extension_config_widget_init (NemoExtensionConfigWidget *self)
{
    self->extensions = NULL;
    self->nemo_modules = NULL;
    self->module_objects = NULL;

    self->settings = g_settings_new ("org.nemo.plugins");
    g_signal_connect (self->settings, "changed::" BLACKLIST_KEY, G_CALLBACK (on_settings_changed), self);

    GtkWidget *label = nemo_config_base_widget_get_label (NEMO_CONFIG_BASE_WIDGET (self));

    gchar *title = g_strdup (_("Extensions"));
    gchar *markup = g_strdup_printf ("<b>%s</b>", title);

    gtk_label_set_markup (GTK_LABEL (label), markup);

    g_free (title);
    g_free (markup);

    refresh_widget (self);
}

GtkWidget *
nemo_extension_config_widget_new (void)
{
  return g_object_new (NEMO_TYPE_EXTENSION_CONFIG_WIDGET, NULL);
}
