/* nemo-archive-bar.c: Extract toolbar for archive views.
 *
 * Hosts an Extract Selected button (enabled when there is a selection) and an
 * Extract All button. Both run async via nemo_archive_extract_async() and
 * surface success/failure as a transient info bar message — keeping the UI
 * simple for v1 at the cost of no in-progress UI.
 */

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "nemo-archive-bar.h"
#include "nemo-view.h"
#include "nemo-window.h"

#include <libnemo-private/nemo-archive.h>
#include <libnemo-private/nemo-file.h>
#include <eel/eel-vfs-extensions.h>

enum {
    PROP_VIEW = 1,
    NUM_PROPERTIES
};

enum {
    RESPONSE_EXTRACT_SELECTED = 1,
    RESPONSE_EXTRACT_ALL
};

struct _NemoArchiveBarPrivate {
    NemoView *view;
    gulong selection_handler_id;
    GtkWidget *extract_selected_button;
};

G_DEFINE_TYPE_WITH_PRIVATE (NemoArchiveBar, nemo_archive_bar, GTK_TYPE_INFO_BAR)

static gchar *
archive_path_from_view (NemoView *view)
{
    gchar *uri = nemo_view_get_uri (view);
    gchar *archive_path = NULL;
    eel_archive_uri_parse (uri, &archive_path, NULL);
    g_free (uri);
    return archive_path;
}

static GList *
inside_paths_from_selection (NemoView *view)
{
    GList *files = nemo_view_get_selection (view);
    GList *paths = NULL;

    for (GList *l = files; l != NULL; l = l->next) {
        char *uri = nemo_file_get_uri (NEMO_FILE (l->data));
        char *inside = NULL;
        if (eel_archive_uri_parse (uri, NULL, &inside) && inside != NULL && inside[0] != '\0') {
            paths = g_list_prepend (paths, inside); /* transfer */
        } else {
            g_free (inside);
        }
        g_free (uri);
    }
    nemo_file_list_free (files);
    return g_list_reverse (paths);
}

typedef struct {
    NemoArchiveBar *bar;
    NemoArchive    *archive;
    GList          *owned_paths;  /* GList<char*> we own */
    GFile          *destination;
} ExtractCtx;

static void
extract_ctx_free (ExtractCtx *ctx)
{
    g_list_free_full (ctx->owned_paths, g_free);
    g_clear_object (&ctx->archive);
    g_clear_object (&ctx->destination);
    g_free (ctx);
}

static void
show_message (NemoArchiveBar *bar, const char *text, GtkMessageType type)
{
    gtk_info_bar_set_message_type (GTK_INFO_BAR (bar), type);
    GtkWidget *content = gtk_info_bar_get_content_area (GTK_INFO_BAR (bar));
    GList *children = gtk_container_get_children (GTK_CONTAINER (content));
    if (children != NULL) {
        GtkWidget *label = GTK_WIDGET (children->data);
        if (GTK_IS_LABEL (label)) {
            gtk_label_set_text (GTK_LABEL (label), text);
        }
        g_list_free (children);
    }
}

static void
on_extract_done (GObject *source, GAsyncResult *result, gpointer user_data)
{
    ExtractCtx *ctx = user_data;
    GError *error = NULL;

    if (nemo_archive_extract_finish (NEMO_ARCHIVE (source), result, &error)) {
        char *dest_label = g_file_get_basename (ctx->destination);
        char *msg = g_strdup_printf (_("Extracted to %s"), dest_label);
        show_message (ctx->bar, msg, GTK_MESSAGE_INFO);
        g_free (dest_label);
        g_free (msg);
    } else {
        char *msg = g_strdup_printf (_("Extraction failed: %s"),
                                     error ? error->message : "");
        show_message (ctx->bar, msg, GTK_MESSAGE_WARNING);
        g_free (msg);
        g_clear_error (&error);
    }
    extract_ctx_free (ctx);
}

/* Pops a folder picker, then runs nemo_archive_extract_async.
 * paths is GList<char*>; this function takes ownership and frees on return. */
static void
prompt_and_extract (NemoArchiveBar *bar, GList *paths)
{
    char *archive_path;
    NemoArchive *archive;
    GtkFileChooserNative *chooser;
    GtkWindow *parent;
    char *archive_basename;
    char *suggested;
    int response;
    GFile *destination = NULL;

    archive_path = archive_path_from_view (bar->priv->view);
    if (archive_path == NULL) {
        g_list_free_full (paths, g_free);
        return;
    }
    archive = nemo_archive_get_or_create (archive_path);
    if (archive == NULL) {
        g_free (archive_path);
        g_list_free_full (paths, g_free);
        return;
    }

    parent = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (bar)));
    chooser = gtk_file_chooser_native_new (_("Extract Archive"),
                                           parent,
                                           GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                           _("Extract"),
                                           _("Cancel"));

    {
        char *parent_path = g_path_get_dirname (archive_path);
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), parent_path);
        g_free (parent_path);
    }

    archive_basename = g_path_get_basename (archive_path);
    /* Strip extension twice for .tar.gz / .tar.xz / .tar.bz2 etc. */
    suggested = eel_filename_strip_extension (archive_basename);
    {
        char *again = eel_filename_strip_extension (suggested);
        if (g_strcmp0 (again, suggested) != 0) {
            g_free (suggested);
            suggested = again;
        } else {
            g_free (again);
        }
    }
    g_free (archive_basename);

    response = gtk_native_dialog_run (GTK_NATIVE_DIALOG (chooser));
    if (response == GTK_RESPONSE_ACCEPT) {
        GFile *chosen = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (chooser));
        if (chosen != NULL) {
            destination = g_file_get_child (chosen, suggested);
            g_object_unref (chosen);
        }
    }
    g_object_unref (chooser);
    g_free (suggested);

    if (destination == NULL) {
        g_free (archive_path);
        g_object_unref (archive);
        g_list_free_full (paths, g_free);
        return;
    }

    ExtractCtx *ctx = g_new0 (ExtractCtx, 1);
    ctx->bar = bar;
    ctx->archive = archive; /* owned */
    ctx->owned_paths = paths;
    ctx->destination = destination; /* owned */

    show_message (bar, _("Extracting…"), GTK_MESSAGE_INFO);

    nemo_archive_extract_async (archive, paths, destination, NULL,
                                on_extract_done, ctx);

    g_free (archive_path);
}

static void
update_button_sensitivity (NemoArchiveBar *bar)
{
    int count = nemo_view_get_selection_count (bar->priv->view);
    gtk_widget_set_sensitive (bar->priv->extract_selected_button, count > 0);
}

static void
selection_changed_cb (NemoView *view, NemoArchiveBar *bar)
{
    update_button_sensitivity (bar);
}

static void
disconnect_view (NemoArchiveBar *bar)
{
    if (bar->priv->selection_handler_id != 0) {
        g_signal_handler_disconnect (bar->priv->view, bar->priv->selection_handler_id);
        bar->priv->selection_handler_id = 0;
    }
}

static void
nemo_archive_bar_dispose (GObject *object)
{
    NemoArchiveBar *bar = NEMO_ARCHIVE_BAR (object);
    disconnect_view (bar);
    G_OBJECT_CLASS (nemo_archive_bar_parent_class)->dispose (object);
}

static void
nemo_archive_bar_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
    NemoArchiveBar *bar = NEMO_ARCHIVE_BAR (object);

    switch (prop_id) {
    case PROP_VIEW:
        bar->priv->view = g_value_get_object (value);
        bar->priv->selection_handler_id =
            g_signal_connect (bar->priv->view, "selection-changed",
                              G_CALLBACK (selection_changed_cb), bar);
        g_signal_connect_object (bar->priv->view, "destroy",
                                 G_CALLBACK (disconnect_view), bar,
                                 G_CONNECT_SWAPPED);
        update_button_sensitivity (bar);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
response_cb (GtkInfoBar *infobar, gint response_id, gpointer user_data)
{
    NemoArchiveBar *bar = NEMO_ARCHIVE_BAR (infobar);

    switch (response_id) {
    case RESPONSE_EXTRACT_SELECTED: {
        GList *paths = inside_paths_from_selection (bar->priv->view);
        if (paths != NULL) {
            prompt_and_extract (bar, paths);
        }
        break;
    }
    case RESPONSE_EXTRACT_ALL:
        prompt_and_extract (bar, NULL); /* NULL = extract everything */
        break;
    default:
        break;
    }
}

static void
nemo_archive_bar_init (NemoArchiveBar *bar)
{
    GtkWidget *content_area;
    GtkWidget *action_area;
    GtkWidget *label;

    bar->priv = nemo_archive_bar_get_instance_private (bar);
    content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (bar));
    action_area = gtk_info_bar_get_action_area (GTK_INFO_BAR (bar));

    gtk_orientable_set_orientation (GTK_ORIENTABLE (action_area),
                                    GTK_ORIENTATION_HORIZONTAL);

    label = gtk_label_new (_("Archive"));
    gtk_style_context_add_class (gtk_widget_get_style_context (label),
                                 "nemo-cluebar-label");
    gtk_widget_show (label);
    gtk_container_add (GTK_CONTAINER (content_area), label);

    bar->priv->extract_selected_button =
        gtk_info_bar_add_button (GTK_INFO_BAR (bar),
                                 _("Extract Selected"),
                                 RESPONSE_EXTRACT_SELECTED);
    gtk_widget_set_tooltip_text (bar->priv->extract_selected_button,
                                 _("Extract the selected items from this archive."));

    gtk_info_bar_add_button (GTK_INFO_BAR (bar),
                             _("Extract All"),
                             RESPONSE_EXTRACT_ALL);

    g_signal_connect (bar, "response", G_CALLBACK (response_cb), bar);
}

static void
nemo_archive_bar_class_init (NemoArchiveBarClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->set_property = nemo_archive_bar_set_property;
    object_class->dispose = nemo_archive_bar_dispose;

    g_object_class_install_property (object_class,
                                     PROP_VIEW,
                                     g_param_spec_object ("view", "view",
                                                          "the NemoView",
                                                          NEMO_TYPE_VIEW,
                                                          G_PARAM_WRITABLE |
                                                          G_PARAM_CONSTRUCT_ONLY |
                                                          G_PARAM_STATIC_STRINGS));
}

GtkWidget *
nemo_archive_bar_new (NemoView *view)
{
    return g_object_new (NEMO_TYPE_ARCHIVE_BAR, "view", view, NULL);
}
