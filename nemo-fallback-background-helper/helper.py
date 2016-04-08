#! /usr/bin/python3

from gi.repository import Gtk, CinnamonDesktop, Gdk, Gio

class NemoBackgroundHelper:
    def __init__(self, schema_id):
        self.bg = CinnamonDesktop.BG()
        self.bg.connect("changed", self.on_bg_changed)

        self.bg_settings = Gio.Settings(schema_id=schema_id)
        self.bg_settings.connect("change-event", self.on_bg_settings_changed)

        self.bg.load_from_preferences(self.bg_settings)

        self.windows = []

        self.screen = Gdk.Screen.get_default()

    def setup_backgrounds(self):
        window = self.screen.get_root_window()

        self.bg.create_and_set_surface_as_root(window, self.screen)

    def on_bg_changed(self, bg):
        pass

    def on_bg_settings_changed(self, settings, keys, n_keys):
        self.bg.load_from_preferences(self.bg_settings)
        self.setup_backgrounds()
