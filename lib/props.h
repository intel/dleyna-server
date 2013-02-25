/*
 * dleyna
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
 * Mark Ryan <mark.d.ryan@intel.com>
 *
 */

#ifndef MSU_PROPS_H__
#define MSU_PROPS_H__

#include <libgupnp-av/gupnp-av.h>
#include "async.h"

#define MSU_UPNP_MASK_PROP_PARENT			(1LL << 0)
#define MSU_UPNP_MASK_PROP_TYPE				(1LL << 1)
#define MSU_UPNP_MASK_PROP_PATH				(1LL << 2)
#define MSU_UPNP_MASK_PROP_DISPLAY_NAME			(1LL << 3)
#define MSU_UPNP_MASK_PROP_CHILD_COUNT			(1LL << 4)
#define MSU_UPNP_MASK_PROP_SEARCHABLE			(1LL << 5)
#define MSU_UPNP_MASK_PROP_URLS				(1LL << 6)
#define MSU_UPNP_MASK_PROP_MIME_TYPE			(1LL << 7)
#define MSU_UPNP_MASK_PROP_ARTIST			(1LL << 8)
#define MSU_UPNP_MASK_PROP_ALBUM			(1LL << 9)
#define MSU_UPNP_MASK_PROP_DATE				(1LL << 10)
#define MSU_UPNP_MASK_PROP_GENRE			(1LL << 11)
#define MSU_UPNP_MASK_PROP_DLNA_PROFILE			(1LL << 12)
#define MSU_UPNP_MASK_PROP_TRACK_NUMBER			(1LL << 13)
#define MSU_UPNP_MASK_PROP_SIZE				(1LL << 14)
#define MSU_UPNP_MASK_PROP_DURATION			(1LL << 15)
#define MSU_UPNP_MASK_PROP_BITRATE			(1LL << 16)
#define MSU_UPNP_MASK_PROP_SAMPLE_RATE			(1LL << 17)
#define MSU_UPNP_MASK_PROP_BITS_PER_SAMPLE		(1LL << 18)
#define MSU_UPNP_MASK_PROP_WIDTH			(1LL << 19)
#define MSU_UPNP_MASK_PROP_HEIGHT			(1LL << 20)
#define MSU_UPNP_MASK_PROP_COLOR_DEPTH			(1LL << 21)
#define MSU_UPNP_MASK_PROP_ALBUM_ART_URL		(1LL << 22)
#define MSU_UPNP_MASK_PROP_RESOURCES			(1LL << 23)
#define MSU_UPNP_MASK_PROP_URL				(1LL << 24)
#define MSU_UPNP_MASK_PROP_REFPATH			(1LL << 25)
#define MSU_UPNP_MASK_PROP_RESTRICTED			(1LL << 26)
#define MSU_UPNP_MASK_PROP_DLNA_MANAGED			(1LL << 27)
#define MSU_UPNP_MASK_PROP_CREATOR			(1LL << 28)
#define MSU_UPNP_MASK_PROP_ARTISTS			(1LL << 29)
#define MSU_UPNP_MASK_PROP_CREATE_CLASSES		(1LL << 30)
#define MSU_UPNP_MASK_PROP_OBJECT_UPDATE_ID		(1LL << 31)
#define MSU_UPNP_MASK_PROP_UPDATE_COUNT			(1LL << 32)
#define MSU_UPNP_MASK_PROP_CONTAINER_UPDATE_ID		(1LL << 33)
#define MSU_UPNP_MASK_PROP_TOTAL_DELETED_CHILD_COUNT	(1LL << 34)

#define MSU_UPNP_MASK_ALL_PROPS 0xffffffffffffffff

typedef struct msu_prop_map_t_ msu_prop_map_t;
struct msu_prop_map_t_ {
	const gchar *upnp_prop_name;
	msu_upnp_prop_mask type;
	gboolean filter;
	gboolean searchable;
	gboolean updateable;
};

void msu_prop_maps_new(GHashTable **property_map, GHashTable **filter_map);

msu_upnp_prop_mask msu_props_parse_filter(GHashTable *filter_map,
					  GVariant *filter,
					  gchar **upnp_filter);

gboolean msu_props_parse_update_filter(GHashTable *filter_map,
				       GVariant *to_add_update,
				       GVariant *to_delete,
				       msu_upnp_prop_mask *mask,
				       gchar **upnp_filter);

void msu_props_add_device(GUPnPDeviceInfo *proxy,
			  const msu_device_t *device,
			  GVariantBuilder *vb);

GVariant *msu_props_get_device_prop(GUPnPDeviceInfo *proxy,
				    const msu_device_t *device,
				    const gchar *prop);

gboolean msu_props_add_object(GVariantBuilder *item_vb,
			      GUPnPDIDLLiteObject *object,
			      const char *root_path,
			      const gchar *parent_path,
			      msu_upnp_prop_mask filter_mask);

GVariant *msu_props_get_object_prop(const gchar *prop, const gchar *root_path,
				    GUPnPDIDLLiteObject *object);

void msu_props_add_container(GVariantBuilder *item_vb,
			     GUPnPDIDLLiteContainer *object,
			     msu_upnp_prop_mask filter_mask,
			     gboolean *have_child_count);

void msu_props_add_child_count(GVariantBuilder *item_vb, gint value);

GVariant *msu_props_get_container_prop(const gchar *prop,
				       GUPnPDIDLLiteObject *object);

void msu_props_add_resource(GVariantBuilder *item_vb,
			    GUPnPDIDLLiteObject *object,
			    msu_upnp_prop_mask filter_mask,
			    const gchar *protocol_info);

void msu_props_add_item(GVariantBuilder *item_vb,
			GUPnPDIDLLiteObject *object,
			const gchar *root_path,
			msu_upnp_prop_mask filter_mask,
			const gchar *protocol_info);

GVariant *msu_props_get_item_prop(const gchar *prop, const gchar *root_path,
				  GUPnPDIDLLiteObject *object,
				  const gchar *protocol_info);

const gchar *msu_props_media_spec_to_upnp_class(const gchar *m2spec_class);

const gchar *msu_props_upnp_class_to_media_spec(const gchar *upnp_class);

#endif
