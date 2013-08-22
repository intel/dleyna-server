/*
 * dLeyna
 *
 * Copyright (C) 2012-2013 Intel Corporation. All rights reserved.
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
 * Regis Merlino <regis.merlino@intel.com>
 *
 */

#ifndef DLEYNA_SERVER_INTERFACE_H__
#define DLEYNA_SERVER_INTERFACE_H__

enum dls_manager_interface_type_ {
	DLS_MANAGER_INTERFACE_MANAGER,
	DLS_MANAGER_INTERFACE_INFO_PROPERTIES,
	DLS_MANAGER_INTERFACE_INFO_MAX
};

enum dls_interface_type_ {
	DLS_INTERFACE_INFO_PROPERTIES,
	DLS_INTERFACE_INFO_OBJECT,
	DLS_INTERFACE_INFO_CONTAINER,
	DLS_INTERFACE_INFO_ITEM,
	DLS_INTERFACE_INFO_DEVICE,
	DLS_INTERFACE_INFO_MAX
};

#define DLS_INTERFACE_PROPERTIES "org.freedesktop.DBus.Properties"
#define DLS_INTERFACE_MEDIA_CONTAINER "org.gnome.UPnP.MediaContainer2"
#define DLS_INTERFACE_MEDIA_OBJECT "org.gnome.UPnP.MediaObject2"
#define DLS_INTERFACE_MEDIA_ITEM "org.gnome.UPnP.MediaItem2"

/* Manager Properties */
#define DLS_INTERFACE_PROP_WHITE_LIST_ENTRIES "WhiteListEntries"
#define DLS_INTERFACE_PROP_WHITE_LIST_ENABLED "WhiteListEnabled"

/* Object Properties */
#define DLS_INTERFACE_PROP_PATH "Path"
#define DLS_INTERFACE_PROP_PARENT "Parent"
#define DLS_INTERFACE_PROP_RESTRICTED "Restricted"
#define DLS_INTERFACE_PROP_DISPLAY_NAME "DisplayName"
#define DLS_INTERFACE_PROP_TYPE "Type"
#define DLS_INTERFACE_PROP_TYPE_EX "TypeEx"
#define DLS_INTERFACE_PROP_CREATOR "Creator"
#define DLS_INTERFACE_PROP_DLNA_MANAGED "DLNAManaged"
#define DLS_INTERFACE_PROP_OBJECT_UPDATE_ID "ObjectUpdateID"

/* Item Properties */
#define DLS_INTERFACE_PROP_REFPATH "RefPath"
#define DLS_INTERFACE_PROP_ARTIST "Artist"
#define DLS_INTERFACE_PROP_ARTISTS "Artists"
#define DLS_INTERFACE_PROP_ALBUM "Album"
#define DLS_INTERFACE_PROP_DATE "Date"
#define DLS_INTERFACE_PROP_GENRE "Genre"
#define DLS_INTERFACE_PROP_TRACK_NUMBER "TrackNumber"
#define DLS_INTERFACE_PROP_ALBUM_ART_URL "AlbumArtURL"
#define DLS_INTERFACE_PROP_RESOURCES "Resources"

/* Container Properties */
#define DLS_INTERFACE_PROP_SEARCHABLE "Searchable"
#define DLS_INTERFACE_PROP_CHILD_COUNT "ChildCount"
#define DLS_INTERFACE_PROP_CREATE_CLASSES "CreateClasses"
#define DLS_INTERFACE_PROP_CONTAINER_UPDATE_ID "ContainerUpdateID"
#define DLS_INTERFACE_PROP_TOTAL_DELETED_CHILD_COUNT "TotalDeletedChildCount"

/* Device Properties */
#define DLS_INTERFACE_PROP_LOCATION "Location"
#define DLS_INTERFACE_PROP_UDN "UDN"
#define DLS_INTERFACE_PROP_DEVICE_TYPE "DeviceType"
#define DLS_INTERFACE_PROP_FRIENDLY_NAME "FriendlyName"
#define DLS_INTERFACE_PROP_MANUFACTURER "Manufacturer"
#define DLS_INTERFACE_PROP_MANUFACTURER_URL "ManufacturerUrl"
#define DLS_INTERFACE_PROP_MODEL_DESCRIPTION "ModelDescription"
#define DLS_INTERFACE_PROP_MODEL_NAME "ModelName"
#define DLS_INTERFACE_PROP_MODEL_NUMBER "ModelNumber"
#define DLS_INTERFACE_PROP_MODEL_URL "ModelURL"
#define DLS_INTERFACE_PROP_SERIAL_NUMBER "SerialNumber"
#define DLS_INTERFACE_PROP_PRESENTATION_URL "PresentationURL"
#define DLS_INTERFACE_PROP_ICON_URL "IconURL"
#define DLS_INTERFACE_PROP_SV_DLNA_CAPABILITIES "DLNACaps"
#define DLS_INTERFACE_PROP_SV_SEARCH_CAPABILITIES "SearchCaps"
#define DLS_INTERFACE_PROP_SV_SORT_CAPABILITIES "SortCaps"
#define DLS_INTERFACE_PROP_SV_SORT_EXT_CAPABILITIES "SortExtCaps"
#define DLS_INTERFACE_PROP_SV_FEATURE_LIST "FeatureList"
#define DLS_INTERFACE_PROP_SV_SERVICE_RESET_TOKEN "ServiceResetToken"

/* Resources Properties */
#define DLS_INTERFACE_PROP_MIME_TYPE "MIMEType"
#define DLS_INTERFACE_PROP_DLNA_PROFILE "DLNAProfile"
#define DLS_INTERFACE_PROP_DLNA_CONVERSION "DLNAConversion"
#define DLS_INTERFACE_PROP_DLNA_OPERATION "DLNAOperation"
#define DLS_INTERFACE_PROP_DLNA_FLAGS "DLNAFlags"
#define DLS_INTERFACE_PROP_SIZE "Size"
#define DLS_INTERFACE_PROP_DURATION "Duration"
#define DLS_INTERFACE_PROP_BITRATE "Bitrate"
#define DLS_INTERFACE_PROP_SAMPLE_RATE "SampleRate"
#define DLS_INTERFACE_PROP_BITS_PER_SAMPLE "BitsPerSample"
#define DLS_INTERFACE_PROP_WIDTH "Width"
#define DLS_INTERFACE_PROP_HEIGHT "Height"
#define DLS_INTERFACE_PROP_COLOR_DEPTH "ColorDepth"
#define DLS_INTERFACE_PROP_URLS "URLs"
#define DLS_INTERFACE_PROP_URL "URL"
#define DLS_INTERFACE_PROP_UPDATE_COUNT "UpdateCount"

/* Evented State Variable Properties */
#define DLS_INTERFACE_PROP_ESV_SYSTEM_UPDATE_ID "SystemUpdateID"

#define DLS_INTERFACE_GET_VERSION "GetVersion"
#define DLS_INTERFACE_GET_SERVERS "GetServers"
#define DLS_INTERFACE_RESCAN "Rescan"
#define DLS_INTERFACE_RELEASE "Release"
#define DLS_INTERFACE_SET_PROTOCOL_INFO "SetProtocolInfo"
#define DLS_INTERFACE_PREFER_LOCAL_ADDRESSES "PreferLocalAddresses"

#define DLS_INTERFACE_WHITE_LIST_ENABLE "WhiteListEnable"
#define DLS_INTERFACE_WHITE_LIST_ADD_ENTRIES "WhiteListAddEntries"
#define DLS_INTERFACE_WHITE_LIST_REMOVE_ENTRIES "WhiteListRemoveEntries"
#define DLS_INTERFACE_WHITE_LIST_CLEAR "WhiteListClear"

#define DLS_INTERFACE_FOUND_SERVER "FoundServer"
#define DLS_INTERFACE_LOST_SERVER "LostServer"

#define DLS_INTERFACE_LIST_CHILDREN "ListChildren"
#define DLS_INTERFACE_LIST_CHILDREN_EX "ListChildrenEx"
#define DLS_INTERFACE_LIST_ITEMS "ListItems"
#define DLS_INTERFACE_LIST_ITEMS_EX "ListItemsEx"
#define DLS_INTERFACE_LIST_CONTAINERS "ListContainers"
#define DLS_INTERFACE_LIST_CONTAINERS_EX "ListContainersEx"
#define DLS_INTERFACE_SEARCH_OBJECTS "SearchObjects"
#define DLS_INTERFACE_SEARCH_OBJECTS_EX "SearchObjectsEx"
#define DLS_INTERFACE_UPDATE "Update"

#define DLS_INTERFACE_GET_COMPATIBLE_RESOURCE "GetCompatibleResource"

#define DLS_INTERFACE_GET "Get"
#define DLS_INTERFACE_GET_ALL "GetAll"
#define DLS_INTERFACE_INTERFACE_NAME "InterfaceName"
#define DLS_INTERFACE_PROPERTY_NAME "PropertyName"
#define DLS_INTERFACE_PROPERTIES_VALUE "Properties"
#define DLS_INTERFACE_VALUE "value"
#define DLS_INTERFACE_CHILD_TYPES "ChildTypes"

#define DLS_INTERFACE_VERSION "Version"
#define DLS_INTERFACE_SERVERS "Servers"

#define DLS_INTERFACE_CRITERIA "Criteria"
#define DLS_INTERFACE_DICT "Dictionary"
#define DLS_INTERFACE_PATH "Path"
#define DLS_INTERFACE_QUERY "Query"
#define DLS_INTERFACE_PROTOCOL_INFO "ProtocolInfo"
#define DLS_INTERFACE_PREFER "Prefer"

#define DLS_INTERFACE_OFFSET "Offset"
#define DLS_INTERFACE_MAX "Max"
#define DLS_INTERFACE_FILTER "Filter"
#define DLS_INTERFACE_CHILDREN "Children"
#define DLS_INTERFACE_SORT_BY "SortBy"
#define DLS_INTERFACE_TOTAL_ITEMS "TotalItems"

#define DLS_INTERFACE_PROPERTIES_CHANGED "PropertiesChanged"
#define DLS_INTERFACE_CHANGED_PROPERTIES "ChangedProperties"
#define DLS_INTERFACE_INVALIDATED_PROPERTIES "InvalidatedProperties"
#define DLS_INTERFACE_ESV_CONTAINER_UPDATE_IDS "ContainerUpdateIDs"
#define DLS_INTERFACE_CONTAINER_PATHS_ID "ContainerPathsIDs"
#define DLS_INTERFACE_ESV_LAST_CHANGE "LastChange"
#define DLS_INTERFACE_LAST_CHANGE_STATE_EVENT "StateEvent"

#define DLS_INTERFACE_DELETE "Delete"

#define DLS_INTERFACE_CREATE_CONTAINER "CreateContainer"
#define DLS_INTERFACE_CREATE_CONTAINER_IN_ANY "CreateContainerInAnyContainer"

#define DLS_INTERFACE_UPLOAD "Upload"
#define DLS_INTERFACE_UPLOAD_TO_ANY "UploadToAnyContainer"
#define DLS_INTERFACE_GET_UPLOAD_STATUS "GetUploadStatus"
#define DLS_INTERFACE_GET_UPLOAD_IDS "GetUploadIDs"
#define DLS_INTERFACE_CANCEL_UPLOAD "CancelUpload"
#define DLS_INTERFACE_TOTAL "Total"
#define DLS_INTERFACE_LENGTH "Length"
#define DLS_INTERFACE_FILE_PATH "FilePath"
#define DLS_INTERFACE_UPLOAD_ID "UploadId"
#define DLS_INTERFACE_UPLOAD_IDS "UploadIDs"
#define DLS_INTERFACE_UPLOAD_STATUS "UploadStatus"
#define DLS_INTERFACE_UPLOAD_UPDATE "UploadUpdate"
#define DLS_INTERFACE_TO_ADD_UPDATE "ToAddUpdate"
#define DLS_INTERFACE_TO_DELETE "ToDelete"
#define DLS_INTERFACE_CANCEL "Cancel"
#define DLS_INTERFACE_GET_ICON "GetIcon"
#define DLS_INTERFACE_RESOLUTION "Resolution"
#define DLS_INTERFACE_ICON_BYTES "Bytes"
#define DLS_INTERFACE_MIME_TYPE "MimeType"
#define DLS_INTERFACE_REQ_MIME_TYPE "RequestedMimeType"

#define DLS_INTERFACE_GET_METADATA "GetMetaData"
#define DLS_INTERFACE_METADATA "MetaData"

#define DLS_INTERFACE_CREATE_REFERENCE "CreateReference"
#define DLS_INTERFACE_REFPATH "RefPath"

#define DLS_INTERFACE_ENTRY_LIST "EntryList"
#define DLS_INTERFACE_IS_ENABLED "IsEnabled"

#endif /* DLEYNA_SERVER_INTERFACE_H__ */
