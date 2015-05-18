/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Suite 500, Boston, MA 02110-1335, USA.
 *
 */

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "nemo-interesting-folder-bar.h"
#include "nemo-application.h"

#include "nemo-view.h"
#include <libnemo-private/nemo-file-operations.h>
#include <libnemo-private/nemo-file-utilities.h>
#include <libnemo-private/nemo-file.h>
#include <libnemo-private/nemo-trash-monitor.h>

#define NEMO_INTERESTING_FOLDER_BAR_GET_PRIVATE(o)\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), NEMO_TYPE_INTERESTING_FOLDER_BAR, NemoInterestingFolderBarPrivate))

enum {
	PROP_VIEW = 1,
    PROP_TYPE,
	NUM_PROPERTIES
};

enum {
    INTERESTING_FOLDER_BAR_ACTION_OPEN_DOC = 1,
    INTERESTING_FOLDER_BAR_SCRIPT_OPEN_DOC,
    INTERESTING_FOLDER_BAR_FIX_CACHE,
    INTERESTING_FOLDER_BAR_FIX_CACHE_DISMISS
};

struct NemoInterestingFolderBarPrivate
{
	NemoView *view;
    InterestingFolderType type;
	gulong selection_handler_id;
};

G_DEFINE_TYPE (NemoInterestingFolderBar, nemo_interesting_folder_bar, GTK_TYPE_INFO_BAR);

static void
interesting_folder_bar_response_cb (GtkInfoBar *infobar,
                                          gint  response_id,
                                      gpointer  user_data)
{
    NemoInterestingFolderBar *bar;
    GFile *f = NULL;

    bar = NEMO_INTERESTING_FOLDER_BAR (infobar);

    switch (response_id) {
        case INTERESTING_FOLDER_BAR_ACTION_OPEN_DOC:
            f = g_file_new_for_path ("/usr/share/nemo/actions/sample.nemo_action");
            if (g_file_query_exists (f, NULL))
                nemo_view_activate_file (bar->priv->view, nemo_file_get (f), NEMO_WINDOW_OPEN_FLAG_NEW_WINDOW);
            g_object_unref (f);
            break;
        case INTERESTING_FOLDER_BAR_SCRIPT_OPEN_DOC:
            f = g_file_new_for_path ("/usr/share/nemo/script-info.md");
            if (g_file_query_exists (f, NULL))
                nemo_view_activate_file (bar->priv->view, nemo_file_get (f), NEMO_WINDOW_OPEN_FLAG_NEW_WINDOW);
            g_object_unref (f);
            break;
        case INTERESTING_FOLDER_BAR_FIX_CACHE:
            g_spawn_command_line_sync ("sh -c \"pkexec nemo --fix-cache\"", NULL, NULL, NULL, NULL);
            nemo_application_clear_cache_flag (nemo_application_get_singleton ());
            nemo_window_slot_reload (nemo_view_get_nemo_window_slot (bar->priv->view));
            gtk_widget_hide (GTK_WIDGET (infobar));
            break;
        case INTERESTING_FOLDER_BAR_FIX_CACHE_DISMISS:
            nemo_application_clear_cache_flag (nemo_application_get_singleton ());
            gtk_widget_hide (GTK_WIDGET (infobar));
            break;
        default:
            break;
    }
}

static void
nemo_interesting_folder_bar_set_property (GObject      *object,
				 guint         prop_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
	NemoInterestingFolderBar *bar;

	bar = NEMO_INTERESTING_FOLDER_BAR (object);

	switch (prop_id) {
	case PROP_VIEW:
		bar->priv->view = g_value_get_object (value);
		break;
    case PROP_TYPE:
        bar->priv->type = g_value_get_int (value);
        break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
nemo_interesting_folder_bar_constructed (GObject *obj)
{
    G_OBJECT_CLASS (nemo_interesting_folder_bar_parent_class)->constructed (obj);

    NemoInterestingFolderBar *bar = NEMO_INTERESTING_FOLDER_BAR (obj);

    GtkWidget *content_area, *action_area, *w;
    GtkWidget *label;

    content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (bar));
    action_area = gtk_info_bar_get_action_area (GTK_INFO_BAR (bar));

    gtk_orientable_set_orientation (GTK_ORIENTABLE (action_area),
                    GTK_ORIENTATION_HORIZONTAL);

    switch (bar->priv->type) {
        case TYPE_ACTIONS_FOLDER:
            label = gtk_label_new (_("Actions: Action files can be added to this folder and will appear in the menu."));
            w = gtk_info_bar_add_button (GTK_INFO_BAR (bar),
                                         _("More info"),
                                         INTERESTING_FOLDER_BAR_ACTION_OPEN_DOC);
            gtk_widget_set_tooltip_text (w, _("View a sample action file with documentation"));
            break;
        case TYPE_SCRIPTS_FOLDER:
            label = gtk_label_new (_("Scripts: All executable files in this folder will appear in the "
                                     "Scripts menu."));
            w = gtk_info_bar_add_button (GTK_INFO_BAR (bar),
                                         _("More info"),
                                         INTERESTING_FOLDER_BAR_SCRIPT_OPEN_DOC);
            gtk_widget_set_tooltip_text (w, _("View additional information about creating scripts"));
            break;
        case TYPE_CACHE_BAD:
            label = gtk_label_new (_("A problem has been detected with your thumbnail cache.  Fixing will require administrative privileges."));
            w = gtk_info_bar_add_button (GTK_INFO_BAR (bar),
                                         _("Fix now"),
                                         INTERESTING_FOLDER_BAR_FIX_CACHE);
            w = gtk_info_bar_add_button (GTK_INFO_BAR (bar),
                                         _("Dismiss"),
                                         INTERESTING_FOLDER_BAR_FIX_CACHE_DISMISS);
            break;
        default:
            label = gtk_label_new ("undefined");
            break;
    }

    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    gtk_style_context_add_class (gtk_widget_get_style_context (label),
                     "nemo-cluebar-label");
    gtk_widget_show (label);
    gtk_container_add (GTK_CONTAINER (content_area), label);

    g_signal_connect (bar, "response",
              G_CALLBACK (interesting_folder_bar_response_cb), bar);
}

static void
nemo_interesting_folder_bar_class_init (NemoInterestingFolderBarClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = nemo_interesting_folder_bar_set_property;
    object_class->constructed = nemo_interesting_folder_bar_constructed;

	g_object_class_install_property (object_class,
					 PROP_VIEW,
					 g_param_spec_object ("view",
							      "view",
							      "the NemoView",
							      NEMO_TYPE_VIEW,
							      G_PARAM_WRITABLE |
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (object_class,
                     PROP_TYPE,
                     g_param_spec_int ("type",
                                  "type",
                                  "the InterestingFolderType",
                                  TYPE_NONE_FOLDER,
                                  TYPE_CACHE_BAD,
                                  TYPE_NONE_FOLDER,
                                  G_PARAM_WRITABLE |
                                  G_PARAM_CONSTRUCT_ONLY |
                                  G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (klass, sizeof (NemoInterestingFolderBarPrivate));
}

static void
nemo_interesting_folder_bar_init (NemoInterestingFolderBar *bar)
{
	bar->priv = NEMO_INTERESTING_FOLDER_BAR_GET_PRIVATE (bar);
    bar->priv->type = TYPE_NONE_FOLDER;
}

GtkWidget *
nemo_interesting_folder_bar_new (NemoView *view, InterestingFolderType type)
{
return g_object_new (NEMO_TYPE_INTERESTING_FOLDER_BAR,
                     "view", view,
                     "type", type,
                     NULL);
}

GtkWidget *
nemo_interesting_folder_bar_new_for_location (NemoView *view, GFile *location)
{
    InterestingFolderType type = TYPE_NONE_FOLDER;
    gchar *path = NULL;
    GFile *tmp_loc = NULL;

    path = g_build_filename (g_get_user_data_dir (), "nemo", "actions", NULL);
    tmp_loc = g_file_new_for_path (path);

    if (g_file_equal (location, tmp_loc)) {
        type = TYPE_ACTIONS_FOLDER;
        goto out;
    }

    g_free (path);
    g_object_unref (tmp_loc);

    path = g_build_filename (g_get_user_data_dir (), "nemo", "scripts", NULL);
    tmp_loc = g_file_new_for_path (path);

    if (g_file_equal (location, tmp_loc))
        type = TYPE_SCRIPTS_FOLDER;

out:
    g_free (path);
    g_object_unref (tmp_loc);

    return type == TYPE_NONE_FOLDER ? NULL : nemo_interesting_folder_bar_new (view, type);
}