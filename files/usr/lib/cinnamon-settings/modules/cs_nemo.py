#!/usr/bin/env python

from SettingsWidgets import *
from gi.repository import Nemo
import sys

sys.path.append('/usr/share/nemo-python/extensions')

class Module:
    def __init__(self, content_box):
        keywords = _("nemo, extension, action, script")
        sidePage = SidePage(_("Nemo Plugins"), "nemo", keywords, content_box, 480, module=self)
        self.sidePage = sidePage
        self.name = "nemo"
        self.comment = _("Manage Nemo Plugins")
        self.category = "prefs"

    def on_module_selected(self):
        if not self.loaded:
            print "Loading Nemo Plugins module"
            w = Nemo.PluginManagerWidget.new()
            w.expand = True
            self.sidePage.add_widget(w)