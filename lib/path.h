/*
 * dleyna
 *
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef MSU_PATH_H__
#define MSU_PATH_H__

#include <glib.h>

gboolean msu_path_get_non_root_id(const gchar *object_path,
				  const gchar **slash_before_id);
gboolean msu_path_get_path_and_id(const gchar *object_path, gchar **root_path,
				  gchar **id, GError **error);
gchar *msu_path_from_id(const gchar *root_path, const gchar *id);

#endif
