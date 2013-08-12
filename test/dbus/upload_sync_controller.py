# upload_sync_controller
#
# Copyright (C) 2013 Intel Corporation. All rights reserved.
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

import ConfigParser
import os
import shutil
import urllib
import dbus
import gio
import glib

from mediaconsole import UPNP, MediaObject, Container, Device

class _UscUpnp(UPNP):
    def __init__(self):
        UPNP.__init__(self)

    def get_servers(self):
        return self._manager.GetServers()

class _UscDevice(Device):
    def __init__(self, path):
        Device.__init__(self, path)

    def create_container_in_any(self, name, type, child_types):
        return self._deviceIF.CreateContainerInAnyContainer(name, type,
                                                            child_types)

class _UscContainer(Container):
    def __init__(self, path):
        Container.__init__(self, path)

    def create_container(self, name, type, child_types):
        return self._containerIF.CreateContainer(name, type, child_types)

    def upload(self, name, file_path):
        return self._containerIF.Upload(name, file_path)

class UscError(Exception):
    """An Upload Sync Controller error."""

    def __init__(self, message):
        """
        message: description of the error
        """

        Exception.__init__(self, message)
        self.message = message

    def __str__(self):
        return 'DscError: ' + self.message

class _UscStore(object):
    REMOTE_ID_OPTION = 'remote_id'
    TYPE_OPTION = 'type'

    def __init__(self, root_path, server_id):
        if not os.path.exists(root_path):
            os.makedirs(root_path)

        self.__config_path = os.path.join(root_path, server_id + '.conf')

        self.__config = ConfigParser.ConfigParser()

    def initialize(self):
        self.__config.read(self.__config_path)

    def __write_config(self):
        with open(self.__config_path, 'wb') as configfile:
            self.__config.write(configfile)

    def remove(self):
        if os.path.exists(self.__config_path):
            os.remove(self.__config_path)

    def __parent_deleted(self, path, deleted_containers):
        for del_path in deleted_containers:
            if path.startswith(del_path):
                return True

        return False

    def __remove_file(self, path):
        id = self.__config.get(path, self.REMOTE_ID_OPTION)
        print u'\tDeleting server object {0}'.format(id)
        try:
            obj = MediaObject(id)
            obj.delete()
        except:
            pass

        print u'\tDeleting local cache {0}'.format(path)
        self.__config.remove_section(path)

    def sync_deleted_files(self, path):
        deleted_containers = []

        sections = self.__config.sections()
        if not sections:
            return

        for path in sections:
            if self.__parent_deleted(path, deleted_containers):
                print u'\tDeleting local cache {0}'.format(path)
                self.__config.remove_section(path)
            elif not os.path.exists(path):
                if self.__config.get(path, self.TYPE_OPTION) == 'container':
                    deleted_containers.append(path + '/')

                self.__remove_file(path)

        self.__write_config()

    def __add_file(self, container, name, parent):
        new_path = os.path.join(parent, name)
        if os.path.isdir(new_path):
            if self.__config.has_section(new_path):
                id = self.__config.get(new_path, self.REMOTE_ID_OPTION)
            else:
                print u'\tCreating server container for {0}'.format(new_path)
                id = container.create_container(name, 'container', ['*'])
    
                print u'\tStoring cached data for {0}'.format(id)
                self.__config.add_section(new_path)
                self.__config.set(new_path, self.REMOTE_ID_OPTION, id)
                self.__config.set(new_path, self.TYPE_OPTION, 'container')

            new_container = _UscContainer(id)
            self.sync_added_files(new_container, new_path)
        elif not self.__config.has_section(new_path):
            print u'\tUploading file {0}'.format(new_path)
            (up, id) = container.upload(name, new_path)

            print u'\tStoring cached data for {0}'.format(id)
            self.__config.add_section(new_path)
            self.__config.set(new_path, self.REMOTE_ID_OPTION, id)
            self.__config.set(new_path, self.TYPE_OPTION, 'item')

    def sync_added_files(self, container, path):
        children = os.listdir(path)
        for child in children:
            self.__add_file(container, child, path)

        self.__write_config()

class UscController(object):
    """An Upload Sync Controller sample app.

    The Upload Sync Controller propagates changes in a local folder to media
    servers (DMS/M-DMS) to be added to their list of available content.
    Media servers must expose the 'content-synchronization' capability to
    be managed by this controller.

    The four main methods are servers(), track(), add_remote() and sync().
    * servers() lists the media servers available on the network
    * track() is used to set the local folder that is to be synchronized
    * add_server() is used to add a media server to the list of servers that
      are to be synchronized
    * start_sync() launches the servers synchronisation from the local storage

    Sample usage:
    >>> controller.servers()
    >>> controller.track('/home/user/local_folder')
    >>> controller.add_server('/com/intel/dLeynaServer/server/0')
    >>> controller.start_sync()
    """

    CONFIG_PATH = os.environ['HOME'] + '/.config/upload-sync-controller.conf'
    ROOT_CONTAINER_ID_OPTION = 'root_container_id'
    DATA_PATH_SECTION = '__paths__'
    DATA_PATH_OPTION = 'data_path'
    TRACK_PATH_OPTION = 'track_path'

    def __init__(self, rel_path = None):
        """
        rel_path: if provided, contains the relative local database path,
                  from the user's HOME directory.
                  If not provided, the local database path will be
                  '$HOME/upload-sync-controller'
        """

        self.__upnp = _UscUpnp()

        self.__config = ConfigParser.ConfigParser()
        self.__config.read(UscController.CONFIG_PATH)
        
        if rel_path:
            self.__set_data_path(rel_path)
        elif not self.__config.has_section(UscController.DATA_PATH_SECTION):
            self.__set_data_path('upload-sync-controller')
            
        self.__store_path = self.__config.get(UscController.DATA_PATH_SECTION,
                                              UscController.DATA_PATH_OPTION)
        try:
            self.__track_path = self.__config.get(
                                              UscController.DATA_PATH_SECTION,
                                              UscController.TRACK_PATH_OPTION)
        except:
            self.__track_path = None

    def __write_config(self):
        with open(UscController.CONFIG_PATH, 'wb') as configfile:
            self.__config.write(configfile)

    def __set_data_path(self, rel_path):
        data_path = os.environ['HOME'] + '/' + rel_path

        if not self.__config.has_section(UscController.DATA_PATH_SECTION):
            self.__config.add_section(UscController.DATA_PATH_SECTION)

        self.__config.set(UscController.DATA_PATH_SECTION,
                          UscController.DATA_PATH_OPTION, data_path)

        self.__write_config()

    def __check_trackable(self, server):
        try:
            try:
                srt = server.get_prop('ServiceResetToken')
            except:
                raise DscError("'ServiceResetToken' variable not supported")

            try:
                dlna_caps = server.get_prop('DLNACaps')
                if not 'content-synchronization' in dlna_caps:
                    raise
            except:
                raise DscError("'content-synchronization' cap not supported")

            try:
                search_caps = server.get_prop('SearchCaps')
                if not [x for x in search_caps if 'ObjectUpdateID' in x]:
                    raise
                if not [x for x in search_caps if 'ContainerUpdateID' in x]:
                    raise
            except:
                raise DscError("'objectUpdateID' search cap not supported")

            return srt
        except DscError as err:
            print err
            return None

    def __need_sync(self, servers):
        for item in servers:
            device = _UscDevice(item)
            uuid = device.get_prop('UDN')

            if self.__config.has_section(uuid):
                yield device, uuid, self.__config.has_option(uuid,
                                    UscController.ROOT_CONTAINER_ID_OPTION)

    def track(self, track_path):
        """Sets the local folder that is to be synchronized."""
        
        self.__track_path = track_path
        self.__config.set(UscController.DATA_PATH_SECTION,
                          UscController.TRACK_PATH_OPTION, track_path)

        self.__write_config()

    def start_sync(self):
        """Performs a synchronization for all the media servers.

        Displays some progress information during the process.
        """

        if not self.__track_path:
            print u'Error: track path is not set'
            return

        count = 0
        print u'Syncing...'
        for device, uuid, init in self.__need_sync(self.__upnp.get_servers()):
            count += 1

            store = _UscStore(self.__store_path, uuid)
            store.initialize()

            if not init:
                print u'Performing initial sync for server {0}:'.format(uuid)
                root = device.create_container_in_any('usc_root', 'container',
                                                      ['image'])
                self.__config.set(uuid, UscController.ROOT_CONTAINER_ID_OPTION,
                                  root)
                self.__write_config()
            else:
                print u'Performing normal sync for server {0}:'.format(uuid)
                root = self.__config.get(uuid,
                                         UscController.ROOT_CONTAINER_ID_OPTION)

            print u'\tRoot container is {0}'.format(root)
            root_container = _UscContainer(root)
            store.sync_deleted_files(self.__track_path)
            store.sync_added_files(root_container, self.__track_path)
            print u'Done.'

        if count == 0:
            print u'Nothing to do, stopping.'
            return

        # TODO: monitor changes

    def servers(self):
        """Displays media servers available on the network.

        Displays media servers information as well as the synchronized status.
        """

        print u'Running servers:'

        for item in self.__upnp.get_servers():
            try:
                server = Container(item)
                try:
                    folder_name = server.get_prop('FriendlyName')
                except Exception:
                    folder_name = server.get_prop('DisplayName')
                device = Device(item)
                dev_uuid = device.get_prop('UDN')
                dev_path = device.get_prop('Path')

                print u'{0:<25}  Synchronized({2})  {3}  {1}'.format(
                                            folder_name,
                                            dev_path,
                                            self.__config.has_section(dev_uuid),
                                            dev_uuid)

            except dbus.exceptions.DBusException as err:
                print u'Cannot retrieve properties for ' + item
                print str(err).strip()[:-1]

    def synchronized(self):
        """Displays the list of servers currently synchronized."""

        print u'Synchronized servers:'

        for name in self.__config.sections():
            if name != UscController.DATA_PATH_SECTION:
                print u'{0:<30}'.format(name)

    def add_server(self, server_path):
        """Adds a media server to the controller's list.

        server_path: d-bus path for the media server
        """

        server = Device(server_path)
        server_uuid = server.get_prop('UDN')
        
        if not self.__config.has_section(server_uuid):
            srt = self.__check_trackable(server)
            if srt != None:
                self.__config.add_section(server_uuid)
                self.__write_config()
            else:
                print u"Sorry, the server {0} has no such capability and " \
                        "will not be synchronized.".format(server_path)

    def __remove_server(self, server_uuid):
        try:
            root = self.__config.get(server_uuid,
                                     UscController.ROOT_CONTAINER_ID_OPTION)
            MediaObject(root).delete()
        except:
            pass
        self.__config.remove_section(server_uuid)
        
        store = _UscStore(self.__store_path, server_uuid)
        store.remove()

    def remove_server(self, server_path):
        """Removes a media server from the controller's list.

        Also removes the server side synchronized file and folders.
        server_path: d-bus path for the media server
        """

        server = Device(server_path)
        server_uuid = server.get_prop('UDN')

        if self.__config.has_section(server_uuid):
            self.__remove_server(server_uuid)

        self.__write_config()

    def reset(self):
        """Removes all media servers from the controller's list
        
        Also removes the server side synchronized file and folders.
        """

        for name in self.__config.sections():
            if name != UscController.DATA_PATH_SECTION:
                self.__remove_server(name)

        self.__write_config()

if __name__ == '__main__':
    print u'An Upload Sync Controller sample app.'
    print
    controller = UscController()
    controller.servers()
    print
    print u'"controller" instance is ready for use.'
    print u'Type "help(UscController)" for more details and usage samples.'
