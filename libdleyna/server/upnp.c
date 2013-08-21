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

#include <string.h>

#include <libgssdp/gssdp-resource-browser.h>
#include <libgupnp/gupnp-control-point.h>
#include <libgupnp/gupnp-error.h>

#include <libdleyna/core/error.h>
#include <libdleyna/core/log.h>
#include <libdleyna/core/service-task.h>

#include "async.h"
#include "device.h"
#include "interface.h"
#include "path.h"
#include "search.h"
#include "sort.h"
#include "upnp.h"

struct dls_upnp_t_ {
	dleyna_connector_id_t connection;
	const dleyna_connector_dispatch_cb_t *interface_info;
	GHashTable *filter_map;
	GHashTable *property_map;
	dls_upnp_callback_t found_server;
	dls_upnp_callback_t lost_server;
	GUPnPContextManager *context_manager;
	void *user_data;
	GHashTable *server_udn_map;
	GHashTable *server_uc_map;
	guint counter;
};

/* Private structure used in service task */
typedef struct prv_device_new_ct_t_ prv_device_new_ct_t;
struct prv_device_new_ct_t_ {
	dls_upnp_t *upnp;
	char *udn;
	gchar *ip_address;
	dls_device_t *device;
	const dleyna_task_queue_key_t *queue_id;
};

static void prv_device_new_free(prv_device_new_ct_t *priv_t)
{
	if (priv_t) {
		g_free(priv_t->udn);
		g_free(priv_t->ip_address);
		g_free(priv_t);
	}
}

static void prv_device_chain_end(gboolean cancelled, gpointer data)
{
	dls_device_t *device;
	prv_device_new_ct_t *priv_t = (prv_device_new_ct_t *)data;

	DLEYNA_LOG_DEBUG("Enter");

	device = priv_t->device;

	if (cancelled)
		goto on_clear;

	DLEYNA_LOG_DEBUG("Notify new server available: %s", device->path);
	g_hash_table_insert(priv_t->upnp->server_udn_map, g_strdup(priv_t->udn),
			    device);
	priv_t->upnp->found_server(device->path, priv_t->upnp->user_data);

on_clear:

	g_hash_table_remove(priv_t->upnp->server_uc_map, priv_t->udn);

	if (cancelled)
		dls_device_delete(device);

	prv_device_new_free(priv_t);

	DLEYNA_LOG_DEBUG_NL();
}

static void prv_device_context_switch_end(gboolean cancelled, gpointer data)
{
	prv_device_new_ct_t *priv_t = (prv_device_new_ct_t *)data;

	DLEYNA_LOG_DEBUG("Enter");

	g_hash_table_remove(priv_t->upnp->server_uc_map, priv_t->udn);
	prv_device_new_free(priv_t);

	DLEYNA_LOG_DEBUG("Exit");
}

static const dleyna_task_queue_key_t *prv_create_device_queue(
						prv_device_new_ct_t **priv_t)
{
	const dleyna_task_queue_key_t *queue_id;

	*priv_t = g_new0(prv_device_new_ct_t, 1);

	queue_id = dleyna_task_processor_add_queue(
			dls_server_get_task_processor(),
			dleyna_service_task_create_source(),
			DLS_SERVER_SINK,
			DLEYNA_TASK_QUEUE_FLAG_AUTO_REMOVE,
			dleyna_service_task_process_cb,
			dleyna_service_task_cancel_cb,
			dleyna_service_task_delete_cb);
	dleyna_task_queue_set_finally(queue_id, prv_device_chain_end);
	dleyna_task_queue_set_user_data(queue_id, *priv_t);


	return queue_id;
}

static void prv_update_device_context(prv_device_new_ct_t *priv_t,
				      dls_upnp_t *upnp, const char *udn,
				      dls_device_t *device,
				      const gchar *ip_address,
				      const dleyna_task_queue_key_t *queue_id)
{
	priv_t->upnp = upnp;
	priv_t->udn = g_strdup(udn);
	priv_t->ip_address = g_strdup(ip_address);
	priv_t->queue_id = queue_id;
	priv_t->device = device;

	g_hash_table_insert(upnp->server_uc_map, g_strdup(udn), priv_t);
}

static void prv_server_available_cb(GUPnPControlPoint *cp,
				    GUPnPDeviceProxy *proxy,
				    gpointer user_data)
{
	dls_upnp_t *upnp = user_data;
	const char *udn;
	dls_device_t *device;
	const gchar *ip_address;
	dls_device_context_t *context;
	const dleyna_task_queue_key_t *queue_id;
	unsigned int i;
	prv_device_new_ct_t *priv_t;

	udn = gupnp_device_info_get_udn((GUPnPDeviceInfo *)proxy);

	ip_address = gupnp_context_get_host_ip(
		gupnp_control_point_get_context(cp));

	if (!udn || !ip_address)
		goto on_error;

	DLEYNA_LOG_DEBUG("UDN %s", udn);
	DLEYNA_LOG_DEBUG("IP Address %s", ip_address);

	device = g_hash_table_lookup(upnp->server_udn_map, udn);

	if (!device) {
		priv_t = g_hash_table_lookup(upnp->server_uc_map, udn);

		if (priv_t)
			device = priv_t->device;
	}

	if (!device) {
		DLEYNA_LOG_DEBUG("Device not found. Adding");
		DLEYNA_LOG_DEBUG_NL();

		queue_id = prv_create_device_queue(&priv_t);

		device = dls_device_new(upnp->connection, proxy, ip_address,
					upnp->interface_info,
					upnp->property_map, upnp->counter,
					queue_id);

		prv_update_device_context(priv_t, upnp, udn, device, ip_address,
					  queue_id);

		upnp->counter++;
	} else {
		DLEYNA_LOG_DEBUG("Device Found");

		for (i = 0; i < device->contexts->len; ++i) {
			context = g_ptr_array_index(device->contexts, i);
			if (!strcmp(context->ip_address, ip_address))
				break;
		}

		if (i == device->contexts->len) {
			DLEYNA_LOG_DEBUG("Adding Context");
			(void) dls_device_append_new_context(device, ip_address,
							     proxy);
		}

		DLEYNA_LOG_DEBUG_NL();
	}

on_error:

	return;
}

static gboolean prv_subscribe_to_contents_change(gpointer user_data)
{
	dls_device_t *device = user_data;

	device->timeout_id = 0;
	dls_device_subscribe_to_contents_change(device);

	return FALSE;
}

static void prv_server_unavailable_cb(GUPnPControlPoint *cp,
				      GUPnPDeviceProxy *proxy,
				      gpointer user_data)
{
	dls_upnp_t *upnp = user_data;
	const char *udn;
	dls_device_t *device;
	const gchar *ip_address;
	unsigned int i;
	dls_device_context_t *context;
	gboolean subscribed;
	gboolean construction_ctx = FALSE;
	gboolean under_construction = FALSE;
	prv_device_new_ct_t *priv_t;
	const dleyna_task_queue_key_t *queue_id;

	DLEYNA_LOG_DEBUG("Enter");

	udn = gupnp_device_info_get_udn((GUPnPDeviceInfo *)proxy);

	ip_address = gupnp_context_get_host_ip(
		gupnp_control_point_get_context(cp));

	if (!udn || !ip_address)
		goto on_error;

	DLEYNA_LOG_DEBUG("UDN %s", udn);
	DLEYNA_LOG_DEBUG("IP Address %s", ip_address);

	device = g_hash_table_lookup(upnp->server_udn_map, udn);

	if (!device) {
		priv_t = g_hash_table_lookup(upnp->server_uc_map, udn);

		if (priv_t) {
			device = priv_t->device;
			under_construction = TRUE;
		}
	}

	if (!device) {
		DLEYNA_LOG_WARNING("Device not found. Ignoring");
		goto on_error;
	}

	for (i = 0; i < device->contexts->len; ++i) {
		context = g_ptr_array_index(device->contexts, i);
		if (!strcmp(context->ip_address, ip_address))
			break;
	}

	if (i >= device->contexts->len)
		goto on_error;

	subscribed = context->subscribed;
	if (under_construction)
		construction_ctx = !strcmp(context->ip_address,
					   priv_t->ip_address);

	(void) g_ptr_array_remove_index(device->contexts, i);

	if (device->contexts->len == 0) {
		if (!under_construction) {
			DLEYNA_LOG_DEBUG("Last Context lost. Delete device");
			upnp->lost_server(device->path, upnp->user_data);
			g_hash_table_remove(upnp->server_udn_map, udn);
		} else {
			DLEYNA_LOG_WARNING(
				"Device under construction. Cancelling");

			dleyna_task_processor_cancel_queue(priv_t->queue_id);
		}
	} else if (under_construction && construction_ctx) {
		DLEYNA_LOG_WARNING(
			"Device under construction. Switching context");

		/* Cancel previous contruction task chain */
		g_hash_table_remove(priv_t->upnp->server_uc_map, priv_t->udn);
		dleyna_task_queue_set_finally(priv_t->queue_id,
					      prv_device_context_switch_end);
		dleyna_task_processor_cancel_queue(priv_t->queue_id);

		/* Create a new construction task chain */
		context = dls_device_get_context(device, NULL);
		queue_id = prv_create_device_queue(&priv_t);
		prv_update_device_context(priv_t, upnp, udn, device,
					  context->ip_address, queue_id);

		/* Start tasks from current construction step */
		dls_device_construct(device,
				     context,
				     upnp->connection,
				     upnp->interface_info,
				     upnp->property_map,
				     queue_id);

	} else if (subscribed && !device->timeout_id) {
		DLEYNA_LOG_DEBUG("Subscribe on new context");

		device->timeout_id = g_timeout_add_seconds(1,
				prv_subscribe_to_contents_change,
				device);
	}

on_error:

	DLEYNA_LOG_DEBUG("Exit");
	DLEYNA_LOG_DEBUG_NL();

	return;
}

static void prv_on_context_available(GUPnPContextManager *context_manager,
				     GUPnPContext *context,
				     gpointer user_data)
{
	dls_upnp_t *upnp = user_data;
	GUPnPControlPoint *cp;

	cp = gupnp_control_point_new(
		context,
		"urn:schemas-upnp-org:device:MediaServer:1");

	g_signal_connect(cp, "device-proxy-available",
			 G_CALLBACK(prv_server_available_cb), upnp);

	g_signal_connect(cp, "device-proxy-unavailable",
			 G_CALLBACK(prv_server_unavailable_cb), upnp);

	gssdp_resource_browser_set_active(GSSDP_RESOURCE_BROWSER(cp), TRUE);
	gupnp_context_manager_manage_control_point(upnp->context_manager, cp);
	g_object_unref(cp);
}

dls_upnp_t *dls_upnp_new(dleyna_connector_id_t connection,
			 const dleyna_connector_dispatch_cb_t *dispatch_table,
			 dls_upnp_callback_t found_server,
			 dls_upnp_callback_t lost_server,
			 void *user_data)
{
	dls_upnp_t *upnp = g_new0(dls_upnp_t, 1);

	upnp->connection = connection;
	upnp->interface_info = dispatch_table;
	upnp->user_data = user_data;
	upnp->found_server = found_server;
	upnp->lost_server = lost_server;

	upnp->server_udn_map = g_hash_table_new_full(g_str_hash, g_str_equal,
						     g_free,
						     dls_device_delete);

	upnp->server_uc_map = g_hash_table_new_full(g_str_hash, g_str_equal,
						    g_free, NULL);

	dls_prop_maps_new(&upnp->property_map, &upnp->filter_map);

	upnp->context_manager = gupnp_context_manager_create(0);

	g_signal_connect(upnp->context_manager, "context-available",
			 G_CALLBACK(prv_on_context_available),
			 upnp);

	return upnp;
}

void dls_upnp_delete(dls_upnp_t *upnp)
{
	if (upnp) {
		g_object_unref(upnp->context_manager);
		g_hash_table_unref(upnp->property_map);
		g_hash_table_unref(upnp->filter_map);
		g_hash_table_unref(upnp->server_udn_map);
		g_hash_table_unref(upnp->server_uc_map);
		g_free(upnp);
	}
}

GVariant *dls_upnp_get_server_ids(dls_upnp_t *upnp)
{
	GVariantBuilder vb;
	GHashTableIter iter;
	gpointer value;
	dls_device_t *device;
	GVariant *retval;

	DLEYNA_LOG_DEBUG("Enter");

	g_variant_builder_init(&vb, G_VARIANT_TYPE("ao"));

	g_hash_table_iter_init(&iter, upnp->server_udn_map);
	while (g_hash_table_iter_next(&iter, NULL, &value)) {
		device = value;
		DLEYNA_LOG_DEBUG("Have device %s", device->path);
		g_variant_builder_add(&vb, "o", device->path);
	}

	retval = g_variant_ref_sink(g_variant_builder_end(&vb));

	DLEYNA_LOG_DEBUG("Exit");

	return retval;
}

GHashTable *dls_upnp_get_server_udn_map(dls_upnp_t *upnp)
{
	return upnp->server_udn_map;
}

void dls_upnp_get_children(dls_upnp_t *upnp, dls_client_t *client,
			   dls_task_t *task,
			   dls_upnp_task_complete_t cb)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_async_bas_t *cb_task_data;
	gchar *upnp_filter = NULL;
	gchar *sort_by = NULL;

	DLEYNA_LOG_DEBUG("Enter");

	DLEYNA_LOG_DEBUG("Path: %s", task->target.path);
	DLEYNA_LOG_DEBUG("Start: %u", task->ut.get_children.start);
	DLEYNA_LOG_DEBUG("Count: %u", task->ut.get_children.count);

	cb_data->cb = cb;
	cb_task_data = &cb_data->ut.bas;

	cb_task_data->filter_mask =
		dls_props_parse_filter(upnp->filter_map,
				       task->ut.get_children.filter,
				       &upnp_filter);

	DLEYNA_LOG_DEBUG("Filter Mask 0x%"G_GUINT64_FORMAT"x",
			 cb_task_data->filter_mask);

	sort_by = dls_sort_translate_sort_string(upnp->filter_map,
						 task->ut.get_children.sort_by);
	if (!sort_by) {
		DLEYNA_LOG_WARNING("Invalid Sort Criteria");

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_BAD_QUERY,
					     "Sort Criteria are not valid");
		goto on_error;
	}

	DLEYNA_LOG_DEBUG("Sort By %s", sort_by);

	cb_task_data->protocol_info = client->protocol_info;

	dls_device_get_children(client, task, upnp_filter, sort_by);

on_error:

	if (!cb_data->action)
		(void) g_idle_add(dls_async_task_complete, cb_data);

	g_free(sort_by);
	g_free(upnp_filter);

	DLEYNA_LOG_DEBUG("Exit with %s", !cb_data->action ? "FAIL" : "SUCCESS");
}

void dls_upnp_get_all_props(dls_upnp_t *upnp, dls_client_t *client,
			    dls_task_t *task,
			    dls_upnp_task_complete_t cb)
{
	gboolean root_object;
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_async_get_all_t *cb_task_data;

	DLEYNA_LOG_DEBUG("Enter");

	DLEYNA_LOG_DEBUG("Path: %s", task->target.path);
	DLEYNA_LOG_DEBUG("Interface %s", task->ut.get_prop.interface_name);

	cb_data->cb = cb;
	cb_task_data = &cb_data->ut.get_all;

	root_object = task->target.id[0] == '0' && task->target.id[1] == 0;

	DLEYNA_LOG_DEBUG("Root Object = %d", root_object);

	cb_task_data->protocol_info = client->protocol_info;

	dls_device_get_all_props(client, task, root_object);

	DLEYNA_LOG_DEBUG("Exit with SUCCESS");
}

void dls_upnp_get_prop(dls_upnp_t *upnp, dls_client_t *client,
		       dls_task_t *task,
		       dls_upnp_task_complete_t cb)
{
	gboolean root_object;
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_async_get_prop_t *cb_task_data;
	dls_prop_map_t *prop_map;
	dls_task_get_prop_t *task_data;

	DLEYNA_LOG_DEBUG("Enter");

	DLEYNA_LOG_DEBUG("Path: %s", task->target.path);
	DLEYNA_LOG_DEBUG("Interface %s", task->ut.get_prop.interface_name);
	DLEYNA_LOG_DEBUG("Prop.%s", task->ut.get_prop.prop_name);

	task_data = &task->ut.get_prop;
	cb_data->cb = cb;
	cb_task_data = &cb_data->ut.get_prop;

	root_object = task->target.id[0] == '0' && task->target.id[1] == 0;

	DLEYNA_LOG_DEBUG("Root Object = %d", root_object);

	cb_task_data->protocol_info = client->protocol_info;
	prop_map = g_hash_table_lookup(upnp->filter_map, task_data->prop_name);

	dls_device_get_prop(client, task, prop_map, root_object);

	DLEYNA_LOG_DEBUG("Exit with SUCCESS");
}

void dls_upnp_search(dls_upnp_t *upnp, dls_client_t *client,
		     dls_task_t *task,
		     dls_upnp_task_complete_t cb)
{
	gchar *upnp_filter = NULL;
	gchar *upnp_query = NULL;
	gchar *sort_by = NULL;
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_async_bas_t *cb_task_data;

	DLEYNA_LOG_DEBUG("Enter");

	DLEYNA_LOG_DEBUG("Path: %s", task->target.path);
	DLEYNA_LOG_DEBUG("Query: %s", task->ut.search.query);
	DLEYNA_LOG_DEBUG("Start: %u", task->ut.search.start);
	DLEYNA_LOG_DEBUG("Count: %u", task->ut.search.count);

	cb_data->cb = cb;
	cb_task_data = &cb_data->ut.bas;

	cb_task_data->filter_mask =
		dls_props_parse_filter(upnp->filter_map,
				       task->ut.search.filter, &upnp_filter);

	DLEYNA_LOG_DEBUG("Filter Mask 0x%"G_GUINT64_FORMAT"x",
			 cb_task_data->filter_mask);

	upnp_query = dls_search_translate_search_string(upnp->filter_map,
							task->ut.search.query);
	if (!upnp_query) {
		DLEYNA_LOG_WARNING("Query string is not valid:%s",
				   task->ut.search.query);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_BAD_QUERY,
					     "Query string is not valid.");
		goto on_error;
	}

	DLEYNA_LOG_DEBUG("UPnP Query %s", upnp_query);

	sort_by = dls_sort_translate_sort_string(upnp->filter_map,
						 task->ut.search.sort_by);
	if (!sort_by) {
		DLEYNA_LOG_WARNING("Invalid Sort Criteria");

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_BAD_QUERY,
					     "Sort Criteria are not valid");
		goto on_error;
	}

	DLEYNA_LOG_DEBUG("Sort By %s", sort_by);

	cb_task_data->protocol_info = client->protocol_info;

	dls_device_search(client, task, upnp_filter, upnp_query, sort_by);
on_error:

	if (!cb_data->action)
		(void) g_idle_add(dls_async_task_complete, cb_data);

	g_free(sort_by);
	g_free(upnp_query);
	g_free(upnp_filter);

	DLEYNA_LOG_DEBUG("Exit with %s", !cb_data->action ? "FAIL" : "SUCCESS");
}

void dls_upnp_get_resource(dls_upnp_t *upnp, dls_client_t *client,
			   dls_task_t *task,
			   dls_upnp_task_complete_t cb)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_async_get_all_t *cb_task_data;
	gchar *upnp_filter = NULL;

	DLEYNA_LOG_DEBUG("Enter");

	DLEYNA_LOG_DEBUG("Protocol Info: %s ", task->ut.resource.protocol_info);

	cb_data->cb = cb;
	cb_task_data = &cb_data->ut.get_all;

	DLEYNA_LOG_DEBUG("Root Path %s Id %s", task->target.root_path,
			 task->target.id);

	cb_task_data->filter_mask =
		dls_props_parse_filter(upnp->filter_map,
				       task->ut.resource.filter, &upnp_filter);

	DLEYNA_LOG_DEBUG("Filter Mask 0x%"G_GUINT64_FORMAT"x",
			 cb_task_data->filter_mask);

	dls_device_get_resource(client, task, upnp_filter);

	g_free(upnp_filter);

	DLEYNA_LOG_DEBUG("Exit");
}

static gboolean prv_compute_mime_and_class(dls_task_t *task,
					   dls_async_upload_t *cb_task_data,
					   GError **error)
{
	gchar *content_type = NULL;

	if (!g_file_test(task->ut.upload.file_path,
			 G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS)) {
		DLEYNA_LOG_WARNING(
			"File %s does not exist or is not a regular file",
			task->ut.upload.file_path);

		*error = g_error_new(DLEYNA_SERVER_ERROR,
				     DLEYNA_ERROR_OBJECT_NOT_FOUND,
				     "File %s does not exist or is not a regular file",
				     task->ut.upload.file_path);
		goto on_error;
	}

	content_type = g_content_type_guess(task->ut.upload.file_path, NULL, 0,
					    NULL);

	if (!content_type) {
		DLEYNA_LOG_WARNING("Unable to determine Content Type for %s",
				   task->ut.upload.file_path);

		*error = g_error_new(DLEYNA_SERVER_ERROR, DLEYNA_ERROR_BAD_MIME,
				     "Unable to determine Content Type for %s",
				     task->ut.upload.file_path);
		goto on_error;
	}

	cb_task_data->mime_type = g_content_type_get_mime_type(content_type);
	g_free(content_type);

	if (!cb_task_data->mime_type) {
		DLEYNA_LOG_WARNING("Unable to determine MIME Type for %s",
				   task->ut.upload.file_path);

		*error = g_error_new(DLEYNA_SERVER_ERROR, DLEYNA_ERROR_BAD_MIME,
				     "Unable to determine MIME Type for %s",
				     task->ut.upload.file_path);
		goto on_error;
	}

	if (g_content_type_is_a(cb_task_data->mime_type, "image/*")) {
		cb_task_data->object_class = "object.item.imageItem";
	} else if (g_content_type_is_a(cb_task_data->mime_type, "audio/*")) {
		cb_task_data->object_class = "object.item.audioItem";
	} else if (g_content_type_is_a(cb_task_data->mime_type, "video/*")) {
		cb_task_data->object_class = "object.item.videoItem";
	} else {
		DLEYNA_LOG_WARNING("Unsupported MIME Type %s",
				   cb_task_data->mime_type);

		*error = g_error_new(DLEYNA_SERVER_ERROR, DLEYNA_ERROR_BAD_MIME,
				     "Unsupported MIME Type %s",
				     cb_task_data->mime_type);
		goto on_error;
	}

	return TRUE;

on_error:

	return FALSE;
}

void dls_upnp_upload_to_any(dls_upnp_t *upnp, dls_client_t *client,
			    dls_task_t *task,
			    dls_upnp_task_complete_t cb)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_async_upload_t *cb_task_data;

	DLEYNA_LOG_DEBUG("Enter");

	cb_data->cb = cb;
	cb_task_data = &cb_data->ut.upload;

	DLEYNA_LOG_DEBUG("Root Path %s Id %s", task->target.root_path,
			 task->target.id);

	if (strcmp(task->target.id, "0")) {
		DLEYNA_LOG_WARNING("Bad path %s", task->target.path);

		cb_data->error =
			g_error_new(DLEYNA_SERVER_ERROR, DLEYNA_ERROR_BAD_PATH,
				    "UploadToAnyContainer must be executed on a root path");
		goto on_error;
	}

	if (!prv_compute_mime_and_class(task, cb_task_data, &cb_data->error))
		goto on_error;

	DLEYNA_LOG_DEBUG("MIME Type %s", cb_task_data->mime_type);
	DLEYNA_LOG_DEBUG("Object class %s", cb_task_data->object_class);

	dls_device_upload(client, task, "DLNA.ORG_AnyContainer");

on_error:

	if (!cb_data->action)
		(void) g_idle_add(dls_async_task_complete, cb_data);

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_upnp_upload(dls_upnp_t *upnp, dls_client_t *client, dls_task_t *task,
		     dls_upnp_task_complete_t cb)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_async_upload_t *cb_task_data;

	DLEYNA_LOG_DEBUG("Enter");

	cb_data->cb = cb;
	cb_task_data = &cb_data->ut.upload;

	if (!prv_compute_mime_and_class(task, cb_task_data, &cb_data->error))
		goto on_error;

	DLEYNA_LOG_DEBUG("MIME Type %s", cb_task_data->mime_type);
	DLEYNA_LOG_DEBUG("Object class %s", cb_task_data->object_class);

	dls_device_upload(client, task, task->target.id);

on_error:

	if (!cb_data->action)
		(void) g_idle_add(dls_async_task_complete, cb_data);

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_upnp_get_upload_status(dls_upnp_t *upnp, dls_task_t *task)
{
	GError *error = NULL;

	DLEYNA_LOG_DEBUG("Enter");

	DLEYNA_LOG_DEBUG("Root Path %s Id %s", task->target.root_path,
			 task->target.id);

	if (strcmp(task->target.id, "0")) {
		DLEYNA_LOG_WARNING("Bad path %s", task->target.path);

		error = g_error_new(DLEYNA_SERVER_ERROR, DLEYNA_ERROR_BAD_PATH,
				    "GetUploadStatus must be executed on a root path");
		goto on_error;
	}

	(void) dls_device_get_upload_status(task, &error);

on_error:

	if (error) {
		dls_task_fail(task, error);
		g_error_free(error);
	} else {
		dls_task_complete(task);
	}

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_upnp_get_upload_ids(dls_upnp_t *upnp, dls_task_t *task)
{
	GError *error = NULL;

	DLEYNA_LOG_DEBUG("Enter");

	DLEYNA_LOG_DEBUG("Root Path %s Id %s", task->target.root_path,
			 task->target.id);

	if (strcmp(task->target.id, "0")) {
		DLEYNA_LOG_WARNING("Bad path %s", task->target.path);

		error = g_error_new(DLEYNA_SERVER_ERROR, DLEYNA_ERROR_BAD_PATH,
				    "GetUploadIDs must be executed on a root path");
		goto on_error;
	}

	 dls_device_get_upload_ids(task);

on_error:

	if (error) {
		dls_task_fail(task, error);
		g_error_free(error);
	} else {
		dls_task_complete(task);
	}

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_upnp_cancel_upload(dls_upnp_t *upnp, dls_task_t *task)
{
	GError *error = NULL;

	DLEYNA_LOG_DEBUG("Enter");

	DLEYNA_LOG_DEBUG("Root Path %s Id %s", task->target.root_path,
			 task->target.id);

	if (strcmp(task->target.id, "0")) {
		DLEYNA_LOG_WARNING("Bad path %s", task->target.path);

		error = g_error_new(DLEYNA_SERVER_ERROR, DLEYNA_ERROR_BAD_PATH,
				    "CancelUpload must be executed on a root path");
		goto on_error;
	}

	(void) dls_device_cancel_upload(task, &error);

on_error:

	if (error) {
		dls_task_fail(task, error);
		g_error_free(error);
	} else {
		dls_task_complete(task);
	}

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_upnp_delete_object(dls_upnp_t *upnp, dls_client_t *client,
			    dls_task_t *task,
			    dls_upnp_task_complete_t cb)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;

	DLEYNA_LOG_DEBUG("Enter");

	cb_data->cb = cb;

	DLEYNA_LOG_DEBUG("Root Path %s Id %s", task->target.root_path,
			 task->target.id);

	dls_device_delete_object(client, task);

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_upnp_create_container(dls_upnp_t *upnp, dls_client_t *client,
			       dls_task_t *task,
			       dls_upnp_task_complete_t cb)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;

	DLEYNA_LOG_DEBUG("Enter");

	cb_data->cb = cb;

	DLEYNA_LOG_DEBUG("Root Path %s Id %s", task->target.root_path,
			 task->target.id);

	dls_device_create_container(client, task, task->target.id);

	if (!cb_data->action)
		(void) g_idle_add(dls_async_task_complete, cb_data);

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_upnp_create_container_in_any(dls_upnp_t *upnp, dls_client_t *client,
				      dls_task_t *task,
				      dls_upnp_task_complete_t cb)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;

	DLEYNA_LOG_DEBUG("Enter");

	cb_data->cb = cb;

	DLEYNA_LOG_DEBUG("Root Path %s Id %s", task->target.root_path,
			 task->target.id);

	if (strcmp(task->target.id, "0")) {
		DLEYNA_LOG_WARNING("Bad path %s", task->target.path);

		cb_data->error =
			g_error_new(DLEYNA_SERVER_ERROR, DLEYNA_ERROR_BAD_PATH,
				    "CreateContainerInAnyContainer must be executed on a root path");
		goto on_error;
	}

	dls_device_create_container(client, task, "DLNA.ORG_AnyContainer");

on_error:

	if (!cb_data->action)
		(void) g_idle_add(dls_async_task_complete, cb_data);

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_upnp_update_object(dls_upnp_t *upnp, dls_client_t *client,
			    dls_task_t *task,
			    dls_upnp_task_complete_t cb)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_async_update_t *cb_task_data;
	dls_upnp_prop_mask mask;
	gchar *upnp_filter = NULL;
	dls_task_update_t *task_data;

	DLEYNA_LOG_DEBUG("Enter");

	cb_data->cb = cb;
	cb_task_data = &cb_data->ut.update;
	task_data = &task->ut.update;

	DLEYNA_LOG_DEBUG("Root Path %s Id %s", task->target.root_path,
			 task->target.id);

	if (!dls_props_parse_update_filter(upnp->filter_map,
					   task_data->to_add_update,
					   task_data->to_delete,
					   &mask, &upnp_filter)) {
		DLEYNA_LOG_WARNING("Invalid Parameter");

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Invalid Parameter");
		goto on_error;
	}

	cb_task_data->map = upnp->filter_map;

	DLEYNA_LOG_DEBUG("Filter = %s", upnp_filter);
	DLEYNA_LOG_DEBUG("Mask = 0x%"G_GUINT64_FORMAT"x", mask);

	if (mask == 0) {
		DLEYNA_LOG_WARNING("Empty Parameters");

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Empty Parameters");

		goto on_error;
	}

	dls_device_update_object(client, task, upnp_filter);

on_error:

	g_free(upnp_filter);

	if (!cb_data->action)
		(void) g_idle_add(dls_async_task_complete, cb_data);

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_upnp_get_object_metadata(dls_upnp_t *upnp, dls_client_t *client,
				  dls_task_t *task, dls_upnp_task_complete_t cb)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;

	DLEYNA_LOG_DEBUG("Enter");

	cb_data->cb = cb;

	DLEYNA_LOG_DEBUG("Root Path %s Id %s", task->target.root_path,
			 task->target.id);

	dls_device_get_object_metadata(client, task, task->target.id);

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_upnp_create_reference(dls_upnp_t *upnp, dls_client_t *client,
			       dls_task_t *task,
			       dls_upnp_task_complete_t cb)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;

	DLEYNA_LOG_DEBUG("Enter");

	cb_data->cb = cb;

	DLEYNA_LOG_DEBUG("Root Path: %s - Id: %s", task->target.root_path,
			 task->target.id);

	dls_device_create_reference(client, task);

	DLEYNA_LOG_DEBUG("Exit");

	return;
}

void dls_upnp_get_icon(dls_upnp_t *upnp, dls_client_t *client,
		       dls_task_t *task,
		       dls_upnp_task_complete_t cb)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;

	DLEYNA_LOG_DEBUG("Enter");

	cb_data->cb = cb;

	dls_device_get_icon(client, task);

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_upnp_unsubscribe(dls_upnp_t *upnp)
{
	GHashTableIter iter;
	gpointer value;
	dls_device_t *device;

	DLEYNA_LOG_DEBUG("Enter");

	g_hash_table_iter_init(&iter, upnp->server_udn_map);
	while (g_hash_table_iter_next(&iter, NULL, &value)) {
		device = value;
		dls_device_unsubscribe(device);
	}

	DLEYNA_LOG_DEBUG("Exit");
}

static gboolean prv_device_uc_find(gpointer key, gpointer value,
				   gpointer user_data)
{
	prv_device_new_ct_t *priv_t = (prv_device_new_ct_t *)value;

	return (priv_t->device == user_data) ? TRUE : FALSE;
}

static gboolean prv_device_find(gpointer key, gpointer value,
				gpointer user_data)
{
	return (value == user_data) ? TRUE : FALSE;
}

gboolean dls_upnp_device_context_exist(dls_device_t *device,
				       dls_device_context_t *context)
{
	gpointer result;
	guint i;
	gboolean found = FALSE;
	dls_upnp_t *upnp = dls_server_get_upnp();

	if (upnp == NULL)
		goto on_exit;

	/* Check if the device still exist */
	result = g_hash_table_find(upnp->server_udn_map, prv_device_find,
				   device);

	if (result == NULL)
		if (g_hash_table_find(upnp->server_uc_map, prv_device_uc_find,
				      device) == NULL)
			goto on_exit;

	/* Search if the context still exist in the device */
	for (i = 0; i < device->contexts->len; ++i) {
		if (g_ptr_array_index(device->contexts, i) == context) {
			found = TRUE;
			break;
		}
	}

on_exit:
	return found;
}

void dls_upnp_rescan(dls_upnp_t *upnp)
{
	DLEYNA_LOG_DEBUG("re-scanning control points");

	gupnp_context_manager_rescan_control_points(upnp->context_manager);
}

GUPnPContextManager *dls_upnp_get_context_manager(dls_upnp_t *upnp)
{
	return upnp->context_manager;
}
