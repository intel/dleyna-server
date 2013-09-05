#!/usr/bin/env python

# monitor_last_change
#
# Copyright (C) 2012 Intel Corporation. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU Lesser General Public License,
# version 2.1, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
# for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
#
# Regis Merlino <regis.merlino@intel.com>
# Mark Ryan <mark.d.ryan@intel.com>
#

import gobject
import dbus
import dbus.mainloop.glib
import json

def print_properties(props):
    print json.dumps(props, indent=4, sort_keys=True)

def changed(objects, path):
    print "Changed signal from [%s]" % path
    print "Objects:"
    print_properties(objects)

if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    bus = dbus.SessionBus()

    bus.add_signal_receiver(changed,
                            bus_name="com.intel.dleyna-server",
                            signal_name = "Changed",
                            path_keyword="path")

    mainloop = gobject.MainLoop()
    mainloop.run()
