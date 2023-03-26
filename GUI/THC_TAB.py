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
        #GSTAT.connect('homed', lambda w: self.check_state('homed'))
        #GSTAT.connect('motion-mode-changed', self.check_state)
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
                                        "hall_valueval": 125,
                                        "hall_valuemax": 130,
                                        "hall_valuemin": 120,
                                        "hall_valueincr": 1,
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

        # buttons reset coordinates
        self.builder.get_object('gotozero').connect('pressed', self.go_to_zero, 'G90 G0 Z30 X0 Y0 F800')
        self.builder.get_object('zero-xyz').connect('pressed', self.go_to_zero, 'G92 X0 Y0 Z0')
        self.builder.get_object('zero-x').connect('pressed', self.go_to_zero, 'G92 X0')
        self.builder.get_object('zero-y').connect('pressed', self.go_to_zero, 'G92 Y0')
        self.builder.get_object('zero-z').connect('pressed', self.go_to_zero, 'G92 Z0')
        self.builder.get_object('gotoend').connect('pressed', self.gotoend)
        self.builder.get_object('set_coord').connect('pressed', self.setcoord)

        # feed direction
        self.pin_feed_dir_plus = hal_glib.GPin(halcomp.newpin('feed-dir-plus', hal.HAL_BIT, hal.HAL_IN))
        self.pin_feed_dir_plus.connect('value-changed', self.feed_direction_change, 1)

        self.pin_feed_dir_minus = hal_glib.GPin(halcomp.newpin('feed-dir-minus', hal.HAL_BIT, hal.HAL_IN))
        self.pin_feed_dir_minus.connect('value-changed', self.feed_direction_change, -1)

        self.pin_feed_dir = hal_glib.GPin(halcomp.newpin('feed-dir', hal.HAL_FLOAT, hal.HAL_OUT))
        self.pin_feed_dir.value = 1

        self.btn_feed_dir_plus = builder.get_object('btn_feed_plus')
        self.btn_feed_dir_plus.connect('pressed', self.feed_direction_change, 1)
        self.btn_feed_dir_plus.set_sensitive(False)

        self.btn_feed_dir_minus = builder.get_object('btn_feed_minus')
        self.btn_feed_dir_minus.set_sensitive(True)
        self.btn_feed_dir_minus.connect('pressed', self.feed_direction_change, -1)

        self.lbl_feed_dir = self.builder.get_object('lbl_feed_dir')
        self.lbl_feed_dir.set_label('FWD')

        # push-buttons list for change values

        # volts requestion
        self.btn_volts_req_plus = builder.get_object('btn_volts_req_plus')
        self.btn_volts_req_plus.connect('pressed', self.widget_value_change, 'volts_req', 1)
        self.btn_volts_req_minus = builder.get_object('btn_volts_req_minus')
        self.btn_volts_req_minus.connect('pressed', self.widget_value_change, 'volts_req', -1)
        self.pin_volts_req = hal_glib.GPin(halcomp.newpin('volts-req', hal.HAL_FLOAT, hal.HAL_OUT))
        self.pin_volts_req.value = self.volts_reqval
        self.builder.get_object('lbl_volts_req').set_label(str(self.volts_reqval))

        # correction velocity
        self.btn_cor_vel_plus = builder.get_object('btn_cor_vel_plus')
        self.btn_cor_vel_plus.connect('pressed', self.widget_value_change, 'cor_vel', 1)
        self.btn_cor_vel_minus = builder.get_object('btn_cor_vel_minus')
        self.btn_cor_vel_minus.connect('pressed', self.widget_value_change, 'cor_vel', -1)
        self.pin_cor_vel = hal_glib.GPin(halcomp.newpin('cor-vel', hal.HAL_FLOAT, hal.HAL_OUT))
        self.pin_cor_vel.value = self.volts_reqval
        self.builder.get_object('lbl_cor_vel').set_label(str(self.cor_velval))

        # velocity tolerance
        self.btn_vel_tol_plus = builder.get_object('btn_vel_tol_plus')
        self.btn_vel_tol_plus.connect('pressed', self.widget_value_change, 'vel_tol', 1)
        self.btn_vel_tol_minus = builder.get_object('btn_vel_tol_minus')
        self.btn_vel_tol_minus.connect('pressed', self.widget_value_change, 'vel_tol', -1)
        self.pin_vel_tol = hal_glib.GPin(halcomp.newpin('vel-tol', hal.HAL_FLOAT, hal.HAL_OUT))
        self.pin_vel_tol.value = self.vel_tolval
        self.builder.get_object('lbl_vel_tol').set_label(str(self.vel_tolval))

        # hall sensor value
        self.btn_hall_value_plus = builder.get_object('btn_hall_value_plus')
        self.btn_hall_value_plus.connect('pressed', self.widget_value_change, 'hall_value', 1)
        self.btn_hall_value_minus = builder.get_object('btn_hall_value_minus')
        self.btn_hall_value_minus.connect('pressed', self.widget_value_change, 'hall_value', -1)
        self.pin_hall_value = hal_glib.GPin(halcomp.newpin('hall-value', hal.HAL_FLOAT, hal.HAL_OUT))
        self.pin_hall_value.value = self.hall_valueval
        self.builder.get_object('lbl_hall_value').set_label(str(self.hall_valueval))

        # pierce height
        self.btn_pierce_hght_plus = builder.get_object('btn_pierce_hght_plus')
        self.btn_pierce_hght_plus.connect('pressed', self.widget_value_change, 'pierce_hght', 1)
        self.btn_pierce_hght_minus = builder.get_object('btn_pierce_hght_minus')
        self.btn_pierce_hght_minus.connect('pressed', self.widget_value_change, 'pierce_hght', -1)
        self.pin_pierce_hght = hal_glib.GPin(halcomp.newpin('pierce-hght', hal.HAL_FLOAT, hal.HAL_OUT))
        self.pin_pierce_hght.value = self.pierce_hghtval
        self.builder.get_object('lbl_pierce_hght').set_label(str(self.pierce_hghtval))

        # jump height
        self.btn_jump_hght_plus = builder.get_object('btn_jump_hght_plus')
        self.btn_jump_hght_plus.connect('pressed', self.widget_value_change, 'jump_hght', 1)
        self.btn_jump_hght_minus = builder.get_object('btn_jump_hght_minus')
        self.btn_jump_hght_minus.connect('pressed', self.widget_value_change, 'jump_hght', -1)
        self.btn_jump_hght_minus.set_sensitive(False)
        self.pin_jump_hght = hal_glib.GPin(halcomp.newpin('jump-hght', hal.HAL_FLOAT, hal.HAL_OUT))
        self.pin_jump_hght.value = self.jump_hghtval
        self.builder.get_object('lbl_jump_hght').set_label(str(self.jump_hghtval))

        # pierce delay
        self.btn_pierce_del_plus = builder.get_object('btn_pierce_del_plus')
        self.btn_pierce_del_plus.connect('pressed', self.widget_value_change, 'pierce_del', 1)
        self.btn_pierce_del_minus = builder.get_object('btn_pierce_del_minus')
        self.btn_pierce_del_minus.connect('pressed', self.widget_value_change, 'pierce_del', -1)
        self.btn_pierce_del_minus.set_sensitive(False)
        self.pin_pierce_del = hal_glib.GPin(halcomp.newpin('pierce-del', hal.HAL_FLOAT, hal.HAL_OUT))
        self.pin_pierce_del.value = self.pierce_delval
        self.builder.get_object('lbl_pierce_del').set_label(str(self.pierce_delval))

        # cutting height
        self.btn_cut_hght_plus = builder.get_object('btn_cut_hght_plus')
        self.btn_cut_hght_plus.connect('pressed', self.widget_value_change, 'cut_hght', 1)
        self.btn_cut_hght_minus = builder.get_object('btn_cut_hght_minus')
        self.btn_cut_hght_minus.connect('pressed', self.widget_value_change, 'cut_hght', -1)
        self.pin_cut_hght = hal_glib.GPin(halcomp.newpin('cut-hght', hal.HAL_FLOAT, hal.HAL_OUT))
        self.pin_cut_hght.value = self.cut_hghtval
        self.builder.get_object('lbl_cut_hght').set_label(str(self.cut_hghtval))

        # stop delay
        self.btn_stop_del_plus = builder.get_object('btn_stop_del_plus')
        self.btn_stop_del_plus.connect('pressed', self.widget_value_change, 'stop_del', 1)
        self.btn_stop_del_minus = builder.get_object('btn_stop_del_minus')
        self.btn_stop_del_minus.connect('pressed', self.widget_value_change, 'stop_del', -1)
        self.pin_stop_del = hal_glib.GPin(halcomp.newpin('stop-del', hal.HAL_FLOAT, hal.HAL_OUT))
        self.pin_stop_del.value = self.stop_delval
        self.builder.get_object('lbl_stop_del').set_label(str(self.stop_delval))

        # safe Z height
        self.btn_safe_z_plus = builder.get_object('btn_safe_z_plus')
        self.btn_safe_z_plus.connect('pressed', self.widget_value_change, 'safe_z', 1)
        self.btn_safe_z_minus = builder.get_object('btn_safe_z_minus')
        self.btn_safe_z_minus.connect('pressed', self.widget_value_change, 'safe_z', -1)
        self.pin_safe_z = hal_glib.GPin(halcomp.newpin('safe-z', hal.HAL_FLOAT, hal.HAL_OUT))
        self.pin_safe_z.value = self.safe_zval
        self.builder.get_object('lbl_safe_z').set_label(str(self.safe_zval))

        # probing velocity
        self.btn_z_speed_plus = builder.get_object('btn_z_speed_plus')
        self.btn_z_speed_plus.connect('pressed', self.widget_value_change, 'z_speed', 1)
        self.btn_z_speed_minus = builder.get_object('btn_z_speed_minus')
        self.btn_z_speed_minus.connect('pressed', self.widget_value_change, 'z_speed', -1)
        self.pin_z_speed = hal_glib.GPin(halcomp.newpin('z-speed', hal.HAL_FLOAT, hal.HAL_OUT))
        self.pin_z_speed.value = self.z_speedval
        self.builder.get_object('lbl_z_speed').set_label(str(self.z_speedval))

        # TODO дополнить необходимыми вызовами




    def check_state(self, data):
        pass
        '''
        if data == 'homed':
            for i in self.list_btns_set_coord:
                self.builder.get_object(i).set_sensitive(True)
        self.builder.get_object('lbl_print').set_label(str(data))
'''
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

    def feed_direction_change(self, widget, value):
        if isinstance(widget, hal_glib.GPin):
            if widget.get() is True:
                self.pin_feed_dir.value += self.feed_directincr * value
        if isinstance(widget, gtk.Button):
            self.pin_feed_dir.value += self.feed_directincr * value
        if self.pin_feed_dir.value >= self.feed_directmax:
            self.pin_feed_dir.value = self.feed_directmax
            self.btn_feed_dir_plus.set_sensitive(False)
            self.lbl_feed_dir.set_label('FWD')
        elif self.pin_feed_dir.value <= self.feed_directmin:
            self.pin_feed_dir.value = self.feed_directmin
            self.btn_feed_dir_minus.set_sensitive(False)
            self.lbl_feed_dir.set_label('BWD')
        else:
            self.btn_feed_dir_plus.set_sensitive(True)
            self.btn_feed_dir_minus.set_sensitive(True)
            self.lbl_feed_dir.set_label('STOP')

    def widget_value_change(self, widget, name, value):
        self.builder.get_object('lbl_print').set_label(str(type(widget)))
        self.builder.get_object('lbl_print1').set_label(str(name))
        self.builder.get_object('lbl_print2').set_label(str(value))
    # TODO закончить метод



def get_handlers(halcomp, builder, useropts):
    return [PlasmaClass(halcomp, builder, useropts)]
