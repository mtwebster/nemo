/* nemo-archive-directory-file.c
 *
 * NemoFile subclass for entries in an archive. Owns synthetic GFileInfo; the
 * owning NemoArchiveDirectory populates display name, size, mime type, etc.
 * directly on the NemoFile->details after construction.
 */

#include <config.h>
#include "nemo-archive-directory-file.h"

#include "nemo-archive-directory.h"
#include "nemo-directory-private.h"
#include "nemo-file-private.h"
#include <glib/gi18n.h>

G_DEFINE_TYPE (NemoArchiveDirectoryFile, nemo_archive_directory_file, NEMO_TYPE_FILE)

static void
archive_file_monitor_add (NemoFile           *file,
                          gconstpointer       client,
                          NemoFileAttributes  attributes)
{
}

static void
archive_file_monitor_remove (NemoFile      *file,
                             gconstpointer  client)
{
}

static void
archive_file_call_when_ready (NemoFile           *file,
                              NemoFileAttributes  file_attributes,
                              NemoFileCallback    callback,
                              gpointer            callback_data)
{
    /* Synthetic state is always up to date. */
    (* callback) (file, callback_data);
}

static void
archive_file_cancel_call_when_ready (NemoFile         *file,
                                     NemoFileCallback  callback,
                                     gpointer          callback_data)
{
}

static gboolean
archive_file_check_if_ready (NemoFile           *file,
                             NemoFileAttributes  attributes)
{
    return TRUE;
}

static gboolean
archive_file_get_item_count (NemoFile *file,
                             guint    *count,
                             gboolean *count_unreadable)
{
    if (count != NULL) {
        GList *file_list = nemo_directory_get_file_list (file->details->directory);
        *count = g_list_length (file_list);
        nemo_file_list_free (file_list);
    }
    if (count_unreadable != NULL) {
        *count_unreadable = FALSE;
    }
    return TRUE;
}

static NemoRequestStatus
archive_file_get_deep_counts (NemoFile *file,
                              guint    *directory_count,
                              guint    *file_count,
                              guint    *unreadable_directory_count,
                              guint    *hidden_count,
                              goffset  *total_size)
{
    GList *file_list = nemo_directory_get_file_list (file->details->directory);
    guint dirs = 0, files = 0;
    goffset size = 0;

    for (GList *l = file_list; l != NULL; l = l->next) {
        NemoFile *child = NEMO_FILE (l->data);
        if (nemo_file_get_file_type (child) == G_FILE_TYPE_DIRECTORY) {
            dirs++;
        } else {
            files++;
            size += nemo_file_get_size (child);
        }
    }

    if (directory_count != NULL) *directory_count = dirs;
    if (file_count != NULL) *file_count = files;
    if (unreadable_directory_count != NULL) *unreadable_directory_count = 0;
    if (hidden_count != NULL) *hidden_count = 0;
    if (total_size != NULL) *total_size = size;

    nemo_file_list_free (file_list);
    return NEMO_REQUEST_DONE;
}

static char *
archive_file_get_where_string (NemoFile *file)
{
    return g_strdup (_("Archive"));
}

static void
archive_file_set_metadata (NemoFile   *file,
                           const char *key,
                           const char *value)
{
    /* Archive views are ephemeral; persisting per-folder metadata into the
     * archive's URI namespace would just leak. Drop. */
}

static void
archive_file_set_metadata_as_list (NemoFile    *file,
                                   const char  *key,
                                   char       **value)
{
}

static gboolean
archive_file_get_date (NemoFile     *file,
                       NemoDateType  date_type,
                       time_t       *date)
{
    switch (date_type) {
    case NEMO_DATE_TYPE_MODIFIED:
    case NEMO_DATE_TYPE_CHANGED:
    case NEMO_DATE_TYPE_ACCESSED:
        if (file->details->mtime > 0) {
            if (date != NULL) *date = file->details->mtime;
            return TRUE;
        }
        return FALSE;
    default:
        return FALSE;
    }
}

static void
nemo_archive_directory_file_init (NemoArchiveDirectoryFile *self)
{
    NemoFile *file = NEMO_FILE (self);

    /* Tell the rest of nemo to stop asking GIO for our info — we own it. */
    file->details->got_file_info = TRUE;
    file->details->file_info_is_up_to_date = TRUE;
    file->details->got_link_info = TRUE;
    file->details->link_info_is_up_to_date = TRUE;
    file->details->got_directory_count = TRUE;
    file->details->directory_count_is_up_to_date = TRUE;

    /* Read-only by default; NemoArchiveDirectory may override per-entry. */
    file->details->has_permissions = TRUE;
    file->details->can_read = TRUE;
    file->details->can_write = FALSE;
    file->details->can_execute = FALSE;
    file->details->can_delete = FALSE;
    file->details->can_trash = FALSE;
    file->details->can_rename = FALSE;

    file->details->size = 0;
    /* Default to DIRECTORY so the self-owned root file (which represents the
     * archive itself in its parent dir) can be navigated into. Per-entry
     * population in nemo_archive_directory.c overrides this for plain files. */
    file->details->type = G_FILE_TYPE_DIRECTORY;
    if (file->details->mime_type != NULL) {
        g_ref_string_release (file->details->mime_type);
    }
    file->details->mime_type = g_ref_string_new_intern ("inode/directory");
}

static void
nemo_archive_directory_file_class_init (NemoArchiveDirectoryFileClass *klass)
{
    NemoFileClass *file_class = NEMO_FILE_CLASS (klass);

    file_class->default_file_type        = G_FILE_TYPE_DIRECTORY;
    file_class->monitor_add              = archive_file_monitor_add;
    file_class->monitor_remove           = archive_file_monitor_remove;
    file_class->call_when_ready          = archive_file_call_when_ready;
    file_class->cancel_call_when_ready   = archive_file_cancel_call_when_ready;
    file_class->check_if_ready           = archive_file_check_if_ready;
    file_class->get_item_count           = archive_file_get_item_count;
    file_class->get_deep_counts          = archive_file_get_deep_counts;
    file_class->get_where_string         = archive_file_get_where_string;
    file_class->get_date                 = archive_file_get_date;
    file_class->set_metadata             = archive_file_set_metadata;
    file_class->set_metadata_as_list     = archive_file_set_metadata_as_list;
}
