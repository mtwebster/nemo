/* nemo-archive-directory.c
 *
 * NemoDirectory subclass that publishes the contents of an archive (zip, tar,
 * etc.) as a virtual folder. Each entry becomes a NemoArchiveDirectoryFile
 * whose GFileInfo is populated from libarchive's parse instead of disk I/O.
 *
 * One NemoArchiveDirectory per (archive, inside-path) pair; subdirectories
 * within an archive each get their own instance but all share one NemoArchive.
 */

#include <config.h>
#include "nemo-archive-directory.h"

#include "nemo-archive.h"
#include "nemo-archive-directory-file.h"
#include "nemo-directory-private.h"
#include "nemo-file-private.h"
#include "nemo-file.h"
#include <eel/eel-vfs-extensions.h>
#include <glib/gi18n.h>
#include <string.h>

typedef struct {
    gboolean monitor_hidden_files;
    NemoFileAttributes monitor_attributes;
    gconstpointer client;
} ArchiveMonitor;

typedef struct {
    NemoArchiveDirectory *directory;
    NemoDirectoryCallback callback;
    gpointer callback_data;
    NemoFileAttributes wait_for_attributes;
    gboolean wait_for_file_list;
} ArchiveCallback;

struct NemoArchiveDirectoryDetails {
    char *archive_path;        /* canonical filesystem path */
    char *inside_path;         /* "" for archive root, otherwise no leading/trailing '/' */
    NemoArchive *archive;
    gulong entries_added_id;
    gulong done_loading_id;

    GList *files;              /* NemoFile* — child entries we've published */
    GHashTable *file_hash;     /* basename -> NemoFile (borrowed) */

    GList *monitor_list;       /* ArchiveMonitor* */
    GList *callback_list;      /* ArchiveCallback* */

    gboolean listing_started;
    gboolean listing_done;
    gboolean parsed_location;
};

G_DEFINE_TYPE (NemoArchiveDirectory, nemo_archive_directory, NEMO_TYPE_DIRECTORY)

/* Forward decls. */
static void invoke_ready_callbacks (NemoArchiveDirectory *self);
static NemoFile *make_file_for_entry (NemoArchiveDirectory *self,
                                      const NemoArchiveEntry *entry);
static void on_archive_entries_added (NemoArchive *archive,
                                      GPtrArray *added,
                                      NemoArchiveDirectory *self);
static void on_archive_done_loading (NemoArchive *archive,
                                     NemoArchiveDirectory *self);

/* ---- helpers ---- */

static const char *
basename_of (const char *full_path)
{
    const char *slash = strrchr (full_path, '/');
    return (slash != NULL) ? slash + 1 : full_path;
}

static gboolean
entry_is_direct_child (const NemoArchiveEntry *entry, const char *inside_path)
{
    gsize plen;
    const char *tail;

    if (inside_path == NULL || inside_path[0] == '\0') {
        return strchr (entry->full_path, '/') == NULL;
    }
    plen = strlen (inside_path);
    if (strncmp (entry->full_path, inside_path, plen) != 0 ||
        entry->full_path[plen] != '/') {
        return FALSE;
    }
    tail = entry->full_path + plen + 1;
    if (tail[0] == '\0') return FALSE;
    return strchr (tail, '/') == NULL;
}

static void
parse_location (NemoArchiveDirectory *self)
{
    char *uri;

    if (self->details->parsed_location) return;

    uri = nemo_directory_get_uri (NEMO_DIRECTORY (self));
    if (uri == NULL) return;

    eel_archive_uri_parse (uri, &self->details->archive_path, &self->details->inside_path);
    g_free (uri);

    if (self->details->archive_path != NULL) {
        self->details->archive = nemo_archive_get_or_create (self->details->archive_path);
        if (self->details->archive != NULL) {
            self->details->entries_added_id = g_signal_connect (
                self->details->archive, "entries-added",
                G_CALLBACK (on_archive_entries_added), self);
            self->details->done_loading_id = g_signal_connect (
                self->details->archive, "done-loading",
                G_CALLBACK (on_archive_done_loading), self);

            if (nemo_archive_is_loaded (self->details->archive)) {
                self->details->listing_done = TRUE;
            }
        }
    }
    self->details->parsed_location = TRUE;
}

/* Adds any entries already known on NemoArchive to our published file list.
 * Safe to call multiple times — make_file_for_entry dedupes via file_hash. */
static void
absorb_existing_entries (NemoArchiveDirectory *self)
{
    GList *children;
    GList *new_files = NULL;

    if (self->details->archive == NULL) return;

    children = nemo_archive_get_children (self->details->archive,
                                          self->details->inside_path);
    for (GList *l = children; l != NULL; l = l->next) {
        NemoFile *file = make_file_for_entry (self, l->data);
        if (file != NULL) {
            new_files = g_list_prepend (new_files, file);
        }
    }
    g_list_free (children);

    if (new_files != NULL) {
        nemo_directory_emit_files_added (NEMO_DIRECTORY (self), new_files);
        g_list_free (new_files);
    }
}

static void
ensure_listing_started (NemoArchiveDirectory *self)
{
    parse_location (self);

    if (self->details->archive == NULL || self->details->listing_started) {
        return;
    }
    self->details->listing_started = TRUE;

    /* Pick up entries already streamed in by a sibling directory. */
    absorb_existing_entries (self);

    if (self->details->listing_done) {
        nemo_directory_emit_done_loading (NEMO_DIRECTORY (self));
        invoke_ready_callbacks (self);
    } else {
        nemo_archive_list_async (self->details->archive, NULL, NULL, NULL);
    }
}

/* Builds a synthetic NemoFile for one archive entry and adds it to our state
 * (file_list + file_hash). Returns the file (ref held by us) if newly added,
 * or NULL if a file with this basename already exists. */
static NemoFile *
make_file_for_entry (NemoArchiveDirectory *self, const NemoArchiveEntry *entry)
{
    const char *base = basename_of (entry->full_path);
    NemoFile *file;
    char *uri;

    if (g_hash_table_contains (self->details->file_hash, base)) {
        return NULL;
    }

    uri = eel_archive_uri_new (self->details->archive_path, entry->full_path);
    file = nemo_file_get_by_uri (uri);
    g_free (uri);
    if (file == NULL) return NULL;

    /* Populate GFileInfo-ish state. The NemoArchiveDirectoryFile.init has
     * already marked got_file_info/file_info_is_up_to_date. */
    file->details->type = entry->is_dir ? G_FILE_TYPE_DIRECTORY
                                        : G_FILE_TYPE_REGULAR;
    file->details->size = (goffset) entry->size;
    if (entry->mtime > 0) {
        file->details->mtime = (time_t) entry->mtime;
    }
    if (entry->content_type != NULL) {
        if (file->details->mime_type != NULL) {
            g_ref_string_release (file->details->mime_type);
        }
        file->details->mime_type = g_ref_string_new_intern (entry->content_type);

        /* Resolve a themed icon from the content type so the icon resolver
         * has something to work with — otherwise it falls back to the
         * generic text-x-generic icon. */
        g_clear_object (&file->details->icon);
        file->details->icon = g_content_type_get_icon (entry->content_type);
    }
    file->details->permissions = entry->mode;
    file->details->has_permissions = TRUE;
    file->details->can_read = TRUE;
    file->details->can_write = FALSE;
    file->details->can_execute = (entry->mode & 0111) != 0;
    file->details->can_delete = FALSE;
    file->details->can_trash = FALSE;
    file->details->can_rename = FALSE;

    nemo_file_set_display_name (file, base, base, TRUE);

    self->details->files = g_list_prepend (self->details->files, file);
    g_hash_table_insert (self->details->file_hash, g_strdup (base), file);
    return file;
}

/* ---- archive signal handlers ---- */

static void
on_archive_entries_added (NemoArchive          *archive,
                          GPtrArray            *added,
                          NemoArchiveDirectory *self)
{
    GList *new_files = NULL;

    for (guint i = 0; i < added->len; i++) {
        const NemoArchiveEntry *entry = g_ptr_array_index (added, i);
        if (!entry_is_direct_child (entry, self->details->inside_path)) {
            continue;
        }
        NemoFile *file = make_file_for_entry (self, entry);
        if (file != NULL) {
            new_files = g_list_prepend (new_files, file);
        }
    }

    if (new_files != NULL) {
        nemo_directory_emit_files_added (NEMO_DIRECTORY (self), new_files);
        g_list_free (new_files);
    }
}

static void
on_archive_done_loading (NemoArchive          *archive,
                         NemoArchiveDirectory *self)
{
    self->details->listing_done = TRUE;
    nemo_directory_emit_done_loading (NEMO_DIRECTORY (self));
    invoke_ready_callbacks (self);
}

/* ---- monitor + callback bookkeeping ---- */

static void
archive_monitor_add (NemoDirectory         *directory,
                     gconstpointer          client,
                     gboolean               monitor_hidden_files,
                     NemoFileAttributes     file_attributes,
                     NemoDirectoryCallback  callback,
                     gpointer               callback_data)
{
    NemoArchiveDirectory *self = NEMO_ARCHIVE_DIRECTORY (directory);
    ArchiveMonitor *monitor;

    monitor = g_new0 (ArchiveMonitor, 1);
    monitor->monitor_hidden_files = monitor_hidden_files;
    monitor->monitor_attributes = file_attributes;
    monitor->client = client;
    self->details->monitor_list = g_list_prepend (self->details->monitor_list, monitor);

    if (callback != NULL) {
        (* callback) (directory, self->details->files, callback_data);
    }

    ensure_listing_started (self);
}

static void
archive_monitor_remove (NemoDirectory *directory,
                        gconstpointer  client)
{
    NemoArchiveDirectory *self = NEMO_ARCHIVE_DIRECTORY (directory);
    GList *l;

    for (l = self->details->monitor_list; l != NULL; l = l->next) {
        ArchiveMonitor *m = l->data;
        if (m->client == client) {
            self->details->monitor_list = g_list_delete_link (
                self->details->monitor_list, l);
            g_free (m);
            return;
        }
    }
}

static void
invoke_ready_callbacks (NemoArchiveDirectory *self)
{
    GList *callbacks = self->details->callback_list;
    self->details->callback_list = NULL;

    for (GList *l = callbacks; l != NULL; l = l->next) {
        ArchiveCallback *cb = l->data;
        cb->callback (NEMO_DIRECTORY (self), self->details->files, cb->callback_data);
        g_free (cb);
    }
    g_list_free (callbacks);
}

static void
archive_call_when_ready (NemoDirectory         *directory,
                         NemoFileAttributes     file_attributes,
                         gboolean               wait_for_file_list,
                         NemoDirectoryCallback  callback,
                         gpointer               callback_data)
{
    NemoArchiveDirectory *self = NEMO_ARCHIVE_DIRECTORY (directory);

    ensure_listing_started (self);

    if (self->details->listing_done || !wait_for_file_list) {
        if (callback != NULL) {
            (* callback) (directory, self->details->files, callback_data);
        }
        return;
    }

    ArchiveCallback *cb = g_new0 (ArchiveCallback, 1);
    cb->directory = self;
    cb->callback = callback;
    cb->callback_data = callback_data;
    cb->wait_for_attributes = file_attributes;
    cb->wait_for_file_list = wait_for_file_list;
    self->details->callback_list = g_list_append (self->details->callback_list, cb);
}

static void
archive_cancel_callback (NemoDirectory         *directory,
                         NemoDirectoryCallback  callback,
                         gpointer               callback_data)
{
    NemoArchiveDirectory *self = NEMO_ARCHIVE_DIRECTORY (directory);
    GList *l;

    for (l = self->details->callback_list; l != NULL; l = l->next) {
        ArchiveCallback *cb = l->data;
        if (cb->callback == callback && cb->callback_data == callback_data) {
            self->details->callback_list = g_list_delete_link (
                self->details->callback_list, l);
            g_free (cb);
            return;
        }
    }
}

static gboolean
archive_contains_file (NemoDirectory *directory,
                       NemoFile      *file)
{
    NemoArchiveDirectory *self = NEMO_ARCHIVE_DIRECTORY (directory);
    return g_list_find (self->details->files, file) != NULL;
}

static gboolean
archive_are_all_files_seen (NemoDirectory *directory)
{
    NemoArchiveDirectory *self = NEMO_ARCHIVE_DIRECTORY (directory);
    return self->details->listing_done;
}

static gboolean
archive_is_not_empty (NemoDirectory *directory)
{
    NemoArchiveDirectory *self = NEMO_ARCHIVE_DIRECTORY (directory);
    return self->details->files != NULL;
}

static GList *
archive_get_file_list (NemoDirectory *directory)
{
    NemoArchiveDirectory *self = NEMO_ARCHIVE_DIRECTORY (directory);
    return nemo_file_list_copy (self->details->files);
}

static gboolean
archive_is_editable (NemoDirectory *directory)
{
    return FALSE;
}

static void
archive_force_reload (NemoDirectory *directory)
{
    /* For now, listing is immutable for the archive's lifetime. A force-reload
     * would tear down our NemoArchive and rebuild it; deferred until needed. */
}

/* ---- lifecycle ---- */

static void
nemo_archive_directory_init (NemoArchiveDirectory *self)
{
    self->details = g_new0 (NemoArchiveDirectoryDetails, 1);
    self->details->file_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                       g_free, NULL);
}

static void
nemo_archive_directory_dispose (GObject *object)
{
    NemoArchiveDirectory *self = NEMO_ARCHIVE_DIRECTORY (object);

    if (self->details->archive != NULL) {
        g_clear_signal_handler (&self->details->entries_added_id,
                                self->details->archive);
        g_clear_signal_handler (&self->details->done_loading_id,
                                self->details->archive);
        g_clear_object (&self->details->archive);
    }

    g_list_free_full (self->details->files, (GDestroyNotify) nemo_file_unref);
    self->details->files = NULL;

    g_list_free_full (self->details->monitor_list, g_free);
    self->details->monitor_list = NULL;
    g_list_free_full (self->details->callback_list, g_free);
    self->details->callback_list = NULL;

    G_OBJECT_CLASS (nemo_archive_directory_parent_class)->dispose (object);
}

static void
nemo_archive_directory_finalize (GObject *object)
{
    NemoArchiveDirectory *self = NEMO_ARCHIVE_DIRECTORY (object);

    g_clear_pointer (&self->details->archive_path, g_free);
    g_clear_pointer (&self->details->inside_path, g_free);
    g_clear_pointer (&self->details->file_hash, g_hash_table_unref);
    g_free (self->details);

    G_OBJECT_CLASS (nemo_archive_directory_parent_class)->finalize (object);
}

static void
nemo_archive_directory_class_init (NemoArchiveDirectoryClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    NemoDirectoryClass *directory_class = NEMO_DIRECTORY_CLASS (klass);

    object_class->dispose = nemo_archive_directory_dispose;
    object_class->finalize = nemo_archive_directory_finalize;

    directory_class->contains_file       = archive_contains_file;
    directory_class->call_when_ready     = archive_call_when_ready;
    directory_class->cancel_callback     = archive_cancel_callback;
    directory_class->file_monitor_add    = archive_monitor_add;
    directory_class->file_monitor_remove = archive_monitor_remove;
    directory_class->force_reload        = archive_force_reload;
    directory_class->are_all_files_seen  = archive_are_all_files_seen;
    directory_class->is_not_empty        = archive_is_not_empty;
    directory_class->get_file_list       = archive_get_file_list;
    directory_class->is_editable         = archive_is_editable;
}
