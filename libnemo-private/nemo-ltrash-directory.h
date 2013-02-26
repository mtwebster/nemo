/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-
  
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
  
   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street - Suite 500,
   Boston, MA 02110-1335, USA.
  
*/

#ifndef NEMO_LTRASH_DIRECTORY_H
#define NEMO_LTRASH_DIRECTORY_H

#include <libnemo-private/nemo-vfs-directory.h>

#define NEMO_TYPE_LTRASH_DIRECTORY nemo_ltrash_directory_get_type()
#define NEMO_LTRASH_DIRECTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), NEMO_TYPE_LTRASH_DIRECTORY, NemoLTrashDirectory))
#define NEMO_LTRASH_DIRECTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), NEMO_TYPE_LTRASH_DIRECTORY, NemoLTrashDirectoryClass))
#define NEMO_IS_LTRASH_DIRECTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NEMO_TYPE_LTRASH_DIRECTORY))
#define NEMO_IS_LTRASH_DIRECTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), NEMO_TYPE_LTRASH_DIRECTORY))
#define NEMO_LTRASH_DIRECTORY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), NEMO_TYPE_LTRASH_DIRECTORY, NemoLTrashDirectoryClass))

typedef struct NemoLTrashDirectoryDetails NemoLTrashDirectoryDetails;

typedef struct {
	NemoVFSDirectory parent_slot;
} NemoLTrashDirectory;

typedef struct {
	NemoVFSDirectoryClass parent_slot;
} NemoLTrashDirectoryClass;

GType   nemo_ltrash_directory_get_type (void);

#endif /* NEMO_LTRASH_DIRECTORY_H */
