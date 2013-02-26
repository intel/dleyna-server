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
 * Ludovic Ferrandis <ludovic.ferrandis@intel.com>
 * Regis Merlino <regis.merlino@intel.com>
 *
 */

#include <libdleyna/core/log.h>
#include <libdleyna/core/task-processor.h>

#include "device.h"
#include "service-task.h"

struct dls_service_task_t_ {
	dleyna_task_atom_t base; /* pseudo inheritance - MUST be first field */
	GUPnPServiceProxyActionCallback callback;
	GUPnPServiceProxyAction *p_action;
	GDestroyNotify free_func;
	gpointer user_data;
	dls_service_task_action t_action;
	dls_device_t *device;
	GUPnPServiceProxy *proxy;
};

const char *dls_service_task_create_source(void)
{
	static unsigned int cpt = 1;
	static char source[20];

	g_snprintf(source, 20, "source-%d", cpt);
	cpt++;

	return source;
}

void dls_service_task_add(const dleyna_task_queue_key_t *queue_id,
			  dls_service_task_action action,
			  dls_device_t *device,
			  GUPnPServiceProxy *proxy,
			  GUPnPServiceProxyActionCallback action_cb,
			  GDestroyNotify free_func,
			  gpointer cb_user_data)
{
	dls_service_task_t *task;

	task = g_new0(dls_service_task_t, 1);

	task->t_action = action;
	task->callback = action_cb;
	task->free_func = free_func;
	task->user_data = cb_user_data;
	task->device = device;
	task->proxy = proxy;

	if (proxy != NULL)
		g_object_add_weak_pointer((G_OBJECT(proxy)),
					  (gpointer *)&task->proxy);

	dleyna_task_queue_add_task(queue_id, &task->base);
}

void dls_service_task_begin_action_cb(GUPnPServiceProxy *proxy,
				      GUPnPServiceProxyAction *action,
				      gpointer user_data)
{
	dls_service_task_t *task = (dls_service_task_t *)user_data;

	task->p_action = NULL;
	task->callback(proxy, action, task->user_data);

	dleyna_task_queue_task_completed(task->base.queue_id);
}

void dls_service_task_process_cb(dleyna_task_atom_t *atom, gpointer user_data)
{
	gboolean failed = FALSE;
	dls_service_task_t *task = (dls_service_task_t *)atom;

	task->p_action = task->t_action(task, task->proxy, &failed);

	if (failed)
		dleyna_task_processor_cancel_queue(task->base.queue_id);
	else if (!task->p_action)
		dleyna_task_queue_task_completed(task->base.queue_id);
}

void dls_service_task_cancel_cb(dleyna_task_atom_t *atom, gpointer user_data)
{
	dls_service_task_t *task = (dls_service_task_t *)atom;

	if (task->p_action) {
		if (task->proxy)
			gupnp_service_proxy_cancel_action(task->proxy,
							  task->p_action);
		task->p_action = NULL;

		dleyna_task_queue_task_completed(task->base.queue_id);
	}
}

void dls_service_task_delete_cb(dleyna_task_atom_t *atom, gpointer user_data)
{
	dls_service_task_t *task = (dls_service_task_t *)atom;

	if (task->free_func != NULL)
		task->free_func(task->user_data);

	if (task->proxy != NULL)
		g_object_remove_weak_pointer((G_OBJECT(task->proxy)),
					     (gpointer *)&task->proxy);

	g_free(task);
}

dls_device_t *dls_service_task_get_device(dls_service_task_t *task)
{
	return task->device;
}

gpointer *dls_service_task_get_user_data(dls_service_task_t *task)
{
	return task->user_data;
}
