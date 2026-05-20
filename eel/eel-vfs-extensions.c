/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* eel-vfs-extensions.c - gnome-vfs extensions.  Its likely some of these will
                          be part of gnome-vfs in the future.

   Copyright (C) 1999, 2000 Eazel, Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street - Suite 500,
   Boston, MA 02110-1335, USA.

   Authors: Darin Adler <darin@eazel.com>
	    Pavel Cisler <pavel@eazel.com>
	    Mike Fleming  <mfleming@eazel.com>
            John Sullivan <sullivan@eazel.com>
*/

#include <config.h>
#include "eel-vfs-extensions.h"
#include "eel-glib-extensions.h"
#include "eel-lib-self-check-functions.h"

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>

#include "eel-string.h"

#include <string.h>
#include <stdlib.h>

gboolean
eel_uri_is_trash (const char *uri)
{
	return g_str_has_prefix (uri, "trash:");
}

gboolean
eel_uri_is_recent (const char *uri)
{
	return g_str_has_prefix (uri, "recent:");
}

gboolean
eel_uri_is_favorite (const char *uri)
{
    return g_str_has_prefix (uri, "favorites:");
}

gboolean
eel_uri_is_search (const char *uri)
{
	return g_str_has_prefix (uri, EEL_SEARCH_URI);
}

gboolean
eel_uri_is_desktop (const char *uri)
{
	return g_str_has_prefix (uri, EEL_DESKTOP_URI);
}

gboolean
eel_uri_is_network (const char *uri)
{
    return g_str_has_prefix (uri, "smb:") || g_str_has_prefix (uri, "network:");
}

gboolean
eel_uri_is_computer (const char *uri)
{
    return g_str_has_prefix (uri, "computer:");
}

gboolean
eel_uri_is_archive (const char *uri)
{
	return g_str_has_prefix (uri, EEL_ARCHIVE_URI);
}

/* Builds x-nemo-archive://<escaped-archive-path>/<inside-path>
 *
 * Putting the archive path in the URI authority (host) component lets GIO's
 * g_file_get_parent() walk inside-paths naturally without crossing the
 * archive boundary, and yields parent=NULL at the archive root for the
 * self-owned-file semantics nemo_file_get_internal expects.
 *
 * archive_path must be an absolute filesystem path. inside may be NULL or "" for
 * the archive root; otherwise it's a relative path inside the archive (no
 * leading slash).
 */
char *
eel_archive_uri_new (const char *archive_path,
		     const char *inside)
{
	char *escaped_path;
	char *escaped_inside;
	char *uri;

	g_return_val_if_fail (archive_path != NULL, NULL);
	g_return_val_if_fail (archive_path[0] == '/', NULL);

	/* Escape every character that's special in URI hosts/paths so the
	 * archive path is opaque to GIO's parser. The empty allow list ensures
	 * '/' becomes %2F. */
	escaped_path = g_uri_escape_string (archive_path, NULL, FALSE);

	if (inside == NULL || inside[0] == '\0') {
		uri = g_strconcat (EEL_ARCHIVE_URI, "//", escaped_path, "/", NULL);
	} else {
		const char *trimmed = inside;
		while (*trimmed == '/') trimmed++;
		escaped_inside = g_uri_escape_string (trimmed, "/", FALSE);
		uri = g_strconcat (EEL_ARCHIVE_URI, "//", escaped_path, "/",
				   escaped_inside, NULL);
		g_free (escaped_inside);
	}

	g_free (escaped_path);
	return uri;
}

/* Splits an x-nemo-archive: URI back into archive path and inside path.
 * Both out params receive newly-allocated strings the caller must free.
 * *inside is set to g_strdup("") for archive-root URIs (never NULL on success).
 * Returns FALSE if uri is not a well-formed archive URI.
 */
gboolean
eel_archive_uri_parse (const char  *uri,
		       char       **archive_path,
		       char       **inside)
{
	const char *after_scheme;
	const char *host_start;
	const char *path_start;
	char *host_part;
	char *path_part;
	gsize host_len;

	if (archive_path != NULL) {
		*archive_path = NULL;
	}
	if (inside != NULL) {
		*inside = NULL;
	}

	if (uri == NULL || !eel_uri_is_archive (uri)) {
		return FALSE;
	}

	after_scheme = uri + strlen (EEL_ARCHIVE_URI);
	if (after_scheme[0] != '/' || after_scheme[1] != '/') {
		return FALSE;
	}
	host_start = after_scheme + 2;

	path_start = strchr (host_start, '/');
	if (path_start != NULL) {
		host_len = (gsize) (path_start - host_start);
		path_part = g_uri_unescape_string (path_start + 1, NULL);
	} else {
		host_len = strlen (host_start);
		path_part = g_strdup ("");
	}

	if (host_len == 0) {
		g_free (path_part);
		return FALSE;
	}

	host_part = g_uri_unescape_segment (host_start, host_start + host_len, NULL);
	if (host_part == NULL || host_part[0] != '/') {
		g_free (host_part);
		g_free (path_part);
		return FALSE;
	}

	/* Trim trailing slashes from inside path; archive root is "". */
	{
		gsize plen = strlen (path_part);
		while (plen > 0 && path_part[plen - 1] == '/') {
			path_part[--plen] = '\0';
		}
	}

	if (archive_path != NULL) {
		*archive_path = host_part;
	} else {
		g_free (host_part);
	}
	if (inside != NULL) {
		*inside = path_part;
	} else {
		g_free (path_part);
	}
	return TRUE;
}

gboolean
eel_vfs_supports_uri_scheme (const gchar *scheme)
{
   const gchar * const *supported;
   gint i;

   supported = g_vfs_get_supported_uri_schemes (g_vfs_get_default ());

   for (i = 0; supported[i] != NULL; i++) {
      if (g_strcmp0 (scheme, supported[i]) == 0) {
          return TRUE;
      }
   }

   return FALSE;
}

char *
eel_make_valid_utf8 (const char *name)
{
	GString *string;
	const char *remainder, *invalid;
	int remaining_bytes, valid_bytes;

	string = NULL;
	remainder = name;
	remaining_bytes = strlen (name);

	while (remaining_bytes != 0) {
		if (g_utf8_validate (remainder, remaining_bytes, &invalid)) {
			break;
		}
		valid_bytes = invalid - remainder;

		if (string == NULL) {
			string = g_string_sized_new (remaining_bytes);
		}
		g_string_append_len (string, remainder, valid_bytes);
		g_string_append_c (string, '?');

		remaining_bytes -= valid_bytes + 1;
		remainder = invalid + 1;
	}

	if (string == NULL) {
		return g_strdup (name);
	}

	g_string_append (string, remainder);
	g_string_append (string, _(" (invalid Unicode)"));
	g_assert (g_utf8_validate (string->str, -1, NULL));

	return g_string_free (string, FALSE);
}

char *
eel_filename_get_extension_offset (const char *filename)
{
	char *end, *end2;
	const char *start;

	if (filename == NULL || filename[0] == '\0') {
		return NULL;
	}

	/* basename must have at least one char */
	start = filename + 1;

	end = strrchr (start, '.');
	if (end == NULL || end[1] == '\0') {
		return NULL;
	}

	if (end != start) {
		if (strcmp (end, ".gz") == 0 ||
		    strcmp (end, ".bz2") == 0 ||
		    strcmp (end, ".sit") == 0 ||
                    strcmp (end, ".bz") == 0 ||
                    strcmp (end, ".xz") == 0 ||
		    strcmp (end, ".Z") == 0) {
			end2 = end - 1;
			while (end2 > start &&
			       *end2 != '.') {
				end2--;
			}
			if (end2 != start) {
				end = end2;
			}
		}
	}

	return end;
}

char *
eel_filename_strip_extension (const char * filename_with_extension)
{
	char *filename, *end;

	if (filename_with_extension == NULL) {
		return NULL;
	}

	filename = g_strdup (filename_with_extension);
	end = eel_filename_get_extension_offset (filename);

	if (end && end != filename) {
		*end = '\0';
	}

	return filename;
}

void
eel_filename_get_rename_region (const char           *filename,
				int                  *start_offset,
				int                  *end_offset)
{
	char *filename_without_extension;

	g_return_if_fail (start_offset != NULL);
	g_return_if_fail (end_offset != NULL);

	*start_offset = 0;
	*end_offset = 0;

	g_return_if_fail (filename != NULL);

	filename_without_extension = eel_filename_strip_extension (filename);
	*end_offset = g_utf8_strlen (filename_without_extension, -1);

	g_free (filename_without_extension);
}
