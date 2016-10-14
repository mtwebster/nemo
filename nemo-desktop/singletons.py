#! /usr/bin/python3

import gi

from util import trackers, settings

# Our dbus proxies are abstracted out one level more than really necessary - we have
# clients that the screensaver initializes, that can never fail.  The actual connection
# business to the various dbus address is performed asynchronously from within each client.
# The following clients can fail to establish with their respective dbus interfaces without
# competely breaking the program (or at least that's what we're after) - it just means that
# depending on what fails, you may end up without keyboard shortcut support, or a battery 
# widget, etc...
from dbusdepot.cinnamonClient import CinnamonClient as _CinnamonClient
from dbusdepot.sessionClient import SessionClient as _SessionClient

CinnamonClient = _CinnamonClient()
SessionClient = _SessionClient()

# We only need one instance of CinnamonDesktop.BG - have it listen to bg gsettings changes
# and we just connect to "changed" on the Backgrounds object from our user (the Stage)
gi.require_version('CinnamonDesktop', '3.0')
from gi.repository import CinnamonDesktop

Backgrounds = CinnamonDesktop.BG()
Backgrounds.load_from_preferences(settings.bg_settings)
settings.bg_settings.connect("changed", lambda s,k: Backgrounds.load_from_preferences(s))
