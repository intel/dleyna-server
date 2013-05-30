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

#ifndef DLS_TASK_H__
#define DLS_TASK_H__

#include <gio/gio.h>
#include <glib.h>

#include <libdleyna/core/connector.h>
#include <libdleyna/core/task-atom.h>

#include "server.h"

enum dls_task_type_t_ {
	DLS_TASK_GET_VERSION,
	DLS_TASK_GET_SERVERS,
	DLS_TASK_RESCAN,
	DLS_TASK_GET_CHILDREN,
	DLS_TASK_GET_ALL_PROPS,
	DLS_TASK_GET_PROP,
	DLS_TASK_SEARCH,
	DLS_TASK_GET_RESOURCE,
	DLS_TASK_SET_PREFER_LOCAL_ADDRESSES,
	DLS_TASK_SET_PROTOCOL_INFO,
	DLS_TASK_UPLOAD_TO_ANY,
	DLS_TASK_UPLOAD,
	DLS_TASK_GET_UPLOAD_STATUS,
	DLS_TASK_GET_UPLOAD_IDS,
	DLS_TASK_CANCEL_UPLOAD,
	DLS_TASK_DELETE_OBJECT,
	DLS_TASK_CREATE_CONTAINER,
	DLS_TASK_CREATE_CONTAINER_IN_ANY,
	DLS_TASK_UPDATE_OBJECT,
	DLS_TASK_GET_OBJECT_METADATA,
	DLS_TASK_CREATE_REFERENCE,
	DLS_TASK_GET_ICON
};
typedef enum dls_task_type_t_ dls_task_type_t;

typedef void (*dls_cancel_task_t)(void *handle);

typedef struct dls_task_get_children_t_ dls_task_get_children_t;
struct dls_task_get_children_t_ {
	gboolean containers;
	gboolean items;
	guint start;
	guint count;
	GVariant *filter;
	gchar *sort_by;
};

typedef struct dls_task_get_props_t_ dls_task_get_props_t;
struct dls_task_get_props_t_ {
	gchar *interface_name;
};

typedef struct dls_task_get_prop_t_ dls_task_get_prop_t;
struct dls_task_get_prop_t_ {
	gchar *prop_name;
	gchar *interface_name;
};

typedef struct dls_task_search_t_ dls_task_search_t;
struct dls_task_search_t_ {
	gchar *query;
	guint start;
	guint count;
	gchar *sort_by;
	GVariant *filter;
};

typedef struct dls_task_get_resource_t_ dls_task_get_resource_t;
struct dls_task_get_resource_t_ {
	gchar *protocol_info;
	GVariant *filter;
};

typedef struct dls_task_set_prefer_local_addresses_t_
					dls_task_set_prefer_local_addresses_t;
struct dls_task_set_prefer_local_addresses_t_ {
	gboolean prefer;
};

typedef struct dls_task_set_protocol_info_t_ dls_task_set_protocol_info_t;
struct dls_task_set_protocol_info_t_ {
	gchar *protocol_info;
};

typedef struct dls_task_upload_t_ dls_task_upload_t;
struct dls_task_upload_t_ {
	gchar *display_name;
	gchar *file_path;
};

typedef struct dls_task_upload_action_t_ dls_task_upload_action_t;
struct dls_task_upload_action_t_ {
	guint upload_id;
};

typedef struct dls_task_create_container_t_ dls_task_create_container_t;
struct dls_task_create_container_t_ {
	gchar *display_name;
	gchar *type;
	GVariant *child_types;
};

typedef struct dls_task_update_t_ dls_task_update_t;
struct dls_task_update_t_ {
	GVariant *to_add_update;
	GVariant *to_delete;
};

typedef struct dls_task_create_reference_t_ dls_task_create_reference_t;
struct dls_task_create_reference_t_ {
	gchar *item_path;
};

typedef struct dls_task_target_info_t_ dls_task_target_info_t;
struct dls_task_target_info_t_ {
	gchar *path;
	gchar *root_path;
	gchar *id;
	dls_device_t *device;
};

typedef struct dls_task_get_icon_t_ dls_task_get_icon_t;
struct dls_task_get_icon_t_ {
	gchar *mime_type;
	gchar *resolution;
};

typedef struct dls_task_t_ dls_task_t;
struct dls_task_t_ {
	dleyna_task_atom_t atom; /* pseudo inheritance - MUST be first field */
	dls_task_type_t type;
	dls_task_target_info_t target;
	const gchar *result_format;
	GVariant *result;
	dleyna_connector_msg_id_t invocation;
	gboolean synchronous;
	gboolean multiple_retvals;
	union {
		dls_task_get_children_t get_children;
		dls_task_get_props_t get_props;
		dls_task_get_prop_t get_prop;
		dls_task_search_t search;
		dls_task_get_resource_t resource;
		dls_task_set_prefer_local_addresses_t prefer_local_addresses;
		dls_task_set_protocol_info_t protocol_info;
		dls_task_upload_t upload;
		dls_task_upload_action_t upload_action;
		dls_task_create_container_t create_container;
		dls_task_update_t update;
		dls_task_create_reference_t create_reference;
		dls_task_get_icon_t get_icon;
	} ut;
};

dls_task_t *dls_task_rescan_new(dleyna_connector_msg_id_t invocation);

dls_task_t *dls_task_get_version_new(dleyna_connector_msg_id_t invocation);

dls_task_t *dls_task_get_servers_new(dleyna_connector_msg_id_t invocation);

dls_task_t *dls_task_get_children_new(dleyna_connector_msg_id_t invocation,
				      const gchar *path, GVariant *parameters,
				      gboolean items, gboolean containers,
				      GError **error);

dls_task_t *dls_task_get_children_ex_new(dleyna_connector_msg_id_t invocation,
					 const gchar *path,
					 GVariant *parameters, gboolean items,
					 gboolean containers,
					 GError **error);

dls_task_t *dls_task_get_prop_new(dleyna_connector_msg_id_t invocation,
				  const gchar *path, GVariant *parameters,
				  GError **error);

dls_task_t *dls_task_get_props_new(dleyna_connector_msg_id_t invocation,
				   const gchar *path, GVariant *parameters,
				   GError **error);

dls_task_t *dls_task_search_new(dleyna_connector_msg_id_t invocation,
				const gchar *path, GVariant *parameters,
				GError **error);

dls_task_t *dls_task_search_ex_new(dleyna_connector_msg_id_t invocation,
				   const gchar *path, GVariant *parameters,
				   GError **error);

dls_task_t *dls_task_get_resource_new(dleyna_connector_msg_id_t invocation,
				      const gchar *path, GVariant *parameters,
				      GError **error);

dls_task_t *dls_task_set_protocol_info_new(dleyna_connector_msg_id_t invocation,
					   GVariant *parameters);

dls_task_t *dls_task_prefer_local_addresses_new(
					dleyna_connector_msg_id_t invocation,
					GVariant *parameters);

dls_task_t *dls_task_upload_to_any_new(dleyna_connector_msg_id_t invocation,
				       const gchar *path, GVariant *parameters,
				       GError **error);

dls_task_t *dls_task_upload_new(dleyna_connector_msg_id_t invocation,
				const gchar *path, GVariant *parameters,
				GError **error);

dls_task_t *dls_task_get_upload_status_new(dleyna_connector_msg_id_t invocation,
					   const gchar *path,
					   GVariant *parameters,
					   GError **error);

dls_task_t *dls_task_get_upload_ids_new(dleyna_connector_msg_id_t invocation,
					const gchar *path,
					GError **error);

dls_task_t *dls_task_cancel_upload_new(dleyna_connector_msg_id_t invocation,
				       const gchar *path,
				       GVariant *parameters,
				       GError **error);

dls_task_t *dls_task_delete_new(dleyna_connector_msg_id_t invocation,
				const gchar *path,
				GError **error);

dls_task_t *dls_task_create_container_new_generic(
					dleyna_connector_msg_id_t invocation,
					dls_task_type_t type,
					const gchar *path,
					GVariant *parameters,
					GError **error);

dls_task_t *dls_task_create_reference_new(dleyna_connector_msg_id_t invocation,
					  dls_task_type_t type,
					  const gchar *path,
					  GVariant *parameters,
					  GError **error);

dls_task_t *dls_task_update_new(dleyna_connector_msg_id_t invocation,
				const gchar *path, GVariant *parameters,
				GError **error);

dls_task_t *dls_task_get_metadata_new(dleyna_connector_msg_id_t invocation,
				      const gchar *path,
				      GError **error);

dls_task_t *dls_task_get_icon_new(dleyna_connector_msg_id_t invocation,
				  const gchar *path, GVariant *parameters,
				  GError **error);

void dls_task_cancel(dls_task_t *task);

void dls_task_complete(dls_task_t *task);

void dls_task_fail(dls_task_t *task, GError *error);

void dls_task_delete(dls_task_t *task);

#endif /* DLS_TASK_H__ */
