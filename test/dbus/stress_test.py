# stress-test
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
#

import gobject
import dbus
import dbus.mainloop.glib

def handle_browse_reply(objects):
    print "Total Items: " + str(len(objects))

def handle_error(err):
    if err.get_dbus_name() == 'com.intel.dleyna.Cancelled':
        print "Cancelled..."
    else:
        print "An error occured"
        print err
        loop.quit()

def make_async_calls():
    i = 0
    while i < 5:
        root.ListChildrenEx(0,
                            5,
                            ["DisplayName", "Type"],
                            "-DisplayName",
                            reply_handler=handle_browse_reply,
                            error_handler=handle_error)
        i += 1
    device.Cancel()
    gobject.timeout_add(1000, make_async_calls)

dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

bus = dbus.SessionBus()
root = dbus.Interface(bus.get_object(
        'com.intel.dleyna-server',
        '/com/intel/dLeynaServer/server/0'),
                      'org.gnome.UPnP.MediaContainer2')
device = dbus.Interface(bus.get_object(
        'com.intel.dleyna-server',
        '/com/intel/dLeynaServer/server/0'),
                        'com.intel.dLeynaServer.MediaDevice')

gobject.timeout_add(1000, make_async_calls)

loop = gobject.MainLoop()
loop.run()
