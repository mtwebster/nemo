/* nemo-archive.c: libarchive-backed archive model.
 *
 * The parse runs on a worker thread; batches of entries are shipped to the
 * main thread for storage + signal emission. Each archive is cached via a
 * weak-ref table so concurrent NemoArchiveDirectory instances share work.
 */

#include <config.h>
#include "nemo-archive.h"

#include <eel/eel-vfs-extensions.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define BATCH_SIZE      200
#define BATCH_MS         50
#define BLOCK_SIZE    16384

struct _NemoArchive
{
    GObject parent_instance;

    char *path;

    /* Master entries list. Owned by the main thread; the worker thread
     * builds local batches and ships them across via g_main_context_invoke. */
    GPtrArray *entries;          /* NemoArchiveEntry*, free_func attached */

    gboolean   listing_started;
    gboolean   is_loaded;
    gboolean   load_failed;
    gboolean   is_encrypted;
    GError    *load_error;       /* set when load_failed */

    /* Pending list tasks waiting on the current parse. */
    GList     *pending_tasks;    /* GTask* */
};

enum {
    SIGNAL_ENTRIES_ADDED,
    SIGNAL_DONE_LOADING,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (NemoArchive, nemo_archive, G_TYPE_OBJECT)

/* ---- shared-instance cache ---- */

static GMutex   cache_mutex;
static GHashTable *cache; /* char* -> GWeakRef* */

static void
entry_free (gpointer data)
{
    NemoArchiveEntry *e = data;
    if (e == NULL) return;
    g_free (e->full_path);
    g_free (e->content_type);
    g_free (e);
}

static void
nemo_archive_finalize (GObject *object)
{
    NemoArchive *self = NEMO_ARCHIVE (object);

    g_mutex_lock (&cache_mutex);
    if (cache != NULL && self->path != NULL) {
        g_hash_table_remove (cache, self->path);
    }
    g_mutex_unlock (&cache_mutex);

    g_clear_pointer (&self->path, g_free);
    g_clear_pointer (&self->entries, g_ptr_array_unref);
    g_clear_error (&self->load_error);
    g_list_free_full (self->pending_tasks, g_object_unref);

    G_OBJECT_CLASS (nemo_archive_parent_class)->finalize (object);
}

static void
nemo_archive_init (NemoArchive *self)
{
    self->entries = g_ptr_array_new_with_free_func (entry_free);
}

static void
nemo_archive_class_init (NemoArchiveClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = nemo_archive_finalize;

    /* Fired on the main thread when a batch of entries is appended.
     * GPtrArray* of NemoArchiveEntry* — pointers borrowed; do not free.
     * The GPtrArray itself is owned by the emitter; receivers must not
     * retain it past the signal handler. */
    signals[SIGNAL_ENTRIES_ADDED] = g_signal_new (
        "entries-added",
        NEMO_TYPE_ARCHIVE,
        G_SIGNAL_RUN_LAST,
        0, NULL, NULL, NULL,
        G_TYPE_NONE, 1, G_TYPE_POINTER);

    signals[SIGNAL_DONE_LOADING] = g_signal_new (
        "done-loading",
        NEMO_TYPE_ARCHIVE,
        G_SIGNAL_RUN_LAST,
        0, NULL, NULL, NULL,
        G_TYPE_NONE, 0);
}

/* ---- mime hint list ----
 *
 * Seeded from file-roller's src/fr-archive-libarchive.c mime table. libarchive
 * itself doesn't expose a runtime list; we use this as a coarse filter to
 * decide whether to attempt the archive path. The final accept/reject
 * decision still comes from nemo_archive_probe_path() opening the file.
 */
static const char * const archive_mime_hints[] = {
    "application/zip",
    "application/x-zip-compressed",
    "application/x-tar",
    "application/x-compressed-tar",
    "application/x-bzip-compressed-tar",
    "application/x-xz-compressed-tar",
    "application/x-lzma-compressed-tar",
    "application/x-lzip-compressed-tar",
    "application/x-tzo",
    "application/gzip",
    "application/x-gzip",
    "application/x-bzip",
    "application/x-bzip2",
    "application/x-xz",
    "application/x-lzma",
    "application/x-lzip",
    "application/x-7z-compressed",
    "application/x-rar",
    "application/x-rar-compressed",
    "application/x-cpio",
    "application/x-cd-image",
    "application/x-iso9660-image",
    "application/x-archive",
    "application/x-ar",
    "application/x-lha",
    "application/x-lzh",
    "application/x-xar",
    "application/x-cab",
    "application/vnd.ms-cab-compressed",
    "application/x-rpm",
    "application/x-deb",
    NULL,
};

gboolean
nemo_archive_mime_type_is_candidate (const char *mime_type)
{
    if (mime_type == NULL) {
        return FALSE;
    }
    for (int i = 0; archive_mime_hints[i] != NULL; i++) {
        if (g_strcmp0 (mime_type, archive_mime_hints[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

gboolean
nemo_archive_probe_path (const char *path)
{
    struct archive *a;
    int r;

    if (path == NULL) {
        return FALSE;
    }

    a = archive_read_new ();
    archive_read_support_format_all (a);
    archive_read_support_filter_all (a);

    r = archive_read_open_filename (a, path, BLOCK_SIZE);
    if (r == ARCHIVE_OK) {
        /* Force libarchive to identify the format by reading one header. */
        struct archive_entry *ae;
        r = archive_read_next_header (a, &ae);
    }
    archive_read_free (a);
    return (r == ARCHIVE_OK || r == ARCHIVE_EOF);
}

/* ---- cache lookup ---- */

NemoArchive *
nemo_archive_get_or_create (const char *archive_path)
{
    NemoArchive *self = NULL;
    GWeakRef *ref;
    char *canonical;

    if (archive_path == NULL || archive_path[0] != '/') {
        return NULL;
    }

    canonical = g_canonicalize_filename (archive_path, NULL);

    g_mutex_lock (&cache_mutex);
    if (cache == NULL) {
        cache = g_hash_table_new_full (g_str_hash, g_str_equal,
                                       g_free, (GDestroyNotify) g_weak_ref_clear);
        /* GWeakRef structs require g_weak_ref_clear; wrap with a free func
         * that runs clear then frees the heap struct. */
    }

    ref = g_hash_table_lookup (cache, canonical);
    if (ref != NULL) {
        self = g_weak_ref_get (ref);
    }

    if (self == NULL) {
        self = g_object_new (NEMO_TYPE_ARCHIVE, NULL);
        self->path = g_strdup (canonical);

        if (ref == NULL) {
            ref = g_new0 (GWeakRef, 1);
            g_weak_ref_init (ref, self);
            g_hash_table_insert (cache, g_strdup (canonical), ref);
        } else {
            /* Reuse existing weak ref slot. */
            g_weak_ref_set (ref, self);
        }
    }
    g_mutex_unlock (&cache_mutex);

    g_free (canonical);
    return self;
}

const char *
nemo_archive_get_path (NemoArchive *self)
{
    g_return_val_if_fail (NEMO_IS_ARCHIVE (self), NULL);
    return self->path;
}

gboolean
nemo_archive_is_loaded (NemoArchive *self)
{
    g_return_val_if_fail (NEMO_IS_ARCHIVE (self), FALSE);
    return self->is_loaded;
}

gboolean
nemo_archive_load_failed (NemoArchive *self)
{
    g_return_val_if_fail (NEMO_IS_ARCHIVE (self), FALSE);
    return self->load_failed;
}

gboolean
nemo_archive_is_encrypted (NemoArchive *self)
{
    g_return_val_if_fail (NEMO_IS_ARCHIVE (self), FALSE);
    return self->is_encrypted;
}

const GPtrArray *
nemo_archive_get_entries (NemoArchive *self)
{
    g_return_val_if_fail (NEMO_IS_ARCHIVE (self), NULL);
    return self->entries;
}

/* Returns whether `path` (no leading or trailing '/') is a direct child of
 * `prefix` (no trailing '/'). Both already normalized. */
static gboolean
path_is_direct_child (const char *path, const char *prefix)
{
    gsize plen;
    const char *tail;
    const char *next_slash;

    if (prefix == NULL || prefix[0] == '\0') {
        /* Root: child has no slash. */
        return strchr (path, '/') == NULL;
    }

    plen = strlen (prefix);
    if (strncmp (path, prefix, plen) != 0 || path[plen] != '/') {
        return FALSE;
    }
    tail = path + plen + 1;
    if (tail[0] == '\0') {
        return FALSE; /* Same as prefix, not a child. */
    }
    next_slash = strchr (tail, '/');
    return next_slash == NULL;
}

GList *
nemo_archive_get_children (NemoArchive *self, const char *inside_prefix)
{
    GList *out = NULL;
    char *normalized;
    gsize plen;

    g_return_val_if_fail (NEMO_IS_ARCHIVE (self), NULL);

    if (inside_prefix == NULL) {
        normalized = g_strdup ("");
    } else {
        normalized = g_strdup (inside_prefix);
        plen = strlen (normalized);
        if (plen > 0 && normalized[plen - 1] == '/') {
            normalized[plen - 1] = '\0';
        }
    }

    for (guint i = 0; i < self->entries->len; i++) {
        NemoArchiveEntry *e = g_ptr_array_index (self->entries, i);
        if (path_is_direct_child (e->full_path, normalized)) {
            out = g_list_prepend (out, e);
        }
    }

    g_free (normalized);
    return g_list_reverse (out);
}

/* ---- worker -> main-thread batch delivery ---- */

typedef struct {
    NemoArchive *archive; /* strong ref */
    GPtrArray   *batch;   /* NemoArchiveEntry* with entry_free as free func */
} BatchDelivery;

static gboolean
deliver_batch_idle (gpointer data)
{
    BatchDelivery *d = data;
    GPtrArray *added;

    /* Move ownership of batch entries into the master array. */
    added = g_ptr_array_new (); /* borrowed pointers for the signal */
    for (guint i = 0; i < d->batch->len; i++) {
        NemoArchiveEntry *e = g_ptr_array_index (d->batch, i);
        g_ptr_array_add (d->archive->entries, e);
        g_ptr_array_add (added, e);
    }
    /* Don't let the batch's free func nuke the entries we just gave away. */
    g_ptr_array_set_free_func (d->batch, NULL);
    g_ptr_array_unref (d->batch);

    g_signal_emit (d->archive, signals[SIGNAL_ENTRIES_ADDED], 0, added);
    g_ptr_array_unref (added);

    g_object_unref (d->archive);
    g_free (d);
    return G_SOURCE_REMOVE;
}

static void
ship_batch (NemoArchive *self, GPtrArray **batch_ref)
{
    BatchDelivery *d;

    if (*batch_ref == NULL || (*batch_ref)->len == 0) {
        return;
    }

    d = g_new0 (BatchDelivery, 1);
    d->archive = g_object_ref (self);
    d->batch = *batch_ref;
    *batch_ref = g_ptr_array_new_with_free_func (entry_free);

    g_main_context_invoke (NULL, deliver_batch_idle, d);
}

/* ---- listing worker ---- */

typedef struct {
    char    *content_type_cache; /* unused; reserved for memoization */
} ListData;

static char *
guess_content_type (const char *filename, gboolean is_dir)
{
    if (is_dir) {
        return g_strdup ("inode/directory");
    }
    /* Filename-only guess avoids reading file contents. */
    return g_content_type_guess (filename, NULL, 0, NULL);
}

static char *
normalize_entry_path (const char *raw)
{
    const char *p;
    char *out;
    gsize len;

    if (raw == NULL) {
        return NULL;
    }

    p = raw;
    /* Strip leading "./" or "/" occurrences. */
    while (*p == '/' || (p[0] == '.' && p[1] == '/')) {
        p += (*p == '/') ? 1 : 2;
    }
    if (*p == '\0') {
        return NULL; /* The archive's own root entry — ignored. */
    }

    out = g_strdup (p);
    len = strlen (out);
    /* Trim trailing slashes; the is_dir flag carries that info. */
    while (len > 0 && out[len - 1] == '/') {
        out[--len] = '\0';
    }
    if (len == 0) {
        g_free (out);
        return NULL;
    }
    /* Reject anything attempting parent-dir traversal. */
    if (g_str_has_prefix (out, "../") || strstr (out, "/../") != NULL ||
        g_str_has_suffix (out, "/..") || g_strcmp0 (out, "..") == 0) {
        g_free (out);
        return NULL;
    }
    return out;
}

static char *
decode_pathname (struct archive_entry *ae)
{
    const char *u8;
    const char *raw;
    char *converted;

    u8 = archive_entry_pathname_utf8 (ae);
    if (u8 != NULL && g_utf8_validate (u8, -1, NULL)) {
        return g_strdup (u8);
    }

    raw = archive_entry_pathname (ae);
    if (raw == NULL) {
        return NULL;
    }
    /* Try locale → UTF-8 (covers common CP1252/Shift-JIS-with-locale cases). */
    converted = g_locale_to_utf8 (raw, -1, NULL, NULL, NULL);
    if (converted != NULL) {
        return converted;
    }
    /* Last resort: sanitize invalid bytes. */
    return eel_make_valid_utf8 (raw);
}

/* Runs on a worker thread. */
static void
list_thread (GTask        *task,
             gpointer      source_object,
             gpointer      task_data,
             GCancellable *cancellable)
{
    NemoArchive *self = NEMO_ARCHIVE (source_object);
    struct archive *a;
    struct archive_entry *ae;
    GPtrArray *batch;
    gint64 last_flush;
    int r;
    gboolean encountered_encrypted = FALSE;

    a = archive_read_new ();
    archive_read_support_format_all (a);
    archive_read_support_filter_all (a);

    r = archive_read_open_filename (a, self->path, BLOCK_SIZE);
    if (r != ARCHIVE_OK) {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                 "%s", archive_error_string (a));
        archive_read_free (a);
        return;
    }

    batch = g_ptr_array_new_with_free_func (entry_free);
    last_flush = g_get_monotonic_time ();

    while (!g_cancellable_is_cancelled (cancellable)) {
        char *path;
        NemoArchiveEntry *entry;
        gboolean is_dir;

        r = archive_read_next_header (a, &ae);
        if (r == ARCHIVE_EOF) {
            break;
        }
        if (r < ARCHIVE_WARN) {
            g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                     "%s", archive_error_string (a));
            g_ptr_array_unref (batch);
            archive_read_free (a);
            return;
        }

        if (archive_entry_is_encrypted (ae)) {
            encountered_encrypted = TRUE;
        }

        path = decode_pathname (ae);
        if (path == NULL) {
            continue;
        }
        is_dir = (archive_entry_filetype (ae) == AE_IFDIR);

        entry = g_new0 (NemoArchiveEntry, 1);
        entry->full_path = normalize_entry_path (path);
        g_free (path);
        if (entry->full_path == NULL) {
            g_free (entry);
            continue;
        }
        entry->size = is_dir ? 0 : (guint64) archive_entry_size (ae);
        entry->mtime = archive_entry_mtime_is_set (ae)
                        ? (gint64) archive_entry_mtime (ae)
                        : -1;
        entry->mode = (guint32) archive_entry_mode (ae);
        entry->is_dir = is_dir;
        {
            const char *base = strrchr (entry->full_path, '/');
            base = (base != NULL) ? base + 1 : entry->full_path;
            entry->content_type = guess_content_type (base, is_dir);
        }

        g_ptr_array_add (batch, entry);

        if (batch->len >= BATCH_SIZE ||
            (g_get_monotonic_time () - last_flush) >= (gint64) BATCH_MS * 1000) {
            ship_batch (self, &batch);
            last_flush = g_get_monotonic_time ();
        }
    }

    archive_read_free (a);

    if (g_cancellable_is_cancelled (cancellable)) {
        g_ptr_array_unref (batch);
        g_task_return_error_if_cancelled (task);
        return;
    }

    /* Final batch. */
    ship_batch (self, &batch);
    g_ptr_array_unref (batch);

    self->is_encrypted = encountered_encrypted;
    g_task_return_boolean (task, TRUE);
}

/* Runs on the main thread when the worker finishes. Sets state and emits the
 * archive-level done-loading signal, then finishes any queued list tasks. */
static void
list_finished_main (GObject *source, GAsyncResult *result, gpointer user_data)
{
    NemoArchive *self = NEMO_ARCHIVE (source);
    GError *error = NULL;
    gboolean ok;
    GList *queued;

    ok = g_task_propagate_boolean (G_TASK (result), &error);
    self->is_loaded = TRUE;
    self->load_failed = !ok;
    if (error != NULL) {
        g_clear_error (&self->load_error);
        self->load_error = error; /* take ownership */
    }

    g_signal_emit (self, signals[SIGNAL_DONE_LOADING], 0);

    queued = self->pending_tasks;
    self->pending_tasks = NULL;
    for (GList *l = queued; l != NULL; l = l->next) {
        GTask *t = l->data;
        if (ok) {
            g_task_return_boolean (t, TRUE);
        } else {
            g_task_return_new_error (t, G_IO_ERROR, G_IO_ERROR_FAILED,
                                     "%s",
                                     self->load_error ? self->load_error->message
                                                      : "archive listing failed");
        }
        g_object_unref (t);
    }
    g_list_free (queued);
}

void
nemo_archive_list_async (NemoArchive         *self,
                         GCancellable        *cancellable,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data)
{
    GTask *task;

    g_return_if_fail (NEMO_IS_ARCHIVE (self));

    task = g_task_new (self, cancellable, callback, user_data);
    g_task_set_source_tag (task, nemo_archive_list_async);

    if (self->is_loaded) {
        if (self->load_failed) {
            g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                     "%s",
                                     self->load_error ? self->load_error->message
                                                      : "archive listing failed");
        } else {
            g_task_return_boolean (task, TRUE);
        }
        g_object_unref (task);
        return;
    }

    if (self->listing_started) {
        /* Another caller already kicked off the listing; wait for it. */
        self->pending_tasks = g_list_append (self->pending_tasks, task);
        return;
    }

    self->listing_started = TRUE;

    /* Internal task drives the worker thread; its completion fans out to all
     * queued tasks via list_finished_main. */
    {
        GTask *worker = g_task_new (self, cancellable, list_finished_main, NULL);
        g_task_set_source_tag (worker, list_thread);
        self->pending_tasks = g_list_append (self->pending_tasks, task);
        g_task_run_in_thread (worker, list_thread);
        g_object_unref (worker);
    }
}

gboolean
nemo_archive_list_finish (NemoArchive   *self,
                          GAsyncResult  *result,
                          GError       **error)
{
    g_return_val_if_fail (NEMO_IS_ARCHIVE (self), FALSE);
    g_return_val_if_fail (g_task_is_valid (result, self), FALSE);
    return g_task_propagate_boolean (G_TASK (result), error);
}

/* ---- extraction ---- */

typedef struct {
    GHashTable *wanted;   /* char* (full_path) -> GINT_TO_POINTER(1); NULL = extract all */
    char       *destdir; /* absolute path, no trailing slash */
} ExtractData;

static void
extract_data_free (gpointer data)
{
    ExtractData *e = data;
    if (e == NULL) return;
    g_clear_pointer (&e->wanted, g_hash_table_unref);
    g_free (e->destdir);
    g_free (e);
}

/* Returns TRUE if `path` (entry path) is in wanted set, OR a descendant of
 * any wanted entry (so selecting a folder extracts its tree), OR wanted is
 * NULL (extract everything). */
static gboolean
extract_wants (GHashTable *wanted, const char *path)
{
    if (wanted == NULL) {
        return TRUE;
    }
    if (g_hash_table_contains (wanted, path)) {
        return TRUE;
    }
    /* Check prefix match against each wanted directory entry. */
    GHashTableIter it;
    gpointer key;
    g_hash_table_iter_init (&it, wanted);
    while (g_hash_table_iter_next (&it, &key, NULL)) {
        const char *w = key;
        gsize wlen = strlen (w);
        if (strncmp (path, w, wlen) == 0 && path[wlen] == '/') {
            return TRUE;
        }
    }
    return FALSE;
}

/* Joins destdir + entry_path, ensuring the result stays within destdir
 * (defeats ../ traversal). Returns NULL if the resulting path would escape. */
static char *
safe_join (const char *destdir, const char *entry_path)
{
    char *normalized = normalize_entry_path (entry_path);
    char *full;

    if (normalized == NULL) {
        return NULL;
    }
    full = g_build_filename (destdir, normalized, NULL);
    g_free (normalized);

    /* Double-check final canonical form is inside destdir. */
    {
        char *canonical = g_canonicalize_filename (full, NULL);
        gsize ddlen = strlen (destdir);
        gboolean inside = (strncmp (canonical, destdir, ddlen) == 0 &&
                           (canonical[ddlen] == '\0' || canonical[ddlen] == '/'));
        g_free (canonical);
        if (!inside) {
            g_free (full);
            return NULL;
        }
    }
    return full;
}

static gboolean
write_one_entry (struct archive *a,
                 struct archive_entry *ae,
                 const char *destdir,
                 GError **error)
{
    char *src_path;
    char *dest_path;
    mode_t mode;
    const void *buf;
    size_t buf_size;
    la_int64_t offset;
    int r;

    src_path = decode_pathname (ae);
    if (src_path == NULL) {
        return TRUE; /* skip silently */
    }
    dest_path = safe_join (destdir, src_path);
    g_free (src_path);
    if (dest_path == NULL) {
        g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME,
                             _("Refusing to extract an entry outside the destination."));
        return FALSE;
    }

    mode = (mode_t) archive_entry_mode (ae);

    if (archive_entry_filetype (ae) == AE_IFDIR) {
        if (g_mkdir_with_parents (dest_path, 0755) != 0 && errno != EEXIST) {
            g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errno),
                         "%s", g_strerror (errno));
            g_free (dest_path);
            return FALSE;
        }
        g_free (dest_path);
        return TRUE;
    }

    /* Ensure parent dir exists. */
    {
        char *parent = g_path_get_dirname (dest_path);
        if (g_mkdir_with_parents (parent, 0755) != 0 && errno != EEXIST) {
            g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errno),
                         "%s", g_strerror (errno));
            g_free (parent);
            g_free (dest_path);
            return FALSE;
        }
        g_free (parent);
    }

    if (archive_entry_filetype (ae) == AE_IFLNK) {
        const char *target = archive_entry_symlink (ae);
        if (target != NULL) {
            unlink (dest_path);
            if (symlink (target, dest_path) != 0) {
                g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errno),
                             "%s", g_strerror (errno));
                g_free (dest_path);
                return FALSE;
            }
        }
        g_free (dest_path);
        return TRUE;
    }

    if (archive_entry_filetype (ae) != AE_IFREG) {
        /* devices, fifos, sockets — skip */
        g_free (dest_path);
        return TRUE;
    }

    int fd = g_open (dest_path, O_WRONLY | O_CREAT | O_TRUNC,
                     (mode & 0777) ? (mode & 0777) : 0644);
    if (fd < 0) {
        g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errno),
                     "%s", g_strerror (errno));
        g_free (dest_path);
        return FALSE;
    }

    for (;;) {
        r = archive_read_data_block (a, &buf, &buf_size, &offset);
        if (r == ARCHIVE_EOF) {
            r = ARCHIVE_OK;
            break;
        }
        if (r < ARCHIVE_OK) {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                         "%s", archive_error_string (a));
            close (fd);
            g_free (dest_path);
            return FALSE;
        }
        if (lseek (fd, (off_t) offset, SEEK_SET) < 0) {
            g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errno),
                         "%s", g_strerror (errno));
            close (fd);
            g_free (dest_path);
            return FALSE;
        }
        const char *p = buf;
        size_t remaining = buf_size;
        while (remaining > 0) {
            ssize_t written = write (fd, p, remaining);
            if (written < 0) {
                if (errno == EINTR) continue;
                g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errno),
                             "%s", g_strerror (errno));
                close (fd);
                g_free (dest_path);
                return FALSE;
            }
            p += written;
            remaining -= (size_t) written;
        }
    }
    close (fd);
    g_free (dest_path);
    return TRUE;
}

static void
extract_thread (GTask        *task,
                gpointer      source_object,
                gpointer      task_data,
                GCancellable *cancellable)
{
    NemoArchive *self = NEMO_ARCHIVE (source_object);
    ExtractData *e = task_data;
    struct archive *a;
    struct archive_entry *ae;
    int r;

    if (g_mkdir_with_parents (e->destdir, 0755) != 0 && errno != EEXIST) {
        g_task_return_new_error (task, G_IO_ERROR, g_io_error_from_errno (errno),
                                 "%s", g_strerror (errno));
        return;
    }

    a = archive_read_new ();
    archive_read_support_format_all (a);
    archive_read_support_filter_all (a);

    if (archive_read_open_filename (a, self->path, BLOCK_SIZE) != ARCHIVE_OK) {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                 "%s", archive_error_string (a));
        archive_read_free (a);
        return;
    }

    while (!g_cancellable_is_cancelled (cancellable)) {
        char *path;
        gboolean want;
        GError *werr = NULL;

        r = archive_read_next_header (a, &ae);
        if (r == ARCHIVE_EOF) break;
        if (r < ARCHIVE_WARN) {
            g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                     "%s", archive_error_string (a));
            archive_read_free (a);
            return;
        }

        path = decode_pathname (ae);
        if (path == NULL) continue;

        {
            char *norm = normalize_entry_path (path);
            g_free (path);
            if (norm == NULL) continue;
            want = extract_wants (e->wanted, norm);
            g_free (norm);
        }
        if (!want) {
            archive_read_data_skip (a);
            continue;
        }

        if (!write_one_entry (a, ae, e->destdir, &werr)) {
            g_task_return_error (task, werr);
            archive_read_free (a);
            return;
        }
    }

    archive_read_free (a);

    if (g_cancellable_is_cancelled (cancellable)) {
        g_task_return_error_if_cancelled (task);
        return;
    }
    g_task_return_boolean (task, TRUE);
}

void
nemo_archive_extract_async (NemoArchive         *self,
                            GList               *entry_paths,
                            GFile               *destination,
                            GCancellable        *cancellable,
                            GAsyncReadyCallback  callback,
                            gpointer             user_data)
{
    GTask *task;
    ExtractData *e;
    char *dest_path;

    g_return_if_fail (NEMO_IS_ARCHIVE (self));
    g_return_if_fail (G_IS_FILE (destination));

    dest_path = g_file_get_path (destination);
    if (dest_path == NULL) {
        task = g_task_new (self, cancellable, callback, user_data);
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                                 _("Extraction destination must be on a native filesystem."));
        g_object_unref (task);
        return;
    }

    e = g_new0 (ExtractData, 1);
    e->destdir = g_canonicalize_filename (dest_path, NULL);
    g_free (dest_path);

    if (entry_paths != NULL) {
        e->wanted = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
        for (GList *l = entry_paths; l != NULL; l = l->next) {
            g_hash_table_insert (e->wanted, g_strdup ((const char *) l->data),
                                 GINT_TO_POINTER (1));
        }
    }

    task = g_task_new (self, cancellable, callback, user_data);
    g_task_set_source_tag (task, nemo_archive_extract_async);
    g_task_set_task_data (task, e, extract_data_free);
    g_task_run_in_thread (task, extract_thread);
    g_object_unref (task);
}

gboolean
nemo_archive_extract_finish (NemoArchive   *self,
                             GAsyncResult  *result,
                             GError       **error)
{
    g_return_val_if_fail (NEMO_IS_ARCHIVE (self), FALSE);
    g_return_val_if_fail (g_task_is_valid (result, self), FALSE);
    return g_task_propagate_boolean (G_TASK (result), error);
}
