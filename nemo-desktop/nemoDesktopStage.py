#! /usr/bin/python3

from gi.repository import Gtk, Gdk

import status
import constants as c
import singletons
from nemoDesktopMonitorView import NemoDesktopMonitorView
from util import utils, trackers, settings
from util.fader import Fader

class NemoDesktopStage(Gtk.Window):
    """
    The NemoDesktopStage is the toplevel window for nemo-desktop.

    It's the first thing made, the last thing destroyed, and all other
    widgets live inside of it (or rather, inside the GtkOverlay below)

    It is Gtk.WindowType.POPUP to avoid being managed/composited by muffin,
    and to prevent animation during its creation and destruction.

    The Stage reponds pretty much only to the instructions of the
    ScreensaverManager.
    """
    def __init__(self, screen, manager, away_message):
        Gtk.Window.__init__(self,
                            type=Gtk.WindowType.POPUP,
                            decorated=False,
                            type_hint=Gdk.WindowTypeHint.DESKTOP,
                            skip_taskbar_hint=True)

        self.get_style_context().add_class("nemo-desktop-window")

        trackers.con_tracker_get().connect(singletons.Backgrounds,
                                           "changed", 
                                           self.on_bg_changed)

        self.destroying = False

        self.manager = manager
        self.screen = screen

        self.monitors = []
        self.overlay = None

        self.get_style_context().remove_class("background")

        self.set_events(self.get_events() |
                        Gdk.EventMask.POINTER_MOTION_MASK |
                        Gdk.EventMask.BUTTON_PRESS_MASK |
                        Gdk.EventMask.BUTTON_RELEASE_MASK |
                        Gdk.EventMask.KEY_PRESS_MASK |
                        Gdk.EventMask.KEY_RELEASE_MASK |
                        Gdk.EventMask.EXPOSURE_MASK |
                        Gdk.EventMask.VISIBILITY_NOTIFY_MASK |
                        Gdk.EventMask.ENTER_NOTIFY_MASK |
                        Gdk.EventMask.LEAVE_NOTIFY_MASK |
                        Gdk.EventMask.FOCUS_CHANGE_MASK)

        self.update_geometry()
        self.set_opacity(0.0)

        self.overlay = Gtk.Overlay()
        self.fader = Fader(self)

        trackers.con_tracker_get().connect(self.overlay,
                                           "realize",
                                           self.on_realized)

        trackers.con_tracker_get().connect(self.overlay,
                                           "get-child-position",
                                           self.position_overlay_child)

        self.overlay.show_all()
        self.add(self.overlay)

        # This filter suppresses any other windows that might share
        # our window group in muffin, from showing up over the Stage.
        # For instance: Chrome and Firefox native notifications.
        self.gdk_filter = CScreensaver.GdkEventFilter()

        trackers.con_tracker_get().connect(self.screen,
                                           "monitors-changed",
                                           self.on_screen_changed)

        trackers.con_tracker_get().connect(self.screen,
                                           "size-changed",
                                           self.on_screen_changed)

    def on_screen_changed(self, screen, data=None):
        self.destroy_monitor_views()

        self.update_geometry()
        self.size_to_screen()

        self.setup_monitors()

    def transition_in(self, effect_time, callback):
        """
        This is the primary way of making the Stage visible.
        """
        self.realize()
        self.fader.fade_in(effect_time, callback)

    def transition_out(self, effect_time, callback):
        """
        This is the primary way of destroying the stage.  This can
        end up being called multiple times, so we keep track of if we've
        already started a transition, and ignore further calls.
        """
        if self.destroying:
            return

        self.destroying = True

        self.fader.cancel()

        self.fader.fade_out(effect_time, callback)

    def on_realized(self, widget):
        """
        Repositions the window when it is realized, to cover the entire
        GdkScreen (a rectangle exactly encompassing all monitors.)

        From here we also proceed to construct all overlay children and
        activate our window suppressor.
        """
        self.size_to_screen()
        self.setup_monitors()

        self.gdk_filter.start(self)

    def size_to_screen(self):
        window = self.get_window()

        utils.override_user_time(window)
        window.move_resize(self.rect.x, self.rect.y, self.rect.width, self.rect.height)

    def destroy_stage(self):
        """
        Performs all tear-down necessary to destroy the Stage, destroying
        all children in the process, and finally destroying itself.
        """
        trackers.con_tracker_get().disconnect(singletons.Backgrounds,
                                              "changed",
                                              self.on_bg_changed)

        self.destroy_monitor_views()
        self.fader = None

        self.monitors = []

        self.gdk_filter.stop()
        self.gdk_filter = None

        trackers.con_tracker_get().disconnect(self.screen,
                                              "monitors-changed",
                                              self.on_screen_changed)

        trackers.con_tracker_get().disconnect(self.screen,
                                              "size-changed",
                                              self.on_screen_changed)

        self.destroy()

    def setup_monitors(self):
        """
        Iterate through the monitors, and create MonitorViews for each one
        to cover them.
        """
        self.monitors = []

        n = self.screen.get_n_monitors()

        for index in range(n):
            monitor = NemoDesktopMonitorView(self.screen, index)

            image = Gtk.Image()

            singletons.Backgrounds.create_and_set_gtk_image (image,
                                                             monitor.rect.width,
                                                             monitor.rect.height)

            monitor.set_initial_wallpaper_image(image)

            self.monitors.append(monitor)

            self.add_child_widget(monitor)
            self.put_on_bottom(monitor)

            monitor.reveal()

    def on_bg_changed(self, bg):
        """
        Callback for our GnomeBackground instance, this tells us when
        the background settings have changed, so we can update our wallpaper.
        """
        for monitor in self.monitors:
            image = Gtk.Image()

            singletons.Backgrounds.create_and_set_gtk_image (image,
                                                  monitor.rect.width,
                                                  monitor.rect.height)

            monitor.set_next_wallpaper_image(image)

    def destroy_monitor_views(self):
        """
        Destroy all MonitorViews
        """
        for monitor in self.monitors:
            monitor.destroy()
            del monitor

    def update_geometry(self):
        """
        Override BaseWindow.update_geometry() - the Stage should always be the
        GdkScreen size
        """
        self.rect = Gdk.Rectangle()
        self.rect.x = 0
        self.rect.y = 0
        self.rect.width = self.screen.get_width()
        self.rect.height = self.screen.get_height()

        hints = Gdk.Geometry()
        hints.min_width = self.rect.width
        hints.min_height = self.rect.height
        hints.max_width = self.rect.width
        hints.max_height = self.rect.height
        hints.base_width = self.rect.width
        hints.base_height = self.rect.height

        self.set_geometry_hints(self, hints, Gdk.WindowHints.MIN_SIZE | Gdk.WindowHints.MAX_SIZE | Gdk.WindowHints.BASE_SIZE)

# Overlay window management

    def add_child_widget(self, widget):
        """
        Add a new child to the overlay
        """
        self.overlay.add_overlay(widget)

    def put_on_top(self, widget):
        """
        Move the widget to the top of the overlay
        (Over everything else)
        """
        self.overlay.reorder_overlay(widget, -1)
        self.overlay.queue_draw()

    def put_on_bottom(self, widget):
        """
        Move the widget to the bottom of the overlay
        (Under everything else)
        """
        self.overlay.reorder_overlay(widget, 0)
        self.overlay.queue_draw()

    def position_overlay_child(self, overlay, child, allocation):
        """
        Callback for our GtkOverlay, think of this as a mini-
        window manager for our Stage.

        Depending on what type child is, we position it differently.
        We always call child.get_preferred_size() whether we plan to use
        it or not - this prevents allocation warning spew, particularly in
        Gtk >= 3.20.

        Returning True says, yes draw it.  Returning False tells it to skip
        drawing.

        If a new widget type is introduced that spawns directly on the stage,
        it must have its own handling code here.
        """
        if isinstance(child, NemoDesktopMonitorView):
            """
            MonitorView is always the size and position of its assigned monitor.
            This is calculated and stored by the child in child.rect)
            """
            w, h = child.get_preferred_size()
            allocation.x = child.rect.x
            allocation.y = child.rect.y
            allocation.width = child.rect.width
            allocation.height = child.rect.height

            return True

        return False
