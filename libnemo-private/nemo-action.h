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

#ifndef NEMO_ACTION_H
#define NEMO_ACTION_H

#include <gtk/gtk.h>
#include <glib.h>
#include "nemo-file.h"

#define NEMO_TYPE_ACTION nemo_action_get_type()
#define NEMO_ACTION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), NEMO_TYPE_ACTION, NemoAction))
#define NEMO_ACTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), NEMO_TYPE_ACTION, NemoActionClass))
#define NEMO_IS_ACTION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NEMO_TYPE_ACTION))
#define NEMO_IS_ACTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), NEMO_TYPE_ACTION))
#define NEMO_ACTION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), NEMO_TYPE_ACTION, NemoActionClass))


typedef struct _NemoAction NemoAction;
typedef struct _NemoActionClass NemoActionClass;

typedef enum {
    SELECTION_SINGLE,
    SELECTION_MULTIPLE,
    SELECTION_ANY,
    SELECTION_NONE
} SelectionType;

struct _NemoAction {
	GtkAction parent;
    GKeyFile *key_file;
    SelectionType selection_type;
    gchar **extensions;
    guint ext_length;
    const gchar *exec;
};

struct _NemoActionClass {
	GtkActionClass parent_class;
};

GType		 nemo_action_get_type (void);
GtkAction   *nemo_action_new      (const gchar *name, NemoFile *file);
void         nemo_action_activate (NemoAction *action, GList *selection);
SelectionType nemo_action_get_selection_type (NemoAction *action);
gchar      **nemo_action_get_extension_list (NemoAction *action);
guint        nemo_action_get_extension_count (NemoAction *action);

#endif /* NEMO_ACTION_H */