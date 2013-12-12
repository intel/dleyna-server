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
 * Christophe Guiraud <christophe.guiraud@intel.com>
 *
 */

#include <string.h>
#include <stdlib.h>

#include "xml-util.h"

static xmlNode *prv_get_child_node(xmlNode *node, va_list args)
{
	const gchar *name;

	name = va_arg(args, const gchar *);
	while (name != NULL) {
		node = node->children;
		while (node != NULL) {
			if (node->name != NULL &&
			    !strcmp(name, (char *)node->name))
				break;

			node = node->next;
		}

		if (node == NULL)
			break;

		name = va_arg(args, const gchar *);
	}

	return node;
}

static GList *prv_get_children_list(xmlNode *node, const gchar *name)
{
	GList *child_list = NULL;

	node = node->children;
	while (node != NULL) {
		if (node->name != NULL &&
		    !strcmp(name, (char *)node->name))
			child_list = g_list_prepend(child_list, node);

		node = node->next;
	}

	return child_list;
}

GList *xml_util_get_child_string_list_content_by_name(xmlNode *node, ...)
{
	xmlChar *content;
	va_list args;
	GList *child_list = NULL;
	GList *next;
	GList *str_list = NULL;
	xmlNode *child_list_node;
	xmlNode *child_node;

	va_start(args, node);

	child_node = prv_get_child_node(node, args);

	va_end(args);

	if (child_node != NULL) {
		child_list = prv_get_children_list(child_node->parent,
					(const gchar *)child_node->name);
		next = child_list;
		while (next) {
			child_list_node = (xmlNode *)next->data;

			content = xmlNodeGetContent(child_list_node);

			if (content != NULL) {
				str_list = g_list_prepend(str_list,
						  g_strdup((gchar *)content));

				xmlFree(content);
			}

			next = g_list_next(next);
		}

		g_list_free(child_list);
	}

	return str_list;
}

gchar *xml_util_get_child_string_content_by_name(xmlNode *node, ...)
{
	xmlChar *content;
	va_list args;
	gchar *str = NULL;
	xmlNode *child_node;

	va_start(args, node);

	child_node = prv_get_child_node(node, args);

	va_end(args);

	if (child_node != NULL) {
		content = xmlNodeGetContent(child_node);

		if (content != NULL) {
			str = g_strdup((gchar *)content);

			xmlFree(content);
		}
	}

	return str;
}
