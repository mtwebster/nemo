#!/usr/bin/env python2

from gi.repository import Gtk
import os
import gettext
import ListWidgets


# i18n
gettext.install("nemo", "/usr/share/locale")

_ = gettext.gettext

class NemoPluginManagerWidget(Gtk.Grid):
    def __init__(self):
        Gtk.Grid.__init__(self)

        self.set_margin_left(10)
        self.set_margin_right(10)
        self.set_margin_top(10)
        self.set_margin_bottom(10)
        self.set_row_spacing(10)
        self.set_column_spacing(10)
        self.set_row_homogeneous(True)
        self.set_column_homogeneous(True)

        self.attach(ListWidgets.ActionsList(), 0, 0, 1, 1)
        self.attach(ListWidgets.ScriptsList(), 1, 0, 1, 1)
        self.attach(ListWidgets.ExtensionsList(), 0, 1, 2, 1)

class NemoPluginManagerApp:
    def __init__(self):
        self.window = Gtk.Window(Gtk.WindowType.TOPLEVEL)

        self.window.set_title(_("Nemo Plugin Manager"))
        self.window.set_icon_name("nemo")
        self.window.connect("destroy", Gtk.main_quit)
        self.window.set_default_size(640, 480)

        self.plugin_widget = NemoPluginManagerWidget()
        self.plugin_widget.show()

        self.window.add(self.plugin_widget)

        self.window.show_all()

if __name__ == "__main__":
    app = NemoPluginManagerApp()
    Gtk.main()
