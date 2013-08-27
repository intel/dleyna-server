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

#ifndef DLS_UPNP_H__
#define DLS_UPNP_H__

#include <libdleyna/core/connector.h>

#include "client.h"
#include "async.h"

typedef void (*dls_upnp_callback_t)(const gchar *path, void *user_data);
typedef void (*dls_upnp_task_complete_t)(dls_task_t *task, GError *error);

dls_upnp_t *dls_upnp_new(dleyna_connector_id_t connection,
			 const dleyna_connector_dispatch_cb_t *dispatch_table,
			 dls_upnp_callback_t found_server,
			 dls_upnp_callback_t lost_server,
			 void *user_data);

void dls_upnp_delete(dls_upnp_t *upnp);

GVariant *dls_upnp_get_server_ids(dls_upnp_t *upnp);

GHashTable *dls_upnp_get_server_udn_map(dls_upnp_t *upnp);

void dls_upnp_get_children(dls_upnp_t *upnp, dls_client_t *client,
			   dls_task_t *task,
			   dls_upnp_task_complete_t cb);

void dls_upnp_get_all_props(dls_upnp_t *upnp, dls_client_t *client,
			    dls_task_t *task,
			    dls_upnp_task_complete_t cb);

void dls_upnp_get_prop(dls_upnp_t *upnp, dls_client_t *client,
		       dls_task_t *task,
		       dls_upnp_task_complete_t cb);

void dls_upnp_search(dls_upnp_t *upnp, dls_client_t *client,
		     dls_task_t *task,
		     dls_upnp_task_complete_t cb);

void dls_upnp_browse_objects(dls_upnp_t *upnp, dls_client_t *client,
			     dls_task_t *task,
			     dls_upnp_task_complete_t cb);

void dls_upnp_get_resource(dls_upnp_t *upnp, dls_client_t *client,
			   dls_task_t *task,
			   dls_upnp_task_complete_t cb);

void dls_upnp_upload_to_any(dls_upnp_t *upnp, dls_client_t *client,
			    dls_task_t *task,
			    dls_upnp_task_complete_t cb);

void dls_upnp_upload(dls_upnp_t *upnp, dls_client_t *client,
		     dls_task_t *task,
		     dls_upnp_task_complete_t cb);

void dls_upnp_get_upload_status(dls_upnp_t *upnp, dls_task_t *task);

void dls_upnp_get_upload_ids(dls_upnp_t *upnp, dls_task_t *task);

void dls_upnp_cancel_upload(dls_upnp_t *upnp, dls_task_t *task);

void dls_upnp_delete_object(dls_upnp_t *upnp, dls_client_t *client,
			    dls_task_t *task,
			    dls_upnp_task_complete_t cb);

void dls_upnp_create_container(dls_upnp_t *upnp, dls_client_t *client,
			       dls_task_t *task,
			       dls_upnp_task_complete_t cb);

void dls_upnp_create_container_in_any(dls_upnp_t *upnp, dls_client_t *client,
				      dls_task_t *task,
				      dls_upnp_task_complete_t cb);

void dls_upnp_update_object(dls_upnp_t *upnp, dls_client_t *client,
			    dls_task_t *task,
			    dls_upnp_task_complete_t cb);

void dls_upnp_get_object_metadata(dls_upnp_t *upnp, dls_client_t *client,
				  dls_task_t *task,
				  dls_upnp_task_complete_t cb);

void dls_upnp_create_reference(dls_upnp_t *upnp, dls_client_t *client,
			       dls_task_t *task,
			       dls_upnp_task_complete_t cb);

void dls_upnp_get_icon(dls_upnp_t *upnp, dls_client_t *client,
		       dls_task_t *task,
		       dls_upnp_task_complete_t cb);

void dls_upnp_unsubscribe(dls_upnp_t *upnp);

gboolean dls_upnp_device_context_exist(dls_device_t *device,
				       dls_device_context_t *context);

void dls_upnp_rescan(dls_upnp_t *upnp);

GUPnPContextManager *dls_upnp_get_context_manager(dls_upnp_t *upnp);

#endif /* DLS_UPNP_H__ */
