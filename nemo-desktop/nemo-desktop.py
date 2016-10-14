#! /usr/bin/python3

import gi
gi.require_version('Gtk', '3.0')

from gi.repository import Gtk, Gdk
import signal
import gettext
import argparse
import os

import config
from util import utils
from nemoDesktopManager import NemoDesktopManager

signal.signal(signal.SIGINT, signal.SIG_DFL)
gettext.install("nemo", "/usr/share/locale")

class Main:
    """
    This is the main entry point to the program, and it shows up
    in the process list.  We do any theme preparation here as well.

    We start the ScreensaverService from here.
    """
    def __init__(self):
        parser = argparse.ArgumentParser(description='Nemo Desktop')
        parser.add_argument('--version', dest='version', action='store_true',
                            help='Display the current version')
        args = parser.parse_args()

        if args.version:
            print("nemo-desktop %s" % (config.VERSION))
            quit()

        Gtk.Settings.get_default().connect("notify::gtk-theme-name", self.on_theme_changed)
        self.do_style_overrides()

        NemoDesktopManager()
        Gtk.main()

if __name__ == "__main__":
    main = Main()



