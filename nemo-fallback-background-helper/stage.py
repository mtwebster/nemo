#! /usr/bin/python3

from gi.repository import Gtk, Gdk, GObject
import random

# import utils
import trackers
import settings
import constants as c
from fader import Fader
from monitorView import MonitorView

class Stage(Gtk.Window):
    def __init__(self, screen, manager):
        Gtk.Window.__init__(self,
                            type=Gtk.WindowType.TOPLEVEL,
                            decorated=False,
                            skip_taskbar_hint=True,
                            skip_pager_hint=True)

        trackers.con_tracker_get().connect(settings.bg,
                                           "changed", 
                                           self.on_bg_changed)

        self.set_type_hint(Gdk.WindowTypeHint.DESKTOP)

        self.manager = manager
        self.screen = screen
        self.monitors = []
        self.last_focus_monitor = -1
        self.overlay = None

        self.get_style_context().remove_class("background")

        self.update_geometry()

        # self.set_keep_above(True)
        # self.fullscreen()

        self.overlay = Gtk.Overlay()

        trackers.con_tracker_get().connect(self.overlay,
                                           "realize",
                                           self.on_realized)

        trackers.con_tracker_get().connect(self.overlay,
                                           "get-child-position",
                                           self.position_overlay_child)

        self.overlay.show_all()
        self.add(self.overlay)

        self.present()

    # def do_realize(self):
    #     window = Gdk.Screen.get_default().get_root_window()

    #     self.set_window(window)

    def on_realized(self, widget):
        window = self.get_window()
        # window.lower()
        # window.set_fullscreen_mode(Gdk.FullscreenMode.ALL_MONITORS)
        window.move_resize(self.rect.x, self.rect.y, self.rect.width, self.rect.height)

        self.setup_monitors()

        GObject.idle_add(window.lower)

    def destroy_stage(self):
        trackers.con_tracker_get().disconnect(settings.bg,
                                              "changed",
                                              self.on_bg_changed)

        for monitor in self.monitors:
            monitor.destroy()

        self.fader = None

        self.destroy()

    def setup_monitors(self):
        n = self.screen.get_n_monitors()

        for index in range(n):
            monitor = MonitorView(self.screen, index)

            image = Gtk.Image()

            settings.bg.create_and_set_gtk_image (image,
                                                  monitor.rect.width,
                                                  monitor.rect.height)

            monitor.set_initial_wallpaper_image(image)

            self.monitors.append(monitor)

            self.add_child_widget(monitor)
            self.put_on_bottom(monitor)

            monitor.set_reveal_child(True)

            monitor.queue_draw()

    def on_bg_changed(self, bg):
        for monitor in self.monitors:
            image = Gtk.Image()

            settings.bg.create_and_set_gtk_image (image,
                                                  monitor.rect.width,
                                                  monitor.rect.height)

            monitor.set_next_wallpaper_image(image)

# Timer stuff - after a certain time, the unlock dialog will cancel itself.
# This timer is suspended during authentication, and any time a new user event is received

    # Override BaseWindow.update_geometry
    def update_geometry(self):
        self.rect = Gdk.Rectangle()
        self.rect.x = 0
        self.rect.y = 0
        self.rect.width = self.screen.get_width()
        self.rect.height = self.screen.get_height()

# Overlay window management #

    def add_child_widget(self, widget):
        self.overlay.add_overlay(widget)

    def put_on_top(self, widget):
        self.overlay.reorder_overlay(widget, -1)
        self.overlay.queue_draw()

    def put_on_bottom(self, widget):
        self.overlay.reorder_overlay(widget, 0)
        self.overlay.queue_draw()

    def position_overlay_child(self, overlay, child, allocation):
        if isinstance(child, MonitorView):
            w, h = child.get_preferred_size()
            allocation.x = child.rect.x
            allocation.y = child.rect.y
            allocation.width = child.rect.width
            allocation.height = child.rect.height

            return True

        return False
