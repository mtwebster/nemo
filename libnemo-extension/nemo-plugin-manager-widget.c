/* nemo-plugin-manager-widget.c */

/*  A GtkWidget that can be inserted into a UI that provides a simple interface for
 *  managing the loading of extensions, actions and scripts
 */

#include <config.h>
#include "nemo-plugin-manager-widget.h"
#include <glib.h>

G_DEFINE_TYPE (NemoPluginManagerWidget, nemo_plugin_manager_widget, GTK_TYPE_BIN);

static void
nemo_plugin_manager_widget_class_init (NemoPluginManagerWidgetClass *klass)
{
}

static void
nemo_plugin_manager_widget_init (NemoPluginManagerWidget *self)
{
    GtkWidget *grid = gtk_grid_new ();
    self->grid = grid;

    gtk_widget_set_margin_start (grid, 10);
    gtk_widget_set_margin_end (grid, 10);
    gtk_widget_set_margin_top (grid, 10);
    gtk_widget_set_margin_bottom (grid, 10);
    gtk_grid_set_row_spacing (GTK_GRID (grid), 10);
    gtk_grid_set_column_spacing (GTK_GRID (grid), 10);
    gtk_grid_set_row_homogeneous (GTK_GRID (grid), TRUE);
    gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);

    gtk_grid_attach (GTK_GRID (grid), nemo_action_config_widget_new (), 0, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (grid), nemo_script_config_widget_new (), 1, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (grid), nemo_extension_config_widget_new (), 0, 1, 2, 1);

    gtk_container_add (GTK_CONTAINER (self), grid);

    gtk_widget_show_all (GTK_WIDGET (self));
}

NemoPluginManagerWidget *
nemo_plugin_manager_widget_new (void)
{
  return g_object_new (NEMO_TYPE_PLUGIN_MANAGER_WIDGET, NULL);
}

GtkWidget *
nemo_plugin_manager_widget_get_grid (NemoPluginManagerWidget *manager)
{
    return manager->grid;
}

GtkWidget *
nemo_plugin_manager_widget_get_action_widget (NemoPluginManagerWidget *manager)
{
    return manager->action_widget;
}

GtkWidget *
nemo_plugin_manager_widget_get_script_widget (NemoPluginManagerWidget *manager)
{
    return manager->script_widget;
}

GtkWidget *
nemo_plugin_manager_widget_get_extension_widget (NemoPluginManagerWidget *manager)
{
    return manager->extension_widget;
}

