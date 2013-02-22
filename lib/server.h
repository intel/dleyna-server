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

#ifndef SERVER_H__
#define SERVER_H__

#include <libdleyna/core/connector.h>
#include <libdleyna/core/task-processor.h>

#define DLEYNA_SERVER_SINK "dleyna-server"

typedef struct msu_device_t_ msu_device_t;
typedef struct msu_device_context_t_ msu_device_context_t;
typedef struct msu_upnp_t_ msu_upnp_t;

gboolean dls_server_get_object_info(const gchar *object_path,
				    gchar **root_path,
				    gchar **object_id,
				    msu_device_t **device,
				    GError **error);
msu_upnp_t *dls_server_get_upnp(void);
dleyna_task_processor_t *dls_server_get_task_processor(void);
const dleyna_connector_t *dls_server_get_connector(void);

#endif /* SERVER_H__ */
