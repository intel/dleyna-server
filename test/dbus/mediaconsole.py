# mediaconsole
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
# Mark Ryan <mark.d.ryan@intel.com>
#

import dbus
import sys
import json

from xml.dom import minidom

def print_properties(props):
    print json.dumps(props, indent=4, sort_keys=True)

class MediaObject(object):

    def __init__(self, path):
        bus = dbus.SessionBus()
        self._propsIF = dbus.Interface(bus.get_object(
                'com.intel.dleyna-server', path),
                                        'org.freedesktop.DBus.Properties')
        self.__objIF = dbus.Interface(bus.get_object(
                'com.intel.dleyna-server', path),
                                      'org.gnome.UPnP.MediaObject2')

    def get_props(self, iface = ""):
        return self._propsIF.GetAll(iface)

    def get_prop(self, prop_name, iface = ""):
        return self._propsIF.Get(iface, prop_name)

    def print_prop(self, prop_name, iface = ""):
        print_properties(self._propsIF.Get(iface, prop_name))

    def print_props(self, iface = ""):
        print_properties(self._propsIF.GetAll(iface))

    def print_dms_id(self):
        path = self._propsIF.Get("", "Path")
        dms_id = path[path.rfind("/") + 1:]
        i = 0
        while i+1 < len(dms_id):
            num = dms_id[i] + dms_id[i+1]
            sys.stdout.write(unichr(int(num, 16)))
            i = i + 2
        print

    def print_metadata(self):
        metadata = self.__objIF.GetMetaData()
        print minidom.parseString(metadata).toprettyxml(indent=' '*4)

    def delete(self):
        return self.__objIF.Delete()

    def update(self, to_add_update, to_delete):
        return self.__objIF.Update(to_add_update, to_delete)

    def get_metadata(self):
        return self.__objIF.GetMetaData()

class Item(MediaObject):
    def __init__(self, path):
        MediaObject.__init__(self, path)
        bus = dbus.SessionBus()
        self._itemIF = dbus.Interface(bus.get_object(
                'com.intel.dleyna-server', path),
                                       'org.gnome.UPnP.MediaItem2')

    def print_compatible_resource(self, protocol_info, fltr):
        print_properties(self._itemIF.GetCompatibleResource(protocol_info,
                                                             fltr))

class Container(MediaObject):

    def __init__(self, path):
        MediaObject.__init__(self, path)
        bus = dbus.SessionBus()
        self._containerIF = dbus.Interface(bus.get_object(
                'com.intel.dleyna-server', path),
                                            'org.gnome.UPnP.MediaContainer2')

    def list_children(self, offset, count, fltr, sort=""):
        objects = self._containerIF.ListChildrenEx(offset, count, fltr, sort)
        for item in objects:
            print_properties(item)
            print ""

    def list_containers(self, offset, count, fltr, sort=""):
        objects = self._containerIF.ListContainersEx(offset, count, fltr, sort)
        for item in objects:
            print_properties(item)
            print ""

    def list_items(self, offset, count, fltr, sort=""):
        objects = self._containerIF.ListItemsEx(offset, count, fltr, sort)
        for item in objects:
            print_properties(item)
            print ""

    def search(self, query, offset, count, fltr, sort=""):
        objects, total = self._containerIF.SearchObjectsEx(query, offset,
                                                            count, fltr, sort)
        print "Total Items: " + str(total)
        print
        for item in objects:
            print_properties(item)
            print ""

    def tree(self, level=0):
        objects = self._containerIF.ListChildren(
            0, 0, ["DisplayName", "Path", "Type"])
        for props in objects:
            print (" " * (level * 4) + props["DisplayName"] +
                   " : (" + props["Path"]+ ")")
            if props["Type"] == "container":
                Container(props["Path"]).tree(level + 1)

    def upload(self, name, file_path):
        (tid, path) = self._containerIF.Upload(name, file_path)
        print "Transfer ID: " + str(tid)
        print u"Path: " + path

    def create_container(self, name, type, child_types):
        path = self._containerIF.CreateContainer(name, type, child_types)
        print u"New container path: " + path


    def print_compatible_resource(self, protocol_info, fltr):
        print_properties(self._containerIF.GetCompatibleResource(protocol_info,
								 fltr))

class Device(Container):

    def __init__(self, path):
        Container.__init__(self, path)
        bus = dbus.SessionBus()
        self._deviceIF = dbus.Interface(bus.get_object(
                'com.intel.dleyna-server', path),
                                         'com.intel.dLeynaServer.MediaDevice')

    def upload_to_any(self, name, file_path):
        (tid, path) = self._deviceIF.UploadToAnyContainer(name, file_path)
        print "Transfer ID: " + str(tid)
        print u"Path: " + path

    def create_container_in_any(self, name, type, child_types):
        path = self._deviceIF.CreateContainerInAnyContainer(name, type,
                                                             child_types)
        print u"New container path: " + path

    def get_upload_status(self, id):
        (status, length, total) = self._deviceIF.GetUploadStatus(id)
        print "Status: " + status
        print "Length: " + str(length)
        print "Total: " + str(total)

    def get_upload_ids(self):
        upload_ids  = self._deviceIF.GetUploadIDs()
        print_properties(upload_ids)

    def cancel_upload(self, id):
        self._deviceIF.CancelUpload(id)

    def cancel(self):
        return self._deviceIF.Cancel()


class UPNP(object):

    def __init__(self):
        bus = dbus.SessionBus()
        self._manager = dbus.Interface(bus.get_object(
                'com.intel.dleyna-server',
                '/com/intel/dLeynaServer'),
                                        'com.intel.dLeynaServer.Manager')

    def servers(self):
        for i in self._manager.GetServers():
            try:
                server = Container(i)
                try:
                    folderName = server.get_prop("FriendlyName");
                except Exception:
                    folderName = server.get_prop("DisplayName");
                print u'{0:<30}{1:<30}'.format(folderName , i)
            except dbus.exceptions.DBusException, err:
                print u"Cannot retrieve properties for " + i
                print str(err).strip()[:-1]

    def version(self):
        print self._manager.GetVersion()

    def set_protocol_info(self, protocol_info):
        self._manager.SetProtocolInfo(protocol_info)

    def prefer_local_addresses(self, prefer):
        self._manager.PreferLocalAddresses(prefer)
