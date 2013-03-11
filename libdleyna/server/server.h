/*
 * dLeyna
 *
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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
 * Regis Merlino <regis.merlino@intel.com>
 *
 */

#ifndef DLS_SERVER_H__
#define DLS_SERVER_H__

#include <libdleyna/core/connector.h>
#include <libdleyna/core/task-processor.h>

#define DLS_SERVER_SINK "dleyna-server"

typedef struct dls_device_t_ dls_device_t;
typedef struct dls_device_context_t_ dls_device_context_t;
typedef struct dls_upnp_t_ dls_upnp_t;

gboolean dls_server_get_object_info(const gchar *object_path,
				    gchar **root_path,
				    gchar **object_id,
				    dls_device_t **device,
				    GError **error);

dls_upnp_t *dls_server_get_upnp(void);

dleyna_task_processor_t *dls_server_get_task_processor(void);

const dleyna_connector_t *dls_server_get_connector(void);

#endif /* DLS_SERVER_H__ */
