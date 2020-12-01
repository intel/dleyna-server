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
 * Mark Ryan <mark.d.ryan@intel.com>
 * Regis Merlino <regis.merlino@intel.com>
 *
 */

#include <glib.h>
#include <glib-unix.h>

#include <libdleyna/core/main-loop.h>
#include <libdleyna/server/control-point-server.h>

#define DLS_SERVER_SERVICE_NAME "dleyna-server-service"

static gboolean prv_quit_handler(gpointer user_data)
{
	dleyna_main_loop_quit();

	return FALSE;
}

int main(int argc, char *argv[])
{
	g_unix_signal_add (SIGTERM, prv_quit_handler, NULL);
	g_unix_signal_add (SIGINT, prv_quit_handler, NULL);
	int retval = 1;

	return dleyna_main_loop_start(DLS_SERVER_SERVICE_NAME,
					dleyna_control_point_get_server(),
					NULL);

	return retval;
}
