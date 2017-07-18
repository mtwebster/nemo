/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   nemo-thumbnails.h: Thumbnail code for icon factory.
 
   Copyright (C) 2000, 2001 Eazel, Inc.
   Copyright (C) 2002, 2003 Red Hat, Inc.
  
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
  
   Author: Andy Hertzfeld <andy@eazel.com>
*/

#include <config.h>
#include "nemo-thumbnails.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API

#include "nemo-directory-notify.h"
#include "nemo-global-preferences.h"
#include "nemo-file-utilities.h"
#include <eel/eel-graphic-effects.h>
#include <gtk/gtk.h>

#define DEBUG_FLAG NEMO_DEBUG_PIXBUF_CACHE
#include <libnemo-private/nemo-debug.h>

#include "nemo-file-private.h"
#include "nemo-directory-private.h"
#include "nemo-icon-info.h"
#include "nemo-pixbuf-cache.h"

#define BATCH_SIZE 100

#define NEMO_THUMBNAIL_FRAME_LEFT 3
#define NEMO_THUMBNAIL_FRAME_TOP 3
#define NEMO_THUMBNAIL_FRAME_RIGHT 3
#define NEMO_THUMBNAIL_FRAME_BOTTOM 3

/* structure used for generating pixbufs */

typedef struct {
    gchar            *file_uri;
    gchar            *thumbnail_path;
    gint              size;
    gint              max_width;
    gint              scale;
    NemoFileIconFlags flags;
    gchar            *hash_key;
} NemoPixbufInfo;

/*
 * Thumbnail thread state.
 */

/* The id of the idle handler used to start the thumbnail thread, or 0 if no
   idle handler is currently registered. */
static guint pixbuf_thread_starter_id = 0;
static GCancellable *pixbufs_cancellable;

/* Our mutex used when accessing data shared between the main thread and the
   thumbnail thread, i.e. the thumbnail_thread_is_running flag and the
   thumbnails_to_make list. */
static GMutex pixbuf_thread_mutex;

static int batch_count = 0;
/* A flag to indicate whether a thumbnail thread is running, so we don't
   start more than one. Lock thumbnails_mutex when accessing this. */
static volatile gboolean pixbuf_thread_is_running = FALSE;
/* The list of NemoThumbnailInfo structs containing information about the
   thumbnails we are making. Lock thumbnails_mutex when accessing this. */
static volatile GQueue pixbufs_to_make = G_QUEUE_INIT;
/* Quickly check if uri is in thumbnails_to_make list */
static GHashTable *pixbufs_to_make_hash = NULL;

/* The currently thumbnailed icon. it also exists in the thumbnails_to_make list
 * to avoid adding it again. Lock thumbnails_mutex when accessing this. */
static NemoPixbufInfo *currently_loading = NULL;

/* cache of loaded pixbufs, with the keys encoded to be able to lookup an icon
 * given the file uri and the timestamp of the original pixbuf request.
 *
 *  URI:::size:::max_width:::scale:::flags
 * "file:///home/mtwebster/.face:::48:::100:::1:::12354564321"
 */
static GMutex pixbuf_cache_mutex;
static GHashTable *pixbuf_cache;

extern int cached_thumbnail_size;

static gboolean pixbuf_thread_starter_cb (gpointer data);

static void
free_pixbuf_info (NemoPixbufInfo *info)
{
	g_free (info->file_uri);
    g_free (info->hash_key);
    g_free (info->thumbnail_path);
	g_slice_free (NemoPixbufInfo, info);
}

static GdkPixbuf *
nemo_get_thumbnail_frame (void)
{
	static GdkPixbuf *thumbnail_frame = NULL;

	if (thumbnail_frame == NULL) {
		GInputStream *stream = g_resources_open_stream ("/org/nemo/icons/thumbnail_frame.png", 0, NULL);
		if (stream != NULL) {
			thumbnail_frame = gdk_pixbuf_new_from_stream (stream, NULL, NULL);
			g_object_unref (stream);
		}
	}
	
	return thumbnail_frame;
}

static void
frame_image (GdkPixbuf **pixbuf)
{
	GdkPixbuf *pixbuf_with_frame, *frame;
	int left_offset, top_offset, right_offset, bottom_offset;
		
	/* The pixbuf isn't already framed (i.e., it was not made by
	 * an old Nemo), so we must embed it in a frame.
	 */

	frame = nemo_get_thumbnail_frame ();
	if (frame == NULL) {
		return;
	}
	
	left_offset = NEMO_THUMBNAIL_FRAME_LEFT;
	top_offset = NEMO_THUMBNAIL_FRAME_TOP;
	right_offset = NEMO_THUMBNAIL_FRAME_RIGHT;
	bottom_offset = NEMO_THUMBNAIL_FRAME_BOTTOM;
	
	pixbuf_with_frame = eel_embed_image_in_frame
		(*pixbuf, frame,
		 left_offset, top_offset, right_offset, bottom_offset);
	g_object_unref (*pixbuf);	

	*pixbuf = pixbuf_with_frame;
}

static void
pad_top_and_bottom (GdkPixbuf **pixbuf,
                    gint        extra_height)
{
    GdkPixbuf *pixbuf_with_padding;
    GdkRectangle rect;
    GdkRGBA transparent = { 0, 0, 0, 0.0 };
    cairo_surface_t *surface;
    cairo_t *cr;
    gint width, height;

    width = gdk_pixbuf_get_width (*pixbuf);
    height = gdk_pixbuf_get_height (*pixbuf);

    surface = gdk_window_create_similar_image_surface (NULL,
                                                       CAIRO_FORMAT_ARGB32,
                                                       width,
                                                       height + extra_height,
                                                       0);

    cr = cairo_create (surface);

    rect.x = 0;
    rect.y = 0;
    rect.width = width;
    rect.height = height + extra_height;

    gdk_cairo_rectangle (cr, &rect);
    gdk_cairo_set_source_rgba (cr, &transparent);
    cairo_fill (cr);

    gdk_cairo_set_source_pixbuf (cr,
                                 *pixbuf,
                                 0,
                                 extra_height / 2);
    cairo_paint (cr);

    pixbuf_with_padding = gdk_pixbuf_get_from_surface (surface,
                                                       0,
                                                       0,
                                                       width,
                                                       height + extra_height);

    g_object_unref (*pixbuf);
    cairo_surface_destroy (surface);
    cairo_destroy (cr);

    *pixbuf = pixbuf_with_padding;
}

/***************************************************************************
 * Thumbnail Thread Functions.
 ***************************************************************************/

// void
// nemo_thumbnail_remove_from_queue (const char *file_uri)
// {
//     GList *node;

//     if (DEBUGGING) {
//         g_message ("(Remove from queue) Locking thread mutex");
//     }

//     g_mutex_lock (&thumbnails_mutex);

//     /*********************************
//      * MUTEX LOCKED
//      *********************************/

//     if (thumbnails_to_make_hash) {
//         node = g_hash_table_lookup (thumbnails_to_make_hash, file_uri);
        
//         if (node && node->data != currently_thumbnailing) {
//             g_hash_table_remove (thumbnails_to_make_hash, file_uri);
//             free_thumbnail_info (node->data);
//             g_queue_delete_link ((GQueue *)&thumbnails_to_make, node);
//         }
//     }

//     /*********************************
//      * MUTEX UNLOCKED
//      *********************************/

//     if (DEBUGGING) {
//         g_message ("(Remove from queue) Unlocking thread mutex");
//     }

//     g_mutex_unlock (&thumbnails_mutex);
// }

// void
// nemo_thumbnail_prioritize (const char *file_uri)
// {
//     GList *node;

//     if (DEBUGGING) {
//         g_message ("(Prioritize) Locking thread mutex");
//     }

//     g_mutex_lock (&thumbnails_mutex);

//     /*********************************
//      * MUTEX LOCKED
//      *********************************/

//     if (thumbnails_to_make_hash) {
//         node = g_hash_table_lookup (thumbnails_to_make_hash, file_uri);

//         if (node && node->data != currently_thumbnailing) {
//             g_queue_unlink ((GQueue *)&thumbnails_to_make, node);
//             g_queue_push_head_link ((GQueue *)&thumbnails_to_make, node);
//         }
//     }
    
//     /*********************************
//      * MUTEX UNLOCKED
//      *********************************/
    
//     if (DEBUGGING) {
//         g_message ("(Prioritize) Unlocking thread mutex");
//     }

//     g_mutex_unlock (&thumbnails_mutex);
// }

/* This is a one-shot idle callback called from the main loop to call
   notify_file_changed() for a thumbnail. It frees the uri afterwards.
   We do this in an idle callback as I don't think nemo_file_changed() is
   thread-safe. */
static gboolean
pixbuf_thread_notify_file_changed (gpointer file_uri)
{
    NemoFile *file;

    file = nemo_file_get_by_uri ((char *) file_uri);

    if (DEBUGGING) {
        g_message ("(Pixbuf Thread) Notifying file changed file:%p uri: %s", file, (char*) file_uri);
    }

    if (file != NULL) {
        GList *files;

        files = g_list_prepend (NULL, file);
        nemo_directory_emit_files_changed (file->details->directory, files);
        g_list_free (files);
    }

    g_free (file_uri);

    return FALSE;
}

static void
on_pixbuf_thread_finished (GObject      *source,
                           GAsyncResult *res,
                           gpointer      user_data)
{
    gboolean success;
    GError *error;

    error = NULL;
    success = g_task_propagate_boolean (G_TASK (res), &error);

    if (error != NULL) {
        g_warning ("Error with pixbuf thread: %s", error->message);
        g_error_free (error);
    }

    if (DEBUGGING) {
        g_message ("(Main Thread) pixbuf thread finished");
    }

    /* Thread is no longer running, no need to lock mutex */
    pixbuf_thread_is_running = FALSE;

    batch_count = 0;

    if (g_hash_table_size (pixbufs_to_make_hash) > 0) {
        pixbuf_thread_starter_id = g_idle_add_full (G_PRIORITY_LOW, pixbuf_thread_starter_cb, NULL, NULL);
    }
}

static void
original_finalized_notify (gpointer  user_data,
                           GObject  *object)
{
    GdkPixbuf *scaled_pixbuf;
    gchar *lookup_key;

    lookup_key = (gchar *) user_data;

    if (DEBUGGING) {
        g_message ("(Main Thread) notify original pixbuf finalized, unref scaled copies of: %s", user_data);
    }

    g_mutex_lock (&pixbuf_cache_mutex);

    scaled_pixbuf = g_hash_table_lookup (pixbuf_cache, lookup_key);

    g_assert (scaled_pixbuf != NULL);

    g_object_unref (scaled_pixbuf);
    g_hash_table_remove (pixbuf_cache, lookup_key);

    g_mutex_unlock (&pixbuf_cache_mutex);

    g_free (lookup_key);
}

/* pixbuf_thread is invoked as a separate thread to load pixbufs. */
static void
pixbuf_thread (GTask        *task,
               gpointer      source_object,
               gpointer      task_data,
               GCancellable *cancellable)
{
    NemoPixbufInfo *info;
    GdkPixbuf *raw_pixbuf, *scaled_pixbuf;
    GList *node;
    gchar *key_copy;
    GError *error;

    info = NULL;

	/* We loop until there are no more pixbufs to make, at which point
	   we exit the thread. */
	while (batch_count < BATCH_SIZE) {
        if (DEBUGGING) {
            g_message ("(Pixbuf Thread) Locking thread mutex");
        }

        g_mutex_lock (&pixbuf_thread_mutex);

		/*********************************
		 * MUTEX LOCKED
		 *********************************/

		/* Pop the last thumbnail we just made off the head of the
		   list and free it. I did this here so we only have to lock
		   the mutex once per thumbnail, rather than once before
		   creating it and once after.
		*/
        if (currently_loading) {
            g_assert (info == currently_loading);

            node = g_hash_table_lookup (pixbufs_to_make_hash, info->hash_key);

            g_assert (node != NULL);

            g_hash_table_remove (pixbufs_to_make_hash, info->hash_key);
            free_pixbuf_info (info);
            g_queue_delete_link ((GQueue *)&pixbufs_to_make, node);
        }

        currently_loading = NULL;

		/* If there are no more pixbufs to make, unlock the mutex, and
		   exit the thread. */
		if (g_queue_is_empty ((GQueue *)&pixbufs_to_make)) {
            if (DEBUGGING) {
                g_message ("(Pixbuf Thread) Exiting");
            }

            g_mutex_unlock (&pixbuf_thread_mutex);
            g_task_return_boolean (task, TRUE);
            return;
		}

		/* Get the next one to make. We leave it on the list until it
		   is created so the main thread doesn't add it again while we
		   are creating it. */
		info = g_queue_peek_head ((GQueue *)&pixbufs_to_make);
		currently_loading = info;

		/*********************************
		 * MUTEX UNLOCKED
		 *********************************/

        if (DEBUGGING) {
            g_message ("(Pixbuf Thread) Unlocking thread mutex");
        }

        g_mutex_unlock (&pixbuf_thread_mutex);

		/* Create the thumbnail. */
        if (DEBUGGING) {
            g_message ("(Pixbuf Thread) Loading pixbuf: %s",
                       info->file_uri);
        }

        int w, h, s, modified_size;
        double thumb_scale;

        if (info->flags & NEMO_FILE_ICON_FLAGS_FORCE_THUMBNAIL_SIZE) {
            modified_size = info->size * info->scale;
        } else {
            modified_size = info->size * info->scale * cached_thumbnail_size / NEMO_ICON_SIZE_STANDARD;
            DEBUG ("Modifying icon size to %d, as our cached thumbnail size is %d",
                   modified_size, cached_thumbnail_size);
        }

        error = NULL;

        raw_pixbuf = gdk_pixbuf_new_from_file (info->thumbnail_path, &error);

        if (error != NULL) {
            g_warning ("Could not load pixbuf from file: %s", error->message);
            g_clear_error (&error);
            scaled_pixbuf = raw_pixbuf;
            goto done;
        }

        if (raw_pixbuf) {
            GdkPixbuf *tmp;

            tmp = gdk_pixbuf_apply_embedded_orientation (raw_pixbuf);
            g_object_unref (raw_pixbuf);
            
            raw_pixbuf = tmp;
        }

        /* Don't scale up if more than 25%, then read the original
           image instead. We don't want to compare to exactly 100%,
           since the zoom level 150% gives thumbnails at 144, which is
           ok to scale up from 128. */
        if (modified_size > 128 * 1.25 * info->scale) {
            scaled_pixbuf = g_object_ref (raw_pixbuf);
            goto done;
        }

        w = gdk_pixbuf_get_width (raw_pixbuf);
        h = gdk_pixbuf_get_height (raw_pixbuf);

        if (info->flags & NEMO_FILE_ICON_FLAGS_PIN_HEIGHT_FOR_DESKTOP) {
            g_assert (info->max_width > 0);

            thumb_scale = (gdouble) modified_size / h;

            if (w * thumb_scale > info->max_width) {
                thumb_scale = (gdouble) info->max_width / w;
            }

            s = thumb_scale * h;
        } else {
            s = MAX (w, h);         
            /* Don't scale up small thumbnails in the standard view */
            if (s <= cached_thumbnail_size) {
                thumb_scale = (double) info->size / NEMO_ICON_SIZE_STANDARD;
            }
            else {
                thumb_scale = (double)modified_size / s;
            }
            /* Make sure that icons don't get smaller than NEMO_ICON_SIZE_SMALLEST */
            if (s * thumb_scale <= NEMO_ICON_SIZE_SMALLEST) {
                thumb_scale = (double) NEMO_ICON_SIZE_SMALLEST / s;
            }
        }

        scaled_pixbuf = gdk_pixbuf_scale_simple (raw_pixbuf,
                                                 MAX (w * thumb_scale, 1),
                                                 MAX (h * thumb_scale, 1),
                                                 GDK_INTERP_BILINEAR);

        /* We don't want frames around small icons */
        if (!gdk_pixbuf_get_has_alpha (raw_pixbuf) || s >= 128 * info->scale) {
            frame_image (&scaled_pixbuf);
        }

        if (info->flags & NEMO_FILE_ICON_FLAGS_PIN_HEIGHT_FOR_DESKTOP) {
            gint check_height;

            check_height = gdk_pixbuf_get_height (scaled_pixbuf);

            if (check_height < info->size) {
                pad_top_and_bottom (&scaled_pixbuf, info->size - check_height);
            }
        }

        g_object_unref (raw_pixbuf);

done:
        key_copy = g_strdup (info->hash_key);

        // g_object_weak_ref (G_OBJECT (info->original),
        //                    (GWeakNotify) original_finalized_notify,
        //                    key_copy);

        if (DEBUGGING) {
            g_message ("(Pixbuf Thread) Locking cache mutex. Adding %s to cache.", key_copy);
        }

        g_mutex_lock (&pixbuf_cache_mutex);

        g_hash_table_insert (pixbuf_cache, key_copy, scaled_pixbuf);

        if (DEBUGGING) {
            g_message ("(Pixbuf Thread) Unlocking cache mutex");
        }

        g_mutex_unlock (&pixbuf_cache_mutex);

		/* We need to call nemo_file_changed(), but I don't think that is
		   thread safe. So add an idle handler and do it from the main loop. */
        g_idle_add_full (G_PRIORITY_DEFAULT,
                         pixbuf_thread_notify_file_changed,
                         g_strdup (info->file_uri),
                         NULL);

        batch_count++;
    }

    g_task_return_boolean (task, TRUE);
}

/* This function is added as a very low priority idle function to start the
   thread to create any needed thumbnails. It is added with a very low priority
   so that it doesn't delay showing the directory in the icon/list views.
   We want to show the files in the directory as quickly as possible. */
static gboolean
pixbuf_thread_starter_cb (gpointer data)
{
    GTask *pixbuf_task;

    pixbufs_cancellable = g_cancellable_new ();

    if (DEBUGGING) {
        g_message ("(Main Thread) Creating pixbuf thread");
    }

    batch_count = 0;
    currently_loading = NULL;

    pixbuf_task = g_task_new (NULL,
                              pixbufs_cancellable,
                              on_pixbuf_thread_finished,
                              NULL);

    /* We set a flag to indicate the thread is running, so we don't create
       a new one. We don't need to lock a mutex here, as the thumbnail
       thread isn't running yet. And we know we won't create the thread
       twice, as we also check thumbnail_thread_starter_id before
       scheduling this idle function. */
    pixbuf_thread_is_running = TRUE;

    g_task_run_in_thread (pixbuf_task, pixbuf_thread);

    pixbuf_thread_starter_id = 0;
    return FALSE;
}

static gboolean
init_tables_func (gpointer user_data)
{
    if (pixbufs_to_make_hash == NULL) {
        pixbufs_to_make_hash = g_hash_table_new (g_str_hash,
                                                 g_str_equal);
    }

    if (pixbuf_cache == NULL) {
        pixbuf_cache = g_hash_table_new (g_str_hash,
                                         g_str_equal);
    }

    return TRUE;
}

GdkPixbuf *
nemo_pixbuf_cache_request_pixbuf_for_file (NemoFile         *file,
                                           int               size,
                                           int               max_width,
                                           int               scale,
                                           NemoFileIconFlags flags)
{
    static GOnce init_tables = G_ONCE_INIT;
    gchar *file_uri;
    gchar *lookup_key;
    gpointer existing;

    file_uri = nemo_file_get_uri (file);
    lookup_key = g_strdup_printf ("%s::%d::%d::%d::%d", file_uri, size, max_width, scale, flags);

    g_once (&init_tables, (GThreadFunc) init_tables_func, NULL);

    if (DEBUGGING) {
        g_message ("(Main Thread) Locking cache mutex.  Looking up: %s", lookup_key);
    }

    g_mutex_lock (&pixbuf_cache_mutex);

    /*********************************
     * MUTEX LOCKED
     *********************************/

    /* Check if it is already in the pixbuf cache. */
    existing = g_hash_table_lookup (pixbuf_cache, lookup_key);

    if (DEBUGGING) {
        g_message ("(Main Thread) Unlocking cache mutex");
    }

    g_mutex_unlock (&pixbuf_cache_mutex);

    if (existing != NULL) {
        if (DEBUGGING) {
            g_message ("(Main Thread) Found existing pixbuf: %s", lookup_key);
        }

        g_free (lookup_key);
        g_free (file_uri);

        return GDK_PIXBUF (existing);
    } else {
        if (DEBUGGING) {
            g_message ("(Main Thread) Locking thread mutex");
        }

        g_mutex_lock (&pixbuf_thread_mutex);

        /* Check if it is already in the list of pixbufs to make. */
        existing = g_hash_table_lookup (pixbufs_to_make_hash, lookup_key);

        if (existing == NULL) {
            NemoPixbufInfo *info;
            GList *node;

            info = g_slice_new0 (NemoPixbufInfo);
            info->file_uri = file_uri;
            info->thumbnail_path = g_strdup (file->details->thumbnail_path);
            info->size = size;
            info->max_width = max_width;
            info->scale = scale;
            info->flags = flags;
            info->hash_key = lookup_key;

            /* Add the pixbuf info to the list. */

            if (DEBUGGING) {
                g_message ("(Main Thread) Adding pixbuf to be loaded: %s",
                           info->file_uri);
            }

            g_queue_push_tail ((GQueue *)&pixbufs_to_make, info);
            node = g_queue_peek_tail_link ((GQueue *)&pixbufs_to_make);
            g_hash_table_insert (pixbufs_to_make_hash,
                                 lookup_key,
                                 node);

            /* If the thumbnail thread isn't running, and we haven't
               scheduled an idle function to start it up, do that now.
               We don't want to start it until all the other work is done,
               so the GUI will be updated as quickly as possible.*/

            if (pixbuf_thread_is_running == FALSE &&
                pixbuf_thread_starter_id == 0) {
                pixbuf_thread_starter_id = g_idle_add_full (G_PRIORITY_LOW, pixbuf_thread_starter_cb, NULL, NULL);
            }
        }

        /*********************************
         * MUTEX UNLOCKED
         *********************************/

        if (DEBUGGING) {
            g_message ("(Main Thread) Unlocking thread mutex");
        }

        g_mutex_unlock (&pixbuf_thread_mutex);
    }

    return NULL;
}
