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
#include <libgupnp/gupnp-error.h>
#include <libgupnp-dlna/gupnp-dlna-profile.h>
#include <libgupnp-dlna/gupnp-dlna-profile-guesser.h>
#include <libgupnp-av/gupnp-media-collection.h>
#include <libgupnp-av/gupnp-cds-last-change-parser.h>
#include <libsoup/soup.h>

#include <libdleyna/core/error.h>
#include <libdleyna/core/log.h>
#include <libdleyna/core/service-task.h>

#include "device.h"
#include "interface.h"
#include "path.h"
#include "server.h"

#define DLS_SYSTEM_UPDATE_VAR "SystemUpdateID"
#define DLS_CONTAINER_UPDATE_VAR "ContainerUpdateIDs"
#define DLS_LAST_CHANGE_VAR "LastChange"
#define DLS_DMS_DEVICE_TYPE "urn:schemas-upnp-org:device:MediaServer:"

#define DLS_UPLOAD_STATUS_IN_PROGRESS "IN_PROGRESS"
#define DLS_UPLOAD_STATUS_CANCELLED "CANCELLED"
#define DLS_UPLOAD_STATUS_ERROR "ERROR"
#define DLS_UPLOAD_STATUS_COMPLETED "COMPLETED"

typedef gboolean(*dls_device_count_cb_t)(dls_async_task_t *cb_data,
					 gint count);

typedef struct dls_device_count_data_t_ dls_device_count_data_t;
struct dls_device_count_data_t_ {
	dls_device_count_cb_t cb;
	dls_async_task_t *cb_data;
};

typedef struct dls_device_object_builder_t_ dls_device_object_builder_t;
struct dls_device_object_builder_t_ {
	GVariantBuilder *vb;
	gchar *id;
	gboolean needs_child_count;
};

typedef struct dls_device_upload_t_ dls_device_upload_t;
struct dls_device_upload_t_ {
	SoupSession *soup_session;
	SoupMessage *msg;
	GMappedFile *mapped_file;
	gchar *body;
	gsize body_length;
	const gchar *status;
	guint64 bytes_uploaded;
	guint64 bytes_to_upload;
};

typedef struct dls_device_upload_job_t_ dls_device_upload_job_t;
struct dls_device_upload_job_t_ {
	gint upload_id;
	dls_device_t *device;
	guint remove_idle;
};

typedef struct dls_device_download_t_ dls_device_download_t;
struct dls_device_download_t_ {
	SoupSession *session;
	SoupMessage *msg;
	dls_async_task_t *task;
};

/* Private structure used in chain task */
typedef struct prv_new_device_ct_t_ prv_new_device_ct_t;
struct prv_new_device_ct_t_ {
	dls_device_t *dev;
	dleyna_connector_id_t connection;
	const dleyna_connector_dispatch_cb_t *vtable;
	GHashTable *property_map;
};

static void prv_get_child_count(dls_async_task_t *cb_data,
				dls_device_count_cb_t cb, const gchar *id);
static void prv_retrieve_child_count_for_list(dls_async_task_t *cb_data);
static void prv_container_update_cb(GUPnPServiceProxy *proxy,
				const char *variable,
				GValue *value,
				gpointer user_data);
static void prv_system_update_cb(GUPnPServiceProxy *proxy,
				const char *variable,
				GValue *value,
				gpointer user_data);
static void prv_last_change_cb(GUPnPServiceProxy *proxy,
				const char *variable,
				GValue *value,
				gpointer user_data);
static void prv_upload_delete(gpointer up);
static void prv_upload_job_delete(gpointer up);
static void prv_get_sr_token_for_props(GUPnPServiceProxy *proxy,
			     const dls_device_t *device,
			     dls_async_task_t *cb_data);

static void prv_object_builder_delete(void *dob)
{
	dls_device_object_builder_t *builder = dob;

	if (builder) {
		if (builder->vb)
			g_variant_builder_unref(builder->vb);

		g_free(builder->id);
		g_free(builder);
	}
}

static void prv_count_data_new(dls_async_task_t *cb_data,
					  dls_device_count_cb_t cb,
					  dls_device_count_data_t **count_data)
{
	dls_device_count_data_t *cd;

	cd = g_new(dls_device_count_data_t, 1);
	cd->cb = cb;
	cd->cb_data = cb_data;

	*count_data = cd;
}

static void prv_context_unsubscribe(dls_device_context_t *ctx)
{
	if (ctx->timeout_id) {
		(void) g_source_remove(ctx->timeout_id);
		ctx->timeout_id = 0;
	}

	if (ctx->subscribed) {
		gupnp_service_proxy_remove_notify(
					ctx->service_proxy,
					DLS_SYSTEM_UPDATE_VAR,
					prv_system_update_cb,
					ctx->device);
		gupnp_service_proxy_remove_notify(
					ctx->service_proxy,
					DLS_CONTAINER_UPDATE_VAR,
					prv_container_update_cb,
					ctx->device);
		gupnp_service_proxy_remove_notify(
					ctx->service_proxy,
					DLS_LAST_CHANGE_VAR,
					prv_last_change_cb,
					ctx->device);

		gupnp_service_proxy_set_subscribed(ctx->service_proxy,
						   FALSE);

		ctx->subscribed = FALSE;
	}
}

static void prv_context_delete(gpointer context)
{
	dls_device_context_t *ctx = context;

	if (ctx) {
		prv_context_unsubscribe(ctx);

		if (ctx->device_proxy)
			g_object_unref(ctx->device_proxy);

		if (ctx->service_proxy)
			g_object_unref(ctx->service_proxy);

		g_free(ctx->ip_address);
		g_free(ctx);
	}
}

static void prv_context_new(const gchar *ip_address,
				GUPnPDeviceProxy *proxy,
				dls_device_t *device,
				dls_device_context_t **context)
{
	const gchar *service_type =
		"urn:schemas-upnp-org:service:ContentDirectory";
	dls_device_context_t *ctx = g_new(dls_device_context_t, 1);

	ctx->ip_address = g_strdup(ip_address);
	ctx->device_proxy = proxy;
	ctx->device = device;
	g_object_ref(proxy);
	ctx->service_proxy = (GUPnPServiceProxy *)
		gupnp_device_info_get_service((GUPnPDeviceInfo *)proxy,
					      service_type);
	ctx->subscribed = FALSE;
	ctx->timeout_id = 0;

	*context = ctx;
}

void dls_device_delete(void *device)
{
	dls_device_t *dev = device;

	if (dev) {
		DLEYNA_LOG_DEBUG("Deleting device");

		dev->shutting_down = TRUE;
		g_hash_table_unref(dev->upload_jobs);
		g_hash_table_unref(dev->uploads);

		if (dev->timeout_id)
			(void) g_source_remove(dev->timeout_id);

		if (dev->id)
			(void) dls_server_get_connector()->unpublish_subtree(
						dev->connection, dev->id);

		g_ptr_array_unref(dev->contexts);
		g_free(dev->path);
		g_variant_unref(dev->search_caps);
		g_variant_unref(dev->sort_caps);
		g_variant_unref(dev->sort_ext_caps);
		g_variant_unref(dev->feature_list);
		g_free(dev->icon.mime_type);
		g_free(dev->icon.bytes);
		g_free(dev);
	}
}

void dls_device_unsubscribe(void *device)
{
	unsigned int i;
	dls_device_t *dev = device;
	dls_device_context_t *context;

	if (dev) {
		for (i = 0; i < dev->contexts->len; ++i) {
			context = g_ptr_array_index(dev->contexts, i);
			prv_context_unsubscribe(context);
		}
	}
}

static void prv_last_change_decode(GUPnPCDSLastChangeEntry *entry,
				   GVariantBuilder *array,
				   const char *root_path)
{
	GUPnPCDSLastChangeEvent event;
	GVariant *state;
	const char *object_id;
	const char *parent_id;
	const char *mclass;
	const char *media_class;
	char *key[] = {"ADD", "DEL", "MOD", "DONE"};
	char *parent_path;
	char *path = NULL;
	gboolean sub_update;
	guint32 update_id;

	object_id = gupnp_cds_last_change_entry_get_object_id(entry);
	if (!object_id)
		goto on_error;

	sub_update = gupnp_cds_last_change_entry_is_subtree_update(entry);
	update_id = gupnp_cds_last_change_entry_get_update_id(entry);
	path = dls_path_from_id(root_path, object_id);
	event = gupnp_cds_last_change_entry_get_event(entry);

	switch (event) {
	case GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED:
		parent_id = gupnp_cds_last_change_entry_get_parent_id(entry);
		if (!parent_id)
			goto on_error;

		mclass = gupnp_cds_last_change_entry_get_class(entry);
		if (!mclass)
			goto on_error;

		media_class = dls_props_upnp_class_to_media_spec_ex(mclass);
		if (!media_class)
			goto on_error;

		parent_path = dls_path_from_id(root_path, parent_id);
		state = g_variant_new("(oubos)", path, update_id, sub_update,
				      parent_path, media_class);
		g_free(parent_path);
		break;
	case GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_REMOVED:
	case GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED:
		state = g_variant_new("(oub)", path, update_id, sub_update);
		break;
	case GUPNP_CDS_LAST_CHANGE_EVENT_ST_DONE:
		state = g_variant_new("(ou)", path, update_id);
		break;
	case GUPNP_CDS_LAST_CHANGE_EVENT_INVALID:
	default:
		goto on_error;
		break;
	}

	g_variant_builder_add(array, "(sv)", key[event - 1], state);

on_error:

	g_free(path);
}

static void prv_last_change_cb(GUPnPServiceProxy *proxy,
			       const char *variable,
			       GValue *value,
			       gpointer user_data)
{
	const gchar *last_change;
	GVariantBuilder array;
	GVariant *val;
	dls_device_t *device = user_data;
	GUPnPCDSLastChangeParser *parser;
	GList *list;
	GList *next;
	GError *error = NULL;

	last_change = g_value_get_string(value);

	DLEYNA_LOG_DEBUG_NL();
	DLEYNA_LOG_DEBUG("LastChange XML: %s", last_change);
	DLEYNA_LOG_DEBUG_NL();

	parser = gupnp_cds_last_change_parser_new();
	list = gupnp_cds_last_change_parser_parse(parser, last_change, &error);

	if (error != NULL) {
		DLEYNA_LOG_WARNING(
			"gupnp_cds_last_change_parser_parse parsing failed: %s",
			error->message);
		goto on_error;
	}

	g_variant_builder_init(&array, G_VARIANT_TYPE("a(sv)"));
	next = list;
	while (next) {
		prv_last_change_decode(next->data, &array, device->path);
		gupnp_cds_last_change_entry_unref(next->data);
		next = g_list_next(next);
	}

	val = g_variant_new("(@a(sv))", g_variant_builder_end(&array));

	(void) dls_server_get_connector()->notify(device->connection,
					   device->path,
					   DLEYNA_SERVER_INTERFACE_MEDIA_DEVICE,
					   DLS_INTERFACE_ESV_LAST_CHANGE,
					   val,
					   NULL);

on_error:

	g_list_free(list);
	g_object_unref(parser);

	if (error != NULL)
		g_error_free(error);
}

static void prv_build_container_update_array(const gchar *root_path,
					     const gchar *value,
					     GVariantBuilder *builder)
{
	gchar **str_array;
	int pos = 0;
	gchar *path;
	guint id;

	str_array = g_strsplit(value, ",", 0);

	DLEYNA_LOG_DEBUG_NL();

	while (str_array[pos] && str_array[pos + 1]) {
		path = dls_path_from_id(root_path, str_array[pos++]);
		id = atoi(str_array[pos++]);
		g_variant_builder_add(builder, "(ou)", path, id);
		DLEYNA_LOG_DEBUG("@Id [%s] - Path [%s] - id[%d]",
				 str_array[pos-2], path, id);
		g_free(path);
	}

	DLEYNA_LOG_DEBUG_NL();

	g_strfreev(str_array);
}

static void prv_container_update_cb(GUPnPServiceProxy *proxy,
				    const char *variable,
				    GValue *value,
				    gpointer user_data)
{
	dls_device_t *device = user_data;
	GVariantBuilder array;

	g_variant_builder_init(&array, G_VARIANT_TYPE("a(ou)"));

	prv_build_container_update_array(device->path,
					 g_value_get_string(value),
					 &array);

	(void) dls_server_get_connector()->notify(
				device->connection,
				device->path,
				DLEYNA_SERVER_INTERFACE_MEDIA_DEVICE,
				DLS_INTERFACE_ESV_CONTAINER_UPDATE_IDS,
				g_variant_new("(@a(ou))",
					      g_variant_builder_end(&array)),
				NULL);
}

static void prv_system_update_cb(GUPnPServiceProxy *proxy,
				 const char *variable,
				 GValue *value,
				 gpointer user_data)
{
	GVariantBuilder *array;
	GVariant *val;
	dls_device_t *device = user_data;
	guint suid = g_value_get_uint(value);

	DLEYNA_LOG_DEBUG("System Update %u", suid);

	device->system_update_id = suid;

	array = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(array, "{sv}",
			      DLS_INTERFACE_PROP_ESV_SYSTEM_UPDATE_ID,
			      g_variant_new_uint32(suid));
	val = g_variant_new("(s@a{sv}as)", DLEYNA_SERVER_INTERFACE_MEDIA_DEVICE,
			    g_variant_builder_end(array),
			    NULL);

	(void) dls_server_get_connector()->notify(device->connection,
					   device->path,
					   DLS_INTERFACE_PROPERTIES,
					   DLS_INTERFACE_PROPERTIES_CHANGED,
					   val,
					   NULL);

	g_variant_builder_unref(array);
}

static gboolean prv_re_enable_subscription(gpointer user_data)
{
	dls_device_context_t *context = user_data;

	context->timeout_id = 0;

	return FALSE;
}

static void prv_subscription_lost_cb(GUPnPServiceProxy *proxy,
				     const GError *reason,
				     gpointer user_data)
{
	dls_device_context_t *context = user_data;

	if (!context->timeout_id) {
		gupnp_service_proxy_set_subscribed(context->service_proxy,
						   TRUE);
		context->timeout_id = g_timeout_add_seconds(
						10,
						prv_re_enable_subscription,
						context);
	} else {
		g_source_remove(context->timeout_id);
		gupnp_service_proxy_remove_notify(context->service_proxy,
						  DLS_SYSTEM_UPDATE_VAR,
						  prv_system_update_cb,
						  context->device);
		gupnp_service_proxy_remove_notify(context->service_proxy,
						  DLS_CONTAINER_UPDATE_VAR,
						  prv_container_update_cb,
						  context->device);
		gupnp_service_proxy_remove_notify(context->service_proxy,
						  DLS_LAST_CHANGE_VAR,
						  prv_last_change_cb,
						  context->device);

		context->timeout_id = 0;
		context->subscribed = FALSE;
	}
}

void dls_device_subscribe_to_contents_change(dls_device_t *device)
{
	dls_device_context_t *context;

	context = dls_device_get_context(device, NULL);

	DLEYNA_LOG_DEBUG("Subscribe for events on context: %s",
			 context->ip_address);

	gupnp_service_proxy_add_notify(context->service_proxy,
				       DLS_SYSTEM_UPDATE_VAR,
				       G_TYPE_UINT,
				       prv_system_update_cb,
				       device);

	gupnp_service_proxy_add_notify(context->service_proxy,
				       DLS_CONTAINER_UPDATE_VAR,
				       G_TYPE_STRING,
				       prv_container_update_cb,
				       device);

	gupnp_service_proxy_add_notify(context->service_proxy,
				       DLS_LAST_CHANGE_VAR,
				       G_TYPE_STRING,
				       prv_last_change_cb,
				       device);

	context->subscribed = TRUE;
	gupnp_service_proxy_set_subscribed(context->service_proxy, TRUE);

	g_signal_connect(context->service_proxy,
			 "subscription-lost",
			 G_CALLBACK(prv_subscription_lost_cb),
			 context);
}

static void prv_feature_list_add_feature(gchar *root_path,
					 GUPnPFeature *feature,
					 GVariantBuilder *vb)
{
	GVariantBuilder vbo;
	GVariant *var_obj;
	const char *name;
	const char *version;
	const char *obj_id;
	gchar **obj;
	gchar **saved;
	gchar *path;

	name = gupnp_feature_get_name(feature);
	version = gupnp_feature_get_version(feature);
	obj_id = gupnp_feature_get_object_ids(feature);

	g_variant_builder_init(&vbo, G_VARIANT_TYPE("ao"));

	if (obj_id != NULL && *obj_id) {
		obj = g_strsplit(obj_id, ",", 0);
		saved = obj;

		while (obj && *obj) {
			path = dls_path_from_id(root_path, *obj);
			g_variant_builder_add(&vbo, "o", path);
			g_free(path);
			obj++;
		}

		g_strfreev(saved);
	}

	var_obj = g_variant_builder_end(&vbo);
	g_variant_builder_add(vb, "(ss@ao)", name, version, var_obj);
}

static void prv_get_feature_list_analyze(dls_device_t *device, gchar *result)
{
	GUPnPFeatureListParser *parser;
	GUPnPFeature *feature;
	GList *list;
	GList *item;
	GError *error = NULL;
	GVariantBuilder vb;
#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
	char *str;
#endif
	parser = gupnp_feature_list_parser_new();
	list = gupnp_feature_list_parser_parse_text(parser, result, &error);

	if (error != NULL) {
		DLEYNA_LOG_WARNING("GetFeatureList parsing failed: %s",
				   error->message);
		goto on_exit;
	}

	g_variant_builder_init(&vb, G_VARIANT_TYPE("a(ssao)"));
	item = list;

	while (item != NULL) {
		feature = (GUPnPFeature *)item->data;
		prv_feature_list_add_feature(device->path, feature, &vb);
		g_object_unref(feature);
		item = g_list_next(item);
	}

	device->feature_list = g_variant_ref_sink(g_variant_builder_end(&vb));

#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
	str = g_variant_print(device->feature_list, FALSE);
	DLEYNA_LOG_DEBUG("%s = %s", DLS_INTERFACE_PROP_SV_FEATURE_LIST, str);
	g_free(str);
#endif

on_exit:
	g_list_free(list);
	g_object_unref(parser);

	if (error != NULL)
		g_error_free(error);
}

static void prv_get_feature_list_cb(GUPnPServiceProxy *proxy,
				    GUPnPServiceProxyAction *action,
				    gpointer user_data)
{
	gchar *result = NULL;
	gboolean end;
	GError *error = NULL;
	prv_new_device_ct_t *priv_t = (prv_new_device_ct_t *)user_data;

	priv_t->dev->construct_step++;
	end = gupnp_service_proxy_end_action(proxy, action, &error,
					     "FeatureList", G_TYPE_STRING,
					     &result, NULL);

	if (!end || (result == NULL)) {
		DLEYNA_LOG_WARNING("GetFeatureList operation failed: %s",
				   ((error != NULL) ? error->message
						    : "Invalid result"));
		goto on_error;
	}

	DLEYNA_LOG_DEBUG("GetFeatureList result: %s", result);

	prv_get_feature_list_analyze(priv_t->dev, result);

on_error:
	if (error != NULL)
		g_error_free(error);

	g_free(result);
}

static GUPnPServiceProxyAction *prv_get_feature_list(
						dleyna_service_task_t *task,
						GUPnPServiceProxy *proxy,
						gboolean *failed)
{
	*failed = FALSE;

	return gupnp_service_proxy_begin_action(
					proxy, "GetFeatureList",
					dleyna_service_task_begin_action_cb,
					task, NULL);
}

static void prv_get_sort_ext_capabilities_analyze(dls_device_t *device,
						  gchar *result)
{
	gchar **caps;
	gchar **saved;
#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
	gchar *props;
#endif
	GVariantBuilder sort_ext_caps_vb;

	g_variant_builder_init(&sort_ext_caps_vb, G_VARIANT_TYPE("as"));

	caps = g_strsplit(result, ",", 0);
	saved = caps;

	while (caps && *caps) {
		g_variant_builder_add(&sort_ext_caps_vb, "s", *caps);
		caps++;
	}

	g_strfreev(saved);

	device->sort_ext_caps = g_variant_ref_sink(g_variant_builder_end(
							&sort_ext_caps_vb));

#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
	props = g_variant_print(device->sort_ext_caps, FALSE);
	DLEYNA_LOG_DEBUG("%s = %s",
			 DLS_INTERFACE_PROP_SV_SORT_EXT_CAPABILITIES, props);
	g_free(props);
#endif
}

static void prv_get_sort_ext_capabilities_cb(GUPnPServiceProxy *proxy,
					   GUPnPServiceProxyAction *action,
					   gpointer user_data)
{
	gchar *result = NULL;
	gboolean end;
	GError *error = NULL;
	prv_new_device_ct_t *priv_t = (prv_new_device_ct_t *)user_data;

	priv_t->dev->construct_step++;
	end = gupnp_service_proxy_end_action(proxy, action, &error,
					     "SortExtensionCaps",
					     G_TYPE_STRING, &result, NULL);

	if (!end || (result == NULL)) {
		DLEYNA_LOG_WARNING(
			"GetSortExtensionCapabilities operation failed: %s",
			((error != NULL) ? error->message : "Invalid result"));
		goto on_error;
	}

	DLEYNA_LOG_DEBUG("GetSortExtensionCapabilities result: %s", result);

	prv_get_sort_ext_capabilities_analyze(priv_t->dev, result);

on_error:

	if (error)
		g_error_free(error);

	g_free(result);
}

static GUPnPServiceProxyAction *prv_get_sort_ext_capabilities(
						dleyna_service_task_t *task,
						GUPnPServiceProxy *proxy,
						gboolean *failed)
{
	*failed = FALSE;

	return gupnp_service_proxy_begin_action(
					proxy,
					"GetSortExtensionCapabilities",
					dleyna_service_task_begin_action_cb,
					task, NULL);
}

static void prv_get_capabilities_analyze(GHashTable *property_map,
					 gchar *result,
					 GVariant **variant)
{
	gchar **caps;
	gchar **saved;
	gchar *prop_name;
	GVariantBuilder caps_vb;

	g_variant_builder_init(&caps_vb, G_VARIANT_TYPE("as"));

	if (!strcmp(result, "*")) {
		g_variant_builder_add(&caps_vb, "s", "*");
	} else {
		caps = g_strsplit(result, ",", 0);
		saved = caps;

		while (caps && *caps) {
			prop_name = g_hash_table_lookup(property_map, *caps);

			if (prop_name) {
				g_variant_builder_add(&caps_vb, "s", prop_name);

				/* TODO: Okay this is not very nice.  A better
				   way to fix this would be to change the p_map
				   to be many : many. */

				if (!strcmp(*caps, "upnp:class"))
					g_variant_builder_add(
						&caps_vb, "s",
						DLS_INTERFACE_PROP_TYPE_EX);
			}

			caps++;
		}

		g_strfreev(saved);
	}

	*variant = g_variant_ref_sink(g_variant_builder_end(&caps_vb));

#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
	prop_name = g_variant_print(*variant, FALSE);
	DLEYNA_LOG_DEBUG("%s = %s", "   Variant", prop_name);
	g_free(prop_name);
#endif
}

static void prv_get_sort_capabilities_cb(GUPnPServiceProxy *proxy,
					 GUPnPServiceProxyAction *action,
					 gpointer user_data)
{
	gchar *result = NULL;
	gboolean end;
	GError *error = NULL;
	prv_new_device_ct_t *priv_t = (prv_new_device_ct_t *)user_data;

	priv_t->dev->construct_step++;
	end = gupnp_service_proxy_end_action(proxy, action, &error, "SortCaps",
					     G_TYPE_STRING, &result, NULL);

	if (!end || (result == NULL)) {
		DLEYNA_LOG_WARNING("GetSortCapabilities operation failed: %s",
				   ((error != NULL) ? error->message
						    : "Invalid result"));
		goto on_error;
	}

	DLEYNA_LOG_DEBUG("GetSortCapabilities result: %s", result);

	prv_get_capabilities_analyze(priv_t->property_map, result,
				     &priv_t->dev->sort_caps);

on_error:

	if (error)
		g_error_free(error);

	g_free(result);
}

static GUPnPServiceProxyAction *prv_get_sort_capabilities(
					dleyna_service_task_t *task,
					GUPnPServiceProxy *proxy,
					gboolean *failed)
{
	*failed = FALSE;

	return gupnp_service_proxy_begin_action(
					proxy,
					"GetSortCapabilities",
					dleyna_service_task_begin_action_cb,
					task, NULL);
}

static void prv_get_search_capabilities_cb(GUPnPServiceProxy *proxy,
					   GUPnPServiceProxyAction *action,
					   gpointer user_data)
{
	gchar *result = NULL;
	gboolean end;
	GError *error = NULL;
	prv_new_device_ct_t *priv_t = (prv_new_device_ct_t *)user_data;

	priv_t->dev->construct_step++;
	end = gupnp_service_proxy_end_action(proxy, action, &error,
					     "SearchCaps", G_TYPE_STRING,
					     &result, NULL);

	if (!end || (result == NULL)) {
		DLEYNA_LOG_WARNING("GetSearchCapabilities operation failed: %s",
				   ((error != NULL) ? error->message
						    : "Invalid result"));
		goto on_error;
	}

	DLEYNA_LOG_DEBUG("GetSearchCapabilities result: %s", result);

	prv_get_capabilities_analyze(priv_t->property_map, result,
				     &priv_t->dev->search_caps);

on_error:

	if (error)
		g_error_free(error);

	g_free(result);
}

static GUPnPServiceProxyAction *prv_get_search_capabilities(
					dleyna_service_task_t *task,
					GUPnPServiceProxy *proxy,
					gboolean *failed)
{
	*failed = FALSE;

	return gupnp_service_proxy_begin_action(
					proxy, "GetSearchCapabilities",
					dleyna_service_task_begin_action_cb,
					task, NULL);
}

static GUPnPServiceProxyAction *prv_subscribe(dleyna_service_task_t *task,
					      GUPnPServiceProxy *proxy,
					      gboolean *failed)
{
	dls_device_t *device;

	device = (dls_device_t *)dleyna_service_task_get_user_data(task);

	device->construct_step++;
	dls_device_subscribe_to_contents_change(device);

	*failed = FALSE;

	return NULL;
}

static gboolean prv_subtree_interface_filter(const gchar *object_path,
					     const gchar *node,
					     const gchar *interface)
{
	gboolean root_object = FALSE;
	const gchar *slash;
	gboolean retval = TRUE;

	/* All objects in the hierarchy support the same interface.  Strictly
	   speaking this is not correct as it will allow ListChildren to be
	   executed on a mediaitem object.  However, returning the correct
	   interface here would be too inefficient.  We would need to either
	   cache the type of all objects encountered so far or issue a UPnP
	   request here to determine the objects type.  Best to let the client
	   call ListChildren on a item.  This will lead to an error when we
	   execute the UPnP command and we can return an error then.

	   We do know however that the root objects are containers.  Therefore
	   we can remove the MediaItem2 interface from the root containers.  We
	   also know that only the root objects suport the MediaDevice
	   interface.
	*/

	if (dls_path_get_non_root_id(object_path, &slash))
		root_object = !slash;

	if (root_object && !strcmp(interface, DLS_INTERFACE_MEDIA_ITEM))
		retval = FALSE;
	else if (!root_object && !strcmp(interface,
					 DLEYNA_SERVER_INTERFACE_MEDIA_DEVICE))
		retval = FALSE;

	return retval;
}

static GUPnPServiceProxyAction *prv_declare(dleyna_service_task_t *task,
					    GUPnPServiceProxy *proxy,
					    gboolean *failed)
{
	guint id;
	dls_device_t *device;
	prv_new_device_ct_t *priv_t;

	priv_t = (prv_new_device_ct_t *)dleyna_service_task_get_user_data(task);
	device = priv_t->dev;
	device->construct_step++;

	id = dls_server_get_connector()->publish_subtree(priv_t->connection,
						  device->path,
						  priv_t->vtable,
						  DLS_INTERFACE_INFO_MAX,
						  prv_subtree_interface_filter);
	if (id) {
		device->id = id;

		device->uploads = g_hash_table_new_full(
						g_int_hash,
						g_int_equal,
						g_free,
						prv_upload_delete);
		device->upload_jobs = g_hash_table_new_full(
						g_int_hash,
						g_int_equal,
						g_free,
						prv_upload_job_delete);

	} else {
		DLEYNA_LOG_WARNING("dleyna_connector_publish_subtree FAILED");
	}

	*failed = (!id);

	return NULL;
}

void dls_device_construct(
			dls_device_t *dev,
			dls_device_context_t *context,
			dleyna_connector_id_t connection,
			const dleyna_connector_dispatch_cb_t *dispatch_table,
			GHashTable *property_map,
			const dleyna_task_queue_key_t *queue_id)
{
	prv_new_device_ct_t *priv_t;
	GUPnPServiceProxy *s_proxy;

	DLEYNA_LOG_DEBUG("Current step: %d", dev->construct_step);

	priv_t = g_new0(prv_new_device_ct_t, 1);

	priv_t->dev = dev;
	priv_t->connection = connection;
	priv_t->vtable = dispatch_table;
	priv_t->property_map = property_map;

	s_proxy = context->service_proxy;

	if (dev->construct_step < 1)
		dleyna_service_task_add(queue_id, prv_get_search_capabilities,
					s_proxy,
					prv_get_search_capabilities_cb, NULL,
					priv_t);

	if (dev->construct_step < 2)
		dleyna_service_task_add(queue_id, prv_get_sort_capabilities,
					s_proxy,
					prv_get_sort_capabilities_cb, NULL,
					priv_t);

	if (dev->construct_step < 3)
		dleyna_service_task_add(queue_id, prv_get_sort_ext_capabilities,
					s_proxy,
					prv_get_sort_ext_capabilities_cb, NULL,
					priv_t);

	if (dev->construct_step < 4)
		dleyna_service_task_add(queue_id, prv_get_feature_list, s_proxy,
					prv_get_feature_list_cb, NULL, priv_t);

	/* The following task should always be completed */
	dleyna_service_task_add(queue_id, prv_subscribe, s_proxy,
				NULL, NULL, dev);

	if (dev->construct_step < 6)
		dleyna_service_task_add(queue_id, prv_declare, s_proxy,
					NULL, g_free, priv_t);

	dleyna_task_queue_start(queue_id);
}

dls_device_t *dls_device_new(
			dleyna_connector_id_t connection,
			GUPnPDeviceProxy *proxy,
			const gchar *ip_address,
			const dleyna_connector_dispatch_cb_t *dispatch_table,
			GHashTable *property_map,
			guint counter,
			const dleyna_task_queue_key_t *queue_id)
{
	dls_device_t *dev;
	gchar *new_path;
	dls_device_context_t *context;

	DLEYNA_LOG_DEBUG("New Device on %s", ip_address);

	new_path = g_strdup_printf("%s/%u", DLEYNA_SERVER_PATH, counter);
	DLEYNA_LOG_DEBUG("Server Path %s", new_path);

	dev = g_new0(dls_device_t, 1);

	dev->connection = connection;
	dev->contexts = g_ptr_array_new_with_free_func(prv_context_delete);
	dev->path = new_path;

	context = dls_device_append_new_context(dev, ip_address, proxy);

	dls_device_construct(dev, context, connection, dispatch_table,
			     property_map, queue_id);

	return dev;
}

dls_device_context_t *dls_device_append_new_context(dls_device_t *device,
						    const gchar *ip_address,
						    GUPnPDeviceProxy *proxy)
{
	dls_device_context_t *context;

	prv_context_new(ip_address, proxy, device, &context);
	g_ptr_array_add(device->contexts, context);

	return context;
}

dls_device_t *dls_device_from_path(const gchar *path, GHashTable *device_list)
{
	GHashTableIter iter;
	gpointer value;
	dls_device_t *device;
	dls_device_t *retval = NULL;

	g_hash_table_iter_init(&iter, device_list);

	while (g_hash_table_iter_next(&iter, NULL, &value)) {
		device = value;

		if (!strcmp(device->path, path)) {
			retval = device;
			break;
		}
	}

	return retval;
}

dls_device_context_t *dls_device_get_context(const dls_device_t *device,
					     dls_client_t *client)
{
	dls_device_context_t *context;
	unsigned int i;
	const char ip4_local_prefix[] = "127.0.0.";
	gboolean prefer_local;
	gboolean is_local;

	prefer_local = (client && client->prefer_local_addresses);

	for (i = 0; i < device->contexts->len; ++i) {
		context = g_ptr_array_index(device->contexts, i);

		is_local = (!strncmp(context->ip_address, ip4_local_prefix,
						sizeof(ip4_local_prefix) - 1) ||
			    !strcmp(context->ip_address, "::1") ||
			    !strcmp(context->ip_address, "0:0:0:0:0:0:0:1"));

		if (prefer_local == is_local)
			break;
	}

	if (i == device->contexts->len)
		context = g_ptr_array_index(device->contexts, 0);

	return context;
}

static void prv_found_child(GUPnPDIDLLiteParser *parser,
			    GUPnPDIDLLiteObject *object,
			    gpointer user_data)
{
	dls_async_task_t *cb_data = user_data;
	dls_task_t *task = &cb_data->task;
	dls_task_get_children_t *task_data = &task->ut.get_children;
	dls_async_bas_t *cb_task_data = &cb_data->ut.bas;
	dls_device_object_builder_t *builder;
	gboolean have_child_count;

	DLEYNA_LOG_DEBUG("Enter");

	builder = g_new0(dls_device_object_builder_t, 1);

	if (GUPNP_IS_DIDL_LITE_CONTAINER(object)) {
		if (!task_data->containers)
			goto on_error;
	} else {
		if (!task_data->items)
			goto on_error;
	}

	builder->vb = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));

	if (!dls_props_add_object(builder->vb, object, task->target.root_path,
				  task->target.path, cb_task_data->filter_mask))
		goto on_error;

	if (GUPNP_IS_DIDL_LITE_CONTAINER(object)) {
		dls_props_add_container(builder->vb,
					(GUPnPDIDLLiteContainer *)object,
					cb_task_data->filter_mask,
					cb_task_data->protocol_info,
					&have_child_count);

		if (!have_child_count && (cb_task_data->filter_mask &
					  DLS_UPNP_MASK_PROP_CHILD_COUNT)) {
			builder->needs_child_count = TRUE;
			builder->id = g_strdup(
				gupnp_didl_lite_object_get_id(object));
			cb_task_data->need_child_count = TRUE;
		}
	} else {
		dls_props_add_item(builder->vb, object,
				   task->target.root_path,
				   cb_task_data->filter_mask,
				   cb_task_data->protocol_info);
	}

	g_ptr_array_add(cb_task_data->vbs, builder);

	DLEYNA_LOG_DEBUG("Exit with SUCCESS");

	return;

on_error:

	prv_object_builder_delete(builder);

	DLEYNA_LOG_DEBUG("Exit with FAIL");
}

static GVariant *prv_children_result_to_variant(dls_async_task_t *cb_data)
{
	guint i;
	dls_device_object_builder_t *builder;
	dls_async_bas_t *cb_task_data = &cb_data->ut.bas;
	GVariantBuilder vb;

	g_variant_builder_init(&vb, G_VARIANT_TYPE("aa{sv}"));

	for (i = 0; i < cb_task_data->vbs->len; ++i) {
		builder = g_ptr_array_index(cb_task_data->vbs, i);
		g_variant_builder_add(&vb, "@a{sv}",
				      g_variant_builder_end(builder->vb));
	}

	return  g_variant_builder_end(&vb);
}

static void prv_get_search_ex_result(dls_async_task_t *cb_data)
{
	GVariant *out_params[2];
	dls_async_bas_t *cb_task_data = &cb_data->ut.bas;

	out_params[0] = prv_children_result_to_variant(cb_data);
	out_params[1] = g_variant_new_uint32(cb_task_data->max_count);

	cb_data->task.result = g_variant_ref_sink(
					g_variant_new_tuple(out_params, 2));
}

static void prv_get_children_result(dls_async_task_t *cb_data)
{
	GVariant *retval = prv_children_result_to_variant(cb_data);
	cb_data->task.result =  g_variant_ref_sink(retval);
}

static gboolean prv_child_count_for_list_cb(dls_async_task_t *cb_data,
					    gint count)
{
	dls_async_bas_t *cb_task_data = &cb_data->ut.bas;
	dls_device_object_builder_t *builder;

	builder = g_ptr_array_index(cb_task_data->vbs, cb_task_data->retrieved);
	dls_props_add_child_count(builder->vb, count);
	cb_task_data->retrieved++;
	prv_retrieve_child_count_for_list(cb_data);

	return cb_task_data->retrieved >= cb_task_data->vbs->len;
}

static void prv_retrieve_child_count_for_list(dls_async_task_t *cb_data)
{
	dls_async_bas_t *cb_task_data = &cb_data->ut.bas;
	dls_device_object_builder_t *builder;
	guint i;

	for (i = cb_task_data->retrieved; i < cb_task_data->vbs->len; ++i) {
		builder = g_ptr_array_index(cb_task_data->vbs, i);

		if (builder->needs_child_count)
			break;
	}

	cb_task_data->retrieved = i;

	if (i < cb_task_data->vbs->len)
		prv_get_child_count(cb_data, prv_child_count_for_list_cb,
				    builder->id);
	else
		cb_task_data->get_children_cb(cb_data);
}

static void prv_get_children_cb(GUPnPServiceProxy *proxy,
				GUPnPServiceProxyAction *action,
				gpointer user_data)
{
	gchar *result = NULL;
	const gchar *message;
	gboolean end;
	GUPnPDIDLLiteParser *parser = NULL;
	GError *error = NULL;
	dls_async_task_t *cb_data = user_data;
	dls_async_bas_t *cb_task_data = &cb_data->ut.bas;

	DLEYNA_LOG_DEBUG("Enter");

	end = gupnp_service_proxy_end_action(cb_data->proxy, cb_data->action,
					     &error, "Result",
					     G_TYPE_STRING, &result, NULL);

	if (!end || (result == NULL)) {
		message = (error != NULL) ? error->message : "Invalid result";
		DLEYNA_LOG_WARNING("Browse operation failed: %s", message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Browse operation failed: %s",
					     message);
		goto on_error;
	}

	DLEYNA_LOG_DEBUG("GetChildren result: %s", result);

	parser = gupnp_didl_lite_parser_new();

	g_signal_connect(parser, "object-available" ,
			 G_CALLBACK(prv_found_child), cb_data);

	cb_task_data->vbs = g_ptr_array_new_with_free_func(
		prv_object_builder_delete);

	if (!gupnp_didl_lite_parser_parse_didl(parser, result, &error) &&
	    error->code != GUPNP_XML_ERROR_EMPTY_NODE) {
		DLEYNA_LOG_WARNING("Unable to parse results of browse: %s",
				   error->message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Unable to parse results of browse: %s",
					     error->message);
		goto on_error;
	}

	if (cb_task_data->need_child_count) {
		DLEYNA_LOG_DEBUG("Need to retrieve ChildCounts");

		cb_task_data->get_children_cb = prv_get_children_result;
		prv_retrieve_child_count_for_list(cb_data);
		goto no_complete;
	} else {
		prv_get_children_result(cb_data);
	}

on_error:

	(void) g_idle_add(dls_async_task_complete, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);

no_complete:

	if (error)
		g_error_free(error);

	if (parser)
		g_object_unref(parser);

	g_free(result);

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_device_get_children(dls_client_t *client,
			     dls_task_t *task,
			     const gchar *upnp_filter, const gchar *sort_by)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_device_context_t *context;

	DLEYNA_LOG_DEBUG("Enter");

	context = dls_device_get_context(task->target.device, client);

	cb_data->action =
		gupnp_service_proxy_begin_action(context->service_proxy,
						 "Browse",
						 prv_get_children_cb,
						 cb_data,
						 "ObjectID", G_TYPE_STRING,
						 task->target.id,

						 "BrowseFlag", G_TYPE_STRING,
						 "BrowseDirectChildren",

						 "Filter", G_TYPE_STRING,
						 upnp_filter,

						 "StartingIndex", G_TYPE_INT,
						 task->ut.get_children.start,
						 "RequestedCount", G_TYPE_INT,
						 task->ut.get_children.count,
						 "SortCriteria", G_TYPE_STRING,
						 sort_by,
						 NULL);

	cb_data->proxy = context->service_proxy;

	g_object_add_weak_pointer((G_OBJECT(context->service_proxy)),
				  (gpointer *)&cb_data->proxy);

	cb_data->cancel_id = g_cancellable_connect(
					cb_data->cancellable,
					G_CALLBACK(dls_async_task_cancelled_cb),
					cb_data, NULL);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_get_item(GUPnPDIDLLiteParser *parser,
			 GUPnPDIDLLiteObject *object,
			 gpointer user_data)
{
	dls_async_task_t *cb_data = user_data;
	dls_async_get_all_t *cb_task_data = &cb_data->ut.get_all;

	if (!GUPNP_IS_DIDL_LITE_CONTAINER(object))
		dls_props_add_item(cb_task_data->vb, object,
				   cb_data->task.target.root_path,
				   DLS_UPNP_MASK_ALL_PROPS,
				   cb_task_data->protocol_info);
	else
		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_UNKNOWN_INTERFACE,
					     "Interface not supported on container.");
}

static void prv_get_container(GUPnPDIDLLiteParser *parser,
		       GUPnPDIDLLiteObject *object,
		       gpointer user_data)
{
	dls_async_task_t *cb_data = user_data;
	dls_async_get_all_t *cb_task_data = &cb_data->ut.get_all;
	gboolean have_child_count;

	if (GUPNP_IS_DIDL_LITE_CONTAINER(object)) {
		dls_props_add_container(cb_task_data->vb,
					(GUPnPDIDLLiteContainer *)object,
					DLS_UPNP_MASK_ALL_PROPS,
					cb_task_data->protocol_info,
					&have_child_count);
		if (!have_child_count)
			cb_task_data->need_child_count = TRUE;
	} else {
		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_UNKNOWN_INTERFACE,
					     "Interface not supported on item.");
	}
}

static void prv_get_object(GUPnPDIDLLiteParser *parser,
			   GUPnPDIDLLiteObject *object,
			   gpointer user_data)
{
	dls_async_task_t *cb_data = user_data;
	dls_async_get_all_t *cb_task_data = &cb_data->ut.get_all;
	const char *object_id;
	const char *parent_id;
	const char *parent_path;
	gchar *path = NULL;

	object_id = gupnp_didl_lite_object_get_id(object);
	if (!object_id)
		goto on_error;

	parent_id = gupnp_didl_lite_object_get_parent_id(object);
	if (!parent_id)
		goto on_error;

	if (!strcmp(object_id, "0") || !strcmp(parent_id, "-1")) {
		parent_path = cb_data->task.target.root_path;
	} else {
		path = dls_path_from_id(cb_data->task.target.root_path,
					parent_id);
		parent_path = path;
	}

	if (!dls_props_add_object(cb_task_data->vb, object,
				  cb_data->task.target.root_path,
				  parent_path, DLS_UPNP_MASK_ALL_PROPS))
		goto on_error;

	g_free(path);

	return;

on_error:

	cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
				     DLEYNA_ERROR_BAD_RESULT,
				     "Unable to retrieve mandatory object properties");
	g_free(path);
}

static void prv_get_all(GUPnPDIDLLiteParser *parser,
			GUPnPDIDLLiteObject *object,
			gpointer user_data)
{
	dls_async_task_t *cb_data = user_data;
	dls_async_get_all_t *cb_task_data = &cb_data->ut.get_all;
	gboolean have_child_count;

	prv_get_object(parser, object, user_data);

	if (!cb_data->error) {
		if (GUPNP_IS_DIDL_LITE_CONTAINER(object)) {
			dls_props_add_container(
				cb_task_data->vb,
				(GUPnPDIDLLiteContainer *)
				object, DLS_UPNP_MASK_ALL_PROPS,
				cb_task_data->protocol_info,
				&have_child_count);
			if (!have_child_count)
				cb_task_data->need_child_count = TRUE;
		} else {
			dls_props_add_item(cb_task_data->vb,
					   object,
					   cb_data->task.target.root_path,
					   DLS_UPNP_MASK_ALL_PROPS,
					   cb_task_data->protocol_info);
		}
	}
}

static gboolean prv_subscribed(const dls_device_t *device)
{
	dls_device_context_t *context;
	unsigned int i;
	gboolean subscribed = FALSE;

	for (i = 0; i < device->contexts->len; ++i) {
		context = g_ptr_array_index(device->contexts, i);
		if (context->subscribed) {
			subscribed = TRUE;
			break;
		}
	}

	return subscribed;
}

static void prv_system_update_id_for_prop_cb(GUPnPServiceProxy *proxy,
				    GUPnPServiceProxyAction *action,
				    gpointer user_data)
{
	GError *error = NULL;
	const gchar *message;
	gboolean end;
	guint id = G_MAXUINT32;
	dls_async_task_t *cb_data = user_data;

	DLEYNA_LOG_DEBUG("Enter");

	end = gupnp_service_proxy_end_action(proxy, action, &error,
					     "Id", G_TYPE_UINT, &id, NULL);

	if (!end || (id == G_MAXUINT32)) {
		message = (error != NULL) ? error->message : "Invalid result";
		DLEYNA_LOG_WARNING("Unable to retrieve SystemUpdateID: %s",
				   message);

		cb_data->error = g_error_new(
					DLEYNA_SERVER_ERROR,
					DLEYNA_ERROR_OPERATION_FAILED,
					"Unable to retrieve SystemUpdateID: %s",
					message);

		goto on_complete;
	}

	cb_data->task.result = g_variant_ref_sink(g_variant_new_uint32(id));

on_complete:

	(void) g_idle_add(dls_async_task_complete, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);

	if (error)
		g_error_free(error);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_get_system_update_id_for_prop(GUPnPServiceProxy *proxy,
				     const dls_device_t *device,
				     dls_async_task_t *cb_data)
{
	guint suid;

	DLEYNA_LOG_DEBUG("Enter");

	if (prv_subscribed(device)) {
		suid = device->system_update_id;

		cb_data->task.result = g_variant_ref_sink(
						g_variant_new_uint32(suid));

		(void) g_idle_add(dls_async_task_complete, cb_data);

		goto on_complete;
	}

	cb_data->action = gupnp_service_proxy_begin_action(
					proxy, "GetSystemUpdateID",
					prv_system_update_id_for_prop_cb,
					cb_data,
					NULL);

	cb_data->proxy = proxy;
	g_object_add_weak_pointer((G_OBJECT(proxy)),
				  (gpointer *)&cb_data->proxy);

	cb_data->cancel_id = g_cancellable_connect(
					cb_data->cancellable,
					G_CALLBACK(dls_async_task_cancelled_cb),
					cb_data, NULL);

on_complete:

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_system_update_id_for_props_cb(GUPnPServiceProxy *proxy,
				    GUPnPServiceProxyAction *action,
				    gpointer user_data)
{
	GError *error = NULL;
	const gchar *message;
	gboolean end;
	guint id = G_MAXUINT32;
	dls_async_task_t *cb_data = user_data;
	dls_async_get_all_t *cb_task_data = &cb_data->ut.get_all;

	DLEYNA_LOG_DEBUG("Enter");

	end = gupnp_service_proxy_end_action(proxy, action, &error,
					    "Id", G_TYPE_UINT, &id, NULL);

	if (!end || (id == G_MAXUINT32)) {
		message = (error != NULL) ? error->message : "Invalid result";
		DLEYNA_LOG_WARNING("Unable to retrieve SystemUpdateID: %s",
				   message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Unable to retrieve SystemUpdateID: %s",
					     message);

		goto on_complete;
	}

	g_variant_builder_add(cb_task_data->vb, "{sv}",
			      DLS_SYSTEM_UPDATE_VAR,
			      g_variant_new_uint32(id));

	cb_data->task.result = g_variant_ref_sink(g_variant_builder_end(
							cb_task_data->vb));

on_complete:

	if (!cb_data->error)
		prv_get_sr_token_for_props(proxy, cb_data->task.target.device,
					   cb_data);
	else {
		(void) g_idle_add(dls_async_task_complete, cb_data);
		g_cancellable_disconnect(cb_data->cancellable,
					 cb_data->cancel_id);
	}

	if (error)
		g_error_free(error);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_get_system_update_id_for_props(GUPnPServiceProxy *proxy,
				     const dls_device_t *device,
				     dls_async_task_t *cb_data)
{
	dls_async_get_all_t *cb_task_data;
	guint suid;

	DLEYNA_LOG_DEBUG("Enter");

	if (prv_subscribed(device)) {
		suid = device->system_update_id;

		cb_task_data = &cb_data->ut.get_all;

		g_variant_builder_add(cb_task_data->vb, "{sv}",
				      DLS_SYSTEM_UPDATE_VAR,
				      g_variant_new_uint32(suid));

		prv_get_sr_token_for_props(proxy, device, cb_data);

		goto on_complete;
	}

	cb_data->action = gupnp_service_proxy_begin_action(
					proxy, "GetSystemUpdateID",
					prv_system_update_id_for_props_cb,
					cb_data,
					NULL);

	if (cb_data->proxy != NULL)
		g_object_remove_weak_pointer((G_OBJECT(cb_data->proxy)),
					     (gpointer *)&cb_data->proxy);

	cb_data->proxy = proxy;
	g_object_add_weak_pointer((G_OBJECT(proxy)),
				  (gpointer *)&cb_data->proxy);

	if (!cb_data->cancel_id)
		cb_data->cancel_id = g_cancellable_connect(
					cb_data->cancellable,
					G_CALLBACK(dls_async_task_cancelled_cb),
					cb_data, NULL);

on_complete:

	DLEYNA_LOG_DEBUG("Exit");
}

static int prv_get_media_server_version(const dls_device_t *device)
{
	dls_device_context_t *context;
	const char *device_type;
	const char *version;

	context = dls_device_get_context(device, NULL);
	device_type = gupnp_device_info_get_device_type((GUPnPDeviceInfo *)
							context->device_proxy);

	if (!device_type || !g_str_has_prefix(device_type, DLS_DMS_DEVICE_TYPE))
		goto on_error;

	version = device_type + sizeof(DLS_DMS_DEVICE_TYPE) - 1;

	return atoi(version);

on_error:

	return -1;
}

static void prv_service_reset_for_prop_cb(GUPnPServiceProxy *proxy,
					  GUPnPServiceProxyAction *action,
					  gpointer user_data)
{
	GError *error = NULL;
	const gchar *message;
	gchar *token = NULL;
	gboolean end;
	dls_async_task_t *cb_data = user_data;

	DLEYNA_LOG_DEBUG("Enter");

	end = gupnp_service_proxy_end_action(proxy, action, &error,
					     "ResetToken", G_TYPE_STRING,
					     &token, NULL);

	if (!end || (token == NULL)) {
		message = (error != NULL) ? error->message : "Invalid result";
		DLEYNA_LOG_WARNING("Unable to retrieve ServiceResetToken: %s",
				   message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "GetServiceResetToken failed: %s",
					     message);

		goto on_complete;
	}

	cb_data->task.result = g_variant_ref_sink(g_variant_new_string(token));

	DLEYNA_LOG_DEBUG("Service Reset %s", token);

	g_free(token);

on_complete:

	(void) g_idle_add(dls_async_task_complete, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);

	if (error)
		g_error_free(error);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_get_sr_token_for_prop(GUPnPServiceProxy *proxy,
			     const dls_device_t *device,
			     dls_async_task_t *cb_data)
{
	DLEYNA_LOG_DEBUG("Enter");

	if (prv_get_media_server_version(device) < 3) {
		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_UNKNOWN_PROPERTY,
					     "Unknown property");

		(void) g_idle_add(dls_async_task_complete, cb_data);

		goto on_error;
	}

	cb_data->action = gupnp_service_proxy_begin_action(
					proxy, "GetServiceResetToken",
					prv_service_reset_for_prop_cb,
					cb_data,
					NULL);

	cb_data->proxy = proxy;
	g_object_add_weak_pointer((G_OBJECT(proxy)),
				  (gpointer *)&cb_data->proxy);

	cb_data->cancel_id = g_cancellable_connect(
					cb_data->cancellable,
					G_CALLBACK(dls_async_task_cancelled_cb),
					cb_data, NULL);

on_error:

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_service_reset_for_props_cb(GUPnPServiceProxy *proxy,
					  GUPnPServiceProxyAction *action,
					  gpointer user_data)
{
	GError *error = NULL;
	const gchar *message;
	gchar *token = NULL;
	gboolean end;
	dls_async_task_t *cb_data = user_data;
	dls_async_get_all_t *cb_task_data;

	DLEYNA_LOG_DEBUG("Enter");

	end = gupnp_service_proxy_end_action(proxy, action, &error,
					    "ResetToken", G_TYPE_STRING,
					    &token, NULL);

	if (!end || (token == NULL)) {
		message = (error != NULL) ? error->message : "Invalid result";
		DLEYNA_LOG_WARNING("Unable to retrieve ServiceResetToken: %s",
				   message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "GetServiceResetToken failed: %s",
					     message);
		goto on_complete;
	}

	cb_task_data = &cb_data->ut.get_all;
	g_variant_builder_add(cb_task_data->vb, "{sv}",
			      DLS_INTERFACE_PROP_SV_SERVICE_RESET_TOKEN,
			      g_variant_new_string(token));

	cb_data->task.result = g_variant_ref_sink(g_variant_builder_end(
							cb_task_data->vb));

	DLEYNA_LOG_DEBUG("Service Reset %s", token);

	g_free(token);

on_complete:

	(void) g_idle_add(dls_async_task_complete, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);

	if (error)
		g_error_free(error);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_get_sr_token_for_props(GUPnPServiceProxy *proxy,
			     const dls_device_t *device,
			     dls_async_task_t *cb_data)
{
	dls_async_get_all_t *cb_task_data;

	DLEYNA_LOG_DEBUG("Enter");

	if (prv_get_media_server_version(device) < 3) {
		cb_task_data = &cb_data->ut.get_all;

		cb_data->task.result = g_variant_ref_sink(g_variant_builder_end(
							cb_task_data->vb));

		goto on_complete; /* No error here, just skip the property */
	}

	cb_data->action = gupnp_service_proxy_begin_action(
					proxy, "GetServiceResetToken",
					prv_service_reset_for_props_cb,
					cb_data,
					NULL);

	if (cb_data->proxy != NULL)
		g_object_remove_weak_pointer((G_OBJECT(cb_data->proxy)),
					     (gpointer *)&cb_data->proxy);

	cb_data->proxy = proxy;
	g_object_add_weak_pointer((G_OBJECT(proxy)),
				  (gpointer *)&cb_data->proxy);

	if (!cb_data->cancel_id)
		cb_data->cancel_id = g_cancellable_connect(
					cb_data->cancellable,
					G_CALLBACK(dls_async_task_cancelled_cb),
					cb_data, NULL);

	DLEYNA_LOG_DEBUG("Exit");

	return;

on_complete:

	(void) g_idle_add(dls_async_task_complete, cb_data);

	DLEYNA_LOG_DEBUG("Exit");
}

static gboolean prv_get_all_child_count_cb(dls_async_task_t *cb_data,
				       gint count)
{
	dls_async_get_all_t *cb_task_data = &cb_data->ut.get_all;

	dls_props_add_child_count(cb_task_data->vb, count);
	if (cb_task_data->device_object)
		prv_get_system_update_id_for_props(cb_data->proxy,
						   cb_data->task.target.device,
						   cb_data);
	else
		cb_data->task.result = g_variant_ref_sink(g_variant_builder_end(
							cb_task_data->vb));

	return !cb_task_data->device_object;
}

static void prv_get_all_ms2spec_props_cb(GUPnPServiceProxy *proxy,
					 GUPnPServiceProxyAction *action,
					 gpointer user_data)
{
	GError *error = NULL;
	const gchar *message;
	gchar *result = NULL;
	gboolean end;
	GUPnPDIDLLiteParser *parser = NULL;
	dls_async_task_t *cb_data = user_data;
	dls_async_get_all_t *cb_task_data = &cb_data->ut.get_all;

	DLEYNA_LOG_DEBUG("Enter");

	end = gupnp_service_proxy_end_action(cb_data->proxy, cb_data->action,
					    &error, "Result",
					    G_TYPE_STRING, &result, NULL);

	if (!end || (result == NULL)) {
		message = (error != NULL) ? error->message : "Invalid result";
		DLEYNA_LOG_WARNING("Browse operation failed: %s", message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Browse operation failed: %s",
					     message);
		goto on_error;
	}

	DLEYNA_LOG_DEBUG("GetMS2SpecProps result: %s", result);

	parser = gupnp_didl_lite_parser_new();

	g_signal_connect(parser, "object-available" , cb_task_data->prop_func,
			 cb_data);

	if (!gupnp_didl_lite_parser_parse_didl(parser, result, &error)) {
		if (error->code == GUPNP_XML_ERROR_EMPTY_NODE) {
			DLEYNA_LOG_WARNING("Property not defined for object");

			cb_data->error =
				g_error_new(DLEYNA_SERVER_ERROR,
					    DLEYNA_ERROR_UNKNOWN_PROPERTY,
					    "Property not defined for object");
		} else {
			DLEYNA_LOG_WARNING(
				"Unable to parse results of browse: %s",
				error->message);

			cb_data->error =
				g_error_new(DLEYNA_SERVER_ERROR,
					    DLEYNA_ERROR_OPERATION_FAILED,
					    "Unable to parse results of browse: %s",
					    error->message);
		}
		goto on_error;
	}

	if (cb_data->error)
		goto on_error;

	if (cb_task_data->need_child_count) {
		DLEYNA_LOG_DEBUG("Need Child Count");

		prv_get_child_count(cb_data, prv_get_all_child_count_cb,
				    cb_data->task.target.id);

		goto no_complete;
	} else if (cb_data->task.type == DLS_TASK_GET_ALL_PROPS &&
						cb_task_data->device_object) {
		prv_get_system_update_id_for_props(proxy,
						   cb_data->task.target.device,
						   cb_data);

		goto no_complete;
	} else {
		cb_data->task.result = g_variant_ref_sink(g_variant_builder_end(
							cb_task_data->vb));
	}

on_error:

	(void) g_idle_add(dls_async_task_complete, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);

no_complete:

	if (error)
		g_error_free(error);

	if (parser)
		g_object_unref(parser);

	g_free(result);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_get_all_ms2spec_props(dls_device_context_t *context,
				      dls_async_task_t *cb_data)
{
	dls_async_get_all_t *cb_task_data = &cb_data->ut.get_all;
	dls_task_t *task = &cb_data->task;
	dls_task_get_props_t *task_data = &task->ut.get_props;

	DLEYNA_LOG_DEBUG("Enter called");

	if (!strcmp(DLS_INTERFACE_MEDIA_CONTAINER, task_data->interface_name)) {
		cb_task_data->prop_func = G_CALLBACK(prv_get_container);
	} else if (!strcmp(DLS_INTERFACE_MEDIA_ITEM,
		   task_data->interface_name)) {
		cb_task_data->prop_func = G_CALLBACK(prv_get_item);
	} else if (!strcmp(DLS_INTERFACE_MEDIA_OBJECT,
		   task_data->interface_name)) {
		cb_task_data->prop_func = G_CALLBACK(prv_get_object);
	} else  if (!strcmp("", task_data->interface_name)) {
		cb_task_data->prop_func = G_CALLBACK(prv_get_all);
	} else {
		DLEYNA_LOG_WARNING("Interface is unknown.");

		cb_data->error =
			g_error_new(DLEYNA_SERVER_ERROR,
				    DLEYNA_ERROR_UNKNOWN_INTERFACE,
				    "Interface is unknown.");
		goto on_error;
	}

	cb_data->action = gupnp_service_proxy_begin_action(
				context->service_proxy, "Browse",
				prv_get_all_ms2spec_props_cb, cb_data,
				"ObjectID", G_TYPE_STRING, task->target.id,
				"BrowseFlag", G_TYPE_STRING, "BrowseMetadata",
				"Filter", G_TYPE_STRING, "*",
				"StartingIndex", G_TYPE_INT, 0,
				"RequestedCount", G_TYPE_INT, 0,
				"SortCriteria", G_TYPE_STRING,
				"", NULL);

	cb_data->proxy = context->service_proxy;

	g_object_add_weak_pointer((G_OBJECT(context->service_proxy)),
				  (gpointer *)&cb_data->proxy);

	cb_data->cancel_id = g_cancellable_connect(
					cb_data->cancellable,
					G_CALLBACK(dls_async_task_cancelled_cb),
					cb_data, NULL);

	DLEYNA_LOG_DEBUG("Exit with SUCCESS");

	return;

on_error:

	(void) g_idle_add(dls_async_task_complete, cb_data);

	DLEYNA_LOG_DEBUG("Exit with FAIL");

	return;
}

void dls_device_get_all_props(dls_client_t *client,
			      dls_task_t *task,
			      gboolean root_object)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_async_get_all_t *cb_task_data;
	dls_task_get_props_t *task_data = &task->ut.get_props;
	dls_device_context_t *context;

	DLEYNA_LOG_DEBUG("Enter");

	context = dls_device_get_context(task->target.device, client);
	cb_task_data = &cb_data->ut.get_all;

	cb_task_data->vb = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	cb_task_data->device_object = root_object;

	if (!strcmp(task_data->interface_name,
		    DLEYNA_SERVER_INTERFACE_MEDIA_DEVICE)) {
		if (root_object) {
			dls_props_add_device(
				(GUPnPDeviceInfo *)context->device_proxy,
				task->target.device,
				cb_task_data->vb);

			prv_get_system_update_id_for_props(
							context->service_proxy,
							task->target.device,
							cb_data);
		} else {
			cb_data->error =
				g_error_new(DLEYNA_SERVER_ERROR,
					    DLEYNA_ERROR_UNKNOWN_INTERFACE,
					    "Interface is only valid on root objects.");

			(void) g_idle_add(dls_async_task_complete, cb_data);
		}

	} else if (strcmp(task_data->interface_name, "")) {
		cb_task_data->device_object = FALSE;
		prv_get_all_ms2spec_props(context, cb_data);
	} else {
		if (root_object)
			dls_props_add_device(
				(GUPnPDeviceInfo *)context->device_proxy,
				task->target.device,
				cb_task_data->vb);

		prv_get_all_ms2spec_props(context, cb_data);
	}

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_get_object_property(GUPnPDIDLLiteParser *parser,
				    GUPnPDIDLLiteObject *object,
				    gpointer user_data)
{
	dls_async_task_t *cb_data = user_data;
	dls_task_t *task = &cb_data->task;
	dls_task_get_prop_t *task_data = &task->ut.get_prop;

	if (cb_data->task.result)
		goto on_error;

	cb_data->task.result = dls_props_get_object_prop(task_data->prop_name,
							 task->target.root_path,
							 object);

on_error:

	return;
}

static void prv_get_item_property(GUPnPDIDLLiteParser *parser,
				  GUPnPDIDLLiteObject *object,
				  gpointer user_data)
{
	dls_async_task_t *cb_data = user_data;
	dls_task_t *task = &cb_data->task;
	dls_task_get_prop_t *task_data = &task->ut.get_prop;
	dls_async_get_prop_t *cb_task_data = &cb_data->ut.get_prop;

	if (cb_data->task.result)
		goto on_error;

	cb_data->task.result = dls_props_get_item_prop(
						task_data->prop_name,
						task->target.root_path,
						object,
						cb_task_data->protocol_info);

on_error:

	return;
}

static void prv_get_container_property(GUPnPDIDLLiteParser *parser,
				       GUPnPDIDLLiteObject *object,
				       gpointer user_data)
{
	dls_async_task_t *cb_data = user_data;
	dls_task_t *task = &cb_data->task;
	dls_task_get_prop_t *task_data = &task->ut.get_prop;
	dls_async_get_prop_t *cb_task_data = &cb_data->ut.get_prop;

	if (cb_data->task.result)
		goto on_error;

	cb_data->task.result = dls_props_get_container_prop(
						task_data->prop_name,
						object,
						cb_task_data->protocol_info);

on_error:

	return;
}

static void prv_get_all_property(GUPnPDIDLLiteParser *parser,
				 GUPnPDIDLLiteObject *object,
				 gpointer user_data)
{
	dls_async_task_t *cb_data = user_data;

	prv_get_object_property(parser, object, user_data);

	if (cb_data->task.result)
		goto on_error;

	if (GUPNP_IS_DIDL_LITE_CONTAINER(object))
		prv_get_container_property(parser, object, user_data);
	else
		prv_get_item_property(parser, object, user_data);

on_error:

	return;
}

static gboolean prv_get_child_count_cb(dls_async_task_t *cb_data,
				   gint count)
{
	DLEYNA_LOG_DEBUG("Enter");

	DLEYNA_LOG_DEBUG("Count %d", count);

	cb_data->task.result =  g_variant_ref_sink(
					g_variant_new_uint32((guint) count));

	DLEYNA_LOG_DEBUG("Exit");

	return TRUE;
}

static void prv_count_children_cb(GUPnPServiceProxy *proxy,
				  GUPnPServiceProxyAction *action,
				  gpointer user_data)
{
	dls_device_count_data_t *count_data = user_data;
	dls_async_task_t *cb_data = count_data->cb_data;
	GError *error = NULL;
	const gchar *message;
	guint count = G_MAXUINT32;
	gboolean complete = FALSE;
	gboolean end;

	DLEYNA_LOG_DEBUG("Enter");

	end = gupnp_service_proxy_end_action(cb_data->proxy, cb_data->action,
					    &error,
					    "TotalMatches", G_TYPE_UINT, &count,
					    NULL);

	if (!end || (count == G_MAXUINT32)) {
		message = (error != NULL) ? error->message : "Invalid result";
		DLEYNA_LOG_WARNING("Browse operation failed: %s", message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Browse operation failed: %s",
					     message);
		goto on_error;
	}

	complete = count_data->cb(cb_data, count);

on_error:

	g_free(user_data);

	if (cb_data->error || complete) {
		(void) g_idle_add(dls_async_task_complete, cb_data);
		g_cancellable_disconnect(cb_data->cancellable,
					 cb_data->cancel_id);
	}

	if (error)
		g_error_free(error);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_get_child_count(dls_async_task_t *cb_data,
				dls_device_count_cb_t cb, const gchar *id)
{
	dls_device_count_data_t *count_data;

	DLEYNA_LOG_DEBUG("Enter");

	prv_count_data_new(cb_data, cb, &count_data);
	cb_data->action =
		gupnp_service_proxy_begin_action(cb_data->proxy,
						 "Browse",
						 prv_count_children_cb,
						 count_data,
						 "ObjectID", G_TYPE_STRING, id,

						 "BrowseFlag", G_TYPE_STRING,
						 "BrowseDirectChildren",

						 "Filter", G_TYPE_STRING, "",

						 "StartingIndex", G_TYPE_INT,
						 0,

						 "RequestedCount", G_TYPE_INT,
						 1,

						 "SortCriteria", G_TYPE_STRING,
						 "",

						 NULL);

	DLEYNA_LOG_DEBUG("Exit with SUCCESS");
}

static void prv_get_ms2spec_prop_cb(GUPnPServiceProxy *proxy,
				    GUPnPServiceProxyAction *action,
				    gpointer user_data)
{
	GError *error = NULL;
	const gchar *message;
	gchar *result = NULL;
	gboolean end;
	GUPnPDIDLLiteParser *parser = NULL;
	dls_async_task_t *cb_data = user_data;
	dls_async_get_prop_t *cb_task_data = &cb_data->ut.get_prop;
	dls_task_get_prop_t *task_data = &cb_data->task.ut.get_prop;

	DLEYNA_LOG_DEBUG("Enter");

	end = gupnp_service_proxy_end_action(cb_data->proxy, cb_data->action,
					    &error, "Result",
					    G_TYPE_STRING, &result, NULL);

	if (!end || (result == NULL)) {
		message = (error != NULL) ? error->message : "Invalid result";
		DLEYNA_LOG_WARNING("Browse operation failed: %s", message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Browse operation failed: %s",
					     message);
		goto on_error;
	}

	DLEYNA_LOG_DEBUG("GetMS2SpecProp result: %s", result);

	parser = gupnp_didl_lite_parser_new();

	g_signal_connect(parser, "object-available" , cb_task_data->prop_func,
			 cb_data);

	if (!gupnp_didl_lite_parser_parse_didl(parser, result, &error)) {
		if (error->code == GUPNP_XML_ERROR_EMPTY_NODE) {
			DLEYNA_LOG_WARNING("Property not defined for object");

			cb_data->error =
				g_error_new(DLEYNA_SERVER_ERROR,
					    DLEYNA_ERROR_UNKNOWN_PROPERTY,
					    "Property not defined for object");
		} else {
			DLEYNA_LOG_WARNING(
				"Unable to parse results of browse: %s",
				error->message);

			cb_data->error =
				g_error_new(DLEYNA_SERVER_ERROR,
					    DLEYNA_ERROR_OPERATION_FAILED,
					    "Unable to parse results of browse: %s",
					    error->message);
		}
		goto on_error;
	}

	if (!cb_data->task.result) {
		DLEYNA_LOG_WARNING("Property not defined for object");

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_UNKNOWN_PROPERTY,
					     "Property not defined for object");
	}

on_error:

	if (cb_data->error && !strcmp(task_data->prop_name,
				      DLS_INTERFACE_PROP_CHILD_COUNT)) {
		DLEYNA_LOG_DEBUG("ChildCount not supported by server");

		g_error_free(cb_data->error);
		cb_data->error = NULL;
		prv_get_child_count(cb_data, prv_get_child_count_cb,
				    cb_data->task.target.id);
	} else {
		(void) g_idle_add(dls_async_task_complete, cb_data);
		g_cancellable_disconnect(cb_data->cancellable,
					 cb_data->cancel_id);
	}

	if (error)
		g_error_free(error);

	if (parser)
		g_object_unref(parser);

	g_free(result);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_get_ms2spec_prop(dls_device_context_t *context,
				 dls_prop_map_t *prop_map,
				 dls_task_get_prop_t *task_data,
				 dls_async_task_t *cb_data)
{
	dls_async_get_prop_t *cb_task_data;
	const gchar *filter;

	DLEYNA_LOG_DEBUG("Enter");

	cb_task_data = &cb_data->ut.get_prop;

	if (!prop_map) {
		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_UNKNOWN_PROPERTY,
					     "Unknown property");
		goto on_error;
	}

	filter = prop_map->filter ? prop_map->upnp_prop_name : "";

	if (!strcmp(DLS_INTERFACE_MEDIA_CONTAINER, task_data->interface_name)) {
		cb_task_data->prop_func =
			G_CALLBACK(prv_get_container_property);
	} else if (!strcmp(DLS_INTERFACE_MEDIA_ITEM,
			   task_data->interface_name)) {
		cb_task_data->prop_func = G_CALLBACK(prv_get_item_property);
	} else if (!strcmp(DLS_INTERFACE_MEDIA_OBJECT,
			   task_data->interface_name)) {
		cb_task_data->prop_func = G_CALLBACK(prv_get_object_property);
	} else  if (!strcmp("", task_data->interface_name)) {
		cb_task_data->prop_func = G_CALLBACK(prv_get_all_property);
	} else {
		DLEYNA_LOG_WARNING("Interface is unknown.%s",
				   task_data->interface_name);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_UNKNOWN_INTERFACE,
					     "Interface is unknown.");
		goto on_error;
	}

	cb_data->action = gupnp_service_proxy_begin_action(
			context->service_proxy, "Browse",
			prv_get_ms2spec_prop_cb,
			cb_data,
			"ObjectID", G_TYPE_STRING, cb_data->task.target.id,
			"BrowseFlag", G_TYPE_STRING,
			"BrowseMetadata",
			"Filter", G_TYPE_STRING, filter,
			"StartingIndex", G_TYPE_INT, 0,
			"RequestedCount", G_TYPE_INT, 0,
			"SortCriteria", G_TYPE_STRING,
			"",
			NULL);

	cb_data->proxy = context->service_proxy;

	g_object_add_weak_pointer((G_OBJECT(context->service_proxy)),
				  (gpointer *)&cb_data->proxy);

	cb_data->cancel_id = g_cancellable_connect(
					cb_data->cancellable,
					G_CALLBACK(dls_async_task_cancelled_cb),
					cb_data, NULL);

	DLEYNA_LOG_DEBUG("Exit with SUCCESS");

	return;

on_error:

	(void) g_idle_add(dls_async_task_complete, cb_data);

	DLEYNA_LOG_DEBUG("Exit with FAIL");

	return;
}

void dls_device_get_prop(dls_client_t *client,
			 dls_task_t *task,
			 dls_prop_map_t *prop_map, gboolean root_object)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_task_get_prop_t *task_data = &task->ut.get_prop;
	dls_device_context_t *context;
	gboolean complete = FALSE;

	DLEYNA_LOG_DEBUG("Enter");

	context = dls_device_get_context(task->target.device, client);

	if (!strcmp(task_data->interface_name,
		    DLEYNA_SERVER_INTERFACE_MEDIA_DEVICE)) {
		if (root_object) {
			if (!strcmp(
				task_data->prop_name,
				DLS_INTERFACE_PROP_ESV_SYSTEM_UPDATE_ID)) {
				prv_get_system_update_id_for_prop(
							context->service_proxy,
							task->target.device,
							cb_data);
			} else if (!strcmp(
				task_data->prop_name,
				DLS_INTERFACE_PROP_SV_SERVICE_RESET_TOKEN)) {
				prv_get_sr_token_for_prop(
							context->service_proxy,
							task->target.device,
							cb_data);
			} else {
				cb_data->task.result =
					dls_props_get_device_prop(
						(GUPnPDeviceInfo *)
						context->device_proxy,
						task->target.device,
						task_data->prop_name);

				if (!cb_data->task.result)
					cb_data->error = g_error_new(
							DLEYNA_SERVER_ERROR,
						DLEYNA_ERROR_UNKNOWN_PROPERTY,
						"Unknown property");

				(void) g_idle_add(dls_async_task_complete,
						  cb_data);
			}

		} else {
			cb_data->error =
				g_error_new(DLEYNA_SERVER_ERROR,
					    DLEYNA_ERROR_UNKNOWN_INTERFACE,
					    "Interface is unknown.");

			(void) g_idle_add(dls_async_task_complete, cb_data);
		}

	} else if (strcmp(task_data->interface_name, "")) {
		prv_get_ms2spec_prop(context, prop_map, &task->ut.get_prop,
				     cb_data);
	} else {
		if (root_object) {
			if (!strcmp(
				task_data->prop_name,
				DLS_INTERFACE_PROP_ESV_SYSTEM_UPDATE_ID)) {
				prv_get_system_update_id_for_prop(
							context->service_proxy,
							task->target.device,
							cb_data);
				complete = TRUE;
			} else if (!strcmp(
				task_data->prop_name,
				DLS_INTERFACE_PROP_SV_SERVICE_RESET_TOKEN)) {
				prv_get_sr_token_for_prop(
							context->service_proxy,
							task->target.device,
							cb_data);
				complete = TRUE;
			} else {
				cb_data->task.result =
						dls_props_get_device_prop(
							(GUPnPDeviceInfo *)
							context->device_proxy,
							task->target.device,
							task_data->prop_name);
				if (cb_data->task.result) {
					(void) g_idle_add(
							dls_async_task_complete,
							cb_data);
					complete = TRUE;
				}
			}
		}

		if (!complete)
			prv_get_ms2spec_prop(context, prop_map,
					     &task->ut.get_prop,
					     cb_data);
	}

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_found_target(GUPnPDIDLLiteParser *parser,
			     GUPnPDIDLLiteObject *object,
			     gpointer user_data)
{
	dls_async_task_t *cb_data = user_data;
	dls_async_bas_t *cb_task_data = &cb_data->ut.bas;
	const char *object_id;
	const char *parent_id;
	const char *parent_path;
	gchar *path = NULL;
	gboolean have_child_count;
	dls_device_object_builder_t *builder;

	DLEYNA_LOG_DEBUG("Enter");

	builder = g_new0(dls_device_object_builder_t, 1);

	object_id = gupnp_didl_lite_object_get_id(object);
	if (!object_id)
		goto on_error;

	parent_id = gupnp_didl_lite_object_get_parent_id(object);
	if (!parent_id)
		goto on_error;

	if (!strcmp(object_id, "0") || !strcmp(parent_id, "-1")) {
		parent_path = cb_data->task.target.root_path;
	} else {
		path = dls_path_from_id(cb_data->task.target.root_path,
					parent_id);
		parent_path = path;
	}

	builder->vb = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));

	if (!dls_props_add_object(builder->vb, object,
				  cb_data->task.target.root_path,
				  parent_path, cb_task_data->filter_mask))
		goto on_error;

	if (GUPNP_IS_DIDL_LITE_CONTAINER(object)) {
		dls_props_add_container(builder->vb,
					(GUPnPDIDLLiteContainer *)object,
					cb_task_data->filter_mask,
					cb_task_data->protocol_info,
					&have_child_count);

		if (!have_child_count && (cb_task_data->filter_mask &
					  DLS_UPNP_MASK_PROP_CHILD_COUNT)) {
			builder->needs_child_count = TRUE;
			builder->id = g_strdup(
				gupnp_didl_lite_object_get_id(object));
			cb_task_data->need_child_count = TRUE;
		}
	} else {
		dls_props_add_item(builder->vb,
				   object,
				   cb_data->task.target.root_path,
				   cb_task_data->filter_mask,
				   cb_task_data->protocol_info);
	}

	g_ptr_array_add(cb_task_data->vbs, builder);
	g_free(path);

	DLEYNA_LOG_DEBUG("Exit with SUCCESS");

	return;

on_error:

	g_free(path);
	prv_object_builder_delete(builder);

	DLEYNA_LOG_DEBUG("Exit with FAIL");
}

static void prv_search_cb(GUPnPServiceProxy *proxy,
			  GUPnPServiceProxyAction *action,
			  gpointer user_data)
{
	gchar *result = NULL;
	const gchar *message;
	gboolean end;
	guint count = G_MAXUINT32;
	GUPnPDIDLLiteParser *parser = NULL;
	GError *error = NULL;
	dls_async_task_t *cb_data = user_data;
	dls_async_bas_t *cb_task_data = &cb_data->ut.bas;

	DLEYNA_LOG_DEBUG("Enter");

	end = gupnp_service_proxy_end_action(cb_data->proxy, cb_data->action,
					     &error,
					     "Result", G_TYPE_STRING, &result,
					     "TotalMatches", G_TYPE_UINT,
					     &count, NULL);

	if (!end || (result == NULL) || (count == G_MAXUINT32)) {
		message = (error != NULL) ? error->message : "Invalid result";
		DLEYNA_LOG_WARNING("Search operation failed %s", message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Search operation failed: %s",
					     message);
		goto on_error;
	}

	cb_task_data->max_count = count;

	parser = gupnp_didl_lite_parser_new();

	cb_task_data->vbs = g_ptr_array_new_with_free_func(
		prv_object_builder_delete);

	g_signal_connect(parser, "object-available" ,
			 G_CALLBACK(prv_found_target), cb_data);

	DLEYNA_LOG_DEBUG("Server Search result: %s", result);

	if (!gupnp_didl_lite_parser_parse_didl(parser, result, &error) &&
	    error->code != GUPNP_XML_ERROR_EMPTY_NODE) {
		DLEYNA_LOG_WARNING("Unable to parse results of search: %s",
				   error->message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Unable to parse results of search: %s",
					     error->message);
		goto on_error;
	}

	if (cb_task_data->need_child_count) {
		DLEYNA_LOG_DEBUG("Need to retrieve child count");

		if (cb_data->task.multiple_retvals)
			cb_task_data->get_children_cb =
				prv_get_search_ex_result;
		else
			cb_task_data->get_children_cb = prv_get_children_result;
		prv_retrieve_child_count_for_list(cb_data);
		goto no_complete;
	} else {
		if (cb_data->task.multiple_retvals)
			prv_get_search_ex_result(cb_data);
		else
			prv_get_children_result(cb_data);
	}

on_error:

	(void) g_idle_add(dls_async_task_complete, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);

no_complete:

	if (parser)
		g_object_unref(parser);

	g_free(result);

	if (error)
		g_error_free(error);

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_device_search(dls_client_t *client,
		       dls_task_t *task,
		       const gchar *upnp_filter, const gchar *upnp_query,
		       const gchar *sort_by)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_device_context_t *context;

	DLEYNA_LOG_DEBUG("Enter");

	context = dls_device_get_context(task->target.device, client);

	cb_data->action = gupnp_service_proxy_begin_action(
			context->service_proxy, "Search",
			prv_search_cb,
			cb_data,
			"ContainerID", G_TYPE_STRING, task->target.id,
			"SearchCriteria", G_TYPE_STRING, upnp_query,
			"Filter", G_TYPE_STRING, upnp_filter,
			"StartingIndex", G_TYPE_INT, task->ut.search.start,
			"RequestedCount", G_TYPE_INT, task->ut.search.count,
			"SortCriteria", G_TYPE_STRING, sort_by,
			NULL);

	cb_data->proxy = context->service_proxy;

	g_object_add_weak_pointer((G_OBJECT(context->service_proxy)),
				  (gpointer *)&cb_data->proxy);

	cb_data->cancel_id = g_cancellable_connect(
					cb_data->cancellable,
					G_CALLBACK(dls_async_task_cancelled_cb),
					cb_data, NULL);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_get_resource(GUPnPDIDLLiteParser *parser,
			     GUPnPDIDLLiteObject *object,
			     gpointer user_data)
{
	dls_async_task_t *cb_data = user_data;
	dls_task_t *task = &cb_data->task;
	dls_task_get_resource_t *task_data = &task->ut.resource;
	dls_async_get_all_t *cb_task_data = &cb_data->ut.get_all;

	DLEYNA_LOG_DEBUG("Enter");

	dls_props_add_resource(cb_task_data->vb, object,
			       cb_task_data->filter_mask,
			       task_data->protocol_info);
}

void dls_device_get_resource(dls_client_t *client,
			     dls_task_t *task,
			     const gchar *upnp_filter)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_async_get_all_t *cb_task_data;
	dls_device_context_t *context;

	context = dls_device_get_context(task->target.device, client);
	cb_task_data = &cb_data->ut.get_all;

	cb_task_data->vb = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	cb_task_data->prop_func = G_CALLBACK(prv_get_resource);
	cb_task_data->device_object = FALSE;

	cb_data->action = gupnp_service_proxy_begin_action(
				context->service_proxy, "Browse",
				prv_get_all_ms2spec_props_cb, cb_data,
				"ObjectID", G_TYPE_STRING, task->target.id,
				"BrowseFlag", G_TYPE_STRING, "BrowseMetadata",
				"Filter", G_TYPE_STRING, upnp_filter,
				"StartingIndex", G_TYPE_INT, 0,
				"RequestedCount", G_TYPE_INT, 0,
				"SortCriteria", G_TYPE_STRING,
				"", NULL);

	cb_data->proxy = context->service_proxy;

	g_object_add_weak_pointer((G_OBJECT(context->service_proxy)),
				  (gpointer *)&cb_data->proxy);

	cb_data->cancel_id = g_cancellable_connect(
					cb_data->cancellable,
					G_CALLBACK(dls_async_task_cancelled_cb),
					cb_data, NULL);

	DLEYNA_LOG_DEBUG("Exit");
}

static gchar *prv_create_new_container_didl(const gchar *parent_id,
					    dls_task_t *task)
{
	GUPnPDIDLLiteWriter *writer;
	GUPnPDIDLLiteObject *item;
	GUPnPDIDLLiteContainer *container;
	GVariantIter iter;
	GVariant *child_type;
	gchar *actual_type;
	GUPnPOCMFlags flags;
	gchar *retval = NULL;

	actual_type = dls_props_media_spec_ex_to_upnp_class(
						task->ut.create_container.type);
	if (!actual_type)
		goto on_error;

	writer = gupnp_didl_lite_writer_new(NULL);
	item = GUPNP_DIDL_LITE_OBJECT(
				gupnp_didl_lite_writer_add_container(writer));
	container = GUPNP_DIDL_LITE_CONTAINER(item);

	gupnp_didl_lite_object_set_id(item, "");
	gupnp_didl_lite_object_set_title(
					item,
					task->ut.create_container.display_name);
	gupnp_didl_lite_object_set_parent_id(item, parent_id);
	gupnp_didl_lite_object_set_upnp_class(item, actual_type);
	g_free(actual_type);
	gupnp_didl_lite_object_set_restricted(item, FALSE);

	flags = GUPNP_OCM_FLAGS_UPLOAD |
		GUPNP_OCM_FLAGS_CREATE_CONTAINER |
		GUPNP_OCM_FLAGS_DESTROYABLE |
		GUPNP_OCM_FLAGS_UPLOAD_DESTROYABLE |
		GUPNP_OCM_FLAGS_CHANGE_METADATA;

	gupnp_didl_lite_object_set_dlna_managed(item, flags);

	g_variant_iter_init(&iter, task->ut.create_container.child_types);
	while ((child_type = g_variant_iter_next_value(&iter))) {
		actual_type = dls_props_media_spec_ex_to_upnp_class(
					g_variant_get_string(child_type, NULL));
		if (actual_type != NULL) {
			gupnp_didl_lite_container_add_create_class(container,
								   actual_type);
			g_free(actual_type);
		}
		g_variant_unref(child_type);
	}

	retval = gupnp_didl_lite_writer_get_string(writer);

	g_object_unref(item);
	g_object_unref(writer);

on_error:

	return retval;
}

static const gchar *prv_get_dlna_profile_name(const gchar *filename)
{
	gchar *uri;
	GError *error = NULL;
	const gchar *profile_name = NULL;
	GUPnPDLNAProfile *profile;
	GUPnPDLNAProfileGuesser *guesser;
	gboolean relaxed_mode = TRUE;
	gboolean extended_mode = TRUE;

	guesser = gupnp_dlna_profile_guesser_new(relaxed_mode, extended_mode);

	uri = g_filename_to_uri(filename, NULL, &error);
	if (uri == NULL) {
		DLEYNA_LOG_WARNING("Unable to convert filename: %s", filename);

		if (error) {
			DLEYNA_LOG_WARNING("Error: %s", error->message);

			g_error_free(error);
		}

		goto on_error;
	}

	profile = gupnp_dlna_profile_guesser_guess_profile_sync(guesser,
								uri,
								5000,
								NULL,
								&error);
	if (profile == NULL) {
		DLEYNA_LOG_WARNING("Unable to guess profile for URI: %s", uri);

		if (error) {
			DLEYNA_LOG_WARNING("Error: %s", error->message);

			g_error_free(error);
		}

		goto on_error;
	}

	profile_name = gupnp_dlna_profile_get_name(profile);

on_error:
	g_object_unref(guesser);

	g_free(uri);

	return profile_name;
}

static gchar *prv_create_upload_didl(const gchar *parent_id, dls_task_t *task,
				     const gchar *object_class,
				     const gchar *mime_type)
{
	GUPnPDIDLLiteWriter *writer;
	GUPnPDIDLLiteObject *item;
	gchar *retval;
	GUPnPProtocolInfo *protocol_info;
	GUPnPDIDLLiteResource *res;
	const gchar *profile;

	writer = gupnp_didl_lite_writer_new(NULL);
	item = GUPNP_DIDL_LITE_OBJECT(gupnp_didl_lite_writer_add_item(writer));

	gupnp_didl_lite_object_set_id(item, "");
	gupnp_didl_lite_object_set_title(item, task->ut.upload.display_name);
	gupnp_didl_lite_object_set_parent_id(item, parent_id);
	gupnp_didl_lite_object_set_upnp_class(item, object_class);
	gupnp_didl_lite_object_set_restricted(item, FALSE);

	protocol_info = gupnp_protocol_info_new();
	gupnp_protocol_info_set_mime_type(protocol_info, mime_type);
	gupnp_protocol_info_set_protocol(protocol_info, "*");
	gupnp_protocol_info_set_network(protocol_info, "*");

	profile = prv_get_dlna_profile_name(task->ut.upload.file_path);
	if (profile != NULL)
		gupnp_protocol_info_set_dlna_profile(protocol_info, profile);

	res = gupnp_didl_lite_object_add_resource(item);
	gupnp_didl_lite_resource_set_protocol_info(res, protocol_info);

	retval = gupnp_didl_lite_writer_get_string(writer);

	g_object_unref(res);
	g_object_unref(protocol_info);
	g_object_unref(item);
	g_object_unref(writer);

	return retval;
}

static void prv_extract_import_uri(GUPnPDIDLLiteParser *parser,
				   GUPnPDIDLLiteObject *object,
				   gpointer user_data)
{
	gchar **import_uri = user_data;
	GList *resources;
	GList *ptr;
	GUPnPDIDLLiteResource *res;
	const gchar *uri;

	if (!*import_uri) {
		resources = gupnp_didl_lite_object_get_resources(object);
		ptr = resources;
		while (ptr) {
			res = ptr->data;
			if (!*import_uri) {
				uri = gupnp_didl_lite_resource_get_import_uri(
					res);
				if (uri)
					*import_uri = g_strdup(uri);
			}
			g_object_unref(res);
			ptr = ptr->next;
		}

		g_list_free(resources);
	}
}

static void prv_upload_delete_cb(GUPnPServiceProxy *proxy,
				 GUPnPServiceProxyAction *action,
				 gpointer user_data)
{
	dls_async_task_t *cb_data = user_data;

	DLEYNA_LOG_DEBUG("Enter");

	(void) gupnp_service_proxy_end_action(cb_data->proxy, cb_data->action,
					      NULL, NULL);
	(void) g_idle_add(dls_async_task_complete, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_upload_job_delete(gpointer up_job)
{
	dls_device_upload_job_t *upload_job = up_job;

	if (up_job) {
		if (upload_job->remove_idle)
			(void) g_source_remove(upload_job->remove_idle);

		g_free(upload_job);
	}
}

static gboolean prv_remove_update_job(gpointer user_data)
{
	dls_device_upload_job_t *upload_job = user_data;
	dls_device_upload_t *upload;

	upload = g_hash_table_lookup(upload_job->device->uploads,
				     &upload_job->upload_id);
	if (upload) {
		g_hash_table_remove(upload_job->device->uploads,
				    &upload_job->upload_id);

		DLEYNA_LOG_DEBUG("Removing Upload Object: %d",
				 upload_job->upload_id);
	}

	upload_job->remove_idle = 0;
	g_hash_table_remove(upload_job->device->upload_jobs,
			    &upload_job->upload_id);

	return FALSE;
}

static void prv_generate_upload_update(dls_device_upload_job_t *upload_job,
				       dls_device_upload_t *upload)
{
	GVariant *args;

	args = g_variant_new("(ustt)", upload_job->upload_id, upload->status,
			     upload->bytes_uploaded, upload->bytes_to_upload);

	DLEYNA_LOG_DEBUG(
		"Emitting: %s (%u %s %"G_GUINT64_FORMAT" %"G_GUINT64_FORMAT")"
		" on %s",
		DLS_INTERFACE_UPLOAD_UPDATE, upload_job->upload_id,
		upload->status, upload->bytes_uploaded,
		upload->bytes_to_upload, upload_job->device->path);

	(void) dls_server_get_connector()->notify(
					upload_job->device->connection,
					upload_job->device->path,
					DLEYNA_SERVER_INTERFACE_MEDIA_DEVICE,
					DLS_INTERFACE_UPLOAD_UPDATE,
					args,
					NULL);
}

static void prv_post_finished(SoupSession *session, SoupMessage *msg,
			      gpointer user_data)
{
	dls_device_upload_job_t *upload_job = user_data;
	dls_device_upload_t *upload;
	gint *upload_id;

	DLEYNA_LOG_DEBUG("Enter");

	DLEYNA_LOG_DEBUG("Upload %u finished.  Code %u Message %s",
			 upload_job->upload_id, msg->status_code,
			 msg->reason_phrase);

	/* This is clumsy but we need to distinguish between two cases:
	   1. We cancel because the process is exitting.
	   2. We cancel because a client has called CancelUpload.

	   We could use custom SOUP error messages to distinguish the cases
	   but device->shutting_down seemed less hacky.

	   We need this check as if we are shutting down it is
	   dangerous to manipulate uploads as we are being called from its
	   destructor.
	*/

	if (upload_job->device->shutting_down) {
		DLEYNA_LOG_DEBUG("Device shutting down. Cancelling Upload");
		goto on_error;
	}

	upload = g_hash_table_lookup(upload_job->device->uploads,
				     &upload_job->upload_id);
	if (upload) {
		upload_job->remove_idle =
			g_timeout_add(30000, prv_remove_update_job, user_data);

		if (SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
			upload->status = DLS_UPLOAD_STATUS_COMPLETED;
			upload->bytes_uploaded = upload->bytes_to_upload;
		} else if (msg->status_code == SOUP_STATUS_CANCELLED) {
			upload->status = DLS_UPLOAD_STATUS_CANCELLED;
		} else {
			upload->status = DLS_UPLOAD_STATUS_ERROR;
		}

		DLEYNA_LOG_DEBUG("Upload Status: %s", upload->status);

		prv_generate_upload_update(upload_job, upload);

		g_object_unref(upload->msg);
		upload->msg = NULL;

		g_object_unref(upload->soup_session);
		upload->soup_session = NULL;

		g_mapped_file_unref(upload->mapped_file);
		upload->mapped_file = NULL;

		g_free(upload->body);
		upload->body = NULL;

		upload_id = g_new(gint, 1);
		*upload_id = upload_job->upload_id;

		g_hash_table_insert(upload_job->device->upload_jobs, upload_id,
				    upload_job);

		upload_job = NULL;
	}

on_error:

	prv_upload_job_delete(upload_job);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_upload_delete(gpointer up)
{
	dls_device_upload_t *upload = up;

	DLEYNA_LOG_DEBUG("Enter");

	if (upload) {
		if (upload->msg) {
			soup_session_cancel_message(upload->soup_session,
						    upload->msg,
						    SOUP_STATUS_CANCELLED);
			g_object_unref(upload->msg);
		}

		if (upload->soup_session)
			g_object_unref(upload->soup_session);

		if (upload->mapped_file)
			g_mapped_file_unref(upload->mapped_file);
		else if (upload->body)
			g_free(upload->body);

		g_free(upload);
	}

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_post_bytes_written(SoupMessage *msg, SoupBuffer *chunk,
				   gpointer user_data)
{
	dls_device_upload_t *upload = user_data;

	upload->bytes_uploaded += chunk->length;
	if (upload->bytes_uploaded > upload->bytes_to_upload)
		upload->bytes_uploaded = upload->bytes_to_upload;
}

static dls_device_upload_t *prv_upload_data_new(const gchar *file_path,
						gchar *body,
						gsize body_length,
						const gchar *import_uri,
						const gchar *mime_type,
						GError **error)
{
	dls_device_upload_t *upload = NULL;
	GMappedFile *mapped_file = NULL;
	gchar *up_body = body;
	gsize up_body_length = body_length;

	DLEYNA_LOG_DEBUG("Enter");

	if (file_path) {
		mapped_file = g_mapped_file_new(file_path, FALSE, NULL);
		if (!mapped_file) {
			DLEYNA_LOG_WARNING("Unable to map %s into memory",
					   file_path);

			*error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_IO,
					     "Unable to map %s into memory",
					     file_path);
			goto on_error;
		}

		up_body = g_mapped_file_get_contents(mapped_file);
		up_body_length = g_mapped_file_get_length(mapped_file);
	}

	upload = g_new0(dls_device_upload_t, 1);

	upload->soup_session = soup_session_async_new();
	upload->msg = soup_message_new("POST", import_uri);
	upload->mapped_file = mapped_file;
	upload->body = body;
	upload->body_length = body_length;

	if (!upload->msg) {
		DLEYNA_LOG_WARNING("Invalid URI %s", import_uri);

		*error = g_error_new(DLEYNA_SERVER_ERROR,
				     DLEYNA_ERROR_BAD_RESULT,
				     "Invalid URI %s", import_uri);
		goto on_error;
	}

	upload->status = DLS_UPLOAD_STATUS_IN_PROGRESS;
	upload->bytes_to_upload = up_body_length;

	soup_message_headers_set_expectations(upload->msg->request_headers,
					      SOUP_EXPECTATION_CONTINUE);

	soup_message_set_request(upload->msg, mime_type, SOUP_MEMORY_STATIC,
				 up_body, up_body_length);
	g_signal_connect(upload->msg, "wrote-body-data",
			 G_CALLBACK(prv_post_bytes_written), upload);

	DLEYNA_LOG_DEBUG("Exit with Success");

	return upload;

on_error:

	prv_upload_delete(upload);

	DLEYNA_LOG_WARNING("Exit with Fail");

	return NULL;
}

static void prv_create_container_cb(GUPnPServiceProxy *proxy,
		 GUPnPServiceProxyAction *action,
		 gpointer user_data)
{
	dls_async_task_t *cb_data = user_data;
	const gchar *message;
	GError *error = NULL;
	gchar *result = NULL;
	gchar *object_id = NULL;
	gchar *object_path;
	gboolean end;

	DLEYNA_LOG_DEBUG("Enter");

	end = gupnp_service_proxy_end_action(cb_data->proxy, cb_data->action,
					     &error, "ObjectID",
					     G_TYPE_STRING, &object_id,
					     "Result", G_TYPE_STRING, &result,
					     NULL);

	if (!end || (object_id == NULL)) {
		message = (error != NULL) ? error->message : "Invalid result";
		DLEYNA_LOG_WARNING("Create Object operation failed: %s",
				   message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Create Object operation failed: %s",
					     message);
		goto on_error;
	}

	object_path = dls_path_from_id(cb_data->task.target.root_path,
				       object_id);
	cb_data->task.result = g_variant_ref_sink(g_variant_new_object_path(
								object_path));
	g_free(object_path);

on_error:

	(void) g_idle_add(dls_async_task_complete, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);

	g_free(object_id);
	g_free(result);

	if (error)
		g_error_free(error);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_generic_upload_cb(dls_async_task_t *cb_data,
				  char *file_path,
				  gchar *body,
				  gsize body_length,
				  const gchar *mime_type)
{
	gchar *object_id = NULL;
	gchar *result = NULL;
	gchar *import_uri = NULL;
	gchar *object_path;
	const gchar *message;
	GError *error = NULL;
	gboolean delete_needed = FALSE;
	gboolean end;
	gint *upload_id;
	GUPnPDIDLLiteParser *parser = NULL;
	GVariant *out_p[2];
	dls_device_upload_t *upload;
	dls_device_upload_job_t *upload_job;

	DLEYNA_LOG_DEBUG("Enter");

	end = gupnp_service_proxy_end_action(cb_data->proxy, cb_data->action,
					     &error, "ObjectID",
					     G_TYPE_STRING, &object_id,
					     "Result", G_TYPE_STRING, &result,
					     NULL);

	if (!end || (object_id == NULL) || (result == NULL)) {
		message = (error != NULL) ? error->message : "Invalid result";
		DLEYNA_LOG_WARNING("Create Object operation failed: %s",
				   message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Create Object operation failed: %s",
					     message);
		goto on_error;
	}

	DLEYNA_LOG_DEBUG_NL();
	DLEYNA_LOG_DEBUG("Create Object Result: %s", result);
	DLEYNA_LOG_DEBUG_NL();

	delete_needed = TRUE;

	parser = gupnp_didl_lite_parser_new();

	g_signal_connect(parser, "object-available" ,
			 G_CALLBACK(prv_extract_import_uri), &import_uri);

	if (!gupnp_didl_lite_parser_parse_didl(parser, result, &error) &&
	    error->code != GUPNP_XML_ERROR_EMPTY_NODE) {
		DLEYNA_LOG_WARNING(
				"Unable to parse results of CreateObject: %s",
				error->message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Unable to parse results of CreateObject: %s",
					     error->message);
		goto on_error;
	}

	if (!import_uri) {
		DLEYNA_LOG_WARNING("Missing Import URI");

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Missing Import URI");
		goto on_error;
	}

	DLEYNA_LOG_DEBUG("Import URI %s", import_uri);

	upload = prv_upload_data_new(file_path, body, body_length,
				     import_uri, mime_type, &cb_data->error);

	if (!upload)
		goto on_error;

	upload_job = g_new0(dls_device_upload_job_t, 1);
	upload_job->device = cb_data->task.target.device;
	upload_job->upload_id = (gint) cb_data->task.target.device->upload_id;

	soup_session_queue_message(upload->soup_session, upload->msg,
				   prv_post_finished, upload_job);
	g_object_ref(upload->msg);

	upload_id = g_new(gint, 1);
	*upload_id = upload_job->upload_id;
	g_hash_table_insert(cb_data->task.target.device->uploads, upload_id,
			    upload);

	object_path = dls_path_from_id(cb_data->task.target.root_path,
				       object_id);

	DLEYNA_LOG_DEBUG("Upload ID %u", *upload_id);
	DLEYNA_LOG_DEBUG("Object ID %s", object_id);
	DLEYNA_LOG_DEBUG("Object Path %s", object_path);

	out_p[0] = g_variant_new_uint32(*upload_id);
	out_p[1] = g_variant_new_object_path(object_path);
	cb_data->task.result = g_variant_ref_sink(g_variant_new_tuple(out_p,
								      2));

	++cb_data->task.target.device->upload_id;
	if (cb_data->task.target.device->upload_id > G_MAXINT)
		cb_data->task.target.device->upload_id = 0;

	g_free(object_path);

on_error:

	if (cb_data->error && delete_needed) {
		DLEYNA_LOG_WARNING(
			"Upload failed deleting created object with id %s",
			object_id);

		cb_data->action = gupnp_service_proxy_begin_action(
					cb_data->proxy, "DestroyObject",
					prv_upload_delete_cb, cb_data,
					"ObjectID", G_TYPE_STRING, object_id,
					NULL);
	} else {
		(void) g_idle_add(dls_async_task_complete, cb_data);
		g_cancellable_disconnect(cb_data->cancellable,
					 cb_data->cancel_id);
	}

	g_free(object_id);
	g_free(import_uri);

	if (parser)
		g_object_unref(parser);

	g_free(result);

	if (error)
		g_error_free(error);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_create_object_upload_cb(GUPnPServiceProxy *proxy,
					GUPnPServiceProxyAction *action,
					gpointer user_data)
{
	dls_async_task_t *cb_data = user_data;

	prv_generic_upload_cb(cb_data,
			      cb_data->task.ut.upload.file_path,
			      NULL, 0,
			      cb_data->ut.upload.mime_type);
}

void dls_device_upload(dls_client_t *client,
		       dls_task_t *task, const gchar *parent_id)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_device_context_t *context;
	gchar *didl;
	dls_async_upload_t *cb_task_data;

	DLEYNA_LOG_DEBUG("Enter");
	DLEYNA_LOG_DEBUG("Uploading file to %s", parent_id);

	context = dls_device_get_context(task->target.device, client);
	cb_task_data = &cb_data->ut.upload;

	didl = prv_create_upload_didl(parent_id, task,
				      cb_task_data->object_class,
				      cb_task_data->mime_type);

	DLEYNA_LOG_DEBUG_NL();
	DLEYNA_LOG_DEBUG("DIDL: %s", didl);
	DLEYNA_LOG_DEBUG_NL();

	cb_data->action = gupnp_service_proxy_begin_action(
					context->service_proxy, "CreateObject",
					prv_create_object_upload_cb, cb_data,
					"ContainerID", G_TYPE_STRING, parent_id,
					"Elements", G_TYPE_STRING, didl,
					NULL);

	cb_data->proxy = context->service_proxy;

	g_object_add_weak_pointer((G_OBJECT(context->service_proxy)),
				  (gpointer *)&cb_data->proxy);

	cb_data->cancel_id = g_cancellable_connect(
					cb_data->cancellable,
					G_CALLBACK(dls_async_task_cancelled_cb),
					cb_data, NULL);

	g_free(didl);

	DLEYNA_LOG_DEBUG("Exit");
}

gboolean dls_device_get_upload_status(dls_task_t *task, GError **error)
{
	dls_device_upload_t *upload;
	gboolean retval = FALSE;
	GVariant *out_params[3];
	guint upload_id;

	DLEYNA_LOG_DEBUG("Enter");

	upload_id = task->ut.upload_action.upload_id;

	upload = g_hash_table_lookup(task->target.device->uploads, &upload_id);
	if (!upload) {
		*error = g_error_new(DLEYNA_SERVER_ERROR,
				     DLEYNA_ERROR_OBJECT_NOT_FOUND,
				     "Unknown Upload ID %u ", upload_id);
		goto on_error;
	}

	out_params[0] = g_variant_new_string(upload->status);
	out_params[1] = g_variant_new_uint64(upload->bytes_uploaded);
	out_params[2] = g_variant_new_uint64(upload->bytes_to_upload);

	DLEYNA_LOG_DEBUG(
		"Upload ( %s %"G_GUINT64_FORMAT" %"G_GUINT64_FORMAT" )",
		upload->status, upload->bytes_uploaded,
		upload->bytes_to_upload);

	task->result = g_variant_ref_sink(g_variant_new_tuple(out_params, 3));

	retval = TRUE;

on_error:

	DLEYNA_LOG_DEBUG("Exit");

	return retval;
}

void dls_device_get_upload_ids(dls_task_t *task)
{
	GVariantBuilder vb;
	GHashTableIter iter;
	gpointer key;

	DLEYNA_LOG_DEBUG("Enter");

	g_variant_builder_init(&vb, G_VARIANT_TYPE("au"));

	g_hash_table_iter_init(&iter, task->target.device->uploads);
	while (g_hash_table_iter_next(&iter, &key, NULL))
		g_variant_builder_add(&vb, "u", (guint32) (*((gint *)key)));

	task->result = g_variant_ref_sink(g_variant_builder_end(&vb));

	DLEYNA_LOG_DEBUG("Exit");
}

gboolean dls_device_cancel_upload(dls_task_t *task, GError **error)
{
	dls_device_upload_t *upload;
	gboolean retval = FALSE;
	guint upload_id;

	DLEYNA_LOG_DEBUG("Enter");

	upload_id = task->ut.upload_action.upload_id;

	upload = g_hash_table_lookup(task->target.device->uploads, &upload_id);
	if (!upload) {
		*error = g_error_new(DLEYNA_SERVER_ERROR,
				     DLEYNA_ERROR_OBJECT_NOT_FOUND,
				     "Unknown Upload ID %u ", upload_id);
		goto on_error;
	}

	if (upload->msg) {
		soup_session_cancel_message(upload->soup_session, upload->msg,
					    SOUP_STATUS_CANCELLED);
		DLEYNA_LOG_DEBUG("Cancelling Upload %u ", upload_id);
	}

	retval = TRUE;

on_error:

	DLEYNA_LOG_DEBUG("Exit");

	return retval;
}

static void prv_destroy_object_cb(GUPnPServiceProxy *proxy,
				  GUPnPServiceProxyAction *action,
				  gpointer user_data)
{
	GError *upnp_error = NULL;
	dls_async_task_t *cb_data = user_data;

	DLEYNA_LOG_DEBUG("Enter");

	if (!gupnp_service_proxy_end_action(cb_data->proxy, cb_data->action,
					    &upnp_error,
					    NULL)) {
		DLEYNA_LOG_WARNING("Destroy Object operation failed: %s",
				   upnp_error->message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Destroy Object operation failed: %s",
					     upnp_error->message);
	}

	(void) g_idle_add(dls_async_task_complete, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);

	if (upnp_error)
		g_error_free(upnp_error);

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_device_delete_object(dls_client_t *client,
			      dls_task_t *task)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_device_context_t *context;

	DLEYNA_LOG_DEBUG("Enter");

	context = dls_device_get_context(task->target.device, client);

	cb_data->action = gupnp_service_proxy_begin_action(
				context->service_proxy, "DestroyObject",
				prv_destroy_object_cb, cb_data,
				"ObjectID", G_TYPE_STRING, task->target.id,
				NULL);

	cb_data->proxy = context->service_proxy;

	g_object_add_weak_pointer((G_OBJECT(context->service_proxy)),
				  (gpointer *)&cb_data->proxy);

	cb_data->cancel_id = g_cancellable_connect(cb_data->cancellable,
					G_CALLBACK(dls_async_task_cancelled_cb),
					cb_data, NULL);

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_device_create_container(dls_client_t *client,
				 dls_task_t *task,
				 const gchar *parent_id)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_device_context_t *context;
	gchar *didl;

	DLEYNA_LOG_DEBUG("Enter");

	context = dls_device_get_context(task->target.device, client);

	didl = prv_create_new_container_didl(parent_id, task);
	if (!didl) {
		DLEYNA_LOG_WARNING("Unable to create didl");

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Unable to create didl");
		goto on_error;
	}

	DLEYNA_LOG_DEBUG("DIDL: %s", didl);

	cb_data->action = gupnp_service_proxy_begin_action(
					context->service_proxy, "CreateObject",
					prv_create_container_cb, cb_data,
					"ContainerID", G_TYPE_STRING, parent_id,
					"Elements", G_TYPE_STRING, didl,
					NULL);

	cb_data->proxy = context->service_proxy;

	g_object_add_weak_pointer((G_OBJECT(context->service_proxy)),
				  (gpointer *)&cb_data->proxy);

	cb_data->cancel_id = g_cancellable_connect(
					cb_data->cancellable,
					G_CALLBACK(dls_async_task_cancelled_cb),
					cb_data, NULL);

	g_free(didl);

on_error:

	DLEYNA_LOG_DEBUG("Exit");

	return;
}

static void prv_update_object_update_cb(GUPnPServiceProxy *proxy,
					GUPnPServiceProxyAction *action,
					gpointer user_data)
{
	GError *upnp_error = NULL;
	dls_async_task_t *cb_data = user_data;

	DLEYNA_LOG_DEBUG("Enter");

	if (!gupnp_service_proxy_end_action(cb_data->proxy, cb_data->action,
					    &upnp_error,
					    NULL)) {
		DLEYNA_LOG_WARNING("Update Object operation failed: %s",
				   upnp_error->message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Update Object operation "
					     " failed: %s",
					     upnp_error->message);
	}

	(void) g_idle_add(dls_async_task_complete, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);

	if (upnp_error)
		g_error_free(upnp_error);

	DLEYNA_LOG_DEBUG("Exit");
}

static gchar *prv_get_current_xml_fragment(GUPnPDIDLLiteObject *object,
					   dls_upnp_prop_mask mask)
{
	gchar *retval = NULL;

	if (mask & DLS_UPNP_MASK_PROP_DISPLAY_NAME)
		retval = gupnp_didl_lite_object_get_title_xml_string(object);
	else if (mask & DLS_UPNP_MASK_PROP_ALBUM)
		retval = gupnp_didl_lite_object_get_album_xml_string(object);
	else if (mask & DLS_UPNP_MASK_PROP_DATE)
		retval = gupnp_didl_lite_object_get_date_xml_string(object);
	else if (mask & DLS_UPNP_MASK_PROP_TYPE_EX)
		retval = gupnp_didl_lite_object_get_upnp_class_xml_string(
			object);
	else if (mask & DLS_UPNP_MASK_PROP_TRACK_NUMBER)
		retval = gupnp_didl_lite_object_get_track_number_xml_string(
			object);
	else if (mask & DLS_UPNP_MASK_PROP_ARTISTS)
		retval = gupnp_didl_lite_object_get_artists_xml_string(object);

	return retval;
}

static gchar *prv_get_new_xml_fragment(GUPnPDIDLLiteObject *object,
				       dls_upnp_prop_mask mask,
				       GVariant *value)
{
	GUPnPDIDLLiteContributor *artist;
	const gchar *artist_name;
	gchar *upnp_class;
	GVariantIter viter;
	gchar *retval = NULL;

	if (mask & DLS_UPNP_MASK_PROP_DISPLAY_NAME) {
		gupnp_didl_lite_object_set_title(
					object,
					g_variant_get_string(value, NULL));

		retval = gupnp_didl_lite_object_get_title_xml_string(object);
	} else if (mask & DLS_UPNP_MASK_PROP_ALBUM) {
		gupnp_didl_lite_object_set_album(
					object,
					g_variant_get_string(value, NULL));

		retval = gupnp_didl_lite_object_get_album_xml_string(object);
	} else if (mask & DLS_UPNP_MASK_PROP_DATE) {
		gupnp_didl_lite_object_set_date(
					object,
					g_variant_get_string(value, NULL));

		retval = gupnp_didl_lite_object_get_date_xml_string(object);
	} else if (mask & DLS_UPNP_MASK_PROP_TYPE_EX) {
		upnp_class = dls_props_media_spec_ex_to_upnp_class(
			g_variant_get_string(value, NULL));
		if (!upnp_class)
			goto on_error;

		gupnp_didl_lite_object_set_upnp_class(object, upnp_class);
		g_free(upnp_class);

		retval = gupnp_didl_lite_object_get_upnp_class_xml_string(
			object);
	} else if (mask & DLS_UPNP_MASK_PROP_TRACK_NUMBER) {
		gupnp_didl_lite_object_set_track_number(
						object,
						g_variant_get_int32(value));

		retval = gupnp_didl_lite_object_get_track_number_xml_string(
			object);
	} else if (mask & DLS_UPNP_MASK_PROP_ARTISTS) {
		gupnp_didl_lite_object_unset_artists(object);

		(void) g_variant_iter_init(&viter, value);

		while (g_variant_iter_next(&viter, "&s", &artist_name)) {
			artist = gupnp_didl_lite_object_add_artist(object);

			gupnp_didl_lite_contributor_set_name(artist,
							     artist_name);
		}

		retval = gupnp_didl_lite_object_get_artists_xml_string(object);
	}

on_error:

	return retval;
}

static void prv_get_xml_fragments(GUPnPDIDLLiteParser *parser,
				  GUPnPDIDLLiteObject *object,
				  gpointer user_data)
{
	GString *current_str;
	GString *new_str;
	gchar *frag1;
	gchar *frag2;
	GVariantIter viter;
	const gchar *prop;
	GVariant *value;
	dls_prop_map_t *prop_map;
	GUPnPDIDLLiteWriter *writer;
	GUPnPDIDLLiteObject *scratch_object;
	gboolean first = TRUE;
	dls_async_task_t *cb_data = user_data;
	dls_async_update_t *cb_task_data = &cb_data->ut.update;
	dls_task_t *task = &cb_data->task;
	dls_task_update_t *task_data = &task->ut.update;

	DLEYNA_LOG_DEBUG("Enter");

	current_str = g_string_new("");
	new_str = g_string_new("");

	writer = gupnp_didl_lite_writer_new(NULL);
	if (GUPNP_IS_DIDL_LITE_CONTAINER(object))
		scratch_object = GUPNP_DIDL_LITE_OBJECT(
			gupnp_didl_lite_writer_add_container(writer));
	else
		scratch_object = GUPNP_DIDL_LITE_OBJECT(
			gupnp_didl_lite_writer_add_item(writer));

	(void) g_variant_iter_init(&viter, task_data->to_add_update);

	while (g_variant_iter_next(&viter, "{&sv}", &prop, &value)) {
		DLEYNA_LOG_DEBUG("to_add_update = %s", prop);

		prop_map = g_hash_table_lookup(cb_task_data->map, prop);

		frag1 = prv_get_current_xml_fragment(object, prop_map->type);
		frag2 = prv_get_new_xml_fragment(scratch_object, prop_map->type,
						 value);

		if (!frag2) {
			DLEYNA_LOG_DEBUG("Unable to set %s.  Skipping", prop);

			g_free(frag1);
			continue;
		}

		if (!first) {
			g_string_append(current_str, ",");
			g_string_append(new_str, ",");
		} else {
			first = FALSE;
		}

		if (frag1) {
			g_string_append(current_str, (const gchar *)frag1);
			g_free(frag1);
		}

		g_string_append(new_str, (const gchar *)frag2);
		g_free(frag2);
	}

	(void) g_variant_iter_init(&viter, task_data->to_delete);

	while (g_variant_iter_next(&viter, "&s", &prop)) {
		DLEYNA_LOG_DEBUG("to_delete = %s", prop);

		prop_map = g_hash_table_lookup(cb_task_data->map, prop);

		frag1 = prv_get_current_xml_fragment(object, prop_map->type);
		if (!frag1)
			continue;

		if (!first)
			g_string_append(current_str, ",");
		else
			first = FALSE;

		g_string_append(current_str, (const gchar *)frag1);
		g_free(frag1);
	}

	cb_task_data->current_tag_value = g_string_free(current_str, FALSE);
	DLEYNA_LOG_DEBUG("current_tag_value = %s",
			 cb_task_data->current_tag_value);

	cb_task_data->new_tag_value = g_string_free(new_str, FALSE);
	DLEYNA_LOG_DEBUG("new_tag_value = %s", cb_task_data->new_tag_value);

	g_object_unref(scratch_object);
	g_object_unref(writer);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_update_object_browse_cb(GUPnPServiceProxy *proxy,
					GUPnPServiceProxyAction *action,
					gpointer user_data)
{
	GError *error = NULL;
	dls_async_task_t *cb_data = user_data;
	dls_async_update_t *cb_task_data = &cb_data->ut.update;
	GUPnPDIDLLiteParser *parser = NULL;
	gchar *result = NULL;
	const gchar *message;
	gboolean end;

	DLEYNA_LOG_DEBUG("Enter");

	end = gupnp_service_proxy_end_action(cb_data->proxy, cb_data->action,
					     &error,
					     "Result", G_TYPE_STRING, &result,
					     NULL);

	if (!end || (result == NULL)) {
		message = (error != NULL) ? error->message : "Invalid result";
		DLEYNA_LOG_WARNING("Browse Object operation failed: %s",
				   message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Browse operation failed: %s",
					     message);
		goto on_error;
	}

	DLEYNA_LOG_DEBUG("dls_device_update_ex_object result: %s", result);

	parser = gupnp_didl_lite_parser_new();

	g_signal_connect(parser, "object-available",
			 G_CALLBACK(prv_get_xml_fragments),
			 cb_data);

	if (!gupnp_didl_lite_parser_parse_didl(parser, result, &error)) {
		if (error->code == GUPNP_XML_ERROR_EMPTY_NODE) {
			DLEYNA_LOG_WARNING("Property not defined for object");

			cb_data->error =
				g_error_new(DLEYNA_SERVER_ERROR,
					    DLEYNA_ERROR_UNKNOWN_PROPERTY,
					    "Property not defined for object");
		} else {
			DLEYNA_LOG_WARNING(
					"Unable to parse results of browse: %s",
					error->message);

			cb_data->error =
				g_error_new(DLEYNA_SERVER_ERROR,
					    DLEYNA_ERROR_OPERATION_FAILED,
					    "Unable to parse results of browse: %s",
					    error->message);
		}

		goto on_error;
	}

	cb_data->action = gupnp_service_proxy_begin_action(
		cb_data->proxy, "UpdateObject",
		prv_update_object_update_cb, cb_data,
		"ObjectID", G_TYPE_STRING, cb_data->task.target.id,
		"CurrentTagValue", G_TYPE_STRING,
		cb_task_data->current_tag_value,
		"NewTagValue", G_TYPE_STRING, cb_task_data->new_tag_value,
		NULL);

	goto no_complete;

on_error:

	(void) g_idle_add(dls_async_task_complete, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);

no_complete:

	if (parser)
		g_object_unref(parser);

	g_free(result);

	if (error)
		g_error_free(error);

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_device_update_object(dls_client_t *client,
			      dls_task_t *task,
			      const gchar *upnp_filter)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_device_context_t *context;

	DLEYNA_LOG_DEBUG("Enter");

	context = dls_device_get_context(task->target.device, client);

	cb_data->action = gupnp_service_proxy_begin_action(
				context->service_proxy, "Browse",
				prv_update_object_browse_cb, cb_data,
				"ObjectID", G_TYPE_STRING, task->target.id,
				"BrowseFlag", G_TYPE_STRING, "BrowseMetadata",
				"Filter", G_TYPE_STRING, upnp_filter,
				"StartingIndex", G_TYPE_INT, 0,
				"RequestedCount", G_TYPE_INT, 0,
				"SortCriteria", G_TYPE_STRING,
				"", NULL);

	cb_data->proxy = context->service_proxy;

	g_object_add_weak_pointer((G_OBJECT(context->service_proxy)),
				  (gpointer *)&cb_data->proxy);

	cb_data->cancel_id = g_cancellable_connect(cb_data->cancellable,
					G_CALLBACK(dls_async_task_cancelled_cb),
					cb_data, NULL);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_get_object_metadata_cb(GUPnPServiceProxy *proxy,
				       GUPnPServiceProxyAction *action,
				       gpointer user_data)
{
	GError *error = NULL;
	dls_async_task_t *cb_data = user_data;
	gchar *result = NULL;
	const gchar *message;
	gboolean end;

	DLEYNA_LOG_DEBUG("Enter");

	end = gupnp_service_proxy_end_action(cb_data->proxy, cb_data->action,
					     &error,
					     "Result", G_TYPE_STRING, &result,
					     NULL);

	if (!end || (result == NULL)) {
		message = (error != NULL) ? error->message : "Invalid result";
		DLEYNA_LOG_WARNING("Browse Object operation failed: %s",
				   message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Browse operation failed: %s",
					     message);
		goto on_complete;
	}

	cb_data->task.result = g_variant_ref_sink(g_variant_new_string(result));

	DLEYNA_LOG_DEBUG("prv_get_object_metadata_cb result: %s", result);

	g_free(result);

on_complete:

	(void) g_idle_add(dls_async_task_complete, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);

	if (error)
		g_error_free(error);

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_device_get_object_metadata(dls_client_t *client,
				    dls_task_t *task,
				    const gchar *upnp_filter)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_device_context_t *context;

	DLEYNA_LOG_DEBUG("Enter");

	context = dls_device_get_context(task->target.device, client);

	cb_data->action = gupnp_service_proxy_begin_action(
				context->service_proxy, "Browse",
				prv_get_object_metadata_cb, cb_data,
				"ObjectID", G_TYPE_STRING, task->target.id,
				"BrowseFlag", G_TYPE_STRING, "BrowseMetadata",
				"Filter", G_TYPE_STRING, "*",
				"StartingIndex", G_TYPE_INT, 0,
				"RequestedCount", G_TYPE_INT, 0,
				"SortCriteria", G_TYPE_STRING, "",
				NULL);

	cb_data->proxy = context->service_proxy;

	g_object_add_weak_pointer((G_OBJECT(context->service_proxy)),
				  (gpointer *)&cb_data->proxy);

	cb_data->cancel_id = g_cancellable_connect(cb_data->cancellable,
					G_CALLBACK(dls_async_task_cancelled_cb),
					cb_data, NULL);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_create_reference_cb(GUPnPServiceProxy *proxy,
				    GUPnPServiceProxyAction *action,
				    gpointer user_data)
{
	GError *error = NULL;
	dls_async_task_t *cb_data = user_data;
	const gchar *message;
	gchar *object_id = NULL;
	gchar *object_path;
	gboolean end;

	DLEYNA_LOG_DEBUG("Enter");

	end = gupnp_service_proxy_end_action(cb_data->proxy, cb_data->action,
					    &error,
					    "NewID", G_TYPE_STRING, &object_id,
					    NULL);
	if (!end || (object_id == NULL)) {
		message = (error != NULL) ? error->message : "Invalid result";
		DLEYNA_LOG_WARNING("CreateReference operation failed: %s",
				   message);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Update Object operation "
					     " failed: %s",
					     message);

		goto on_error;
	}

	object_path = dls_path_from_id(cb_data->task.target.root_path,
				       object_id);

	DLEYNA_LOG_DEBUG("Result @id: %s - Path: %s", object_id, object_path);

	cb_data->task.result = g_variant_ref_sink(g_variant_new_object_path(
								object_path));
	g_free(object_path);

on_error:

	(void) g_idle_add(dls_async_task_complete, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);

	g_free(object_id);

	if (error != NULL)
		g_error_free(error);

	DLEYNA_LOG_DEBUG("Exit");
}

void dls_device_create_reference(dls_client_t *client,
				 dls_task_t *task)
{
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_device_context_t *context;
	gchar *i_root = NULL;
	gchar *i_id = NULL;
	gchar *path = cb_data->task.ut.create_reference.item_path;

	DLEYNA_LOG_DEBUG("Enter");
	DLEYNA_LOG_DEBUG("Create Reference for path: %s", path);

	if (!dls_path_get_path_and_id(path, &i_root, &i_id, NULL)) {
		DLEYNA_LOG_DEBUG("Can't get id for path %s", path);
		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OBJECT_NOT_FOUND,
					     "Unable to find object for path: %s",
					     path);
		goto on_error;
	}

	DLEYNA_LOG_DEBUG("Create Reference for @id: %s", i_id);
	DLEYNA_LOG_DEBUG("        In Container @id: %s", task->target.id);

	context = dls_device_get_context(task->target.device, client);

	cb_data->action = gupnp_service_proxy_begin_action(
				context->service_proxy, "CreateReference",
				prv_create_reference_cb, cb_data,
				"ContainerID", G_TYPE_STRING, task->target.id,
				"ObjectID", G_TYPE_STRING, i_id,
				NULL);

	cb_data->proxy = context->service_proxy;

	g_object_add_weak_pointer((G_OBJECT(context->service_proxy)),
				  (gpointer *)&cb_data->proxy);

	cb_data->cancel_id = g_cancellable_connect(
					cb_data->cancellable,
					G_CALLBACK(dls_async_task_cancelled_cb),
					cb_data, NULL);

on_error:

	g_free(i_root);
	g_free(i_id);

	DLEYNA_LOG_DEBUG("Exit");
}

static void prv_build_icon_result(dls_device_t *device, dls_task_t *task)
{
	GVariant *out_p[2];

	out_p[0] = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,
					     device->icon.bytes,
					     device->icon.size,
					     1);
	out_p[1] = g_variant_new_string(device->icon.mime_type);
	task->result = g_variant_ref_sink(g_variant_new_tuple(out_p, 2));
}

static void prv_get_icon_cancelled(GCancellable *cancellable,
				   gpointer user_data)
{
	dls_device_download_t *download = (dls_device_download_t *)user_data;

	dls_async_task_cancelled_cb(cancellable, download->task);

	if (download->msg) {
		soup_session_cancel_message(download->session, download->msg,
					    SOUP_STATUS_CANCELLED);
		DLEYNA_LOG_DEBUG("Cancelling device icon download");
	}
}

static void prv_free_download_info(dls_device_download_t *download)
{
	if (download->msg)
		g_object_unref(download->msg);
	g_object_unref(download->session);
	g_free(download);
}

static void prv_get_icon_session_cb(SoupSession *session,
				    SoupMessage *msg,
				    gpointer user_data)
{
	dls_device_download_t *download = (dls_device_download_t *)user_data;
	dls_async_task_t *cb_data = (dls_async_task_t *)download->task;
	dls_device_t *device = (dls_device_t *)cb_data->task.target.device;

	if (msg->status_code == SOUP_STATUS_CANCELLED)
		goto out;

	if (SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
		device->icon.size = msg->response_body->length;
		device->icon.bytes = g_malloc(device->icon.size);
		memcpy(device->icon.bytes, msg->response_body->data,
		       device->icon.size);

		prv_build_icon_result(device, &cb_data->task);
	} else {
		DLEYNA_LOG_DEBUG("Failed to GET device icon: %s",
				 msg->reason_phrase);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_OPERATION_FAILED,
					     "Failed to GET device icon");
	}

	(void) g_idle_add(dls_async_task_complete, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);

out:

	prv_free_download_info(download);
}

void dls_device_get_icon(dls_client_t *client,
			 dls_task_t *task)
{
	GUPnPDeviceInfo *info;
	dls_device_context_t *context;
	dls_async_task_t *cb_data = (dls_async_task_t *)task;
	dls_device_t *device = task->target.device;
	gchar *url;
	dls_device_download_t *download;

	if (device->icon.size != 0) {
		prv_build_icon_result(device, task);
		goto end;
	}

	context = dls_device_get_context(device, client);
	info = (GUPnPDeviceInfo *)context->device_proxy;

	url = gupnp_device_info_get_icon_url(info, NULL, -1, -1, -1, FALSE,
					     &device->icon.mime_type, NULL,
					     NULL, NULL);
	if (url == NULL) {
		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_NOT_SUPPORTED,
					     "No icon available");
		goto end;
	}

	download = g_new0(dls_device_download_t, 1);
	download->session = soup_session_async_new();
	download->msg = soup_message_new(SOUP_METHOD_GET, url);
	download->task = cb_data;

	if (!download->msg) {
		DLEYNA_LOG_WARNING("Invalid URL %s", url);

		cb_data->error = g_error_new(DLEYNA_SERVER_ERROR,
					     DLEYNA_ERROR_BAD_RESULT,
					     "Invalid URL %s", url);
		prv_free_download_info(download);
		g_free(url);

		goto end;
	}

	cb_data->cancel_id =
		g_cancellable_connect(cb_data->cancellable,
				      G_CALLBACK(prv_get_icon_cancelled),
				      download, NULL);

	g_object_ref(download->msg);
	soup_session_queue_message(download->session, download->msg,
				   prv_get_icon_session_cb, download);

	g_free(url);

	return;

end:

	(void) g_idle_add(dls_async_task_complete, cb_data);
}
