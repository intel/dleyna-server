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
 * Mark Ryan <mark.d.ryan@intel.com>
 *
 */

#ifndef DLS_PROPS_H__
#define DLS_PROPS_H__

#include <libgupnp-av/gupnp-av.h>
#include "async.h"

typedef enum dls_props_index_t_ dls_props_index_t;

typedef struct dls_prop_map_t_ dls_prop_map_t;
struct dls_prop_map_t_ {
	const gchar *upnp_prop_name;
	const gchar *dls_prop_name;
	dls_upnp_prop_mask type;
	gboolean filter;
	gboolean searchable;
	gboolean updateable;
};

void dls_prop_maps_new(GHashTable **property_map, GHashTable **filter_map);

dls_upnp_prop_mask dls_props_parse_filter(GHashTable *filter_map,
					  GVariant *filter,
					  gchar **upnp_filter);

gboolean dls_props_parse_update_filter(GHashTable *filter_map,
				       GVariant *to_add_update,
				       GVariant *to_delete,
				       dls_upnp_prop_mask *mask,
				       gchar **upnp_filter);

void dls_props_add_device(GUPnPDeviceInfo *proxy,
			  const dls_device_t *device,
			  GVariantBuilder *vb);

GVariant *dls_props_get_device_prop(GUPnPDeviceInfo *proxy,
				    const dls_device_t *device,
				    const gchar *prop);

gboolean dls_props_add_object(GVariantBuilder *item_vb,
			      GUPnPDIDLLiteObject *object,
			      const char *root_path,
			      const gchar *parent_path,
			      dls_upnp_prop_mask filter_mask);

GVariant *dls_props_get_object_prop(const gchar *prop, const gchar *root_path,
				    GUPnPDIDLLiteObject *object);

void dls_props_add_container(GVariantBuilder *item_vb,
			     GUPnPDIDLLiteContainer *object,
			     dls_upnp_prop_mask filter_mask,
			     const gchar *protocol_info,
			     gboolean *have_child_count);

void dls_props_add_child_count(GVariantBuilder *item_vb, gint value);

GVariant *dls_props_get_container_prop(const gchar *prop,
				       GUPnPDIDLLiteObject *object,
				       const gchar *protocol_info);

void dls_props_add_resource(GVariantBuilder *item_vb,
			    GUPnPDIDLLiteObject *object,
			    dls_upnp_prop_mask filter_mask,
			    const gchar *protocol_info);

void dls_props_add_item(GVariantBuilder *item_vb,
			GUPnPDIDLLiteObject *object,
			const gchar *root_path,
			dls_upnp_prop_mask filter_mask,
			const gchar *protocol_info);

GVariant *dls_props_get_item_prop(const gchar *prop, const gchar *root_path,
				  GUPnPDIDLLiteObject *object,
				  const gchar *protocol_info);

const gchar *dls_props_media_spec_to_upnp_class(const gchar *m2spec_class);

const gchar *dls_props_upnp_class_to_media_spec(const gchar *upnp_class);

GVariant *dls_props_get_prop_value(GUPnPDeviceInfo *proxy,
				   GUPnPDIDLLiteObject *object,
				   GUPnPDIDLLiteResource *res,
				   const char *root_path,
				   const gchar *parent_path,
				   const dls_device_t *device,
				   dls_props_index_t prop_index,
				   const gchar *protocol_info,
				   gboolean always);

#endif /* DLS_PROPS_H__ */
