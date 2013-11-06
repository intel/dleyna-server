# download_sync_controller
#
# Copyright (C) 2012-2013 Intel Corporation. All rights reserved.
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

from mediaconsole import UPNP, Container, Device

class _DscUpnp(UPNP):
    def __init__(self):
        UPNP.__init__(self)

    def get_servers(self):
        return self._manager.GetServers()

class _DscContainer(Container):
    def __init__(self, path):
        Container.__init__(self, path)
        self.__path = path

    def find_containers(self):
        return self._containerIF.SearchObjectsEx(
                                        'Type derivedfrom "container"',
                                        0, 0,
                                        ['DisplayName', 'Path', 'Type'], '')[0]
    def find_updates(self, upd_id):
        return self._containerIF.SearchObjectsEx(
                            'ObjectUpdateID > "{0}"'.format(upd_id),
                            0, 0,
                            ['DisplayName', 'Path', 'RefPath', 'URLs',
                             'Type', 'Parent'], '')[0]

    def find_children(self):
        return self._containerIF.ListChildrenEx(0, 0,
                                             ['DisplayName', 'Path', 'RefPath',
                                              'URLs', 'Parent', 'Type'], '')

class DscError(Exception):
    """A Download Sync Controller error."""
    def __init__(self, message):
        """
        message: description of the error

        """
        Exception.__init__(self, message)
        self.message = message

    def __str__(self):
        return 'DscError: ' + self.message

class _DscDownloader(object):
    def __init__(self, url, path):
        self.url = url
        self.path = path

    def download(self):
        urllib.urlretrieve(self.url, self.path)

class _DscStore(object):
    SYNC_SECTION = 'sync_info'
    SYNC_OPTION = 'sync_contents'
    CUR_ID_OPTION = 'current_id'
    MEDIA_SECTION = 'media_info'
    
    NAME_SUFFIX = '-name'

    ITEM_NEW = 1
    ITEM_UPDATE = 2
    CONTAINER_NEW = 3

    def __init__(self, root_path, server_id):
        self.__root_path = root_path + '/' + server_id
        self.__config_path = self.__root_path + '/' + 'tracking.conf'

        self.__config = ConfigParser.ConfigParser()
        self.__cur_id = 0
        self.__sync = False

    def initialize(self, sync):
        if not os.path.exists(self.__root_path):
            os.makedirs(self.__root_path)

        self.__config.read(self.__config_path)

        if not self.__config.has_section(_DscStore.SYNC_SECTION):
            self.__config.add_section(_DscStore.SYNC_SECTION)
            self.__config.set(_DscStore.SYNC_SECTION,
                              _DscStore.CUR_ID_OPTION, '0')
            if sync:
                self.__config.set(_DscStore.SYNC_SECTION,
                                  _DscStore.SYNC_OPTION, 'yes')
            else:
                self.__config.set(_DscStore.SYNC_SECTION,
                                  _DscStore.SYNC_OPTION, 'no')


        if not self.__config.has_section(_DscStore.MEDIA_SECTION):
            self.__config.add_section(_DscStore.MEDIA_SECTION)

        self.__cur_id = self.__config.getint(_DscStore.SYNC_SECTION,
                                             _DscStore.CUR_ID_OPTION)

        self.__sync = self.__config.getboolean(_DscStore.SYNC_SECTION,
                                               _DscStore.SYNC_OPTION)

    def __write_config(self):
        with open(self.__config_path, 'wb') as configfile:
            self.__config.write(configfile)

    def __id_from_path(self, path):
        return os.path.basename(path)

    def __orig_id(self, media_object):
        try:
            return self.__id_from_path(media_object['RefPath'])
        except KeyError:
            return self.__id_from_path(media_object['Path'])

    def __removed_items(self, local_ids, remote_items):
        for local_id in local_ids:
            found = False

            for remote in remote_items:
                remote_id = self.__id_from_path(remote['Path'])
                if local_id.endswith(_DscStore.NAME_SUFFIX) or \
                                                        local_id == remote_id:
                    found = True

            if not found:
                yield local_id

    def __sync_item(self, obj, obj_id, parent_id, status, write_conf):
        orig = self.__orig_id(obj)

        if status == _DscStore.ITEM_UPDATE:
            old_path = self.__config.get(_DscStore.MEDIA_SECTION, orig)
            new_path = self.__create_path_for_name(obj['DisplayName'])
            print u'\tMedia "{0}" updated'.format(obj['DisplayName'])
            print u'\t\tto "{0}"'.format(new_path)
            self.__config.set(_DscStore.MEDIA_SECTION, orig, new_path)
            os.rename(old_path, new_path)
        elif status == _DscStore.ITEM_NEW:
            print u'\tNew media "{0}" tracked'.format(obj['DisplayName'])
            self.__config.set(parent_id, obj_id, orig)
            self.__config.set(parent_id, obj_id + _DscStore.NAME_SUFFIX,
                              obj['DisplayName'])
            if not self.__config.has_option(_DscStore.MEDIA_SECTION, orig) and \
                                                                    self.__sync:
                local_path = self.__create_path_for_name(obj['DisplayName'])
                self.__config.set(_DscStore.MEDIA_SECTION, orig, local_path)
                print u'\tDownloading contents from "{0}"'.format(obj['URLs'][0])
                print u'\t\tinto "{0}"...'.format(local_path)
                downloader = _DscDownloader(obj['URLs'][0], local_path)
                downloader.download()
        else:
            pass

        if write_conf:
            self.__write_config()

    def __create_path_for_name(self, file_name):
        new_path = self.__root_path + '/' + str(self.__cur_id) + '-' + file_name

        self.__cur_id += 1
        self.__config.set(_DscStore.SYNC_SECTION, _DscStore.CUR_ID_OPTION,
                          str(self.__cur_id))

        return new_path

    def remove(self):
        if os.path.exists(self.__root_path):
            shutil.rmtree(self.__root_path)

    def sync_container(self, container, items):
        print u'Syncing container "{0}"...'.format(container['DisplayName'])

        container_id = self.__id_from_path(container['Path'])
        if not self.__config.has_section(container_id):
            self.__config.add_section(container_id)

        for remote in items:
            remote_id = self.__id_from_path(remote['Path'])
            if not self.__config.has_option(container_id, remote_id):
                if remote['Type'] == 'container':
                    status = _DscStore.CONTAINER_NEW
                else:
                    status = _DscStore.ITEM_NEW
                self.__sync_item(remote, remote_id, container_id, status, False)

        for local in self.__removed_items(
                                    self.__config.options(container_id), items):
            if self.__config.has_section(local):
                print u'\tRemoved a container'
                self.__config.remove_option(container_id, local)
                self.__config.remove_section(local)
            else:
                orig = self.__config.get(container_id, local)
                name = self.__config.get(container_id,
                                              local + _DscStore.NAME_SUFFIX)
                print u'\tRemoved media "{0}"'.format(name)
                self.__config.remove_option(container_id, local)
                self.__config.remove_option(container_id,
                                            local + _DscStore.NAME_SUFFIX)
                if local == orig:
                    orig_name = self.__config.get(_DscStore.MEDIA_SECTION, orig)
                    self.__config.remove_option(_DscStore.MEDIA_SECTION, orig)
                    if self.__sync:
                        print u'\tRemoved local downloaded contents "{0}"' \
                                                            .format(orig_name)
                        if os.path.exists(orig_name):
                            os.remove(orig_name)

        self.__write_config()

    def sync_item(self, obj):
        print u'Syncing item "{0}"...'.format(obj['DisplayName'])
        obj_id = self.__id_from_path(obj['Path'])
        parent_id = self.__id_from_path(obj['Parent'])
        if self.__config.has_option(parent_id, obj_id):
            status = _DscStore.ITEM_UPDATE
        else:
            status = _DscStore.ITEM_NEW

        self.__sync_item(obj, obj_id, parent_id, status, True)

class DscController(object):
    """A Download Sync Controller.

    The Download Sync Controller receive changes in the content or metadata
    stored on media servers (DMS/M-DMS) and apply those changes to
    the local storage.
    Media servers must expose the 'content-synchronization' capability to
    be tracked by this controller.

    The three main methods are servers(), track() and sync().
    * servers() lists the media servers available on the network
    * track() is used to add a media server to the list of servers that are
      to be synchronized.
    * sync() launches the servers synchronisation to a local storage

    Sample usage:
    >>> controller.servers()
    >>> controller.track('/com/intel/dLeynaServer/server/0')
    >>> controller.sync()

    """
    CONFIG_PATH = os.environ['HOME'] + '/.config/download-sync-controller.conf'
    SUID_OPTION = 'system_update_id'
    SRT_OPTION = 'service_reset_token'
    SYNC_OPTION = 'sync_contents'
    DATA_PATH_SECTION = '__data_path__'
    DATA_PATH_OPTION = 'path'

    def __init__(self, rel_path = None):
        """
        rel_path: if provided, contains the relative local storage path,
                  from the user's HOME directory.
                  If not provided, the local storage path will be
                  '$HOME/download-sync-controller'

        """
        self.__upnp = _DscUpnp()

        self.__config = ConfigParser.ConfigParser()
        self.__config.read(DscController.CONFIG_PATH)
        
        if rel_path:
            self.__set_data_path(rel_path)
        elif not self.__config.has_section(DscController.DATA_PATH_SECTION):
            self.__set_data_path('download-sync-controller')
            
        self.__store_path = self.__config.get(DscController.DATA_PATH_SECTION,
                                              DscController.DATA_PATH_OPTION)

    def __write_config(self):
        with open(DscController.CONFIG_PATH, 'wb') as configfile:
            self.__config.write(configfile)

    def __set_data_path(self, rel_path):
        data_path = os.environ['HOME'] + '/' + rel_path

        if not self.__config.has_section(DscController.DATA_PATH_SECTION):
            self.__config.add_section(DscController.DATA_PATH_SECTION)

        self.__config.set(DscController.DATA_PATH_SECTION,
                          DscController.DATA_PATH_OPTION, data_path)

        self.__write_config()

    def __need_sync(self, servers):
        for item in servers:
            device = Device(item)
            uuid = device.get_prop('UDN')

            if self.__config.has_section(uuid):
                new_id = device.get_prop('SystemUpdateID')
                new_srt = device.get_prop('ServiceResetToken')
                cur_id = self.__config.getint(uuid, DscController.SUID_OPTION)
                cur_srt = self.__config.get(uuid, DscController.SRT_OPTION)
                if cur_id == -1 or cur_srt != new_srt:
                    print
                    print u'Server {0} needs *full* sync:'.format(uuid)
                    yield item, uuid, 0, new_id, True
                elif cur_id < new_id:
                    print
                    print u'Server {0} needs sync:'.format(uuid)
                    yield item, uuid, cur_id, new_id, False

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
    
    def track(self, server_path, track = True, sync_contents = True):
        """Adds or removes a media server to/from the controller's list.

        server_path: d-bus path for the media server
        track: when 'True', adds a server to the list
               when 'False' removes a server from the list
        sync_contents: when 'True', downloads media contents to the local
                       storage upon synchronization.

        """
        server = Device(server_path)
        server_uuid = server.get_prop('UDN')
        
        if track and not self.__config.has_section(server_uuid):
            srt = self.__check_trackable(server)
            if srt != None:
                self.__config.add_section(server_uuid)

                self.__config.set(server_uuid, DscController.SUID_OPTION, '-1')
                self.__config.set(server_uuid, DscController.SRT_OPTION, srt)
                if sync_contents:
                    self.__config.set(server_uuid, DscController.SYNC_OPTION,
                                      'yes')
                else:
                    self.__config.set(server_uuid, DscController.SYNC_OPTION,
                                      'no')

                self.__write_config()
            else:
                print u"Sorry, the server {0} has no such capability and " \
                        "will not be tracked.".format(server_path)

        elif not track and self.__config.has_section(server_uuid):
            self.__config.remove_section(server_uuid)
            self.__write_config()
            
            store = _DscStore(self.__store_path, server_uuid)
            store.remove()

    def track_reset(self, server_path, sync_contents = True):
        """Removes local contents and meta data for a media server.

        The next synchronization will be a *full* synchronization.

        server_path: d-bus path for the media server
        sync_contents: when 'True', downloads media contents to the local
                       storage upon synchronization.

        """
        self.track(server_path, False, sync_contents)
        self.track(server_path, True, sync_contents)

    def servers(self):
        """Displays media servers available on the network.

        Displays media servers information as well as the tracked status.

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

                print u'{0:<25}  Tracked({2})  {3}  {1}'.format(folder_name,
                                            dev_path,
                                            self.__config.has_option(dev_uuid,
                                                    DscController.SUID_OPTION),
                                            dev_uuid)

            except dbus.exceptions.DBusException as err:
                print u'Cannot retrieve properties for ' + item
                print str(err).strip()[:-1]

    def tracked_servers(self):
        """Displays the list of servers currently tracked by the controller."""
        print u'Tracked servers:'

        for name in self.__config.sections():
            if name != DscController.DATA_PATH_SECTION:
                print u'{0:<30}'.format(name)

    def sync(self):
        """Performs a synchronization for all the tracked media servers.

        Displays some progress information during the process.

        """
        print u'Syncing...'

        for item, uuid, cur, new, full_sync in \
                                    self.__need_sync(self.__upnp.get_servers()):
            sync = self.__config.getboolean(uuid, DscController.SYNC_OPTION)

            if full_sync:
                print u'Resetting local contents for server {0}'.format(uuid)

                self.track_reset(item)

                objects = _DscContainer(item).find_containers()
            else:
                objects = _DscContainer(item).find_updates(cur)

            store = _DscStore(self.__store_path, uuid)
            store.initialize(sync)

            for obj in objects:
                if obj['Type'] == 'container':
                    children = _DscContainer(obj['Path']).find_children()
                    store.sync_container(obj, children)
                else:
                    store.sync_item(obj)

            self.__config.set(uuid, DscController.SUID_OPTION, str(new))
            self.__write_config()

        print
        print u'Done.'

    def reset(self):
        """Removes local contents and meta data for all the tracked servers.

        After the call, the list of tracked servers will be empty. 

        """
        for name in self.__config.sections():
            if name != DscController.DATA_PATH_SECTION:
                self.__config.remove_section(name)

                store = _DscStore(self.__store_path, name)
                store.remove()

        self.__write_config()


if __name__ == '__main__':
    controller = DscController()
    controller.servers()
    print
    print u'"controller" instance is ready for use.'
    print u'Type "help(DscController)" for more details and usage samples.'
