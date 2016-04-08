#! /usr/bin/python3

from gi.repository import Gdk
import time
import sys
import traceback

import constants as c
import trackers
from stage import Stage

class ScreensaverManager:
    def __init__(self):
        self.screen = Gdk.Screen.get_default()

        self.stage = None
        self.stage_fader = None

        self.spawn_stage()

    def spawn_stage(self):
        try:
            self.stage = Stage(self.screen, self)
        except Exception:
            print("Could not spawn screensaver stage:\n")
            traceback.print_exc()
            raise e


