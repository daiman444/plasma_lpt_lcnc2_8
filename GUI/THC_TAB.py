#!/usr/bin/env python2
# -*- coding:UTF-8 -*-

import hal_glib  # needed to make our own hal pins
import hal  # needed to make our own hal pins
import gtk
import linuxcnc

from gladevcp.persistence import IniFile  # we use this one to save the states of the widgets on shut down and restart
from gladevcp.persistence import widget_defaults
from gladevcp.persistence import select_widgets
from gmoccapy import preferences
from gmoccapy import getiniinfo


class PlasmaClass:
    def __init__(self, halcomp, builder, useropts):
        self.command = linuxcnc.command()
        self.stat = linuxcnc.stat()
        self.builder = builder
        self.halcomp = halcomp
        self.defaults = {IniFile.vars: {"pierce_hghtval": 7.0,
                                        "pierce_hghtmax": 15.0,
                                        "pierce_hghtmin": 1.0,
                                        "pierce_hghtincr": 0.5,
                                        "jump_hghtval": 0.0,
                                        "jump_hghtmax": 15.0,
                                        "jump_hghtmin": 0.0,
                                        "jump_hghtincr": 0.5,
                                        "cut_hghtval": 9.0,
                                        "cut_hghtmax": 15.0,
                                        "cut_hghtmin": 0.0,
                                        "cut_hghtincr": 0.5,
                                        "pierce_delval": 0.0,
                                        "pierce_delmax": 5.0,
                                        "pierce_delmin": 0.0,
                                        "pierce_delincr": 0.1,
                                        "safe_zval": 30.0,
                                        "safe_zmax": 100.0,
                                        "safe_zmin": 0.0,
                                        "safe_zincr": 5.0,
                                        "z_speedval": 750.0,
                                        "z_speedmax": 1000.0,
                                        "z_speedmin": 100.0,
                                        "z_speedincr": 50.0,
                                        "stop_delval": 13.0,
                                        "stop_delmax": 20.0,
                                        "stop_delmin": 0.0,
                                        "stop_delincr": 1.0,
                                        "cor_velval": 20.0,
                                        "cor_velmax": 100.0,
                                        "cor_velmin": 0.0,
                                        "cor_velincr": 5.0,
                                        "vel_tolval": 90.0,
                                        "vel_tolmax": 100.0,
                                        "vel_tolmin": 0.0,
                                        "vel_tolincr": 5.0,
                                        "feed_directval": 1,
                                        "feed_directmax": 1,
                                        "feed_directmin": -1,
                                        "feed_directincr": 1,
                                        "volts_reqval": 125,
                                        "volts_reqmax": 130,
                                        "volts_reqmin": 120,
                                        "volts_reqincr": 1,
                                        },
                         IniFile.widgets: widget_defaults(select_widgets([self.builder.get_object("hal-btn-THC")],
                                                                         hal_only=True, output_only=True)),
                         }

        get_ini_info = getiniinfo.GetIniInfo()
        prefs = preferences.preferences(get_ini_info.get_preference_file_path())
        theme_name = prefs.getpref("gtk_theme", "Follow System Theme", str)
        if theme_name == "Follow System Theme":
            theme_name = gtk.settings_get_default().get_property("gtk-theme-name")
        gtk.settings_get_default().set_string_property("gtk-theme-name", theme_name, "")

        self.ini_filename = __name__ + ".var"
        self.ini = IniFile(self.ini_filename, self.defaults, self.builder)
        self.ini.restore_state(self)

        # labels
        self.lbl_print = self.builder.get_object('lbl_print')

        self.buttons = ['frame2', 'frame6', 'frame3', 'scrolledwindow1', ]


    def update_status(self):
        self.stat.poll()
        status = self.stat.estop
        self.lbl_print.set_property('label', str(status))



    def widgets_sensetive(self, w_list, value):
        for name in w_list:
            self.builder.get_object(name).set_sensitive(value)


def get_handlers(halcomp, builder, useropts):
    return [PlasmaClass(halcomp, builder, useropts)]
