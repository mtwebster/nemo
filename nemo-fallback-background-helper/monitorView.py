#! /usr/bin/python3

from gi.repository import Gtk, Gdk, Gio, GLib, GObject, GdkPixbuf
import re
import sys
import math
import cairo

import trackers
from baseWindow import BaseWindow

class WallpaperStack(Gtk.Stack):
    def __init__(self, rect):
        super(WallpaperStack, self).__init__()

        self.rect = rect

        self.set_transition_type(Gtk.StackTransitionType.CROSSFADE)
        self.set_transition_duration(1000)

        self.current = None

    def set_initial_image(self, image):
        self.current = image
        self.current.set_visible(True)

        # trackers.con_tracker_get().connect_after(image,
        #                                          "draw",
        #                                          self.shade_wallpaper)

        self.add(self.current)
        self.set_visible_child(self.current)

    def transition_to_image(self, image):
        self.queued = image
        self.queued.set_visible(True)

        self.add(self.queued)
        self.set_visible_child(self.queued)

        tmp = self.current
        self.current = self.queued
        self.queued = None

        # No need to disconnect the draw handler, it'll be disco'd by the con_tracker's
        # weak_ref callback.

        GObject.idle_add(tmp.destroy)

class MonitorView(BaseWindow):
    def __init__(self, screen, index):
        super(MonitorView, self).__init__()

        self.screen = screen
        self.monitor_index = index

        self.update_geometry()

        self.wallpaper_stack = WallpaperStack(self.rect)
        self.wallpaper_stack.show()
        self.wallpaper_stack.set_halign(Gtk.Align.FILL)
        self.wallpaper_stack.set_valign(Gtk.Align.FILL)

        self.add(self.wallpaper_stack)

        self.show_all()

    def set_initial_wallpaper_image(self, image):
        print("set inititla")
        self.wallpaper_stack.set_initial_image(image)

    def set_next_wallpaper_image(self, image):
        self.wallpaper_stack.transition_to_image(image)

