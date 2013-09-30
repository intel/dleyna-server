/*
 * dLeyna
 *
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Mark Ryan <mark.d.ryan@intel.com>
 * Regis Merlino <regis.merlino@intel.com>
 *
 */

#include <glib.h>
#include <string.h>

#include <libdleyna/core/connector.h>
#include <libdleyna/core/control-point.h>
#include <libdleyna/core/error.h>
#include <libdleyna/core/log.h>
#include <libdleyna/core/task-processor.h>
#include <libdleyna/core/white-list.h>

#include "async.h"
#include "client.h"
#include "control-point-server.h"
#include "device.h"
#include "interface.h"
#include "manager.h"
#include "path.h"
#include "server.h"
#include "upnp.h"

#ifdef UA_PREFIX
	#define DLS_PRG_NAME UA_PREFIX " dLeyna/" VERSION
#else
	#define DLS_PRG_NAME "dLeyna/" VERSION
#endif


typedef struct dls_server_context_t_ dls_server_context_t;
struct dls_server_context_t_ {
	dleyna_connector_id_t connection;
	dleyna_task_processor_t *processor;
	const dleyna_connector_t *connector;
	dleyna_settings_t *settings;
	guint dls_id[DLS_MANAGER_INTERFACE_INFO_MAX];
	GHashTable *watchers;
	dls_upnp_t *upnp;
	dls_manager_t *manager;
};

static dls_server_context_t g_context;

static const gchar g_root_introspection[] =
	"<node>"
	"  <interface name='"DLEYNA_SERVER_INTERFACE_MANAGER"'>"
	"    <method name='"DLS_INTERFACE_GET_VERSION"'>"
	"      <arg type='s' name='"DLS_INTERFACE_VERSION"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_RELEASE"'>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_GET_SERVERS"'>"
	"      <arg type='ao' name='"DLS_INTERFACE_SERVERS"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_RESCAN"'>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_SET_PROTOCOL_INFO"'>"
	"      <arg type='s' name='"DLS_INTERFACE_PROTOCOL_INFO"'"
	"           direction='in'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_PREFER_LOCAL_ADDRESSES"'>"
	"      <arg type='b' name='"DLS_INTERFACE_PREFER"'"
	"           direction='in'/>"
	"    </method>"
	"    <signal name='"DLS_INTERFACE_FOUND_SERVER"'>"
	"      <arg type='o' name='"DLS_INTERFACE_PATH"'/>"
	"    </signal>"
	"    <signal name='"DLS_INTERFACE_LOST_SERVER"'>"
	"      <arg type='o' name='"DLS_INTERFACE_PATH"'/>"
	"    </signal>"
	"    <property type='as' name='"DLS_INTERFACE_PROP_NEVER_QUIT"'"
	"       access='readwrite'/>"
	"    <property type='as' name='"DLS_INTERFACE_PROP_WHITE_LIST_ENTRIES"'"
	"       access='readwrite'/>"
	"    <property type='b' name='"DLS_INTERFACE_PROP_WHITE_LIST_ENABLED"'"
	"       access='readwrite'/>"
	"  </interface>"
	"  <interface name='"DLS_INTERFACE_PROPERTIES"'>"
	"    <method name='"DLS_INTERFACE_GET"'>"
	"      <arg type='s' name='"DLS_INTERFACE_INTERFACE_NAME"'"
	"           direction='in'/>"
	"      <arg type='s' name='"DLS_INTERFACE_PROPERTY_NAME"'"
	"           direction='in'/>"
	"      <arg type='v' name='"DLS_INTERFACE_VALUE"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_GET_ALL"'>"
	"      <arg type='s' name='"DLS_INTERFACE_INTERFACE_NAME"'"
	"           direction='in'/>"
	"      <arg type='a{sv}' name='"DLS_INTERFACE_PROPERTIES_VALUE"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_SET"'>"
	"      <arg type='s' name='"DLS_INTERFACE_INTERFACE_NAME"'"
	"           direction='in'/>"
	"      <arg type='s' name='"DLS_INTERFACE_PROPERTY_NAME"'"
	"           direction='in'/>"
	"      <arg type='v' name='"DLS_INTERFACE_VALUE"'"
	"           direction='in'/>"
	"    </method>"
	"    <signal name='"DLS_INTERFACE_PROPERTIES_CHANGED"'>"
	"      <arg type='s' name='"DLS_INTERFACE_INTERFACE_NAME"'/>"
	"      <arg type='a{sv}' name='"DLS_INTERFACE_CHANGED_PROPERTIES"'/>"
	"      <arg type='as' name='"
	DLS_INTERFACE_INVALIDATED_PROPERTIES"'/>"
	"    </signal>"
	"  </interface>"
	"</node>";

static const gchar g_server_introspection[] =
	"<node>"
	"  <interface name='"DLS_INTERFACE_PROPERTIES"'>"
	"    <method name='"DLS_INTERFACE_GET"'>"
	"      <arg type='s' name='"DLS_INTERFACE_INTERFACE_NAME"'"
	"           direction='in'/>"
	"      <arg type='s' name='"DLS_INTERFACE_PROPERTY_NAME"'"
	"           direction='in'/>"
	"      <arg type='v' name='"DLS_INTERFACE_VALUE"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_GET_ALL"'>"
	"      <arg type='s' name='"DLS_INTERFACE_INTERFACE_NAME"'"
	"           direction='in'/>"
	"      <arg type='a{sv}' name='"DLS_INTERFACE_PROPERTIES_VALUE"'"
	"           direction='out'/>"
	"    </method>"
	"    <signal name='"DLS_INTERFACE_PROPERTIES_CHANGED"'>"
	"      <arg type='s' name='"DLS_INTERFACE_INTERFACE_NAME"'/>"
	"      <arg type='a{sv}' name='"DLS_INTERFACE_CHANGED_PROPERTIES"'/>"
	"      <arg type='as' name='"
	DLS_INTERFACE_INVALIDATED_PROPERTIES"'/>"
	"    </signal>"
	"  </interface>"
	"  <interface name='"DLS_INTERFACE_MEDIA_OBJECT"'>"
	"    <property type='o' name='"DLS_INTERFACE_PROP_PARENT"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_TYPE"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_TYPE_EX"'"
	"       access='read'/>"
	"    <property type='o' name='"DLS_INTERFACE_PROP_PATH"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_DISPLAY_NAME"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_CREATOR"'"
	"       access='read'/>"
	"    <property type='b' name='"DLS_INTERFACE_PROP_RESTRICTED"'"
	"       access='read'/>"
	"    <property type='a{sb}' name='"DLS_INTERFACE_PROP_DLNA_MANAGED"'"
	"       access='read'/>"
	"    <property type='u' name='"DLS_INTERFACE_PROP_OBJECT_UPDATE_ID"'"
	"       access='read'/>"
	"    <method name='"DLS_INTERFACE_DELETE"'>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_UPDATE"'>"
	"      <arg type='a{sv}' name='"DLS_INTERFACE_TO_ADD_UPDATE"'"
	"           direction='in'/>"
	"      <arg type='as' name='"DLS_INTERFACE_TO_DELETE"'"
	"           direction='in'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_GET_METADATA"'>"
	"      <arg type='s' name='"DLS_INTERFACE_METADATA"'"
	"           direction='out'/>"
	"    </method>"
	"  </interface>"
	"  <interface name='"DLS_INTERFACE_MEDIA_CONTAINER"'>"
	"    <method name='"DLS_INTERFACE_LIST_CHILDREN"'>"
	"      <arg type='u' name='"DLS_INTERFACE_OFFSET"'"
	"           direction='in'/>"
	"      <arg type='u' name='"DLS_INTERFACE_MAX"'"
	"           direction='in'/>"
	"      <arg type='as' name='"DLS_INTERFACE_FILTER"'"
	"           direction='in'/>"
	"      <arg type='aa{sv}' name='"DLS_INTERFACE_CHILDREN"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_LIST_CHILDREN_EX"'>"
	"      <arg type='u' name='"DLS_INTERFACE_OFFSET"'"
	"           direction='in'/>"
	"      <arg type='u' name='"DLS_INTERFACE_MAX"'"
	"           direction='in'/>"
	"      <arg type='as' name='"DLS_INTERFACE_FILTER"'"
	"           direction='in'/>"
	"      <arg type='s' name='"DLS_INTERFACE_SORT_BY"'"
	"           direction='in'/>"
	"      <arg type='aa{sv}' name='"DLS_INTERFACE_CHILDREN"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_LIST_CONTAINERS"'>"
	"      <arg type='u' name='"DLS_INTERFACE_OFFSET"'"
	"           direction='in'/>"
	"      <arg type='u' name='"DLS_INTERFACE_MAX"'"
	"           direction='in'/>"
	"      <arg type='as' name='"DLS_INTERFACE_FILTER"'"
	"           direction='in'/>"
	"      <arg type='aa{sv}' name='"DLS_INTERFACE_CHILDREN"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_LIST_CONTAINERS_EX"'>"
	"      <arg type='u' name='"DLS_INTERFACE_OFFSET"'"
	"           direction='in'/>"
	"      <arg type='u' name='"DLS_INTERFACE_MAX"'"
	"           direction='in'/>"
	"      <arg type='as' name='"DLS_INTERFACE_FILTER"'"
	"           direction='in'/>"
	"      <arg type='s' name='"DLS_INTERFACE_SORT_BY"'"
	"           direction='in'/>"
	"      <arg type='aa{sv}' name='"DLS_INTERFACE_CHILDREN"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_LIST_ITEMS"'>"
	"      <arg type='u' name='"DLS_INTERFACE_OFFSET"'"
	"           direction='in'/>"
	"      <arg type='u' name='"DLS_INTERFACE_MAX"'"
	"           direction='in'/>"
	"      <arg type='as' name='"DLS_INTERFACE_FILTER"'"
	"           direction='in'/>"
	"      <arg type='aa{sv}' name='"DLS_INTERFACE_CHILDREN"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_LIST_ITEMS_EX"'>"
	"      <arg type='u' name='"DLS_INTERFACE_OFFSET"'"
	"           direction='in'/>"
	"      <arg type='u' name='"DLS_INTERFACE_MAX"'"
	"           direction='in'/>"
	"      <arg type='as' name='"DLS_INTERFACE_FILTER"'"
	"           direction='in'/>"
	"      <arg type='s' name='"DLS_INTERFACE_SORT_BY"'"
	"           direction='in'/>"
	"      <arg type='aa{sv}' name='"DLS_INTERFACE_CHILDREN"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_SEARCH_OBJECTS"'>"
	"      <arg type='s' name='"DLS_INTERFACE_QUERY"'"
	"           direction='in'/>"
	"      <arg type='u' name='"DLS_INTERFACE_OFFSET"'"
	"           direction='in'/>"
	"      <arg type='u' name='"DLS_INTERFACE_MAX"'"
	"           direction='in'/>"
	"      <arg type='as' name='"DLS_INTERFACE_FILTER"'"
	"           direction='in'/>"
	"      <arg type='aa{sv}' name='"DLS_INTERFACE_CHILDREN"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_SEARCH_OBJECTS_EX"'>"
	"      <arg type='s' name='"DLS_INTERFACE_QUERY"'"
	"           direction='in'/>"
	"      <arg type='u' name='"DLS_INTERFACE_OFFSET"'"
	"           direction='in'/>"
	"      <arg type='u' name='"DLS_INTERFACE_MAX"'"
	"           direction='in'/>"
	"      <arg type='as' name='"DLS_INTERFACE_FILTER"'"
	"           direction='in'/>"
	"      <arg type='s' name='"DLS_INTERFACE_SORT_BY"'"
	"           direction='in'/>"
	"      <arg type='aa{sv}' name='"DLS_INTERFACE_CHILDREN"'"
	"           direction='out'/>"
	"      <arg type='u' name='"DLS_INTERFACE_TOTAL_ITEMS"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_UPLOAD"'>"
	"      <arg type='s' name='"DLS_INTERFACE_PROP_DISPLAY_NAME"'"
	"           direction='in'/>"
	"      <arg type='s' name='"DLS_INTERFACE_FILE_PATH"'"
	"           direction='in'/>"
	"      <arg type='u' name='"DLS_INTERFACE_UPLOAD_ID"'"
	"           direction='out'/>"
	"      <arg type='o' name='"DLS_INTERFACE_PATH"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_CREATE_CONTAINER"'>"
	"      <arg type='s' name='"DLS_INTERFACE_PROP_DISPLAY_NAME"'"
	"           direction='in'/>"
	"      <arg type='s' name='"DLS_INTERFACE_PROP_TYPE"'"
	"           direction='in'/>"
	"      <arg type='as' name='"DLS_INTERFACE_CHILD_TYPES"'"
	"           direction='in'/>"
	"      <arg type='o' name='"DLS_INTERFACE_PATH"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_GET_COMPATIBLE_RESOURCE"'>"
	"      <arg type='s' name='"DLS_INTERFACE_PROTOCOL_INFO"'"
	"           direction='in'/>"
	"      <arg type='as' name='"DLS_INTERFACE_FILTER"'"
	"           direction='in'/>"
	"      <arg type='a{sv}' name='"DLS_INTERFACE_PROPERTIES_VALUE"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_CREATE_REFERENCE"'>"
	"      <arg type='o' name='"DLS_INTERFACE_PATH"'"
	"           direction='in'/>"
	"      <arg type='o' name='"DLS_INTERFACE_REFPATH"'"
	"           direction='out'/>"
	"    </method>"
	"    <property type='u' name='"DLS_INTERFACE_PROP_CHILD_COUNT"'"
	"       access='read'/>"
	"    <property type='b' name='"DLS_INTERFACE_PROP_SEARCHABLE"'"
	"       access='read'/>"
	"    <property type='a(sb)' name='"DLS_INTERFACE_PROP_CREATE_CLASSES"'"
	"       access='read'/>"
	"    <property type='u' name='"DLS_INTERFACE_PROP_CONTAINER_UPDATE_ID"'"
	"       access='read'/>"
	"    <property type='u' name='"
	DLS_INTERFACE_PROP_TOTAL_DELETED_CHILD_COUNT"'"
	"       access='read'/>"
	"    <property type='aa{sv}' name='"DLS_INTERFACE_PROP_RESOURCES"'"
	"       access='read'/>"
	"    <property type='as' name='"DLS_INTERFACE_PROP_URLS"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_MIME_TYPE"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_DLNA_PROFILE"'"
	"       access='read'/>"
	"    <property type='a{sb}' name='"DLS_INTERFACE_PROP_DLNA_CONVERSION"'"
	"       access='read'/>"
	"    <property type='a{sb}' name='"DLS_INTERFACE_PROP_DLNA_OPERATION"'"
	"       access='read'/>"
	"    <property type='a{sb}' name='"DLS_INTERFACE_PROP_DLNA_FLAGS"'"
	"       access='read'/>"
	"    <property type='x' name='"DLS_INTERFACE_PROP_SIZE"'"
	"       access='read'/>"
	"  </interface>"
	"  <interface name='"DLS_INTERFACE_MEDIA_ITEM"'>"
	"    <method name='"DLS_INTERFACE_GET_COMPATIBLE_RESOURCE"'>"
	"      <arg type='s' name='"DLS_INTERFACE_PROTOCOL_INFO"'"
	"           direction='in'/>"
	"      <arg type='as' name='"DLS_INTERFACE_FILTER"'"
	"           direction='in'/>"
	"      <arg type='a{sv}' name='"DLS_INTERFACE_PROPERTIES_VALUE"'"
	"           direction='out'/>"
	"    </method>"
	"    <property type='as' name='"DLS_INTERFACE_PROP_URLS"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_MIME_TYPE"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_ARTIST"'"
	"       access='read'/>"
	"    <property type='as' name='"DLS_INTERFACE_PROP_ARTISTS"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_ALBUM"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_DATE"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_GENRE"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_DLNA_PROFILE"'"
	"       access='read'/>"
	"    <property type='a{sb}' name='"DLS_INTERFACE_PROP_DLNA_CONVERSION"'"
	"       access='read'/>"
	"    <property type='a{sb}' name='"DLS_INTERFACE_PROP_DLNA_OPERATION"'"
	"       access='read'/>"
	"    <property type='a{sb}' name='"DLS_INTERFACE_PROP_DLNA_FLAGS"'"
	"       access='read'/>"
	"    <property type='i' name='"DLS_INTERFACE_PROP_TRACK_NUMBER"'"
	"       access='read'/>"
	"    <property type='x' name='"DLS_INTERFACE_PROP_SIZE"'"
	"       access='read'/>"
	"    <property type='i' name='"DLS_INTERFACE_PROP_DURATION"'"
	"       access='read'/>"
	"    <property type='i' name='"DLS_INTERFACE_PROP_BITRATE"'"
	"       access='read'/>"
	"    <property type='i' name='"DLS_INTERFACE_PROP_SAMPLE_RATE"'"
	"       access='read'/>"
	"    <property type='i' name='"DLS_INTERFACE_PROP_BITS_PER_SAMPLE"'"
	"       access='read'/>"
	"    <property type='i' name='"DLS_INTERFACE_PROP_WIDTH"'"
	"       access='read'/>"
	"    <property type='i' name='"DLS_INTERFACE_PROP_HEIGHT"'"
	"       access='read'/>"
	"    <property type='i' name='"DLS_INTERFACE_PROP_COLOR_DEPTH"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_ALBUM_ART_URL"'"
	"       access='read'/>"
	"    <property type='o' name='"DLS_INTERFACE_PROP_REFPATH"'"
	"       access='read'/>"
	"    <property type='aa{sv}' name='"DLS_INTERFACE_PROP_RESOURCES"'"
	"       access='read'/>"
	"  </interface>"
	"  <interface name='"DLEYNA_SERVER_INTERFACE_MEDIA_DEVICE"'>"
	"    <method name='"DLS_INTERFACE_UPLOAD_TO_ANY"'>"
	"      <arg type='s' name='"DLS_INTERFACE_PROP_DISPLAY_NAME"'"
	"           direction='in'/>"
	"      <arg type='s' name='"DLS_INTERFACE_FILE_PATH"'"
	"           direction='in'/>"
	"      <arg type='u' name='"DLS_INTERFACE_UPLOAD_ID"'"
	"           direction='out'/>"
	"      <arg type='o' name='"DLS_INTERFACE_PATH"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_GET_UPLOAD_STATUS"'>"
	"      <arg type='u' name='"DLS_INTERFACE_UPLOAD_ID"'"
	"           direction='in'/>"
	"      <arg type='s' name='"DLS_INTERFACE_UPLOAD_STATUS"'"
	"           direction='out'/>"
	"      <arg type='t' name='"DLS_INTERFACE_LENGTH"'"
	"           direction='out'/>"
	"      <arg type='t' name='"DLS_INTERFACE_TOTAL"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_GET_UPLOAD_IDS"'>"
	"      <arg type='au' name='"DLS_INTERFACE_TOTAL"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_CANCEL_UPLOAD"'>"
	"      <arg type='u' name='"DLS_INTERFACE_UPLOAD_ID"'"
	"           direction='in'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_CREATE_CONTAINER_IN_ANY"'>"
	"      <arg type='s' name='"DLS_INTERFACE_PROP_DISPLAY_NAME"'"
	"           direction='in'/>"
	"      <arg type='s' name='"DLS_INTERFACE_PROP_TYPE"'"
	"           direction='in'/>"
	"      <arg type='as' name='"DLS_INTERFACE_CHILD_TYPES"'"
	"           direction='in'/>"
	"      <arg type='o' name='"DLS_INTERFACE_PATH"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_CANCEL"'>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_GET_ICON"'>"
	"      <arg type='s' name='"DLS_INTERFACE_REQ_MIME_TYPE"'"
	"           direction='in'/>"
	"      <arg type='s' name='"DLS_INTERFACE_RESOLUTION"'"
	"           direction='in'/>"
	"      <arg type='ay' name='"DLS_INTERFACE_ICON_BYTES"'"
	"           direction='out'/>"
	"      <arg type='s' name='"DLS_INTERFACE_MIME_TYPE"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"DLS_INTERFACE_BROWSE_OBJECTS"'>"
	"      <arg type='ao' name='"DLS_INTERFACE_OBJECTS_PATH"'"
	"           direction='in'/>"
	"      <arg type='as' name='"DLS_INTERFACE_FILTER"'"
	"           direction='in'/>"
	"      <arg type='aa{sv}' name='"DLS_INTERFACE_CHILDREN"'"
	"           direction='out'/>"
	"    </method>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_LOCATION"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_UDN"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_DEVICE_TYPE"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_FRIENDLY_NAME"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_MANUFACTURER"'"
	"       access='read'/>"
	"    <property type='s' name='"
	DLS_INTERFACE_PROP_MANUFACTURER_URL"'"
	"       access='read'/>"
	"    <property type='s' name='"
	DLS_INTERFACE_PROP_MODEL_DESCRIPTION"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_MODEL_NAME"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_MODEL_NUMBER"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_MODEL_URL"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_SERIAL_NUMBER"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_PRESENTATION_URL"'"
	"       access='read'/>"
	"    <property type='s' name='"DLS_INTERFACE_PROP_ICON_URL"'"
	"       access='read'/>"
	"    <property type='a{sv}'name='"
	DLS_INTERFACE_PROP_SV_DLNA_CAPABILITIES"'"
	"       access='read'/>"
	"    <property type='as' name='"
	DLS_INTERFACE_PROP_SV_SEARCH_CAPABILITIES"'"
	"       access='read'/>"
	"    <property type='as' name='"
	DLS_INTERFACE_PROP_SV_SORT_CAPABILITIES"'"
	"       access='read'/>"
	"    <property type='as' name='"
	DLS_INTERFACE_PROP_SV_SORT_EXT_CAPABILITIES"'"
	"       access='read'/>"
	"    <property type='a(ssao)' name='"
	DLS_INTERFACE_PROP_SV_FEATURE_LIST"'"
	"       access='read'/>"
	"    <property type='u' name='"
	DLS_INTERFACE_PROP_ESV_SYSTEM_UPDATE_ID"'"
	"       access='read'/>"
	"    <property type='s' name='"
	DLS_INTERFACE_PROP_SV_SERVICE_RESET_TOKEN"'"
	"       access='read'/>"
	"    <signal name='"DLS_INTERFACE_ESV_CONTAINER_UPDATE_IDS"'>"
	"      <arg type='a(ou)' name='"DLS_INTERFACE_CONTAINER_PATHS_ID"'/>"
	"    </signal>"
	"    <signal name='"DLS_INTERFACE_CHANGED_EVENT"'>"
	"      <arg type='aa{sv}' name='"DLS_INTERFACE_CHANGED_OBJECTS"'/>"
	"    </signal>"
	"    <signal name='"DLS_INTERFACE_UPLOAD_UPDATE"'>"
	"      <arg type='u' name='"DLS_INTERFACE_UPLOAD_ID"'/>"
	"      <arg type='s' name='"DLS_INTERFACE_UPLOAD_STATUS"'/>"
	"      <arg type='t' name='"DLS_INTERFACE_LENGTH"'/>"
	"      <arg type='t' name='"DLS_INTERFACE_TOTAL"'/>"
	"    </signal>"
	"  </interface>"
	"</node>";

static const gchar *g_manager_interfaces[DLS_MANAGER_INTERFACE_INFO_MAX] = {
	/* MUST be in the exact same order as g_root_introspection */
	DLEYNA_SERVER_INTERFACE_MANAGER,
	DLS_INTERFACE_PROPERTIES
};

const dleyna_connector_t *dls_server_get_connector(void)
{
	return g_context.connector;
}

dleyna_task_processor_t *dls_server_get_task_processor(void)
{
	return g_context.processor;
}

static void prv_process_sync_task(dls_task_t *task)
{
	dls_client_t *client;
	const gchar *client_name;

	switch (task->type) {
	case DLS_TASK_GET_VERSION:
		task->result = g_variant_ref_sink(g_variant_new_string(
								VERSION));
		dls_task_complete(task);
		break;
	case DLS_TASK_GET_SERVERS:
		task->result = dls_upnp_get_server_ids(g_context.upnp);
		dls_task_complete(task);
		break;
	case DLS_TASK_RESCAN:
		dls_upnp_rescan(g_context.upnp);
		dls_task_complete(task);
		break;
	case DLS_TASK_SET_PROTOCOL_INFO:
		client_name = dleyna_task_queue_get_source(task->atom.queue_id);
		client = g_hash_table_lookup(g_context.watchers, client_name);
		if (client) {
			g_free(client->protocol_info);
			if (task->ut.protocol_info.protocol_info[0]) {
				client->protocol_info =
					task->ut.protocol_info.protocol_info;
				task->ut.protocol_info.protocol_info = NULL;
			} else {
				client->protocol_info = NULL;
			}
		}
		dls_task_complete(task);
		break;
	case DLS_TASK_SET_PREFER_LOCAL_ADDRESSES:
		client_name = dleyna_task_queue_get_source(task->atom.queue_id);
		client = g_hash_table_lookup(g_context.watchers, client_name);
		if (client) {
			client->prefer_local_addresses =
					task->ut.prefer_local_addresses.prefer;
		}
		dls_task_complete(task);
		break;
	case DLS_TASK_GET_UPLOAD_STATUS:
		dls_upnp_get_upload_status(g_context.upnp, task);
		break;
	case DLS_TASK_GET_UPLOAD_IDS:
		dls_upnp_get_upload_ids(g_context.upnp, task);
		break;
	case DLS_TASK_CANCEL_UPLOAD:
		dls_upnp_cancel_upload(g_context.upnp, task);
		break;
	default:
		goto finished;
		break;
	}

	dleyna_task_queue_task_completed(task->atom.queue_id);

finished:
	return;
}

static void prv_async_task_complete(dls_task_t *task, GError *error)
{
	DLEYNA_LOG_DEBUG("Enter");

	if (!error) {
		dls_task_complete(task);
	} else {
		dls_task_fail(task, error);
		g_error_free(error);
	}

	dleyna_task_queue_task_completed(task->atom.queue_id);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_process_async_task(dls_task_t *task)
{
	dls_async_task_t *async_task = (dls_async_task_t *)task;
	dls_client_t *client;
	const gchar *client_name;

	DLEYNA_LOG_DEBUG("Enter");

	async_task->cancellable = g_cancellable_new();
	client_name = dleyna_task_queue_get_source(task->atom.queue_id);
	client = g_hash_table_lookup(g_context.watchers, client_name);

	switch (task->type) {
	case DLS_TASK_MANAGER_GET_PROP:
		dls_manager_get_prop(g_context.manager, g_context.settings,
				     task, prv_async_task_complete);
		break;
	case DLS_TASK_MANAGER_GET_ALL_PROPS:
		dls_manager_get_all_props(g_context.manager, g_context.settings,
					  task, prv_async_task_complete);
		break;
	case DLS_TASK_MANAGER_SET_PROP:
		dls_manager_set_prop(g_context.manager, g_context.settings,
				     task, prv_async_task_complete);
		break;
	case DLS_TASK_GET_CHILDREN:
		dls_upnp_get_children(g_context.upnp, client, task,
				      prv_async_task_complete);
		break;
	case DLS_TASK_GET_PROP:
		dls_upnp_get_prop(g_context.upnp, client, task,
				  prv_async_task_complete);
		break;
	case DLS_TASK_GET_ALL_PROPS:
		dls_upnp_get_all_props(g_context.upnp, client, task,
				       prv_async_task_complete);
		break;
	case DLS_TASK_SEARCH:
		dls_upnp_search(g_context.upnp, client, task,
				prv_async_task_complete);
		break;
	case DLS_TASK_BROWSE_OBJECTS:
		dls_upnp_browse_objects(g_context.upnp, client, task,
					prv_async_task_complete);
		break;
	case DLS_TASK_GET_RESOURCE:
		dls_upnp_get_resource(g_context.upnp, client, task,
				      prv_async_task_complete);
		break;
	case DLS_TASK_UPLOAD_TO_ANY:
		dls_upnp_upload_to_any(g_context.upnp, client, task,
				       prv_async_task_complete);
		break;
	case DLS_TASK_UPLOAD:
		dls_upnp_upload(g_context.upnp, client, task,
				prv_async_task_complete);
		break;
	case DLS_TASK_DELETE_OBJECT:
		dls_upnp_delete_object(g_context.upnp, client, task,
				       prv_async_task_complete);
		break;
	case DLS_TASK_CREATE_CONTAINER:
		dls_upnp_create_container(g_context.upnp, client, task,
					  prv_async_task_complete);
		break;
	case DLS_TASK_CREATE_CONTAINER_IN_ANY:
		dls_upnp_create_container_in_any(g_context.upnp, client, task,
						 prv_async_task_complete);
		break;
	case DLS_TASK_UPDATE_OBJECT:
		dls_upnp_update_object(g_context.upnp, client, task,
				       prv_async_task_complete);
		break;
	case DLS_TASK_GET_OBJECT_METADATA:
		dls_upnp_get_object_metadata(g_context.upnp, client, task,
					     prv_async_task_complete);
		break;
	case DLS_TASK_CREATE_REFERENCE:
		dls_upnp_create_reference(g_context.upnp, client, task,
					  prv_async_task_complete);
		break;
	case DLS_TASK_GET_ICON:
		dls_upnp_get_icon(g_context.upnp, client, task,
				  prv_async_task_complete);
		break;
	default:
		break;
	}

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_process_task(dleyna_task_atom_t *task, gpointer user_data)
{
	dls_task_t *client_task = (dls_task_t *)task;

	if (client_task->synchronous)
		prv_process_sync_task(client_task);
	else
		prv_process_async_task(client_task);
}

static void prv_cancel_task(dleyna_task_atom_t *task, gpointer user_data)
{
	dls_task_cancel((dls_task_t *)task);
}

static void prv_delete_task(dleyna_task_atom_t *task, gpointer user_data)
{
	dls_task_delete((dls_task_t *)task);
}

static void prv_manager_root_method_call(dleyna_connector_id_t conn,
					 const gchar *sender,
					 const gchar *object,
					 const gchar *interface,
					 const gchar *method,
					 GVariant *parameters,
					 dleyna_connector_msg_id_t invocation);

static void prv_manager_props_method_call(dleyna_connector_id_t conn,
					  const gchar *sender,
					  const gchar *object,
					  const gchar *interface,
					  const gchar *method,
					  GVariant *parameters,
					  dleyna_connector_msg_id_t invocation);

static void prv_object_method_call(dleyna_connector_id_t conn,
				   const gchar *sender,
				   const gchar *object,
				   const gchar *interface,
				   const gchar *method,
				   GVariant *parameters,
				   dleyna_connector_msg_id_t invocation);

static void prv_item_method_call(dleyna_connector_id_t conn,
				 const gchar *sender,
				 const gchar *object,
				 const gchar *interface,
				 const gchar *method,
				 GVariant *parameters,
				 dleyna_connector_msg_id_t invocation);

static void prv_con_method_call(dleyna_connector_id_t conn,
				const gchar *sender,
				const gchar *object,
				const gchar *interface,
				const gchar *method,
				GVariant *parameters,
				dleyna_connector_msg_id_t invocation);

static void prv_props_method_call(dleyna_connector_id_t conn,
				  const gchar *sender,
				  const gchar *object,
				  const gchar *interface,
				  const gchar *method,
				  GVariant *parameters,
				  dleyna_connector_msg_id_t invocation);

static void prv_device_method_call(dleyna_connector_id_t conn,
				   const gchar *sender,
				   const gchar *object,
				   const gchar *interface,
				   const gchar *method,
				   GVariant *parameters,
				   dleyna_connector_msg_id_t invocation);

static const dleyna_connector_dispatch_cb_t
			g_root_vtables[DLS_MANAGER_INTERFACE_INFO_MAX] = {
	/* MUST be in the exact same order as g_root_introspection */
	prv_manager_root_method_call,
	prv_manager_props_method_call
};

static const dleyna_connector_dispatch_cb_t
				g_server_vtables[DLS_INTERFACE_INFO_MAX] = {
	/* MUST be in the exact same order as g_server_introspection */
	prv_props_method_call,
	prv_object_method_call,
	prv_con_method_call,
	prv_item_method_call,
	prv_device_method_call
};

static void prv_remove_client(const gchar *name)
{
	dleyna_task_processor_remove_queues_for_source(g_context.processor,
						       name);

	(void) g_hash_table_remove(g_context.watchers, name);

	if (g_hash_table_size(g_context.watchers) == 0)
		if (!dleyna_settings_is_never_quit(g_context.settings))
			dleyna_task_processor_set_quitting(g_context.processor);
}

static void prv_lost_client(const gchar *name)
{
	DLEYNA_LOG_DEBUG("Lost Client %s", name);

	prv_remove_client(name);
}

static void prv_add_task(dls_task_t *task, const gchar *source,
			 const gchar *sink)
{
	dls_client_t *client;
	const dleyna_task_queue_key_t *queue_id;

	if (!g_hash_table_lookup(g_context.watchers, source)) {
		client = g_new0(dls_client_t, 1);
		client->prefer_local_addresses = TRUE;
		g_context.connector->watch_client(source);
		g_hash_table_insert(g_context.watchers, g_strdup(source),
				    client);
	}

	queue_id = dleyna_task_processor_lookup_queue(g_context.processor,
						      source, sink);
	if (!queue_id)
		queue_id = dleyna_task_processor_add_queue(
					g_context.processor,
					source,
					sink,
					DLEYNA_TASK_QUEUE_FLAG_AUTO_START,
					prv_process_task,
					prv_cancel_task,
					prv_delete_task);

	dleyna_task_queue_add_task(queue_id, &task->atom);
}

static void prv_manager_root_method_call(
				dleyna_connector_id_t conn,
				const gchar *sender, const gchar *object,
				const gchar *interface,
				const gchar *method, GVariant *parameters,
				dleyna_connector_msg_id_t invocation)
{
	dls_task_t *task;

	if (!strcmp(method, DLS_INTERFACE_RELEASE)) {
		prv_remove_client(sender);
		g_context.connector->return_response(invocation, NULL);
		goto finished;
	} else if (!strcmp(method, DLS_INTERFACE_RESCAN)) {
		task = dls_task_rescan_new(invocation);
	} else if (!strcmp(method, DLS_INTERFACE_GET_VERSION)) {
		task = dls_task_get_version_new(invocation);
	} else if (!strcmp(method, DLS_INTERFACE_GET_SERVERS)) {
		task = dls_task_get_servers_new(invocation);
	} else if (!strcmp(method, DLS_INTERFACE_SET_PROTOCOL_INFO)) {
		task = dls_task_set_protocol_info_new(invocation,
						      parameters);
	} else if (!strcmp(method, DLS_INTERFACE_PREFER_LOCAL_ADDRESSES)) {
		task = dls_task_prefer_local_addresses_new(invocation,
							   parameters);
	} else {
		goto finished;
	}

	prv_add_task(task, sender, DLS_SERVER_SINK);

finished:

	return;
}

static void prv_manager_props_method_call(dleyna_connector_id_t conn,
					  const gchar *sender,
					  const gchar *object,
					  const gchar *interface,
					  const gchar *method,
					  GVariant *parameters,
					  dleyna_connector_msg_id_t invocation)
{
	dls_task_t *task;
	GError *error = NULL;

	if (!strcmp(method, DLS_INTERFACE_GET_ALL))
		task = dls_task_manager_get_props_new(invocation, object,
						      parameters, &error);
	else if (!strcmp(method, DLS_INTERFACE_GET))
		task = dls_task_manager_get_prop_new(invocation, object,
						     parameters, &error);
	else if (!strcmp(method, DLS_INTERFACE_SET))
		task = dls_task_manager_set_prop_new(invocation, object,
						     parameters, &error);
	else
		goto finished;

	if (!task) {
		g_context.connector->return_error(invocation, error);
		g_error_free(error);

		goto finished;
	}

	prv_add_task(task, sender, task->target.path);

finished:

	return;
}

gboolean dls_server_get_object_info(const gchar *object_path,
					   gchar **root_path,
					   gchar **object_id,
					   dls_device_t **device,
					   GError **error)
{
	if (!dls_path_get_path_and_id(object_path, root_path, object_id,
				      error)) {
		DLEYNA_LOG_WARNING("Bad object %s", object_path);

		goto on_error;
	}

	*device = dls_device_from_path(*root_path,
				dls_upnp_get_server_udn_map(g_context.upnp));

	if (*device == NULL) {
		DLEYNA_LOG_WARNING("Cannot locate device for %s", *root_path);

		*error = g_error_new(DLEYNA_SERVER_ERROR,
				     DLEYNA_ERROR_OBJECT_NOT_FOUND,
				     "Cannot locate device corresponding to"
				     " the specified path");

		g_free(*root_path);
		g_free(*object_id);

		goto on_error;
	}

	return TRUE;

on_error:

	return FALSE;
}

static const gchar *prv_get_device_id(const gchar *object, GError **error)
{
	dls_device_t *device;
	gchar *root_path;
	gchar *id;

	if (!dls_server_get_object_info(object, &root_path, &id, &device,
					error))
		goto on_error;

	g_free(id);
	g_free(root_path);

	return device->path;

on_error:

	return NULL;
}

static void prv_object_method_call(dleyna_connector_id_t conn,
				   const gchar *sender, const gchar *object,
				   const gchar *interface,
				   const gchar *method, GVariant *parameters,
				   dleyna_connector_msg_id_t invocation)
{
	dls_task_t *task;
	GError *error = NULL;

	if (!strcmp(method, DLS_INTERFACE_DELETE))
		task = dls_task_delete_new(invocation, object, &error);
	else if (!strcmp(method, DLS_INTERFACE_UPDATE))
		task = dls_task_update_new(invocation, object,
					   parameters, &error);
	else if (!strcmp(method, DLS_INTERFACE_GET_METADATA))
		task = dls_task_get_metadata_new(invocation, object, &error);
	else
		goto finished;

	if (!task) {
		g_context.connector->return_error(invocation, error);
		g_error_free(error);

		goto finished;
	}

	prv_add_task(task, sender, task->target.device->path);

finished:

	return;
}

static void prv_item_method_call(dleyna_connector_id_t conn,
				 const gchar *sender, const gchar *object,
				 const gchar *interface,
				 const gchar *method, GVariant *parameters,
				 dleyna_connector_msg_id_t invocation)
{
	dls_task_t *task;
	GError *error = NULL;

	if (!strcmp(method, DLS_INTERFACE_GET_COMPATIBLE_RESOURCE)) {
		task = dls_task_get_resource_new(invocation, object,
						 parameters, &error);

		if (!task) {
			g_context.connector->return_error(invocation, error);
			g_error_free(error);

			goto finished;
		}

		prv_add_task(task, sender, task->target.device->path);
	}

finished:

	return;
}


static void prv_con_method_call(dleyna_connector_id_t conn,
				const gchar *sender,
				const gchar *object,
				const gchar *interface,
				const gchar *method,
				GVariant *parameters,
				dleyna_connector_msg_id_t invocation)
{
	dls_task_t *task;
	GError *error = NULL;

	if (!strcmp(method, DLS_INTERFACE_LIST_CHILDREN))
		task = dls_task_get_children_new(invocation, object,
						 parameters, TRUE,
						 TRUE, &error);
	else if (!strcmp(method, DLS_INTERFACE_LIST_CHILDREN_EX))
		task = dls_task_get_children_ex_new(invocation, object,
						    parameters, TRUE,
						    TRUE, &error);
	else if (!strcmp(method, DLS_INTERFACE_LIST_ITEMS))
		task = dls_task_get_children_new(invocation, object,
						 parameters, TRUE,
						 FALSE, &error);
	else if (!strcmp(method, DLS_INTERFACE_LIST_ITEMS_EX))
		task = dls_task_get_children_ex_new(invocation, object,
						    parameters, TRUE,
						    FALSE, &error);
	else if (!strcmp(method, DLS_INTERFACE_LIST_CONTAINERS))
		task = dls_task_get_children_new(invocation, object,
						 parameters, FALSE,
						 TRUE, &error);
	else if (!strcmp(method, DLS_INTERFACE_LIST_CONTAINERS_EX))
		task = dls_task_get_children_ex_new(invocation, object,
						    parameters, FALSE,
						    TRUE, &error);
	else if (!strcmp(method, DLS_INTERFACE_SEARCH_OBJECTS))
		task = dls_task_search_new(invocation, object,
					   parameters, &error);
	else if (!strcmp(method, DLS_INTERFACE_SEARCH_OBJECTS_EX))
		task = dls_task_search_ex_new(invocation, object,
					      parameters, &error);
	else if (!strcmp(method, DLS_INTERFACE_UPLOAD))
		task = dls_task_upload_new(invocation, object,
					   parameters, &error);
	else if (!strcmp(method, DLS_INTERFACE_CREATE_CONTAINER))
		task = dls_task_create_container_new_generic(invocation,
						DLS_TASK_CREATE_CONTAINER,
						object, parameters, &error);
	else if (!strcmp(method, DLS_INTERFACE_GET_COMPATIBLE_RESOURCE))
		task = dls_task_get_resource_new(invocation, object,
						 parameters, &error);
	else if (!strcmp(method, DLS_INTERFACE_CREATE_REFERENCE))
		task = dls_task_create_reference_new(invocation,
						    DLS_TASK_CREATE_REFERENCE,
						    object, parameters, &error);
	else
		goto finished;

	if (!task) {
		g_context.connector->return_error(invocation, error);
		g_error_free(error);

		goto finished;
	}

	prv_add_task(task, sender, task->target.device->path);

finished:

	return;
}

static void prv_props_method_call(dleyna_connector_id_t conn,
				  const gchar *sender,
				  const gchar *object,
				  const gchar *interface,
				  const gchar *method,
				  GVariant *parameters,
				  dleyna_connector_msg_id_t invocation)
{
	dls_task_t *task;
	GError *error = NULL;

	if (!strcmp(method, DLS_INTERFACE_GET_ALL))
		task = dls_task_get_props_new(invocation, object,
					      parameters, &error);
	else if (!strcmp(method, DLS_INTERFACE_GET))
		task = dls_task_get_prop_new(invocation, object,
					     parameters, &error);
	else
		goto finished;

	if (!task) {
		g_context.connector->return_error(invocation, error);
		g_error_free(error);

		goto finished;
	}

	prv_add_task(task, sender, task->target.device->path);

finished:

	return;
}

static void prv_device_method_call(dleyna_connector_id_t conn,
				   const gchar *sender, const gchar *object,
				   const gchar *interface,
				   const gchar *method, GVariant *parameters,
				   dleyna_connector_msg_id_t invocation)
{
	dls_task_t *task;
	GError *error = NULL;
	const gchar *device_id;
	const dleyna_task_queue_key_t *queue_id;

	if (!strcmp(method, DLS_INTERFACE_UPLOAD_TO_ANY)) {
		task = dls_task_upload_to_any_new(invocation,
						  object, parameters, &error);
	} else if (!strcmp(method, DLS_INTERFACE_CREATE_CONTAINER_IN_ANY)) {
		task = dls_task_create_container_new_generic(
					invocation,
					DLS_TASK_CREATE_CONTAINER_IN_ANY,
					object, parameters, &error);
	} else if (!strcmp(method, DLS_INTERFACE_GET_UPLOAD_STATUS)) {
		task = dls_task_get_upload_status_new(invocation,
						      object, parameters,
						      &error);
	} else if (!strcmp(method, DLS_INTERFACE_GET_UPLOAD_IDS)) {
		task = dls_task_get_upload_ids_new(invocation, object,
						   &error);
	} else if (!strcmp(method, DLS_INTERFACE_CANCEL_UPLOAD)) {
		task = dls_task_cancel_upload_new(invocation, object,
						  parameters, &error);
	} else if (!strcmp(method, DLS_INTERFACE_GET_ICON)) {
		task = dls_task_get_icon_new(invocation, object, parameters,
					     &error);
	} else if (!strcmp(method, DLS_INTERFACE_BROWSE_OBJECTS)) {
		task = dls_task_browse_objects_new(invocation, object,
						   parameters, &error);
	} else if (!strcmp(method, DLS_INTERFACE_CANCEL)) {
		task = NULL;

		device_id = prv_get_device_id(object, &error);
		if (!device_id)
			goto on_error;

		queue_id = dleyna_task_processor_lookup_queue(
							g_context.processor,
							sender,
							device_id);
		if (queue_id)
			dleyna_task_processor_cancel_queue(queue_id);

		g_context.connector->return_response(invocation, NULL);

		goto finished;
	} else {
		goto finished;
	}

on_error:

	if (!task) {
		g_context.connector->return_error(invocation, error);
		g_error_free(error);

		goto finished;
	}

	prv_add_task(task, sender, task->target.device->path);

finished:

	return;
}

static void prv_found_media_server(const gchar *path, void *user_data)
{
	(void) g_context.connector->notify(g_context.connection,
					   DLEYNA_SERVER_OBJECT,
					   DLEYNA_SERVER_INTERFACE_MANAGER,
					   DLS_INTERFACE_FOUND_SERVER,
					   g_variant_new("(o)", path),
					   NULL);
}

static void prv_lost_media_server(const gchar *path, void *user_data)
{
	(void) g_context.connector->notify(g_context.connection,
					   DLEYNA_SERVER_OBJECT,
					   DLEYNA_SERVER_INTERFACE_MANAGER,
					   DLS_INTERFACE_LOST_SERVER,
					   g_variant_new("(o)", path),
					   NULL);

	dleyna_task_processor_remove_queues_for_sink(g_context.processor, path);
}

static void prv_unregister_client(gpointer user_data)
{
	dls_client_t *client = user_data;

	if (client) {
		g_free(client->protocol_info);
		g_free(client);
	}
}

dls_upnp_t *dls_server_get_upnp(void)
{
	return g_context.upnp;
}

static void prv_white_list_init(void)
{
	gboolean enabled;
	GVariant *entries;
	dleyna_white_list_t *wl;

	DLEYNA_LOG_DEBUG("Enter");

	enabled = dleyna_settings_is_white_list_enabled(g_context.settings);
	entries = dleyna_settings_white_list_entries(g_context.settings);

	wl = dls_manager_get_white_list(g_context.manager);

	dleyna_white_list_enable(wl, enabled);
	dleyna_white_list_add_entries(wl, entries);

	DLEYNA_LOG_DEBUG("Exit");
}

static gboolean prv_control_point_start_service(
					dleyna_connector_id_t connection)
{
	gboolean retval = TRUE;
	uint i;

	g_context.connection = connection;

	for (i = 0; i < DLS_MANAGER_INTERFACE_INFO_MAX; i++)
		g_context.dls_id[i] = g_context.connector->publish_object(
						connection,
						DLEYNA_SERVER_OBJECT,
						TRUE,
						g_manager_interfaces[i],
						g_root_vtables + i);

	if (g_context.dls_id[DLS_MANAGER_INTERFACE_MANAGER]) {
		g_context.upnp = dls_upnp_new(connection,
					      g_server_vtables,
					      prv_found_media_server,
					      prv_lost_media_server,
					      NULL);

		g_context.manager = dls_manager_new(connection,
						dls_upnp_get_context_manager(
							g_context.upnp));

		prv_white_list_init();
	} else {
		retval = FALSE;
	}

	return retval;
}

static void prv_control_point_stop_service(void)
{
	uint i;

	if (g_context.manager)
		dls_manager_delete(g_context.manager);

	if (g_context.upnp) {
		dls_upnp_unsubscribe(g_context.upnp);
		dls_upnp_delete(g_context.upnp);
	}

	if (g_context.connection) {
		for (i = 0; i < DLS_MANAGER_INTERFACE_INFO_MAX; i++)
			if (g_context.dls_id[i])
				g_context.connector->unpublish_object(
							g_context.connection,
							g_context.dls_id[i]);
	}
}

static void prv_control_point_initialize(const dleyna_connector_t *connector,
					 dleyna_task_processor_t *processor,
					 dleyna_settings_t *settings)
{
	memset(&g_context, 0, sizeof(g_context));

	g_context.connector = connector;
	g_context.processor = processor;
	g_context.settings = settings;

	g_context.connector->set_client_lost_cb(prv_lost_client);

	g_context.watchers = g_hash_table_new_full(g_str_hash, g_str_equal,
						 g_free, prv_unregister_client);

	g_set_prgname(DLS_PRG_NAME);
}

static void prv_control_point_free(void)
{
	if (g_context.watchers)
		g_hash_table_unref(g_context.watchers);
}

static const gchar *prv_control_point_server_name(void)
{
	return DLEYNA_SERVER_NAME;
}

static const gchar *prv_control_point_server_introspection(void)
{
	return g_server_introspection;
}

static const gchar *prv_control_point_root_introspection(void)
{
	return g_root_introspection;
}

static const gchar *prv_control_point_get_version(void)
{
	return VERSION;
}

static const dleyna_control_point_t g_control_point = {
	prv_control_point_initialize,
	prv_control_point_free,
	prv_control_point_server_name,
	prv_control_point_server_introspection,
	prv_control_point_root_introspection,
	prv_control_point_start_service,
	prv_control_point_stop_service,
	prv_control_point_get_version
};

const dleyna_control_point_t *dleyna_control_point_get_server(void)
{
	return &g_control_point;
}
