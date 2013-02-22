/*
 * dms-info
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
 * Copyright (C) 2012-2013 Intel Corporation. All rights reserved.
 *
 * Mark Ryan <mark.d.ryan@intel.com>
 *
 ******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <stdbool.h>

#include <glib.h>
#include <gio/gio.h>

#define DMS_INFO_SERVICE "com.intel.dleyna-server"
#define DMS_INFO_MANAGER_IF "com.intel.dLeynaServer.Manager"
#define DMS_INFO_MANAGER_OBJ "/com/intel/dLeynaServer"
#define DMS_INFO_GET_SERVERS "GetServers"
#define DMS_INFO_GET_ALL "GetAll"
#define DMS_INFO_PROPERTIES_IF "org.freedesktop.DBus.Properties"

typedef struct dms_info_t_ dms_info_t;
struct dms_info_t_
{
	guint sig_id;
	GMainLoop *main_loop;
	GDBusProxy *manager_proxy;
	GCancellable *cancellable;
	GHashTable *dmss;
	unsigned int async;
};

typedef struct dms_server_data_t_ dms_server_data_t;
struct dms_server_data_t_ {
	GCancellable *cancellable;
	GDBusProxy *proxy;
};

static dms_server_data_t *prv_dms_server_data_new(GDBusProxy *proxy)
{
	dms_server_data_t *data = g_new(dms_server_data_t, 1);
	data->proxy = proxy;
	data->cancellable = g_cancellable_new();
	return data;
}

static void prv_dms_server_data_delete(gpointer user_data)
{
	dms_server_data_t *data = user_data;
	g_object_unref(data->cancellable);
	g_object_unref(data->proxy);
	g_free(data);
}

static void prv_dms_info_free(dms_info_t *info)
{
	if (info->manager_proxy)
		g_object_unref(info->manager_proxy);

	if (info->sig_id)
		(void) g_source_remove(info->sig_id);

	if (info->main_loop)
		g_main_loop_unref(info->main_loop);

	if (info->cancellable)
		g_object_unref(info->cancellable);

	if (info->dmss)
		g_hash_table_unref(info->dmss);
}

static void prv_dump_container_props(GVariant *props)
{
	GVariantIter iter;
	GVariant *dictionary;
	gchar *key;
	GVariant *value;
	gchar *formatted_value;

	dictionary = g_variant_get_child_value(props, 0);
	(void) g_variant_iter_init(&iter, dictionary);

	printf("\n");
	while (g_variant_iter_next(&iter, "{&sv}", &key, &value)) {
		formatted_value = g_variant_print(value, FALSE);
		printf("%s: %s\n", key, formatted_value);
		g_free(formatted_value);
		g_variant_unref(value);
	}
}

static void prv_get_props_cb(GObject *source_object, GAsyncResult *res,
			     gpointer user_data)
{
	GVariant *variant;
	dms_info_t *info = user_data;
	dms_server_data_t *data;
	const gchar *obj_path;

	obj_path =  g_dbus_proxy_get_object_path((GDBusProxy *) source_object);
	data = g_hash_table_lookup(info->dmss, obj_path);

	--info->async;

	if (g_cancellable_is_cancelled(data->cancellable)) {
		if (info->async == 0)
			g_main_loop_quit(info->main_loop);
		printf("Get Properties cancelled.\n");
	} else {
		variant = g_dbus_proxy_call_finish(info->manager_proxy, res,
						   NULL);
		if (!variant) {
			printf("Get Properties failed.\n");
		} else {
			prv_dump_container_props(variant);
			g_variant_unref(variant);
		}
	}
}

static void prv_get_container_props(dms_info_t *info, const gchar *container)
{
	dms_server_data_t *data;
	GDBusProxy *proxy;
	GDBusProxyFlags flags;

	if (!g_hash_table_lookup_extended(info->dmss, container, NULL, NULL)) {
		printf("Container Object Found: %s\n", container);

		/* We'll create these proxies synchronously.  The server
		   is already started and we don't want to retrieve any
		   properties so it should be fast.  Okay, we do want
		   to retrieve properties but we want to do so for all
		   interfaces and not just org.gnome.UPnP.MediaContainer2.
		   To do this we will need to call GetAll("").
		*/

		flags = G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES;
		proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
						      flags, NULL,
						      DMS_INFO_SERVICE,
						      container,
						      DMS_INFO_PROPERTIES_IF,
						      NULL, NULL);
		if (!proxy) {
			printf("Unable to create Container Proxy for %s\n",
			       container);
		} else {
			data = prv_dms_server_data_new(proxy);
			g_hash_table_insert(info->dmss, g_strdup(container),
					    data);
			++info->async;
			g_dbus_proxy_call(proxy, DMS_INFO_GET_ALL,
					  g_variant_new("(s)", ""),
					  G_DBUS_CALL_FLAGS_NONE, -1,
					  data->cancellable,
					  prv_get_props_cb,
					  info);
		}
	}
}

static void prv_get_root_folders(dms_info_t *info, GVariant *servers)
{
	GVariantIter iter;
	GVariant *array;
	const gchar *container;
	gsize count;

	/* Results always seem to be packed inside a tuple.  We need
	   to extract our array from the tuple */

	array = g_variant_get_child_value(servers, 0);
	count = g_variant_iter_init(&iter, array);

	printf("Found %"G_GSIZE_FORMAT" DMS Root Containers\n", count);

	while (g_variant_iter_next(&iter, "o", &container))
		prv_get_container_props(info, container);
}

static void prv_get_servers_cb(GObject *source_object, GAsyncResult *res,
			       gpointer user_data)
{
	GVariant *variant;
	dms_info_t *info = user_data;

	--info->async;

	if (g_cancellable_is_cancelled(info->cancellable)) {
		if (info->async == 0)
			g_main_loop_quit(info->main_loop);
		printf("Get Servers cancelled\n");
	} else {
		variant = g_dbus_proxy_call_finish(info->manager_proxy, res,
						   NULL);
		if (!variant) {
			printf("Get Servers failed.\n");
		} else {
			prv_get_root_folders(info, variant);
			g_variant_unref(variant);
		}
	}
}

static void prv_on_signal(GDBusProxy *proxy, gchar *sender_name,
			  gchar *signal_name, GVariant *parameters,
			  gpointer user_data)
{
	gchar *container;
	dms_info_t *info = user_data;

	if (!strcmp(signal_name, "FoundServer")) {
		g_variant_get(parameters, "(&o)", &container);
		if (!g_hash_table_lookup_extended(info->dmss, container, NULL,
						  NULL)) {
			printf("\nFound DMS %s\n", container);
			prv_get_container_props(info, container);
		}
	} else if (!strcmp(signal_name, "LostServer")) {
		g_variant_get(parameters, "(&o)", &container);
		printf("\nLost DMS %s\n", container);
		(void) g_hash_table_remove(info->dmss, container);
	}
}

static void prv_manager_proxy_created(GObject *source_object,
				      GAsyncResult *result,
				      gpointer user_data)
{
	dms_info_t *info = user_data;
	GDBusProxy *proxy;

	--info->async;

	if (g_cancellable_is_cancelled(info->cancellable)) {
		printf("Manager proxy creation cancelled.\n");
		if (info->async == 0)
			g_main_loop_quit(info->main_loop);
	} else {
		proxy = g_dbus_proxy_new_finish(result, NULL);
		if (!proxy) {
			printf("Unable to create manager proxy.\n");
		} else {
			info->manager_proxy = proxy;

			/* Set up signals to be notified when servers
			   appear and dissapear */

			g_signal_connect(proxy, "g-signal",
					 G_CALLBACK (prv_on_signal), info);

			/* Now we need to retrieve a list of the DMSes on
			   the network.  This involves IPC so we will do
			   this asynchronously.*/

			++info->async;
			g_cancellable_reset(info->cancellable);
			g_dbus_proxy_call(proxy, DMS_INFO_GET_SERVERS,
					  NULL, G_DBUS_CALL_FLAGS_NONE, -1,
					  info->cancellable, prv_get_servers_cb,
					  info);
		}
	}
}

static gboolean prv_quit_handler(GIOChannel *source, GIOCondition condition,
				 gpointer user_data)
{
	dms_info_t *info = user_data;
	GHashTableIter iter;
	gpointer value;
	dms_server_data_t *data;

	/* We cannot quit straight away if asynchronous calls our outstanding.
	   First we need to cancel them.  Each time one is cancel info->async
	   will drop by 1.  Once it reaches 0 the callback function associated
	   with the command being cancelled will quit the mainloop. */

	if (info->async == 0) {
		g_main_loop_quit(info->main_loop);
	} else {
		g_cancellable_cancel(info->cancellable);
		g_hash_table_iter_init(&iter, info->dmss);
		while (g_hash_table_iter_next(&iter, NULL, &value)) {
			data = value;
			g_cancellable_cancel(data->cancellable);
		}
	}

	info->sig_id = 0;

	return FALSE;
}

static bool prv_init_signal_handler(sigset_t mask, dms_info_t *info)
{
	bool retval = false;
	int fd = -1;
	GIOChannel *channel = NULL;

	fd = signalfd(-1, &mask, SFD_NONBLOCK);
	if (fd == -1)
		goto on_error;

	channel = g_io_channel_unix_new(fd);
	g_io_channel_set_close_on_unref(channel, TRUE);

	if (g_io_channel_set_flags(channel, G_IO_FLAG_NONBLOCK, NULL) !=
	    G_IO_STATUS_NORMAL)
		goto on_error;

	if (g_io_channel_set_encoding(channel, NULL, NULL) !=
	    G_IO_STATUS_NORMAL)
		goto on_error;

	info->sig_id = g_io_add_watch(channel, G_IO_IN | G_IO_PRI,
				      prv_quit_handler,
				      info);

	retval = true;

on_error:

	if (channel)
		g_io_channel_unref(channel);

	return retval;
}

int main(int argc, char *argv[])
{
	dms_info_t info;
	sigset_t mask;

	memset(&info, 0, sizeof(info));

	sigemptyset(&mask);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGINT);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
		goto on_error;

	g_type_init();

	/* Create proxy for com.intel.dLeynaServer.Manager.  The Manager
	object has no properties.  We will create the proxy asynchronously
	and use G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES to ensure that
	gio does not contact the server to retrieve remote properties. Creating
	the proxy will force dleyna-media-service be to launched if it is not
	already running. */

	info.cancellable = g_cancellable_new();
	info.async = 1;
	g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION,
				 G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
				 NULL, DMS_INFO_SERVICE, DMS_INFO_MANAGER_OBJ,
				 DMS_INFO_MANAGER_IF, info.cancellable,
				 prv_manager_proxy_created, &info);

	info.dmss = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
					  prv_dms_server_data_delete);

	info.main_loop = g_main_loop_new(NULL, FALSE);

	if (!prv_init_signal_handler(mask, &info))
		goto on_error;

	g_main_loop_run(info.main_loop);

on_error:

	prv_dms_info_free(&info);

	return 0;
}
