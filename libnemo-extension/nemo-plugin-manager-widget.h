/* nemo-plugin-manager-widget.h */

/*  A GtkWidget that can be inserted into a UI that provides a simple interface for
 *  managing the loading of extensions, actions and scripts
 */

#ifndef __NEMO_PLUGIN_MANAGER_WIDGET_H__
#define __NEMO_PLUGIN_MANAGER_WIDGET_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include "nemo-action-config-widget.h"
#include "nemo-script-config-widget.h"
#include "nemo-extension-config-widget.h"


G_BEGIN_DECLS

#define NEMO_TYPE_PLUGIN_MANAGER_WIDGET (nemo_plugin_manager_widget_get_type())

#define NEMO_PLUGIN_MANAGER_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), NEMO_TYPE_PLUGIN_MANAGER_WIDGET, NemoPluginManagerWidget))
#define NEMO_PLUGIN_MANAGER_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), NEMO_TYPE_PLUGIN_MANAGER_WIDGET, NemoPluginManagerWidgetClass))
#define NEMO_IS_PLUGIN_MANAGER_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NEMO_TYPE_PLUGIN_MANAGER_WIDGET))
#define NEMO_IS_PLUGIN_MANAGER_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NEMO_TYPE_PLUGIN_MANAGER_WIDGET))
#define NEMO_PLUGIN_MANAGER_WIDGET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), NEMO_TYPE_PLUGIN_MANAGER_WIDGET, NemoPluginManagerWidgetClass))

typedef struct _NemoPluginManagerWidget NemoPluginManagerWidget;
typedef struct _NemoPluginManagerWidgetClass NemoPluginManagerWidgetClass;

struct _NemoPluginManagerWidget
{
  GtkBin parent;
  GtkWidget *grid;

  GtkWidget *action_widget;
  GtkWidget *script_widget;
  GtkWidget *extension_widget;
};

struct _NemoPluginManagerWidgetClass
{
  GtkBinClass parent_class;
};

GType nemo_plugin_manager_widget_get_type (void);

NemoPluginManagerWidget *nemo_plugin_manager_widget_new                   (void);
GtkWidget               *nemo_plugin_manager_widget_get_grid              (NemoPluginManagerWidget *manager);
GtkWidget               *nemo_plugin_manager_widget_get_action_widget     (NemoPluginManagerWidget *manager);
GtkWidget               *nemo_plugin_manager_widget_get_script_widget     (NemoPluginManagerWidget *manager);
GtkWidget               *nemo_plugin_manager_widget_get_extension_widget  (NemoPluginManagerWidget *manager);

G_END_DECLS

#endif /* __NEMO_PLUGIN_MANAGER_WIDGET_H__ */
