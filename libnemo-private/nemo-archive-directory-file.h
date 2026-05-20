/* nemo-archive-directory-file.h: NemoFile subclass for archive entries.
 *
 * Represents either the archive itself (self-owned) or an entry inside it.
 * Both forms hold synthetic GFileInfo; NemoArchiveDirectory populates them.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef NEMO_ARCHIVE_DIRECTORY_FILE_H
#define NEMO_ARCHIVE_DIRECTORY_FILE_H

#include <libnemo-private/nemo-file.h>

#define NEMO_TYPE_ARCHIVE_DIRECTORY_FILE nemo_archive_directory_file_get_type ()
#define NEMO_ARCHIVE_DIRECTORY_FILE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), NEMO_TYPE_ARCHIVE_DIRECTORY_FILE, NemoArchiveDirectoryFile))
#define NEMO_ARCHIVE_DIRECTORY_FILE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), NEMO_TYPE_ARCHIVE_DIRECTORY_FILE, NemoArchiveDirectoryFileClass))
#define NEMO_IS_ARCHIVE_DIRECTORY_FILE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NEMO_TYPE_ARCHIVE_DIRECTORY_FILE))
#define NEMO_IS_ARCHIVE_DIRECTORY_FILE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), NEMO_TYPE_ARCHIVE_DIRECTORY_FILE))
#define NEMO_ARCHIVE_DIRECTORY_FILE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), NEMO_TYPE_ARCHIVE_DIRECTORY_FILE, NemoArchiveDirectoryFileClass))

typedef struct {
    NemoFile parent_slot;
} NemoArchiveDirectoryFile;

typedef struct {
    NemoFileClass parent_slot;
} NemoArchiveDirectoryFileClass;

GType nemo_archive_directory_file_get_type (void);

#endif /* NEMO_ARCHIVE_DIRECTORY_FILE_H */
