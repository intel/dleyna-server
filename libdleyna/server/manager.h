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
 * Ludovic Ferrandis <ludovic.ferrandis@intel.com>
 *
 */

#ifndef DLS_MANAGER_H__
#define DLS_MANAGER_H__

#include <libdleyna/core/connector.h>
#include <libgupnp/gupnp-context-manager.h>

#include "task.h"

typedef struct dls_manager_t_ dls_manager_t;
typedef void (*dls_manager_task_complete_t)(dls_task_t *task, GError *error);

dls_manager_t *dls_manager_new(dleyna_connector_id_t connection,
			       GUPnPContextManager *connection_manager);

void dls_manager_delete(dls_manager_t *manager);

void dls_manager_wl_enable(dls_task_t *task);

void dls_manager_wl_add_entries(dls_task_t *task);

void dls_manager_wl_remove_entries(dls_task_t *task);

void dls_manager_wl_clear(dls_task_t *task);

void dls_manager_get_all_props(dls_manager_t *manager,
			       dls_task_t *task,
			       dls_manager_task_complete_t cb);

void dls_manager_get_prop(dls_manager_t *manager,
			  dls_task_t *task,
			  dls_manager_task_complete_t cb);

#endif /* DLS_MANAGER_H__ */
