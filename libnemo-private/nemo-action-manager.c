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

#include "nemo-action-manager.h"


G_DEFINE_TYPE (NemoActionManager, nemo_action_manager, G_TYPE_OBJECT);

static void     nemo_action_manager_init       (NemoActionManager      *action_manager);

static void     nemo_action_manager_class_init (NemoActionManagerClass *klass);

static void     nemo_action_manager_get_property  (GObject                    *object,
                                                   guint                       param_id,
                                                   GValue                     *value,
                                                   GParamSpec                 *pspec);

static void     nemo_action_manager_set_property  (GObject                    *object,
                                                   guint                       param_id,
                                                   const GValue               *value,
                                                   GParamSpec                 *pspec);

static void     nemo_action_manager_constructed (GObject *object);

static void     nemo_action_manager_finalize (GObject *gobject);

static gpointer parent_class;

enum 
{
  PROP_0,
  PROP_CONDITIONS
};

static void
nemo_action_manager_init (NemoActionManager *action_manager)
{

}

static void
nemo_action_manager_class_init (NemoActionManagerClass *klass)
{
    GObjectClass         *object_class = G_OBJECT_CLASS(klass);
    parent_class           = g_type_class_peek_parent (klass);
    object_class->finalize = nemo_action_manager_finalize;
    object_class->set_property = nemo_action_manager_set_property;
    object_class->get_property = nemo_action_manager_get_property;
    object_class->constructed = nemo_action_manager_constructed;

}

void
nemo_action_manager_constructed (GObject *object)
{
    G_OBJECT_CLASS (parent_class)->constructed (object);

    NemoActionManager *action_manager = NEMO_ACTION_MANAGER (object);

}

NemoActionManager *
nemo_action_manager_new (const gchar *name, 
                         const gchar *path)
{

    return finish ? g_object_new (NEMO_TYPE_ACTION,
                                  "name", name,
                                  "key-file-path", path,
                                  NULL): NULL;
}

static void
nemo_action_manager_finalize (GObject *object)
{
    NemoActionManager *action = NEMO_ACTION_MANAGER (object);

    G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
nemo_action_manager_set_property (GObject         *object,
                                  guint            prop_id,
                                  const GValue    *value,
                                  GParamSpec      *pspec)
{
  NemoActionManager *action_manager;
  
  action_manager = NEMO_ACTION_MANAGER (object);

  switch (prop_id)
    {
    case PROP_KEY_FILE_PATH:
      nemo_action_set_key_file_path (action, g_value_get_string (value));
      break;
    case PROP_SELECTION_TYPE:
      action_manager->selection_type = g_value_get_int (value);
      break;
    case PROP_EXTENSIONS:
      nemo_action_set_extensions (action, g_value_get_pointer (value));
      break;
    case PROP_MIMES:
      nemo_action_set_mimetypes (action, g_value_get_pointer (value));
      break;
    case PROP_EXEC:
      nemo_action_set_exec (action, g_value_get_string (value));
      break;
    case PROP_PARENT_DIR:
      nemo_action_set_parent_dir (action, g_value_get_string (value));
      break;
    case PROP_USE_PARENT_DIR:
      action_manager->use_parent_dir = g_value_get_boolean (value);
      break;
    case PROP_ORIG_LABEL:
      nemo_action_set_orig_label (action, g_value_get_string (value));
      break;
    case PROP_ORIG_TT:
      nemo_action_set_orig_tt (action, g_value_get_string (value));
      break;
    case PROP_QUOTE_TYPE:
      action_manager->quote_type = g_value_get_int (value);
      break;
    case PROP_SEPARATOR:
      nemo_action_set_separator (action, g_value_get_string (value));
      break;
    case PROP_CONDITIONS:
      nemo_action_set_conditions (action, g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
nemo_action_manager_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  NemoActionManager *action_manager;

  action_manager = NEMO_ACTION_MANAGER (object);

  switch (prop_id)
    {
    case PROP_KEY_FILE_PATH:
      g_value_set_string (value, action_manager->key_file_path);
      break;
    case PROP_SELECTION_TYPE:
      g_value_set_int (value, action_manager->selection_type);
      break;
    case PROP_EXTENSIONS:
      g_value_set_pointer (value, action_manager->extensions);
      break;
    case PROP_MIMES:
      g_value_set_pointer (value, action_manager->mimetypes);
      break;
    case PROP_EXEC:
      g_value_set_string (value, action_manager->exec);
      break;
    case PROP_PARENT_DIR:
      g_value_set_string (value, action_manager->parent_dir);
      break;
    case PROP_USE_PARENT_DIR:
      g_value_set_boolean (value, action_manager->use_parent_dir);
      break;
    case PROP_ORIG_LABEL:
      g_value_set_string (value, action_manager->orig_label);
      break;
    case PROP_ORIG_TT:
      g_value_set_string (value, action_manager->orig_tt);
      break;
    case PROP_QUOTE_TYPE:
      g_value_set_int (value, action_manager->quote_type);
      break;
    case PROP_SEPARATOR:
      g_value_set_string (value, action_manager->separator);
      break;
    case PROP_CONDITIONS:
      g_value_set_pointer (value, action_manager->conditions);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}
