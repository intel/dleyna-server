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
 * Ludovic Ferrandis <ludovic.ferrandis@intel.com>
 *
 */

#include "props_def.h"
#include "interface.h"

static dls_prop_map_t g_props_def[] = {
/* UPnP/DLNA Property Name	dLeyna/MS2 property name			dLeyna property mask		Filter  Search  Update */
{"@childCount",			DLS_INTERFACE_PROP_CHILD_COUNT,			DLS_PROP_MASK_CHILD_COUNT,	TRUE,	TRUE,	FALSE}, /* ChildCount */
{"@id",				DLS_INTERFACE_PROP_PATH,			DLS_PROP_MASK_PATH,		FALSE,	TRUE,	FALSE}, /* Path */
{"@parentID",			DLS_INTERFACE_PROP_PARENT,			DLS_PROP_MASK_PARENT,		FALSE,	TRUE,	FALSE}, /* Parent */
{"@refID",			DLS_INTERFACE_PROP_REFPATH,			DLS_PROP_MASK_REFPATH,		TRUE,	TRUE,	FALSE}, /* RefPath */
{"@restricted",			DLS_INTERFACE_PROP_RESTRICTED,			DLS_PROP_MASK_RESTRICTED,	TRUE,	TRUE,	FALSE}, /* Restricted */
{"@searchable",			DLS_INTERFACE_PROP_SEARCHABLE,			DLS_PROP_MASK_SEARCHABLE,	TRUE,	TRUE,	FALSE}, /* Searchable */

/* UPnP/DLNA Property Name	dLeyna/MS2 property name			dLeyna property mask		Filter  Search  Update */
{"dc:creator",			DLS_INTERFACE_PROP_CREATOR,			DLS_PROP_MASK_CREATOR,		TRUE,	TRUE,	FALSE}, /* Creator */
{"dc:date",			DLS_INTERFACE_PROP_DATE,			DLS_PROP_MASK_DATE,		TRUE,	TRUE,	TRUE},  /* Date */
{"dc:title",			DLS_INTERFACE_PROP_DISPLAY_NAME,		DLS_PROP_MASK_DISPLAY_NAME,	FALSE,	TRUE,	TRUE},  /* DisplayName */

/* UPnP/DLNA Property Name	dLeyna/MS2 property name			dLeyna property mask		Filter  Search  Update */
{"dlna:dlnaManaged",		DLS_INTERFACE_PROP_DLNA_MANAGED,		DLS_PROP_MASK_DLNA_MANAGED,	TRUE,	FALSE,	FALSE}, /* DLNAManaged */

/* UPnP/DLNA Property Name	dLeyna/MS2 property name			dLeyna property mask		Filter  Search  Update */
{"res",				DLS_INTERFACE_PROP_RES_RESOURCES,		DLS_PROP_MASK_RES_RESOURCES,	TRUE,	FALSE,	FALSE}, /* Resources */
{"res",				DLS_INTERFACE_PROP_URL,				DLS_PROP_MASK_URL,		TRUE,	FALSE,	FALSE}, /* URL */
{"res",				DLS_INTERFACE_PROP_URLS,			DLS_PROP_MASK_URLS,		TRUE,	FALSE,	FALSE}, /* URLs */
{"res@bitrate",			DLS_INTERFACE_PROP_BITRATE,			DLS_PROP_MASK_BITRATE,		TRUE,	TRUE,	FALSE}, /* Bitrate */
{"res@bitsPerSample",		DLS_INTERFACE_PROP_BITS_PER_SAMPLE,		DLS_PROP_MASK_BITS_PER_SAMPLE,	TRUE,	TRUE,	FALSE}, /* BitsPerSample */
{"res@colorDepth",		DLS_INTERFACE_PROP_COLOR_DEPTH,			DLS_PROP_MASK_COLOR_DEPTH,	TRUE,	TRUE,	FALSE}, /* ColorDepth */
{"res@duration",		DLS_INTERFACE_PROP_DURATION,			DLS_PROP_MASK_DURATION,		TRUE,	TRUE,	FALSE}, /* Duration */
{"res@protocolInfo",		DLS_INTERFACE_PROP_DLNA_PROFILE,		DLS_PROP_MASK_DLNA_PROFILE,	TRUE,	FALSE,	FALSE}, /* DLNAProfile */
{"res@protocolInfo",		DLS_INTERFACE_PROP_MIME_TYPE,			DLS_PROP_MASK_MIME_TYPE,	TRUE,	FALSE,	FALSE}, /* MIMEType */
{"res@resolution",		DLS_INTERFACE_PROP_HEIGHT,			DLS_PROP_MASK_HEIGHT,		TRUE,	FALSE,	FALSE}, /* Height */
{"res@resolution",		DLS_INTERFACE_PROP_WIDTH,			DLS_PROP_MASK_WIDTH,		TRUE,	FALSE,	FALSE}, /* Width */
{"res@sampleFrequency",		DLS_INTERFACE_PROP_SAMPLE_RATE,			DLS_PROP_MASK_SAMPLE_RATE,	TRUE,	TRUE,	FALSE}, /* SampleRate */
{"res@size",			DLS_INTERFACE_PROP_SIZE,			DLS_PROP_MASK_SIZE,		TRUE,	TRUE,	FALSE}, /* Size */
{"res@updateCount",		DLS_INTERFACE_PROP_UPDATE_COUNT,		DLS_PROP_MASK_UPDATE_COUNT,	TRUE,	TRUE,	FALSE}, /* UpdateCount */

/* UPnP/DLNA Property Name	dLeyna/MS2 property name			dLeyna property mask		Filter  Search  Update */
{"dleyna:flags",		DLS_INTERFACE_PROP_DLNA_CONVERSION,		DLS_PROP_MASK_DLNA_CONVERSION,	FALSE,	FALSE,	FALSE}, /* DLNAConversion */
{"dleyna:flags",		DLS_INTERFACE_PROP_DLNA_OPERATION,		DLS_PROP_MASK_DLNA_OPERATION,	FALSE,	FALSE,	FALSE}, /* DLNAOperation */
{"dleyna:flags",		DLS_INTERFACE_PROP_DLNA_FLAGS,			DLS_PROP_MASK_DLNA_FLAGS,		FALSE,	FALSE,	FALSE}, /* DLNAFlags */

/* UPnP/DLNA Property Name	dLeyna/MS2 property name			dLeyna property mask		Filter  Search  Update */
{"upnp:album",			DLS_INTERFACE_PROP_ALBUM,			DLS_PROP_MASK_ALBUM,		TRUE,	TRUE,	TRUE},  /* Album */
{"upnp:albumArtURI",		DLS_INTERFACE_PROP_ALBUM_ART_URL,		DLS_PROP_MASK_ALBUM_ART_URL,	TRUE,	TRUE,	FALSE}, /* AlbumArtURL */
{"upnp:artist",			DLS_INTERFACE_PROP_ARTIST,			DLS_PROP_MASK_ARTIST,		TRUE,	TRUE,	FALSE}, /* Artist */
{"upnp:artist",			DLS_INTERFACE_PROP_ARTISTS,			DLS_PROP_MASK_ARTISTS,		TRUE,	FALSE,	TRUE},  /* Artists */
{"upnp:class",			DLS_INTERFACE_PROP_TYPE,			DLS_PROP_MASK_TYPE,		FALSE,	TRUE,	TRUE},  /* Type */
{"upnp:containerUpdateID",	DLS_INTERFACE_PROP_CONTAINER_UPDATE_ID,		DLS_PROP_MASK_CONTAINER_UID,	TRUE,	TRUE,	FALSE}, /* ContainerUpdateID */
{"upnp:createClass",		DLS_INTERFACE_PROP_CREATE_CLASSES,		DLS_PROP_MASK_CREATE_CLASSES,	TRUE,	FALSE,	FALSE}, /* CreateClasses */
{"upnp:genre",			DLS_INTERFACE_PROP_GENRE,			DLS_PROP_MASK_GENRE,		TRUE,	TRUE,	FALSE}, /* Genre */
{"upnp:objectUpdateID",		DLS_INTERFACE_PROP_OBJECT_UPDATE_ID,		DLS_PROP_MASK_OBJECT_UID,	TRUE,	TRUE,	FALSE}, /* ObjectUpdateID */
{"upnp:originalTrackNumber",	DLS_INTERFACE_PROP_TRACK_NUMBER,		DLS_PROP_MASK_TRACK_NUMBER,	TRUE,	TRUE,	TRUE},  /* TrackNumber */
{"upnp:totalDeletedChildCount",	DLS_INTERFACE_PROP_TOTAL_DELETED_CHILD_COUNT,	DLS_PROP_MASK_TOTAL_DELETED_CC,	TRUE,	TRUE,	FALSE}, /* TotalDeletedChildCount */

/* UPnP/DLNA Property Name	dLeyna/MS2 property name			dLeyna property mask		Filter  Search  Update */
{"dleyna:device",		DLS_INTERFACE_PROP_LOCATION,			DLS_PROP_MASK_LOCATION,		FALSE,	FALSE,	FALSE}, /* Location */
{"dleyna:device",		DLS_INTERFACE_PROP_UDN,				DLS_PROP_MASK_UDN,		FALSE,	FALSE,	FALSE}, /* UDN */
{"dleyna:device",		DLS_INTERFACE_PROP_DEVICE_TYPE,			DLS_PROP_MASK_DEVICE_TYPE,	FALSE,	FALSE,	FALSE}, /* DeviceType */
{"dleyna:device",		DLS_INTERFACE_PROP_FRIENDLY_NAME,		DLS_PROP_MASK_FRIENDLY_NAME,	FALSE,	FALSE,	FALSE}, /* FriendlyName */
{"dleyna:device",		DLS_INTERFACE_PROP_MANUFACTURER,		DLS_PROP_MASK_MANUFACTURER,	FALSE,	FALSE,	FALSE}, /* Manufacturer */
{"dleyna:device",		DLS_INTERFACE_PROP_MANUFACTURER_URL,		DLS_PROP_MASK_MANUFACTURER_URL,	FALSE,	FALSE,	FALSE}, /* ManufacturerUrl */
{"dleyna:device",		DLS_INTERFACE_PROP_MODEL_DESCRIPTION,		DLS_PROP_MASK_MODEL_DESC,	FALSE,	FALSE,	FALSE}, /* ModelDescription */
{"dleyna:device",		DLS_INTERFACE_PROP_MODEL_NAME,			DLS_PROP_MASK_MODEL_NAME,	FALSE,	FALSE,	FALSE}, /* ModelName */
{"dleyna:device",		DLS_INTERFACE_PROP_MODEL_NUMBER,		DLS_PROP_MASK_MODEL_NUMBER,	FALSE,	FALSE,	FALSE}, /* ModelNumber */
{"dleyna:device",		DLS_INTERFACE_PROP_MODEL_URL,			DLS_PROP_MASK_MODEL_URL,	FALSE,	FALSE,	FALSE}, /* ModelURL */
{"dleyna:device",		DLS_INTERFACE_PROP_SERIAL_NUMBER,		DLS_PROP_MASK_SERIAL_NUMBER,	FALSE,	FALSE,	FALSE}, /* SerialNumber */
{"dleyna:device",		DLS_INTERFACE_PROP_PRESENTATION_URL,		DLS_PROP_MASK_PRESENTATION_URL,	FALSE,	FALSE,	FALSE}, /* PresentationURL */
{"dleyna:device",		DLS_INTERFACE_PROP_ICON_URL,			DLS_PROP_MASK_ICON_URL,		FALSE,	FALSE,	FALSE}, /* IconURL */
{"dleyna:device",		DLS_INTERFACE_PROP_SV_DLNA_CAPABILITIES,	DLS_PROP_MASK_SV_DLNA_CAPS,	FALSE,	FALSE,	FALSE}, /* DLNACaps */
{"dleyna:device",		DLS_INTERFACE_PROP_SV_SEARCH_CAPABILITIES,	DLS_PROP_MASK_SV_SEARCH_CAPS,	FALSE,	FALSE,	FALSE}, /* SearchCaps */
{"dleyna:device",		DLS_INTERFACE_PROP_SV_SORT_CAPABILITIES,	DLS_PROP_MASK_SV_SORT_CAPS,	FALSE,	FALSE,	FALSE}, /* SortCaps */
{"dleyna:device",		DLS_INTERFACE_PROP_SV_SORT_EXT_CAPABILITIES,	DLS_PROP_MASK_SV_SORT_EXT_CAPS,	FALSE,	FALSE,	FALSE}, /* SortExtCaps	 */
{"dleyna:device",		DLS_INTERFACE_PROP_SV_FEATURE_LIST,		DLS_PROP_MASK_SV_FEATURE_LIST,	FALSE,	FALSE,	FALSE}, /* FeatureList */
{"dleyna:device",		DLS_INTERFACE_PROP_SV_SERVICE_RESET_TOKEN,	DLS_PROP_MASK_SV_SRT,		FALSE,	FALSE,	FALSE}, /* ServiceResetToken */
{"dleyna:device",		DLS_INTERFACE_PROP_ESV_SYSTEM_UPDATE_ID,	DLS_PROP_MASK_ESV_SYSTEM_UID,	FALSE,	FALSE,	FALSE}, /* SystemUpdateID */

{NULL,				NULL,						0,				FALSE,	FALSE,	FALSE}
};

dls_prop_map_t *dls_props_def_get(void)
{
	return g_props_def;
}
