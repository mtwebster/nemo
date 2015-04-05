/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nemo-plugin-manager-main.c - Start the plugin manager dialog.
 */

#include <config.h>
// #include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <stdlib.h>

#include <libnemo-extension/nemo-plugin-manager-widget.h>

int
main (int argc, char *argv[])
{
	GtkWidget *window;

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

    gtk_init (0, NULL);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    gtk_window_set_title (GTK_WINDOW (window), _("Nemo Plugin Manager"));
    gtk_window_set_icon_name (GTK_WINDOW (window), "preferences-system");
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

    gtk_window_set_default_size (GTK_WINDOW (window), 640, 480);

    NemoPluginManagerWidget *pm = nemo_plugin_manager_widget_new ();

    gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (pm));

    gtk_widget_show_all (GTK_WIDGET (window));

	gtk_main ();

	return 0;
}
