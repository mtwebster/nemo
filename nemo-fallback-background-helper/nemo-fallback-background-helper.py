#! /usr/bin/python3

import gi
gi.require_version('Gtk', '3.0')

from gi.repository import Gtk, Gdk
from dbus.mainloop.glib import DBusGMainLoop
import signal
import gettext
import argparse
import os

from service import ScreensaverService

signal.signal(signal.SIGINT, signal.SIG_DFL)
gettext.install("nemo", "/usr/share/locale")

class Main:
    def __init__(self):
        ScreensaverService()
        Gtk.main()

if __name__ == "__main__":
    DBusGMainLoop(set_as_default=True)

    main = Main()



