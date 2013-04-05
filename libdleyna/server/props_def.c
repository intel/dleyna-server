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
/* UPnP/DLNA Property Name	dLeyna/MS2 property name			dLeyna property mask				Filter  Search  Update */
{"@childCount",			DLS_INTERFACE_PROP_CHILD_COUNT,			DLS_UPNP_MASK_PROP_CHILD_COUNT,			TRUE,	TRUE,	FALSE}, /* ChildCount */
{"@id",				DLS_INTERFACE_PROP_PATH,			DLS_UPNP_MASK_PROP_PATH,			FALSE,	TRUE,	FALSE}, /* Path */
{"@parentID",			DLS_INTERFACE_PROP_PARENT,			DLS_UPNP_MASK_PROP_PARENT,			FALSE,	TRUE,	FALSE}, /* Parent */
{"@refID",			DLS_INTERFACE_PROP_REFPATH,			DLS_UPNP_MASK_PROP_REFPATH,			TRUE,	TRUE,	FALSE}, /* RefPath */
{"@restricted",			DLS_INTERFACE_PROP_RESTRICTED,			DLS_UPNP_MASK_PROP_RESTRICTED,			TRUE,	TRUE,	FALSE}, /* Restricted */
{"@searchable",			DLS_INTERFACE_PROP_SEARCHABLE,			DLS_UPNP_MASK_PROP_SEARCHABLE,			TRUE,	TRUE,	FALSE}, /* Searchable */

/* UPnP/DLNA Property Name	dLeyna/MS2 property name			dLeyna property mask				Filter  Search  Update */
{"dc:creator",			DLS_INTERFACE_PROP_CREATOR,			DLS_UPNP_MASK_PROP_CREATOR,			TRUE,	TRUE,	FALSE}, /* Creator */
{"dc:date",			DLS_INTERFACE_PROP_DATE,			DLS_UPNP_MASK_PROP_DATE,			TRUE,	TRUE,	TRUE},  /* Date */
{"dc:title",			DLS_INTERFACE_PROP_DISPLAY_NAME,		DLS_UPNP_MASK_PROP_DISPLAY_NAME,		FALSE,	TRUE,	TRUE},  /* DisplayName */

/* UPnP/DLNA Property Name	dLeyna/MS2 property name			dLeyna property mask				Filter  Search  Update */
{"dlna:dlnaManaged",		DLS_INTERFACE_PROP_DLNA_MANAGED,		DLS_UPNP_MASK_PROP_DLNA_MANAGED,		TRUE,	FALSE,	FALSE}, /* DLNAManaged */

/* UPnP/DLNA Property Name	dLeyna/MS2 property name			dLeyna property mask				Filter  Search  Update */
{"res",				DLS_INTERFACE_PROP_RESOURCES,			DLS_UPNP_MASK_PROP_RESOURCES,			TRUE,	FALSE,	FALSE}, /* Resources */
{"res",				DLS_INTERFACE_PROP_URL,				DLS_UPNP_MASK_PROP_URL,				TRUE,	FALSE,	FALSE}, /* URL */
{"res",				DLS_INTERFACE_PROP_URLS,			DLS_UPNP_MASK_PROP_URLS,			TRUE,	FALSE,	FALSE}, /* URLs */
{"res@bitrate",			DLS_INTERFACE_PROP_BITRATE,			DLS_UPNP_MASK_PROP_BITRATE,			TRUE,	TRUE,	FALSE}, /* Bitrate */
{"res@bitsPerSample",		DLS_INTERFACE_PROP_BITS_PER_SAMPLE,		DLS_UPNP_MASK_PROP_BITS_PER_SAMPLE,		TRUE,	TRUE,	FALSE}, /* BitsPerSample */
{"res@colorDepth",		DLS_INTERFACE_PROP_COLOR_DEPTH,			DLS_UPNP_MASK_PROP_COLOR_DEPTH,			TRUE,	TRUE,	FALSE}, /* ColorDepth */
{"res@duration",		DLS_INTERFACE_PROP_DURATION,			DLS_UPNP_MASK_PROP_DURATION,			TRUE,	TRUE,	FALSE}, /* Duration */
{"res@protocolInfo",		DLS_INTERFACE_PROP_DLNA_PROFILE,		DLS_UPNP_MASK_PROP_DLNA_PROFILE,		TRUE,	FALSE,	FALSE}, /* DLNAProfile */
{"res@protocolInfo",		DLS_INTERFACE_PROP_MIME_TYPE,			DLS_UPNP_MASK_PROP_MIME_TYPE,			TRUE,	FALSE,	FALSE}, /* MIMEType */
{"res@resolution",		DLS_INTERFACE_PROP_HEIGHT,			DLS_UPNP_MASK_PROP_HEIGHT,			TRUE,	FALSE,	FALSE}, /* Height */
{"res@resolution",		DLS_INTERFACE_PROP_WIDTH,			DLS_UPNP_MASK_PROP_WIDTH,			TRUE,	FALSE,	FALSE}, /* Width */
{"res@sampleFrequency",		DLS_INTERFACE_PROP_SAMPLE_RATE,			DLS_UPNP_MASK_PROP_SAMPLE_RATE,			TRUE,	TRUE,	FALSE}, /* SampleRate */
{"res@size",			DLS_INTERFACE_PROP_SIZE,			DLS_UPNP_MASK_PROP_SIZE,			TRUE,	TRUE,	FALSE}, /* Size */
{"res@updateCount",		DLS_INTERFACE_PROP_UPDATE_COUNT,		DLS_UPNP_MASK_PROP_UPDATE_COUNT,		TRUE,	TRUE,	FALSE}, /* UpdateCount */

/* UPnP/DLNA Property Name	dLeyna/MS2 property name			dLeyna property mask				Filter  Search  Update */
{"upnp:album",			DLS_INTERFACE_PROP_ALBUM,			DLS_UPNP_MASK_PROP_ALBUM,			TRUE,	TRUE,	TRUE},  /* Album */
{"upnp:albumArtURI",		DLS_INTERFACE_PROP_ALBUM_ART_URL,		DLS_UPNP_MASK_PROP_ALBUM_ART_URL,		TRUE,	TRUE,	FALSE}, /* AlbumArtURL */
{"upnp:artist",			DLS_INTERFACE_PROP_ARTIST,			DLS_UPNP_MASK_PROP_ARTIST,			TRUE,	TRUE,	FALSE}, /* Artist */
{"upnp:artist",			DLS_INTERFACE_PROP_ARTISTS,			DLS_UPNP_MASK_PROP_ARTISTS,			TRUE,	FALSE,	TRUE},  /* Artists */
{"upnp:class",			DLS_INTERFACE_PROP_TYPE,			DLS_UPNP_MASK_PROP_TYPE,			FALSE,	TRUE,	TRUE},  /* Type */
{"upnp:containerUpdateID",	DLS_INTERFACE_PROP_CONTAINER_UPDATE_ID,		DLS_UPNP_MASK_PROP_CONTAINER_UPDATE_ID,		TRUE,	TRUE,	FALSE}, /* ContainerUpdateID */
{"upnp:createClass",		DLS_INTERFACE_PROP_CREATE_CLASSES,		DLS_UPNP_MASK_PROP_CREATE_CLASSES,		TRUE,	FALSE,	FALSE}, /* CreateClasses */
{"upnp:genre",			DLS_INTERFACE_PROP_GENRE,			DLS_UPNP_MASK_PROP_GENRE,			TRUE,	TRUE,	FALSE}, /* Genre */
{"upnp:objectUpdateID",		DLS_INTERFACE_PROP_OBJECT_UPDATE_ID,		DLS_UPNP_MASK_PROP_OBJECT_UPDATE_ID,		TRUE,	TRUE,	FALSE}, /* ObjectUpdateID */
{"upnp:originalTrackNumber",	DLS_INTERFACE_PROP_TRACK_NUMBER,		DLS_UPNP_MASK_PROP_TRACK_NUMBER,		TRUE,	TRUE,	TRUE},  /* TrackNumber */
{"upnp:totalDeletedChildCount",	DLS_INTERFACE_PROP_TOTAL_DELETED_CHILD_COUNT,	DLS_UPNP_MASK_PROP_TOTAL_DELETED_CHILD_COUNT,	TRUE,	TRUE,	FALSE}, /* TotalDeletedChildCount */

{NULL,				NULL,						0,						FALSE,	FALSE,	FALSE}
};


dls_prop_map_t *dls_props_def_get(void)
{
	return g_props_def;
}
