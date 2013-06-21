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


#ifndef DLS_XML_UTIL_H__
#define DLS_XML_UTIL_H__

#include <glib.h>
#include <stdarg.h>
#include <libxml/tree.h>

GList *xml_util_get_child_string_list_content_by_name(xmlNode *node, ...);

gchar *xml_util_get_child_string_content_by_name(xmlNode *node, ...);

#endif /* DLS_XML_UTIL_H__ */
