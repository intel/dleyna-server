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

#include "interface.h"
#include "path.h"
#include "props.h"
#include "search.h"

gchar *dls_search_translate_search_string(GHashTable *filter_map,
					  const gchar *search_string)
{
	GRegex *reg;
	gchar *retval = NULL;
	GMatchInfo *match_info = NULL;
	gchar *prop = NULL;
	gchar *op = NULL;
	gchar *value = NULL;
	const gchar *translated_value;
	dls_prop_map_t *prop_map;
	GString *str;
	gint start_pos;
	gint end_pos;
	gint old_end_pos = 0;
	unsigned int skipped;
	unsigned int search_string_len = strlen(search_string);
	gchar *root_path;
	gchar *id;

	reg = g_regex_new("(\\w+)\\s+(=|!=|<|<=|>|>|contains|doesNotContain|"\
			  "derivedfrom|exists)\\s+"\
			  "(\"[^\"]*\"|true|false)",
			  0, 0, NULL);
	str = g_string_new("");

	g_regex_match(reg, search_string, 0, &match_info);
	while (g_match_info_matches(match_info)) {
		prop = g_match_info_fetch(match_info, 1);
		if (!prop)
			goto on_error;

		op = g_match_info_fetch(match_info, 2);
		if (!op)
			goto on_error;

		value = g_match_info_fetch(match_info, 3);
		if (!value)
			goto on_error;

		/* Handle special cases where we need to translate
		   value as well as property name */

		if (!strcmp(prop, DLS_INTERFACE_PROP_TYPE)) {
			/* Skip the quotes */

			value[strlen(value) - 1] = 0;
			translated_value = dls_props_media_spec_to_upnp_class(
				value + 1);
			if (!translated_value)
				goto on_error;
			g_free(value);
			value = g_strdup_printf("\"%s\"", translated_value);
		} else if (!strcmp(prop, DLS_INTERFACE_PROP_PARENT) ||
			   !strcmp(prop, DLS_INTERFACE_PROP_PATH)) {
			value[strlen(value) - 1] = 0;
			if (!dls_path_get_path_and_id(value + 1, &root_path,
						      &id, NULL))
				goto on_error;
			g_free(root_path);
			g_free(value);
			value = g_strdup_printf("\"%s\"", id);
			g_free(id);
		}

		prop_map = g_hash_table_lookup(filter_map, prop);
		if (!prop_map)
			goto on_error;

		if (!prop_map->searchable)
			goto on_error;

		if (!g_match_info_fetch_pos(match_info, 0, &start_pos,
					    &end_pos))
			goto on_error;

		skipped = start_pos - old_end_pos;
		if (skipped > 0)
			g_string_append_len(str, &search_string[old_end_pos],
					    skipped);
		g_string_append_printf(str, "%s %s %s",
				       prop_map->upnp_prop_name, op, value);
		old_end_pos = end_pos;

		g_free(value);
		g_free(prop);
		g_free(op);

		value = NULL;
		prop = NULL;
		op = NULL;

		g_match_info_next(match_info, NULL);
	}

	skipped = search_string_len - old_end_pos;
	if (skipped > 0)
		g_string_append_len(str, &search_string[old_end_pos],
				    skipped);

	retval = g_string_free(str, FALSE);
	str = NULL;

on_error:

	g_free(value);
	g_free(prop);
	g_free(op);

	if (match_info)
		g_match_info_free(match_info);

	if (str)
		g_string_free(str, TRUE);

	g_regex_unref(reg);

	return retval;
}
