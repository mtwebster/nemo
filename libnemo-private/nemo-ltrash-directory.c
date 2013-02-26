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

#include <config.h>
#include "nemo-ltrash-directory.h"

#include "nemo-directory-private.h"
#include "nemo-file-private.h"

G_DEFINE_TYPE (NemoLTrashDirectory, nemo_ltrash_directory, NEMO_TYPE_VFS_DIRECTORY);

static void
nemo_ltrash_directory_init (NemoLTrashDirectory *directory)
{
    g_printerr ("hey now trash init\n");
}

static gboolean
ltrash_contains_file (NemoDirectory *directory,
		   NemoFile *file)
{
	g_assert (NEMO_IS_LTRASH_DIRECTORY (directory));
	g_assert (NEMO_IS_FILE (file));

	return file->details->directory == directory;
}

static void
ltrash_call_when_ready (NemoDirectory *directory,
		     NemoFileAttributes file_attributes,
		     gboolean wait_for_file_list,
		     NemoDirectoryCallback callback,
		     gpointer callback_data)
{
	g_assert (NEMO_IS_LTRASH_DIRECTORY (directory));

	nemo_directory_call_when_ready_internal
		(directory,
		 NULL,
		 file_attributes,
		 wait_for_file_list,
		 callback,
		 NULL,
		 callback_data);
}

static void
ltrash_cancel_callback (NemoDirectory *directory,
		     NemoDirectoryCallback callback,
		     gpointer callback_data)
{
	g_assert (NEMO_IS_LTRASH_DIRECTORY (directory));

	nemo_directory_cancel_callback_internal
		(directory,
		 NULL,
		 callback,
		 NULL,
		 callback_data);
}

static void
ltrash_file_monitor_add (NemoDirectory *directory,
		      gconstpointer client,
		      gboolean monitor_hidden_files,
		      NemoFileAttributes file_attributes,
		      NemoDirectoryCallback callback,
		      gpointer callback_data)
{
	g_assert (NEMO_IS_LTRASH_DIRECTORY (directory));
	g_assert (client != NULL);

	nemo_directory_monitor_add_internal
		(directory, NULL,
		 client,
		 monitor_hidden_files,
		 file_attributes,
		 callback, callback_data);
}

static void
ltrash_file_monitor_remove (NemoDirectory *directory,
			 gconstpointer client)
{
	g_assert (NEMO_IS_LTRASH_DIRECTORY (directory));
	g_assert (client != NULL);
	
	nemo_directory_monitor_remove_internal (directory, NULL, client);
}

static void
ltrash_force_reload (NemoDirectory *directory)
{
	NemoFileAttributes all_attributes;

	g_assert (NEMO_IS_DIRECTORY (directory));

	all_attributes = nemo_file_get_all_attributes ();
	nemo_directory_force_reload_internal (directory,
						  all_attributes);
}

static gboolean
ltrash_are_all_files_seen (NemoDirectory *directory)
{
	g_assert (NEMO_IS_LTRASH_DIRECTORY (directory));
	
	return directory->details->directory_loaded;
}

static gboolean
ltrash_is_not_empty (NemoDirectory *directory)
{
	g_assert (NEMO_IS_LTRASH_DIRECTORY (directory));
	g_assert (nemo_directory_is_anyone_monitoring_file_list (directory));

	return directory->details->file_list != NULL;
}

static void
nemo_ltrash_directory_class_init (NemoLTrashDirectoryClass *klass)
{
	NemoDirectoryClass *directory_class = NEMO_VFS_DIRECTORY_CLASS (klass);

	directory_class->contains_file = ltrash_contains_file;
	directory_class->call_when_ready = ltrash_call_when_ready;
	directory_class->cancel_callback = ltrash_cancel_callback;
	directory_class->file_monitor_add = ltrash_file_monitor_add;
	directory_class->file_monitor_remove = ltrash_file_monitor_remove;
	directory_class->force_reload = ltrash_force_reload;
	directory_class->are_all_files_seen = ltrash_are_all_files_seen;
	directory_class->is_not_empty = ltrash_is_not_empty;
}
