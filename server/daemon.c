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
#include <sys/signalfd.h>

#include <libdleyna/core/main-loop.h>
#include <libdleyna/server/control-point-server.h>

#define DLS_SERVER_SERVICE_NAME "dleyna-server-service"

static guint g_sig_id;

static gboolean prv_quit_handler(GIOChannel *source, GIOCondition condition,
				 gpointer user_data)
{
	dleyna_main_loop_quit();
	g_sig_id = 0;

	return FALSE;
}

static gboolean prv_init_signal_handler(sigset_t mask)
{
	gboolean retval = FALSE;
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

	g_sig_id = g_io_add_watch(channel, G_IO_IN | G_IO_PRI,
				  prv_quit_handler,
				  NULL);

	retval = TRUE;

on_error:

	if (channel)
		g_io_channel_unref(channel);

	return retval;
}

int main(int argc, char *argv[])
{
	sigset_t mask;
	int retval = 1;

	sigemptyset(&mask);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGINT);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
		goto out;

	if (!prv_init_signal_handler(mask))
		goto out;

	retval = dleyna_main_loop_start(DLS_SERVER_SERVICE_NAME,
					dleyna_control_point_get_server(),
					NULL);

out:

	if (g_sig_id)
		(void) g_source_remove(g_sig_id);

	return retval;
}
