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

#ifndef MSU_DEVICE_H__
#define MSU_DEVICE_H__

#include <libgupnp/gupnp-control-point.h>

#include <libdleyna/core/connector.h>
#include <libdleyna/core/task-processor.h>

#include "async.h"
#include "client.h"
#include "props.h"

struct msu_device_context_t_ {
	gchar *ip_address;
	GUPnPDeviceProxy *device_proxy;
	GUPnPServiceProxy *service_proxy;
	msu_device_t *device;
	gboolean subscribed;
	guint timeout_id;
};

struct msu_device_t_ {
	dleyna_connector_id_t connection;
	guint id;
	gchar *path;
	GPtrArray *contexts;
	guint timeout_id;
	GHashTable *uploads;
	GHashTable *upload_jobs;
	guint upload_id;
	guint system_update_id;
	GVariant *search_caps;
	GVariant *sort_caps;
	GVariant *sort_ext_caps;
	GVariant *feature_list;
	gboolean shutting_down;
};

msu_device_context_t *msu_device_append_new_context(msu_device_t *device,
						    const gchar *ip_address,
						    GUPnPDeviceProxy *proxy);
void msu_device_delete(void *device);
msu_device_t *msu_device_new(
			dleyna_connector_id_t connection,
			GUPnPDeviceProxy *proxy,
			const gchar *ip_address,
			const dleyna_connector_dispatch_cb_t *dispatch_table,
			GHashTable *filter_map,
			guint counter,
			const dleyna_task_queue_key_t *queue_id);

msu_device_t *msu_device_from_path(const gchar *path, GHashTable *device_list);
msu_device_context_t *msu_device_get_context(const msu_device_t *device,
					     msu_client_t *client);
void msu_device_get_children(msu_client_t *client,
			     msu_task_t *task,
			     const gchar *upnp_filter, const gchar *sort_by);
void msu_device_get_all_props(msu_client_t *client,
			      msu_task_t *task,
			      gboolean root_object);
void msu_device_get_prop(msu_client_t *client,
			 msu_task_t *task,
			 msu_prop_map_t *prop_map, gboolean root_object);
void msu_device_search(msu_client_t *client,
		       msu_task_t *task,
		       const gchar *upnp_filter, const gchar *upnp_query,
		       const gchar *sort_by);
void msu_device_get_resource(msu_client_t *client,
			     msu_task_t *task,
			     const gchar *upnp_filter);
void msu_device_subscribe_to_contents_change(msu_device_t *device);
void msu_device_upload(msu_client_t *client,
		       msu_task_t *task, const gchar *parent_id);
gboolean msu_device_get_upload_status(msu_task_t *task, GError **error);
gboolean msu_device_cancel_upload(msu_task_t *task, GError **error);
void msu_device_get_upload_ids(msu_task_t *task);
void msu_device_delete_object(msu_client_t *client,
			      msu_task_t *task);
void msu_device_create_container(msu_client_t *client,
				 msu_task_t *task,
				 const gchar *parent_id);
void msu_device_update_object(msu_client_t *client,
			      msu_task_t *task,
			      const gchar *upnp_filter);

void msu_device_playlist_upload(msu_client_t *client,
				msu_task_t *task,
				const gchar *parent_id);

#endif
