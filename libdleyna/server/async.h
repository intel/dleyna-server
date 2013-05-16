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

#ifndef DLS_ASYNC_H__
#define DLS_ASYNC_H__

#include <libgupnp/gupnp-control-point.h>
#include <libgupnp-av/gupnp-media-collection.h>

#include <libdleyna/core/task-atom.h>

#include "server.h"
#include "task.h"
#include "upnp.h"

typedef struct dls_async_task_t_ dls_async_task_t;
typedef guint64 dls_upnp_prop_mask;

typedef void (*dls_async_cb_t)(dls_async_task_t *cb_data);

typedef struct dls_async_bas_t_ dls_async_bas_t;
struct dls_async_bas_t_ {
	dls_upnp_prop_mask filter_mask;
	GPtrArray *vbs;
	const gchar *protocol_info;
	gboolean need_child_count;
	guint retrieved;
	guint max_count;
	dls_async_cb_t get_children_cb;
};

typedef struct dls_async_get_prop_t_ dls_async_get_prop_t;
struct dls_async_get_prop_t_ {
	GCallback prop_func;
	const gchar *protocol_info;
};

typedef struct dls_async_get_all_t_ dls_async_get_all_t;
struct dls_async_get_all_t_ {
	GCallback prop_func;
	GVariantBuilder *vb;
	dls_upnp_prop_mask filter_mask;
	const gchar *protocol_info;
	gboolean need_child_count;
	gboolean device_object;
};

typedef struct dls_async_upload_t_ dls_async_upload_t;
struct dls_async_upload_t_ {
	const gchar *object_class;
	gchar *mime_type;
};

typedef struct dls_async_update_t_ dls_async_update_t;
struct dls_async_update_t_ {
	gchar *current_tag_value;
	gchar *new_tag_value;
	GHashTable *map;
};

struct dls_async_task_t_ {
	dls_task_t task; /* pseudo inheritance - MUST be first field */
	dls_upnp_task_complete_t cb;
	GError *error;
	GUPnPServiceProxyAction *action;
	GUPnPServiceProxy *proxy;
	GCancellable *cancellable;
	gulong cancel_id;
	union {
		dls_async_bas_t bas;
		dls_async_get_prop_t get_prop;
		dls_async_get_all_t get_all;
		dls_async_upload_t upload;
		dls_async_update_t update;
	} ut;
};

void dls_async_task_delete(dls_async_task_t *cb_data);

gboolean dls_async_task_complete(gpointer user_data);

void dls_async_task_cancelled_cb(GCancellable *cancellable, gpointer user_data);

void dls_async_task_cancel(dls_async_task_t *cb_data);

#endif /* DLS_ASYNC_H__ */
