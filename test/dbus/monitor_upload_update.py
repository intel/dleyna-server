#!/usr/bin/env python

# monitor_upload_update
#
# Copyright (C) 2012-2017 Intel Corporation. All rights reserved.
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

def upload_update(upload_id, status, uploaded, to_upload, path):
    print "UploadUpdate signal from [%s]" % (path)
    print "Upload ID: " + str(upload_id)
    print "Status: " + status
    print "Uploaded: " + str(uploaded)
    print "To Upload: " + str(to_upload)

if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    bus = dbus.SessionBus()

    bus.add_signal_receiver(upload_update,
                            bus_name="com.intel.dleyna-server",
                            signal_name = "UploadUpdate",
                            path_keyword="path")

    mainloop = gobject.MainLoop()
    mainloop.run()
