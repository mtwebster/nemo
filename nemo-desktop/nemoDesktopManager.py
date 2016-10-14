#! /usr/bin/python3

from gi.repository import Gdk, GObject, Gtk
import time
import traceback

import constants as c
import status

from stage import Stage
import singletons
from util import utils, settings, trackers

class NemoDesktopManager(GObject.Object):
    """
    The ScreensaverManager is the central point where most major decision are made, 
    and where ScreensaverService requests are acted upon.
    """
    def __init__(self):
        super(NemoDesktopManager, self).__init__()

        self.screen = Gdk.Screen.get_default()

        status.Active = False

        self.stage = None

        try:
            self.stage = DesktopStage(self.screen, self)
            self.stage.transition_in(250, self.on_spawn_stage_complete)
        except Exception:
            print("Could not spawn nemo-desktop stage:\n")
            traceback.print_exc()
            Gtk.main_quit()

    def despawn_stage(self, effect_time=c.STAGE_DESPAWN_TRANSITION, callback=None):
        """
        Begin destruction of the stage.
        """
        self.stage.transition_out(250, on_despawn_stage_complete)

    def on_spawn_stage_complete(self):
        """
        Called after the stage has faded in.  All user events are now
        redirected to GrabHelper, our status is updated, our active timer
        is started, and emit an active-changed signal (Which is listened to
        by our ConsoleKit client if we're using it, and our own ScreensaverService.)
        """
        status.Active = True

    def on_despawn_stage_complete(self):
        """
        Called after the stage has faded out - the stage is destroyed, our status
        is updated, timer is canceled and active-changed is fired.
        """
        status.Active = False

        self.stage.destroy_stage()
        self.stage = None

        # Ideal time to check for leaking connections that might prevent GC by python and gobject
        if trackers.DEBUG_SIGNALS:
            trackers.con_tracker_get().dump_connections_list()

        if trackers.DEBUG_TIMERS:
            trackers.timer_tracker_get().dump_timer_list()

