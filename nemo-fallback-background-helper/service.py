#! /usr/bin/python3

from gi.repository import Gtk
import dbus, dbus.service, dbus.glib
import signal

import constants as c
from manager import ScreensaverManager

signal.signal(signal.SIGINT, signal.SIG_DFL)


class ScreensaverService(dbus.service.Object):
    def __init__(self):
        bus_name = dbus.service.BusName(c.SS_SERVICE, bus=dbus.SessionBus())
        dbus.service.Object.__init__(self, bus_name, c.SS_PATH)

        self.screen_manager = ScreensaverManager()

