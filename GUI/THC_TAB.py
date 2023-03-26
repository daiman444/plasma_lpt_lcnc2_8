#!/usr/bin/env python2
# -*- coding:UTF-8 -*-

import sys
import os
import hal_glib  # needed to make our own hal pins
import hal  # needed to make our own hal pins
import gobject
import gtk
import linuxcnc

from gladevcp.persistence import IniFile  # we use this one to save the states of the widgets on shut down and restart
from gladevcp.persistence import widget_defaults
from gladevcp.persistence import select_widgets
from gmoccapy import preferences
from gmoccapy import getiniinfo

GSTAT = hal_glib.GStat()
STATUS = linuxcnc.stat()
COMMANDS = linuxcnc.command()
INIPATH = os.environ.get('INI_FILE_NAME', '/dev/null')


class PlasmaClass:
    def __init__(self, halcomp, builder, useropts):
        self.hglib = hal_glib
        self.lcnc = linuxcnc
        self.command = linuxcnc.command()
        self.stat = linuxcnc.stat()
        self.inifile = self.lcnc.ini(INIPATH)
        self.builder = builder
        self.halcomp = halcomp
        GSTAT.connect('motion-mode-changed', self.check_state)
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

        self.list_btns_set_coord = ['gotozero', 'zero-xyz', 'zero-x', 'zero-y', 'zero-z', 'gotoend', 'set_coord',
                                  'btn_feed_minus', 'btn_feed_plus', 'txt_set_coord_x', 'txt_set_coord_y']

# TODO
# нужно структурировать виджеты по их функционалу для распределения когда какие должны быть активны

        #self.builder.get_object('lbl_print').set_label("123")

        for name in self.list_btns_set_coord:
            self.builder.get_object(name).set_sensitive(False)

        # pins in

        # pins out

        # labels


        # buttons
        self.builder.get_object('gotozero').connect('pressed', self.go_to_zero, 'G90 G0 Z30 X0 Y0 F800')
        self.builder.get_object('zero-xyz').connect('pressed', self.go_to_zero, 'G92 X0 Y0 Z0')
        self.builder.get_object('zero-x').connect('pressed', self.go_to_zero, 'G92 X0')
        self.builder.get_object('zero-y').connect('pressed', self.go_to_zero, 'G92 Y0')
        self.builder.get_object('zero-z').connect('pressed', self.go_to_zero, 'G92 Z0')
        self.builder.get_object('gotoend').connect('pressed', self.gotoend)
        self.builder.get_object('set_coord').connect('pressed', self.setcoord)

    def check_state(self, obj, data):
        self.builder.get_object('lbl_print').set_label(str(data))




    def go_to_zero(self, w, d=None):
        self.command.mode(linuxcnc.MODE_MDI)
        self.command.mdi(d)
        self.command.wait_complete()
        self.command.mode(linuxcnc.MODE_MANUAL)

    def gotoend(self, w, d=None):
        x_limit = self.inifile.find('AXIS_X', 'MIN_LIMIT')
        y_limit = self.inifile.find('AXIS_Y', 'MAX_LIMIT')
        self.command.mode(linuxcnc.MODE_MDI)
        self.command.mdi('G53 G00 Z0 ')
        self.command.wait_complete()
        self.command.mdi('G53 X{0} Y{1}'.format(x_limit, y_limit))
        self.command.wait_complete()
        self.command.mode(linuxcnc.MODE_MANUAL)


    def setcoord(self, w, d=None):
        x_coord = self.builder.get_object('txt_set_coord_x').get_text()
        y_coord = self.builder.get_object('txt_set_coord_y').get_text()
        self.command.mode(linuxcnc.MODE_MDI)
        self.command.mdi('G92 X{0} Y{1}'.format(float(x_coord), float(y_coord)))
        self.command.wait_complete()
        self.command.mode(linuxcnc.MODE_MANUAL)



def get_handlers(halcomp, builder, useropts):
    return [PlasmaClass(halcomp, builder, useropts)]
