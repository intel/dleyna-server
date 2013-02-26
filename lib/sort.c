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

#include "props.h"
#include "sort.h"

gchar *dls_sort_translate_sort_string(GHashTable *filter_map,
				      const gchar *sort_string)
{
	GRegex *reg;
	gchar *retval = NULL;
	GMatchInfo *match_info = NULL;
	gchar *prop = NULL;
	gchar *op = NULL;
	dls_prop_map_t *prop_map;
	GString *str;

	if (!g_regex_match_simple(
		    "^((\\+|\\-)([^,\\+\\-]+))?(,(\\+|\\-)([^,\\+\\-]+))*$",
		    sort_string, 0, 0))
		goto no_free;

	reg = g_regex_new("(\\+|\\-)(\\w+)", 0, 0, NULL);
	str = g_string_new("");

	g_regex_match(reg, sort_string, 0, &match_info);
	while (g_match_info_matches(match_info)) {
		op = g_match_info_fetch(match_info, 1);
		if (!op)
			goto on_error;

		prop = g_match_info_fetch(match_info, 2);
		if (!prop)
			goto on_error;

		prop_map = g_hash_table_lookup(filter_map, prop);
		if (!prop_map)
			goto on_error;

		if (!prop_map->searchable)
			goto on_error;

		g_string_append_printf(str, "%s%s,", op,
				       prop_map->upnp_prop_name);

		g_free(prop);
		g_free(op);

		prop = NULL;
		op = NULL;

		g_match_info_next(match_info, NULL);
	}

	if (str->len > 0)
		str = g_string_truncate(str, str->len - 1);
	retval = g_string_free(str, FALSE);

	str = NULL;

on_error:

	g_free(prop);
	g_free(op);

	if (match_info)
		g_match_info_free(match_info);

	if (str)
		g_string_free(str, TRUE);

	g_regex_unref(reg);

no_free:

	return retval;
}
