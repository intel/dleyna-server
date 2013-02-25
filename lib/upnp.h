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

#ifndef MSU_UPNP_H__
#define MSU_UPNP_H__

#include <libdleyna/core/connector.h>

#include "client.h"
#include "async.h"

typedef void (*msu_upnp_callback_t)(const gchar *path, void *user_data);
typedef void (*msu_upnp_task_complete_t)(msu_task_t *task, GError *error);

msu_upnp_t *msu_upnp_new(dleyna_connector_id_t connection,
			 const dleyna_connector_dispatch_cb_t *dispatch_table,
			 msu_upnp_callback_t found_server,
			 msu_upnp_callback_t lost_server,
			 void *user_data);
void msu_upnp_delete(msu_upnp_t *upnp);
GVariant *msu_upnp_get_server_ids(msu_upnp_t *upnp);
GHashTable *msu_upnp_get_server_udn_map(msu_upnp_t *upnp);
void msu_upnp_get_children(msu_upnp_t *upnp, msu_client_t *client,
			   msu_task_t *task,
			   msu_upnp_task_complete_t cb);
void msu_upnp_get_all_props(msu_upnp_t *upnp, msu_client_t *client,
			    msu_task_t *task,
			    msu_upnp_task_complete_t cb);
void msu_upnp_get_prop(msu_upnp_t *upnp, msu_client_t *client,
		       msu_task_t *task,
		       msu_upnp_task_complete_t cb);
void msu_upnp_search(msu_upnp_t *upnp, msu_client_t *client,
		     msu_task_t *task,
		     msu_upnp_task_complete_t cb);
void msu_upnp_get_resource(msu_upnp_t *upnp, msu_client_t *client,
			   msu_task_t *task,
			   msu_upnp_task_complete_t cb);
void msu_upnp_upload_to_any(msu_upnp_t *upnp, msu_client_t *client,
			    msu_task_t *task,
			    msu_upnp_task_complete_t cb);
void msu_upnp_upload(msu_upnp_t *upnp, msu_client_t *client,
		     msu_task_t *task,
		     msu_upnp_task_complete_t cb);
void msu_upnp_get_upload_status(msu_upnp_t *upnp, msu_task_t *task);
void msu_upnp_get_upload_ids(msu_upnp_t *upnp, msu_task_t *task);
void msu_upnp_cancel_upload(msu_upnp_t *upnp, msu_task_t *task);
void msu_upnp_delete_object(msu_upnp_t *upnp, msu_client_t *client,
			    msu_task_t *task,
			    msu_upnp_task_complete_t cb);
void msu_upnp_create_container(msu_upnp_t *upnp, msu_client_t *client,
			       msu_task_t *task,
			       msu_upnp_task_complete_t cb);
void msu_upnp_create_container_in_any(msu_upnp_t *upnp, msu_client_t *client,
				      msu_task_t *task,
				      msu_upnp_task_complete_t cb);
void msu_upnp_update_object(msu_upnp_t *upnp, msu_client_t *client,
			    msu_task_t *task,
			    msu_upnp_task_complete_t cb);
void msu_upnp_create_playlist(msu_upnp_t *upnp, msu_client_t *client,
			      msu_task_t *task,
			      msu_upnp_task_complete_t cb);
void msu_upnp_create_playlist_in_any(msu_upnp_t *upnp, msu_client_t *client,
				     msu_task_t *task,
				     msu_upnp_task_complete_t cb);

gboolean msu_upnp_device_context_exist(msu_device_t *device,
				       msu_device_context_t *context);

#endif
