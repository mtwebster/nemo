/* nemo-archive.h: libarchive-backed model of a single archive file.
 *
 * Owns a parsed list of entries; emits entries incrementally as they're
 * discovered. One instance per absolute archive path; instances are cached
 * via a weak-ref table so callers share the same parse.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef NEMO_ARCHIVE_H
#define NEMO_ARCHIVE_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define NEMO_TYPE_ARCHIVE nemo_archive_get_type ()
G_DECLARE_FINAL_TYPE (NemoArchive, nemo_archive, NEMO, ARCHIVE, GObject)

/* Read-only view of one archive entry. Lifetime is tied to the owning
 * NemoArchive — do not free, do not retain past the archive's lifetime. */
typedef struct {
    char    *full_path;     /* normalized, no leading slash, no "./" */
    guint64  size;
    gint64   mtime;         /* seconds since epoch; -1 if unknown */
    guint32  mode;          /* unix mode bits (POSIX); 0 if unknown */
    char    *content_type;  /* guessed from filename; "inode/directory" for dirs */
    gboolean is_dir;
} NemoArchiveEntry;

/* Look up by absolute filesystem path. Returns a referenced instance — the
 * same one will be returned for the same path as long as another reference
 * keeps it alive. NULL if archive_path is invalid. */
NemoArchive *nemo_archive_get_or_create (const char *archive_path);

const char  *nemo_archive_get_path       (NemoArchive *self);

/* Kick off (or attach to in-progress) listing. The async result carries no
 * data; entries arrive via the "entries-added" and "done-loading" signals.
 * Safe to call repeatedly — subsequent calls share the same parse. */
void     nemo_archive_list_async  (NemoArchive         *self,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data);
gboolean nemo_archive_list_finish (NemoArchive   *self,
                                   GAsyncResult  *result,
                                   GError       **error);

/* Once listed, returns whether listing completed successfully. */
gboolean nemo_archive_is_loaded     (NemoArchive *self);
gboolean nemo_archive_load_failed   (NemoArchive *self);
gboolean nemo_archive_is_encrypted  (NemoArchive *self);

/* Borrowed pointers to all entries (already-emitted ones). Don't free, don't
 * retain past the archive's lifetime. */
const GPtrArray *nemo_archive_get_entries (NemoArchive *self);

/* Borrowed pointers to entries that are immediate children of inside_prefix
 * (no further '/'). Caller owns the GList but not the entries.
 * inside_prefix may be NULL or "" for the archive root. */
GList *nemo_archive_get_children (NemoArchive *self,
                                  const char  *inside_prefix);

/* Extract a subset of entries (by full_path) to destination directory.
 * entry_paths NULL/empty means "extract everything". Paths in entry_paths
 * are gchar* strings owned by the caller; not freed. */
void     nemo_archive_extract_async  (NemoArchive         *self,
                                      GList               *entry_paths,
                                      GFile               *destination,
                                      GCancellable        *cancellable,
                                      GAsyncReadyCallback  callback,
                                      gpointer             user_data);
gboolean nemo_archive_extract_finish (NemoArchive   *self,
                                      GAsyncResult  *result,
                                      GError       **error);

/* Hint list: returns TRUE if mime_type is plausibly an archive supported by
 * libarchive. Does not open the file — call nemo_archive_probe_path() for
 * a definitive answer. */
gboolean nemo_archive_mime_type_is_candidate (const char *mime_type);

/* Cheap on-disk probe: opens the file with libarchive just far enough to
 * identify a known format. Blocks briefly (header read). Returns FALSE on
 * non-native paths, non-existent files, or unsupported formats. */
gboolean nemo_archive_probe_path (const char *path);

G_END_DECLS

#endif /* NEMO_ARCHIVE_H */
