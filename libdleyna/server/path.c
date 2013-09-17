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

#include <stdio.h>
#include <string.h>

#include <libdleyna/core/error.h>

#include "path.h"
#include "server.h"

gboolean dls_path_get_non_root_id(const gchar *object_path,
				  const gchar **slash_before_id)
{
	gboolean retval = FALSE;
	unsigned int offset = strlen(DLEYNA_SERVER_PATH) + 1;

	if (!g_str_has_prefix(object_path, DLEYNA_SERVER_PATH "/"))
		goto on_error;

	if (!object_path[offset])
		goto on_error;

	*slash_before_id = strchr(&object_path[offset], '/');
	retval = TRUE;

on_error:

	return retval;
}

static gchar *prv_object_name_to_id(const gchar *object_name)
{
	gchar *retval = NULL;
	unsigned int object_len = strlen(object_name);
	unsigned int i;
	gint hex;
	gchar byte;

	if (object_len & 1)
		goto on_error;

	retval = g_malloc((object_len >> 1) + 1);

	for (i = 0; i < object_len; i += 2) {
		hex = g_ascii_xdigit_value(object_name[i]);

		if (hex == -1)
			goto on_error;

		byte = hex << 4;
		hex = g_ascii_xdigit_value(object_name[i + 1]);

		if (hex == -1)
			goto on_error;

		byte |= hex;
		retval[i >> 1] = byte;
	}
	retval[i >> 1] = 0;

	return retval;

on_error:

	g_free(retval);

	return NULL;
}

gboolean dls_path_get_path_and_id(const gchar *object_path, gchar **root_path,
				  gchar **id, GError **error)
{
	const gchar *slash;
	gchar *coded_id;

	if (!dls_path_get_non_root_id(object_path, &slash))
		goto on_error;

	if (!slash) {
		*root_path = g_strdup(object_path);
		*id = g_strdup("0");
	} else {
		if (!slash[1])
			goto on_error;

		coded_id = prv_object_name_to_id(slash + 1);

		if (!coded_id)
			goto on_error;

		*root_path = g_strndup(object_path, slash - object_path);
		*id = coded_id;
	}

	return TRUE;

on_error:
	if (error)
		*error = g_error_new(DLEYNA_SERVER_ERROR, DLEYNA_ERROR_BAD_PATH,
				     "object path is badly formed.");

	return FALSE;
}

static gchar *prv_id_to_object_name(const gchar *id)
{
	gchar *retval;
	unsigned int i;
	unsigned int data_len = strlen(id);

	retval = g_malloc((data_len << 1) + 1);
	retval[0] = 0;

	for (i = 0; i < data_len; i++)
		sprintf(&retval[i << 1], "%0x", (guint8) id[i]);

	return retval;
}

gchar *dls_path_from_id(const gchar *root_path, const gchar *id)
{
	gchar *coded_id;
	gchar *path;

	if (!strcmp(id, "0")) {
		path = g_strdup(root_path);
	} else {
		coded_id = prv_id_to_object_name(id);
		path = g_strdup_printf("%s/%s", root_path, coded_id);
		g_free(coded_id);
	}

	return path;
}

char *dls_path_convert_udn_to_path(const char *udn)
{
	char *uuid;
	size_t len;
	size_t dest_len;
	size_t i;

	/* This function will generate a valid dbus path from the udn
	 * We are not going to check the UDN validity. We will try to
	 * convert it anyway. To avoid any security problem, we will
	 * check some limits and possibily return only a partial
	 * UDN. For a better understanding, a valid UDN should be:
	 * UDN = "uuid:4Hex-2Hex-2Hex-Hex-Hex-6Hex"
	 *
	 * The convertion rules are:
	 * 1 - An invalid char will be escaped using its hexa representation
	 *     prefixed with '_': Ex ':' -> '_3A'
	 * 2 - The max size of the converted UDN can be 3 times the original
	 *     size (if all char are not dbus compliant).
	 *     The max size of a dbus path is an UINT32: G_MAXUINT32
	 *     We will limit the of the converted string size to G_MAXUINT32 / 2
	 *     otherwise we will never have space to generate object path.
	 */

	len = strlen(udn);
	dest_len = MIN(len * 3, G_MAXUINT32 / 2);

	uuid = g_malloc(dest_len + 1);
	i = 0;

	while (*udn && (i < dest_len))
	{
		if (g_ascii_isalnum(*udn) || (*udn == '_'))
			uuid[i++] = *udn;
		else
			i += g_snprintf(uuid + i, dest_len + 1,"_%02x", *udn);

		udn++;
	}


	uuid[i]=0;

	return uuid;
}
