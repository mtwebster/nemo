#!/usr/bin/env python2

from gi.repository import Gtk, Gio, GLib, Gdk, GObject
import os
from ctypes import *

GROUP_NAME = "Nemo Action"

class Section(Gtk.Frame):
    def __init__(self, title, key):
        Gtk.Frame.__init__(self)

        self.settings = Gio.Settings("org.nemo.plugins")
        self.key = key

        self.set_shadow_type(Gtk.ShadowType.IN)

        style = self.get_style_context()
        style.add_class("view")

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.add(box)

        toolbar = Gtk.Toolbar.new()
        Gtk.StyleContext.add_class(Gtk.Widget.get_style_context(toolbar), "primary-toolbar")
        label = Gtk.Label.new()
        label.set_markup("<b>%s</b>" % title)
        title_holder = Gtk.ToolItem()
        title_holder.add(label)
        toolbar.add(title_holder)
        box.add(toolbar)

        sw = Gtk.ScrolledWindow()
        sw.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        sw.set_shadow_type(Gtk.ShadowType.IN)
        box.pack_start(sw, True, True, 0)

        self.listbox = Gtk.ListBox()
        self.listbox.set_selection_mode(Gtk.SelectionMode.NONE)
        sw.add(self.listbox)

        toolbar = Gtk.Toolbar.new()
        Gtk.StyleContext.add_class(Gtk.Widget.get_style_context(toolbar), "primary-toolbar")
        bb_holder = Gtk.ToolItem()
        bb_holder.set_expand(True)
        toolbar.add(bb_holder)
        box.add(toolbar)

        self.button_box = Gtk.ButtonBox(orientation=Gtk.Orientation.HORIZONTAL)
        self.button_box.set_layout(Gtk.ButtonBoxStyle.END)
        self.button_box.set_spacing(6)
        bb_holder.add(self.button_box)

        b = Gtk.Button(label="Disable all")
        b.connect("clicked", self.on_disable_all)
        self.button_box.add(b)

        b = Gtk.Button(label="Enable all")
        b.connect("clicked", self.on_enable_all)
        self.button_box.add(b)

    def clear_list(self):
        self.listbox.foreach(lambda x: x.destroy())

    def on_enable_all(self, widget):
        self.settings.set_strv(self.key, [])

    def on_disable_all(self, widget):
        pass

class ActionsList(Section):
    KEY = "disabled-actions"

    def __init__(self):
        Section.__init__(self, _("Actions"), self.KEY)

        self.actions = []
        self.blacklist = self.settings.get_strv(self.KEY)

        self.settings_monitor_id = self.settings.connect("changed::%s" % self.KEY, self.on_settings_changed)

        self.refresh_widget()

    def refresh_widget(self):
        self.actions = []

        # Scan system location
        path = "/usr/share/nemo/actions"
        self.scan_directory(path)

        # Scan user location
        path = os.path.join(GLib.get_user_data_dir(), "nemo", "actions")
        self.scan_directory(path)

        self.clear_list()

        for item in self.actions:
            if not item.active:
                continue

            box = Gtk.HBox()

            check = Gtk.CheckButton()
            check.set_active(item.filename not in self.blacklist)
            check.connect("toggled", self.on_check_toggled, item)
            box.pack_start(check, False, False, 2)

            icon = Gtk.Image()
            if item.stock_id:
                icon.set_from_stock(item.stock_id, Gtk.IconSize.MENU)
            elif item.icon_name:
                icon.set_from_icon_name(item.icon_name, Gtk.IconSize.MENU)
            box.pack_start(icon, False, False, 2)

            label = Gtk.Label(label=item.name)
            box.pack_start(label, False, False, 2)

            eventbox = Gtk.EventBox()
            eventbox.set_above_child(True)
            eventbox.add(box)
            eventbox.connect("button-release-event", self.on_button_release, check, item)
            eventbox.show_all()

            self.listbox.add(eventbox)

    def scan_directory(self, path):
        files = os.listdir(path)
        if len(files):
            for f in files:
                fullpath = os.path.join(path, f)
                if not os.path.isdir(fullpath) and fullpath.endswith(".nemo_action"):
                    self.actions.append(self.ActionFile(f, fullpath))

    def on_settings_changed(self, settings, key):
        self.blacklist = self.settings.get_strv(self.KEY)
        self.refresh_widget()

    def on_button_release(self, widget, event, check, item):
        if event.button == Gdk.BUTTON_PRIMARY:
            check.clicked()

    def on_check_toggled(self, widget, item):
        enable = widget.get_active()
        new_list = []

        if enable:
            for i in self.blacklist:
                if item.filename == i:
                    continue
                new_list.append(i)
        else:
            new_list = self.blacklist
            new_list.append(item.filename)

        self.settings.set_strv(self.KEY, new_list)

    def on_disable_all(self, widget):
        new_list = []
        for i in self.actions:
            new_list.append(i.filename)

        self.settings.set_strv(self.KEY, new_list)

    class ActionFile:
        def __init__(self, filename, path):
            self.filename = filename
            self.path = path

            keyfile = GLib.KeyFile()
            keyfile.load_from_file(path, GLib.KeyFileFlags.NONE)

            try:
                self.name = keyfile.get_locale_string(GROUP_NAME, "Name", None).replace("_", "")
            except:
                self.name = self.filename

            try:
                self.stock_id = keyfile.get_string(GROUP_NAME, "Stock-Id")
            except:
                self.stock_id = None

            try:
                self.icon_name = keyfile.get_string(GROUP_NAME, "Icon-Name")
            except:
                self.icon_name = None

            try:
                self.active = keyfile.get_boolean(GROUP_NAME, "Active")
            except:
                self.active = True


class ScriptsList(Section):
    KEY = "disabled-scripts"
    def __init__(self):
        Section.__init__(self, _("Scripts"), self.KEY)

        self.scripts = []
        self.blacklist = self.settings.get_strv("disabled-scripts")

        self.settings_monitor_id = self.settings.connect("changed::%s" % self.KEY, self.on_settings_changed)

        self.refresh_widget()

    def refresh_widget(self):
        self.scripts = []

        # Scan user location
        path = os.path.join(GLib.get_user_data_dir(), "nemo", "scripts")
        self.scan_directory(path)

        self.clear_list()

        for item in self.scripts:
            box = Gtk.HBox()

            check = Gtk.CheckButton()
            check.set_active(item not in self.blacklist)
            check.connect("toggled", self.on_check_toggled, item)
            box.pack_start(check, False, False, 2)

            icon = Gtk.Image()
            box.pack_start(icon, False, False, 2)

            label = Gtk.Label(label=item)
            box.pack_start(label, False, False, 2)

            eventbox = Gtk.EventBox()
            eventbox.set_above_child(True)
            eventbox.add(box)
            eventbox.connect("button-release-event", self.on_button_release, check, item)
            eventbox.show_all()

            self.listbox.add(eventbox)

    def scan_directory(self, path):
        files = os.listdir(path)
        if len(files):
            for f in files:
                fullpath = os.path.join(path, f)
                if os.path.isdir(fullpath):
                    self.scan_directory(fullpath)
                else:
                    self.scripts.append(f)

    def on_settings_changed(self, settings, key):
        self.blacklist = self.settings.get_strv(self.KEY)
        self.refresh_widget()

    def on_button_release(self, widget, event, check, item):
        if event.button == Gdk.BUTTON_PRIMARY:
            check.clicked()

    def on_check_toggled(self, widget, item):
        enable = widget.get_active()
        new_list = []

        if enable:
            for i in self.blacklist:
                if item == i:
                    continue
                new_list.append(i)
        else:
            new_list = self.blacklist
            new_list.append(item)

        self.settings.set_strv(self.KEY, new_list)

    def on_disable_all(self, widget):
        new_list = []
        for i in self.scripts:
            new_list.append(i)

        self.settings.set_strv(self.KEY, new_list)


class ExtensionsList(Section):
    KEY = "disabled-extensions"

    def __init__(self):
        Section.__init__(self, _("Extensions"), self.KEY)

        self.actions = []
        self.blacklist = self.settings.get_strv(self.KEY)

        self.settings_monitor_id = self.settings.connect("changed::%s" % self.KEY, self.on_settings_changed)

        self.refresh_widget()

    def refresh_widget(self):
        self.actions = []

        # Scan system location
        path = "/usr/lib/nemo/extensions-3.0"
        self.scan_directory(path)

        # Scan user location
        path = os.path.join(GLib.get_user_data_dir(), "nemo", "actions")
        self.scan_directory(path)

        self.clear_list()

        for item in self.actions:
            if not item.active:
                continue

            box = Gtk.HBox()

            check = Gtk.CheckButton()
            check.set_active(item.filename not in self.blacklist)
            check.connect("toggled", self.on_check_toggled, item)
            box.pack_start(check, False, False, 2)

            icon = Gtk.Image()
            if item.stock_id:
                icon.set_from_stock(item.stock_id, Gtk.IconSize.MENU)
            elif item.icon_name:
                icon.set_from_icon_name(item.icon_name, Gtk.IconSize.MENU)
            box.pack_start(icon, False, False, 2)

            label = Gtk.Label(label=item.name)
            box.pack_start(label, False, False, 2)

            eventbox = Gtk.EventBox()
            eventbox.set_above_child(True)
            eventbox.add(box)
            eventbox.connect("button-release-event", self.on_button_release, check, item)
            eventbox.show_all()

            self.listbox.add(eventbox)

    def scan_directory(self, path):
        files = os.listdir(path)
        if len(files):
            for f in files:
                fullpath = os.path.join(path, f)
                if not os.path.isdir(fullpath) and fullpath.endswith(".so"):
                    mod = cdll.LoadLibrary(fullpath)
                    array = (GObject.GType)()
                    res = cast(array, POINTER(GObject.GType))
                    y = c_int()
                    mod.nemo_module_list_types(byref(res), byref(y))
                    # x = cast(res[0], POINTER(GObject.Type))
                    # print x.name()
                    # mod = GLib.Module.open(fullpath, GLib.ModuleFlags.BIND_LAZY | GLib.ModuleFlags.BIND_LOCAL)

    def on_settings_changed(self, settings, key):
        self.blacklist = self.settings.get_strv(self.KEY)
        self.refresh_widget()

    def on_button_release(self, widget, event, check, item):
        if event.button == Gdk.BUTTON_PRIMARY:
            check.clicked()

    def on_check_toggled(self, widget, item):
        enable = widget.get_active()
        new_list = []

        if enable:
            for i in self.blacklist:
                if item.filename == i:
                    continue
                new_list.append(i)
        else:
            new_list = self.blacklist
            new_list.append(item.filename)

        self.settings.set_strv(self.KEY, new_list)

    def on_disable_all(self, widget):
        new_list = []
        for i in self.actions:
            new_list.append(i.filename)

        self.settings.set_strv(self.KEY, new_list)

    class ActionFile:
        def __init__(self, filename, path):
            self.filename = filename
            self.path = path

            keyfile = GLib.KeyFile()
            keyfile.load_from_file(path, GLib.KeyFileFlags.NONE)

            try:
                self.name = keyfile.get_locale_string(GROUP_NAME, "Name", None).replace("_", "")
            except:
                self.name = self.filename

            try:
                self.stock_id = keyfile.get_string(GROUP_NAME, "Stock-Id")
            except:
                self.stock_id = None

            try:
                self.icon_name = keyfile.get_string(GROUP_NAME, "Icon-Name")
            except:
                self.icon_name = None

            try:
                self.active = keyfile.get_boolean(GROUP_NAME, "Active")
            except:
                self.active = True
