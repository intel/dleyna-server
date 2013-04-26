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
#include <libgupnp-av/gupnp-didl-lite-contributor.h>
#include <libgupnp-av/gupnp-dlna.h>
#include <libgupnp-av/gupnp-protocol-info.h>

#include <libdleyna/core/log.h>

#include "device.h"
#include "interface.h"
#include "path.h"
#include "props.h"
#include "props_def.h"

static const gchar gUPnPContainer[] = "object.container";
static const gchar gUPnPAlbum[] = "object.container.album";
static const gchar gUPnPPerson[] = "object.container.person";
static const gchar gUPnPGenre[] = "object.container.genre";
static const gchar gUPnPAudioItem[] = "object.item.audioItem";
static const gchar gUPnPVideoItem[] = "object.item.videoItem";
static const gchar gUPnPImageItem[] = "object.item.imageItem";
static const gchar gUPnPPlaylistItem[] = "object.item.playlistItem";
static const gchar gUPnPItem[] = "object.item";

static const unsigned int gUPnPContainerLen =
	(sizeof(gUPnPContainer) / sizeof(gchar)) - 1;
static const unsigned int gUPnPAlbumLen =
	(sizeof(gUPnPAlbum) / sizeof(gchar)) - 1;
static const unsigned int gUPnPPersonLen =
	(sizeof(gUPnPPerson) / sizeof(gchar)) - 1;
static const unsigned int gUPnPGenreLen =
	(sizeof(gUPnPGenre) / sizeof(gchar)) - 1;
static const unsigned int gUPnPAudioItemLen =
	(sizeof(gUPnPAudioItem) / sizeof(gchar)) - 1;
static const unsigned int gUPnPVideoItemLen =
	(sizeof(gUPnPVideoItem) / sizeof(gchar)) - 1;
static const unsigned int gUPnPImageItemLen =
	(sizeof(gUPnPImageItem) / sizeof(gchar)) - 1;
static const unsigned int gUPnPPlaylistItemLen =
	(sizeof(gUPnPPlaylistItem) / sizeof(gchar)) - 1;
static const unsigned int gUPnPItemLen =
	(sizeof(gUPnPItem) / sizeof(gchar)) - 1;

static const gchar gUPnPPhotoAlbum[] = "object.container.album.photoAlbum";
static const gchar gUPnPMusicAlbum[] = "object.container.album.musicAlbum";
static const gchar gUPnPMusicArtist[] = "object.container.person.musicArtist";
static const gchar gUPnPMovieGenre[] = "object.container.genre.movieGenre";
static const gchar gUPnPMusicGenre[] = "object.container.genre.musicGenre";
static const gchar gUPnPMusicTrack[] = "object.item.audioItem.musicTrack";
static const gchar gUPnPAudioBroadcast[] =
	"object.item.audioItem.audioBroadcast";
static const gchar gUPnPAudioBook[] = "object.item.audioItem.audioBook";
static const gchar gUPnPMovie[] = "object.item.videoItem.movie";
static const gchar gUPnPMusicVideoClip[] =
	"object.item.videoItem.musicVideoClip";
static const gchar gUPnPVideoBroadcast[] =
	"object.item.videoItem.videoBroadcast";
static const gchar gUPnPPhoto[] = "object.item.imageItem.photo";

static const gchar gMediaSpec2Container[] = "container";
static const gchar gMediaSpec2Album[] = "album";
static const gchar gMediaSpec2AlbumPhoto[] = "album.photo";
static const gchar gMediaSpec2AlbumMusic[] = "album.music";
static const gchar gMediaSpec2Person[] = "person";
static const gchar gMediaSpec2PersonMusicArtist[] = "person.musicartist";
static const gchar gMediaSpec2Genre[] = "genre";
static const gchar gMediaSpec2GenreMovie[] = "genre.movie";
static const gchar gMediaSpec2GenreMusic[] = "genre.music";
static const gchar gMediaSpec2AudioMusic[] = "audio.music";
static const gchar gMediaSpec2AudioBroadcast[] = "audio.broadcast";
static const gchar gMediaSpec2AudioBook[] = "audio.book";
static const gchar gMediaSpec2Audio[] = "audio";
static const gchar gMediaSpec2VideoMovie[] = "video.movie";
static const gchar gMediaSpec2VideoMusicClip[] = "video.musicclip";
static const gchar gMediaSpec2VideoBroadcast[] = "video.broadcast";
static const gchar gMediaSpec2Video[] = "video";
static const gchar gMediaSpec2ImagePhoto[] = "image.photo";
static const gchar gMediaSpec2Image[] = "image";
static const gchar gMediaSpec2Playlist[] = "playlist";
static const gchar gMediaSpec2Item[] = "item";

typedef struct dls_prop_dlna_t_ dls_prop_dlna_t;
struct dls_prop_dlna_t_ {
	guint dlna_flag;
	const gchar *prop_name;
};

static const dls_prop_dlna_t g_prop_dlna_ci[] = {
{GUPNP_DLNA_CONVERSION_TRANSCODED,		"Transcoded"},
{0,						NULL}
};

static const dls_prop_dlna_t g_prop_dlna_op[] = {
{GUPNP_DLNA_OPERATION_RANGE,			"RangeSeek"},
{GUPNP_DLNA_OPERATION_TIMESEEK,			"TimeSeek"},
{0,						NULL}
};

static const dls_prop_dlna_t g_prop_dlna_flags[] = {
{GUPNP_DLNA_FLAGS_SENDER_PACED,			"SenderPaced"},
{GUPNP_DLNA_FLAGS_TIME_BASED_SEEK,		"TimeBased"},
{GUPNP_DLNA_FLAGS_BYTE_BASED_SEEK,		"ByteBased"},
{GUPNP_DLNA_FLAGS_PLAY_CONTAINER,		"PlayContainer"},
{GUPNP_DLNA_FLAGS_S0_INCREASE,			"S0Increase"},
{GUPNP_DLNA_FLAGS_SN_INCREASE,			"SNIncrease"},
{GUPNP_DLNA_FLAGS_RTSP_PAUSE,			"RTSPPause"},
{GUPNP_DLNA_FLAGS_STREAMING_TRANSFER_MODE,	"StreamingTM"},
{GUPNP_DLNA_FLAGS_INTERACTIVE_TRANSFER_MODE,	"InteractiveTM"},
{GUPNP_DLNA_FLAGS_BACKGROUND_TRANSFER_MODE,	"BackgroundTM"},
{GUPNP_DLNA_FLAGS_CONNECTION_STALL,		"ConnectionStall"},
{GUPNP_DLNA_FLAGS_DLNA_V15,			"DLNA_V15"},
{0,						NULL}
};

static const dls_prop_dlna_t g_prop_dlna_ocm[] = {
{GUPNP_OCM_FLAGS_UPLOAD,			"Upload"},
{GUPNP_OCM_FLAGS_CREATE_CONTAINER,		"CreateContainer"},
{GUPNP_OCM_FLAGS_DESTROYABLE,			"Delete"},
{GUPNP_OCM_FLAGS_UPLOAD_DESTROYABLE,		"UploadDelete"},
{GUPNP_OCM_FLAGS_CHANGE_METADATA,		"ChangeMeta"},
{0,						NULL}
};

static void test_prop(dls_upnp_prop_mask filter_mask)
{
	dls_upnp_prop_mask current_mask = (1LL << 0);
	int x = 0;
	dls_prop_map_t *prop_t;

	DLEYNA_LOG_DEBUG("##### ENTER ####");

	prop_t = dls_props_def_get();

	while (filter_mask)
	{
		if (filter_mask & 1) {
			DLEYNA_LOG_DEBUG("##### MASK 0x%lX - %"G_GUINT64_FORMAT" Index %d", current_mask, current_mask, x);
			DLEYNA_LOG_DEBUG("##### PROPNAME %s", prop_t[x].dls_prop_name);
		}

		x++;
		current_mask = current_mask << 1;
		filter_mask = filter_mask >> 1;
	}

	DLEYNA_LOG_DEBUG("##### EXIT ####");
}

void dls_prop_maps_new(GHashTable **property_map, GHashTable **filter_map)
{
	dls_prop_map_t *prop_t;
	GHashTable *p_map;
	GHashTable *f_map;
	int i = 0;

	DLEYNA_LOG_DEBUG("##### ENTER ####");

	prop_t = dls_props_def_get();

	p_map = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	f_map = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);

	while (prop_t[i].upnp_prop_name) {

		g_hash_table_insert(f_map,
				    (gpointer)prop_t[i].dls_prop_name,
				    (gpointer)(prop_t + i));

		if (prop_t[i].searchable)
			g_hash_table_insert(p_map,
					    (gpointer)prop_t[i].upnp_prop_name,
					    (gpointer)prop_t[i].dls_prop_name);

		i++;
	}

	*filter_map = f_map;
	*property_map = p_map;

// 	DLEYNA_LOG_DEBUG_NL();
// 	DLEYNA_LOG_DEBUG("##### DEVICE ####");
// 	test_prop(DLS_PROP_MASK_ALL_DEVICE_PROPS);
// 	DLEYNA_LOG_DEBUG_NL();
// 	DLEYNA_LOG_DEBUG("##### OBKECT ####");
// 	test_prop(DLS_UPNP_MASK_ALL_OBJECT_PROPS);
// 	DLEYNA_LOG_DEBUG_NL();
// 	DLEYNA_LOG_DEBUG("##### ITEM ####");
// 	test_prop(DLS_UPNP_MASK_ALL_ITEM_PROPS);
// 	DLEYNA_LOG_DEBUG_NL();
// 	DLEYNA_LOG_DEBUG("##### CONT ####");
// 	test_prop(DLS_UPNP_MASK_ALL_CONTAINER_PROPS);
// 	DLEYNA_LOG_DEBUG_NL();
}

static gchar *prv_compute_upnp_filter(GHashTable *upnp_props)
{
	gpointer key;
	GString *str;
	GHashTableIter iter;

	str = g_string_new("");
	g_hash_table_iter_init(&iter, upnp_props);
	if (g_hash_table_iter_next(&iter, &key, NULL)) {
		g_string_append(str, (const gchar *)key);
		while (g_hash_table_iter_next(&iter, &key, NULL)) {
			g_string_append(str, ",");
			g_string_append(str, (const gchar *)key);
		}
	}

	return g_string_free(str, FALSE);
}

static dls_upnp_prop_mask prv_parse_filter_list(GHashTable *filter_map,
						GVariant *filter,
						gchar **upnp_filter)
{
	GVariantIter viter;
	const gchar *prop;
	dls_prop_map_t *prop_map;
	GHashTable *upnp_props;
	dls_upnp_prop_mask mask = 0;

	upnp_props = g_hash_table_new_full(g_str_hash, g_str_equal,
					   NULL, NULL);
	(void) g_variant_iter_init(&viter, filter);

	while (g_variant_iter_next(&viter, "&s", &prop)) {
		prop_map = g_hash_table_lookup(filter_map, prop);
		if (!prop_map)
			continue;

		mask |= prop_map->type;

		if (!prop_map->filter)
			continue;

		g_hash_table_insert(upnp_props,
				    (gpointer) prop_map->upnp_prop_name, NULL);
	}

	*upnp_filter = prv_compute_upnp_filter(upnp_props);
	g_hash_table_unref(upnp_props);

	return mask;
}

dls_upnp_prop_mask dls_props_parse_filter(GHashTable *filter_map,
					  GVariant *filter,
					  gchar **upnp_filter)
{
	gchar *str;
	gboolean parse_filter = TRUE;
	dls_upnp_prop_mask mask;

	if (g_variant_n_children(filter) == 1) {
		g_variant_get_child(filter, 0, "&s", &str);
		if (!strcmp(str, "*"))
			parse_filter = FALSE;
	}

	if (parse_filter) {
		mask = prv_parse_filter_list(filter_map, filter, upnp_filter);
	} else {
		mask = DLS_PROP_MASK_ALL_PROPS;
		*upnp_filter = g_strdup("*");
	}

	return mask;
}

gboolean dls_props_parse_update_filter(GHashTable *filter_map,
				       GVariant *to_add_update,
				       GVariant *to_delete,
				       dls_upnp_prop_mask *mask,
				       gchar **upnp_filter)
{
	GVariantIter viter;
	const gchar *prop;
	GVariant *value;
	dls_prop_map_t *prop_map;
	GHashTable *upnp_props;
	gboolean retval = FALSE;

	*mask = 0;

	upnp_props = g_hash_table_new_full(g_str_hash, g_str_equal,
					   NULL, NULL);

	(void) g_variant_iter_init(&viter, to_add_update);

	while (g_variant_iter_next(&viter, "{&sv}", &prop, &value)) {
		DLEYNA_LOG_DEBUG("to_add_update = %s", prop);

		prop_map = g_hash_table_lookup(filter_map, prop);

		if ((!prop_map) || (!prop_map->updateable))
			goto on_error;

		*mask |= prop_map->type;

		if (!prop_map->filter)
			continue;

		g_hash_table_insert(upnp_props,
				    (gpointer) prop_map->upnp_prop_name, NULL);
	}

	(void) g_variant_iter_init(&viter, to_delete);

	while (g_variant_iter_next(&viter, "&s", &prop)) {
		DLEYNA_LOG_DEBUG("to_delete = %s", prop);

		prop_map = g_hash_table_lookup(filter_map, prop);

		if ((!prop_map) || (!prop_map->updateable) ||
		    (*mask & prop_map->type) != 0)
			goto on_error;

		*mask |= prop_map->type;

		if (!prop_map->filter)
			continue;

		g_hash_table_insert(upnp_props,
				    (gpointer) prop_map->upnp_prop_name, NULL);
	}

	*upnp_filter = prv_compute_upnp_filter(upnp_props);

	retval = TRUE;

on_error:

	g_hash_table_unref(upnp_props);

	return retval;
}

static void prv_add_string_prop(GVariantBuilder *vb, const gchar *key,
				const gchar *value)
{
	if (value) {
		DLEYNA_LOG_DEBUG("Prop %s = %s", key, value);

		g_variant_builder_add(vb, "{sv}", key,
				      g_variant_new_string(value));
	}
}

static void prv_add_strv_prop(GVariantBuilder *vb, const gchar *key,
			      const gchar **value, unsigned int len)
{
	if (value && *value && len > 0)
		g_variant_builder_add(vb, "{sv}", key,
				      g_variant_new_strv(value, len));
}

static void prv_add_path_prop(GVariantBuilder *vb, const gchar *key,
			      const gchar *value)
{
	if (value) {
		DLEYNA_LOG_DEBUG("Prop %s = %s", key, value);

		g_variant_builder_add(vb, "{sv}", key,
				      g_variant_new_object_path(value));
	}
}

static void prv_add_uint_prop(GVariantBuilder *vb, const gchar *key,
			      unsigned int value)
{
	DLEYNA_LOG_DEBUG("Prop %s = %u", key, value);

	g_variant_builder_add(vb, "{sv}", key, g_variant_new_uint32(value));
}

static void prv_add_int_prop(GVariantBuilder *vb, const gchar *key,
			     int value)
{
	if (value != -1)
		g_variant_builder_add(vb, "{sv}", key,
				      g_variant_new_int32(value));
}

static void prv_add_variant_prop(GVariantBuilder *vb, const gchar *key,
				 GVariant *prop)
{
	if (prop)
		g_variant_builder_add(vb, "{sv}", key, prop);
}

void dls_props_add_child_count(GVariantBuilder *item_vb, gint value)
{
	prv_add_int_prop(item_vb, DLS_INTERFACE_PROP_CHILD_COUNT, value);
}

static void prv_add_bool_prop(GVariantBuilder *vb, const gchar *key,
			      gboolean value)
{
	DLEYNA_LOG_DEBUG("Prop %s = %u", key, value);

	g_variant_builder_add(vb, "{sv}", key, g_variant_new_boolean(value));
}

static void prv_add_int64_prop(GVariantBuilder *vb, const gchar *key,
			       gint64 value)
{
	if (value != -1) {
		DLEYNA_LOG_DEBUG("Prop %s = %"G_GINT64_FORMAT, key, value);

		g_variant_builder_add(vb, "{sv}", key,
				      g_variant_new_int64(value));
	}
}

static void prv_add_list_dlna_str(gpointer data, gpointer user_data)
{
	GVariantBuilder *vb = (GVariantBuilder *)user_data;
	gchar *cap_str = (gchar *)data;
	gchar *str;
	int value = 0;

	if (g_str_has_prefix(cap_str, "srs-rt-retention-period-")) {
		str = cap_str + strlen("srs-rt-retention-period-");
		cap_str = "srs-rt-retention-period";

		if (*str) {
			if (!g_strcmp0(str, "infinity"))
				value = -1;
			else
				value = atoi(str);
		}
	}

	prv_add_uint_prop(vb, cap_str, value);
}

static GVariant *prv_add_list_dlna_prop(GList *list)
{
	GVariantBuilder vb;

	g_variant_builder_init(&vb, G_VARIANT_TYPE("a{sv}"));

	g_list_foreach(list, prv_add_list_dlna_str, &vb);

	return g_variant_builder_end(&vb);
}

static GVariant *prv_get_dlna_flags_dict(guint flags,
					 const dls_prop_dlna_t *dfa)
{
	GVariantBuilder builder;
	gboolean set;
	gint i = 0;

	g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sb}"));

	while (dfa[i].dlna_flag) {
		set = (flags & dfa[i].dlna_flag) != 0;
		g_variant_builder_add(&builder, "{sb}", dfa[i].prop_name, set);
		i++;
	}

	return g_variant_builder_end(&builder);
}

static void prv_add_list_artists_str(gpointer data, gpointer user_data)
{
	GVariantBuilder *vb = (GVariantBuilder *)user_data;
	GUPnPDIDLLiteContributor *contributor = data;
	const char *str;

	str = gupnp_didl_lite_contributor_get_name(contributor);
	g_variant_builder_add(vb, "s", str);
}

static GVariant *prv_get_artists_prop(GList *list)
{
	GVariantBuilder vb;

	g_variant_builder_init(&vb, G_VARIANT_TYPE("as"));
	g_list_foreach(list, prv_add_list_artists_str, &vb);

	return g_variant_builder_end(&vb);
}

void dls_props_add_device(GUPnPDeviceInfo *proxy,
			  const dls_device_t *device,
			  GVariantBuilder *vb)
{
	gchar *str;
	GList *list;
	GVariant *dlna_caps;

	prv_add_string_prop(vb, DLS_INTERFACE_PROP_LOCATION,
			    gupnp_device_info_get_location(proxy));

	prv_add_string_prop(vb, DLS_INTERFACE_PROP_UDN,
			    gupnp_device_info_get_udn(proxy));

	prv_add_string_prop(vb, DLS_INTERFACE_PROP_DEVICE_TYPE,
			    gupnp_device_info_get_device_type(proxy));

	str = gupnp_device_info_get_friendly_name(proxy);
	prv_add_string_prop(vb, DLS_INTERFACE_PROP_FRIENDLY_NAME, str);
	g_free(str);

	str = gupnp_device_info_get_manufacturer(proxy);
	prv_add_string_prop(vb, DLS_INTERFACE_PROP_MANUFACTURER, str);
	g_free(str);

	str = gupnp_device_info_get_manufacturer_url(proxy);
	prv_add_string_prop(vb, DLS_INTERFACE_PROP_MANUFACTURER_URL, str);
	g_free(str);

	str = gupnp_device_info_get_model_description(proxy);
	prv_add_string_prop(vb, DLS_INTERFACE_PROP_MODEL_DESCRIPTION, str);
	g_free(str);

	str = gupnp_device_info_get_model_name(proxy);
	prv_add_string_prop(vb, DLS_INTERFACE_PROP_MODEL_NAME, str);
	g_free(str);

	str = gupnp_device_info_get_model_number(proxy);
	prv_add_string_prop(vb, DLS_INTERFACE_PROP_MODEL_NUMBER, str);
	g_free(str);

	str = gupnp_device_info_get_model_url(proxy);
	prv_add_string_prop(vb, DLS_INTERFACE_PROP_MODEL_URL, str);
	g_free(str);

	str = gupnp_device_info_get_serial_number(proxy);
	prv_add_string_prop(vb, DLS_INTERFACE_PROP_SERIAL_NUMBER, str);
	g_free(str);

	str = gupnp_device_info_get_presentation_url(proxy);
	prv_add_string_prop(vb, DLS_INTERFACE_PROP_PRESENTATION_URL, str);
	g_free(str);

	str = gupnp_device_info_get_icon_url(proxy, NULL, -1, -1, -1, FALSE,
					     NULL, NULL, NULL, NULL);
	prv_add_string_prop(vb, DLS_INTERFACE_PROP_ICON_URL, str);
	g_free(str);

	list = gupnp_device_info_list_dlna_capabilities(proxy);
	if (list != NULL) {
		dlna_caps = prv_add_list_dlna_prop(list);
		g_variant_builder_add(vb, "{sv}",
				      DLS_INTERFACE_PROP_SV_DLNA_CAPABILITIES,
				      dlna_caps);
		g_list_free_full(list, g_free);
	}

	if (device->search_caps != NULL)
		g_variant_builder_add(vb, "{sv}",
				      DLS_INTERFACE_PROP_SV_SEARCH_CAPABILITIES,
				      device->search_caps);

	if (device->sort_caps != NULL)
		g_variant_builder_add(vb, "{sv}",
				      DLS_INTERFACE_PROP_SV_SORT_CAPABILITIES,
				      device->sort_caps);

	if (device->sort_ext_caps != NULL)
		g_variant_builder_add(
				vb, "{sv}",
				DLS_INTERFACE_PROP_SV_SORT_EXT_CAPABILITIES,
				device->sort_ext_caps);

	if (device->feature_list != NULL)
		g_variant_builder_add(vb, "{sv}",
				      DLS_INTERFACE_PROP_SV_FEATURE_LIST,
				      device->feature_list);
}

GVariant *dls_props_get_device_prop(GUPnPDeviceInfo *proxy,
				    const dls_device_t *device,
				    const gchar *prop)
{
	GVariant *dlna_caps = NULL;
	GVariant *retval = NULL;
	const gchar *str = NULL;
	gchar *copy = NULL;
	GList *list;

	if (!strcmp(DLS_INTERFACE_PROP_LOCATION, prop)) {
		str = gupnp_device_info_get_location(proxy);
	} else if (!strcmp(DLS_INTERFACE_PROP_UDN, prop)) {
		str = gupnp_device_info_get_udn(proxy);
	} else if (!strcmp(DLS_INTERFACE_PROP_DEVICE_TYPE, prop)) {
		str = gupnp_device_info_get_device_type(proxy);
	} else if (!strcmp(DLS_INTERFACE_PROP_FRIENDLY_NAME, prop)) {
		copy = gupnp_device_info_get_friendly_name(proxy);
		str = copy;
	} else if (!strcmp(DLS_INTERFACE_PROP_MANUFACTURER, prop)) {
		copy = gupnp_device_info_get_manufacturer(proxy);
		str = copy;
	} else if (!strcmp(DLS_INTERFACE_PROP_MANUFACTURER_URL, prop)) {
		copy = gupnp_device_info_get_manufacturer_url(proxy);
		str = copy;
	} else if (!strcmp(DLS_INTERFACE_PROP_MODEL_DESCRIPTION, prop)) {
		copy = gupnp_device_info_get_model_description(proxy);
		str = copy;
	} else if (!strcmp(DLS_INTERFACE_PROP_MODEL_NAME, prop)) {
		copy = gupnp_device_info_get_model_name(proxy);
		str = copy;
	} else if (!strcmp(DLS_INTERFACE_PROP_MODEL_NUMBER, prop)) {
		copy = gupnp_device_info_get_model_number(proxy);
		str = copy;
	} else if (!strcmp(DLS_INTERFACE_PROP_MODEL_URL, prop)) {
		copy = gupnp_device_info_get_model_url(proxy);
		str = copy;
	} else if (!strcmp(DLS_INTERFACE_PROP_SERIAL_NUMBER, prop)) {
		copy = gupnp_device_info_get_serial_number(proxy);
		str = copy;
	} else if (!strcmp(DLS_INTERFACE_PROP_PRESENTATION_URL, prop)) {
		copy = gupnp_device_info_get_presentation_url(proxy);
		str = copy;
	} else if (!strcmp(DLS_INTERFACE_PROP_ICON_URL, prop)) {
		copy = gupnp_device_info_get_icon_url(proxy, NULL,
						      -1, -1, -1, FALSE,
						      NULL, NULL, NULL, NULL);
		str = copy;
	} else if (!strcmp(DLS_INTERFACE_PROP_SV_DLNA_CAPABILITIES, prop)) {
		list = gupnp_device_info_list_dlna_capabilities(proxy);
		if (list != NULL) {
			dlna_caps = prv_add_list_dlna_prop(list);
			g_list_free_full(list, g_free);
			retval = g_variant_ref_sink(dlna_caps);

#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
			copy = g_variant_print(dlna_caps, FALSE);
			DLEYNA_LOG_DEBUG("Prop %s = %s", prop, copy);
#endif
		}
	} else if (!strcmp(DLS_INTERFACE_PROP_SV_SEARCH_CAPABILITIES, prop)) {
		if (device->search_caps != NULL) {
			retval = g_variant_ref(device->search_caps);

#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
			copy = g_variant_print(device->search_caps, FALSE);
			DLEYNA_LOG_DEBUG("Prop %s = %s", prop, copy);
#endif
		}
	} else if (!strcmp(DLS_INTERFACE_PROP_SV_SORT_CAPABILITIES, prop)) {
		if (device->sort_caps != NULL) {
			retval = g_variant_ref(device->sort_caps);

#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
			copy = g_variant_print(device->sort_caps, FALSE);
			DLEYNA_LOG_DEBUG("Prop %s = %s", prop, copy);
#endif
		}
	} else if (!strcmp(DLS_INTERFACE_PROP_SV_SORT_EXT_CAPABILITIES, prop)) {
		if (device->sort_ext_caps != NULL) {
			retval = g_variant_ref(device->sort_ext_caps);

#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
			copy = g_variant_print(device->sort_ext_caps, FALSE);
			DLEYNA_LOG_DEBUG("Prop %s = %s", prop, copy);
#endif
		}
	} else if (!strcmp(DLS_INTERFACE_PROP_SV_FEATURE_LIST, prop)) {
		if (device->feature_list != NULL) {
			retval = g_variant_ref(device->feature_list);

#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
			copy = g_variant_print(device->feature_list, FALSE);
			DLEYNA_LOG_DEBUG("Prop %s = %s", prop, copy);
#endif
		}
	}

	if (!retval) {
		if (str) {
			DLEYNA_LOG_DEBUG("Prop %s = %s", prop, str);

			retval = g_variant_ref_sink(g_variant_new_string(str));
		}
#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_WARNING
		else
			DLEYNA_LOG_WARNING("Property %s not defined", prop);
#endif
	}

	g_free(copy);

	return retval;
}

static GUPnPDIDLLiteResource *prv_match_resource(GUPnPDIDLLiteResource *res,
						 gchar **pi_str_array)
{
	GUPnPDIDLLiteResource *retval = NULL;
	GUPnPProtocolInfo *res_pi;
	GUPnPProtocolInfo *pi;
	unsigned int i;
	gboolean match;

	if (!pi_str_array) {
		retval = res;
		goto done;
	}

	res_pi = gupnp_didl_lite_resource_get_protocol_info(res);
	if (!res_pi)
		goto done;

	for (i = 0; pi_str_array[i]; ++i) {
		pi = gupnp_protocol_info_new_from_string(pi_str_array[i],
			NULL);
		if (!pi)
			continue;
		match = gupnp_protocol_info_is_compatible(pi, res_pi);
		g_object_unref(pi);
		if (match) {
			retval = res;
			break;
		}
	}
done:

	return retval;
}

static GUPnPDIDLLiteResource *prv_get_matching_resource
	(GUPnPDIDLLiteObject *object, const gchar *protocol_info)
{
	GUPnPDIDLLiteResource *retval = NULL;
	GUPnPDIDLLiteResource *res;
	GList *resources;
	GList *ptr;
	gchar **pi_str_array = NULL;

	if (protocol_info)
		pi_str_array = g_strsplit(protocol_info, ",", 0);

	resources = gupnp_didl_lite_object_get_resources(object);
	ptr = resources;

	while (ptr) {
		res = ptr->data;
		if (!retval) {
			retval = prv_match_resource(res, pi_str_array);
			if (!retval)
				g_object_unref(res);
		} else {
			g_object_unref(res);
		}

		ptr = ptr->next;
	}

	g_list_free(resources);
	if (pi_str_array)
		g_strfreev(pi_str_array);

	return retval;
}

static void prv_parse_common_resources(GVariantBuilder *item_vb,
				       GUPnPDIDLLiteResource *res,
				       dls_upnp_prop_mask filter_mask)
{
	GUPnPProtocolInfo *protocol_info;
	gint64 int64_val;
	guint uint_val;
	const char *str_val;
	GUPnPDLNAConversion conv;
	GUPnPDLNAOperation ope;
	GUPnPDLNAFlags flags;

	if (filter_mask & DLS_PROP_MASK_SIZE) {
		int64_val = gupnp_didl_lite_resource_get_size64(res);
		prv_add_int64_prop(item_vb, DLS_INTERFACE_PROP_SIZE, int64_val);
	}

	if ((filter_mask & DLS_PROP_MASK_UPDATE_COUNT) &&
	    gupnp_didl_lite_resource_update_count_is_set(res)) {
		uint_val = gupnp_didl_lite_resource_get_update_count(res);
		prv_add_uint_prop(item_vb, DLS_INTERFACE_PROP_UPDATE_COUNT,
				  uint_val);
	}

	protocol_info = gupnp_didl_lite_resource_get_protocol_info(res);

	if (filter_mask & DLS_PROP_MASK_DLNA_PROFILE) {
		str_val = gupnp_protocol_info_get_dlna_profile(protocol_info);
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_DLNA_PROFILE,
				    str_val);
	}

	if (filter_mask & DLS_PROP_MASK_DLNA_CONVERSION) {
		conv = gupnp_protocol_info_get_dlna_conversion(protocol_info);
		prv_add_variant_prop(item_vb,
				     DLS_INTERFACE_PROP_DLNA_CONVERSION,
				     prv_get_dlna_flags_dict(
						conv, g_prop_dlna_ci));
	}

	if (filter_mask & DLS_PROP_MASK_DLNA_OPERATION) {
		ope = gupnp_protocol_info_get_dlna_operation(protocol_info);
		prv_add_variant_prop(item_vb,
				     DLS_INTERFACE_PROP_DLNA_OPERATION,
				     prv_get_dlna_flags_dict(
						ope, g_prop_dlna_op));
	}

	if (filter_mask & DLS_PROP_MASK_DLNA_FLAGS) {
		flags = gupnp_protocol_info_get_dlna_flags(protocol_info);
		prv_add_variant_prop(item_vb,
				     DLS_INTERFACE_PROP_DLNA_FLAGS,
				     prv_get_dlna_flags_dict(
						flags, g_prop_dlna_flags));
	}

	if (filter_mask & DLS_PROP_MASK_MIME_TYPE) {
		str_val = gupnp_protocol_info_get_mime_type(protocol_info);
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_MIME_TYPE,
				    str_val);
	}
}

static void prv_parse_all_resources(GVariantBuilder *item_vb,
				    GUPnPDIDLLiteResource *res,
				    dls_upnp_prop_mask filter_mask)
{
	int int_val;

	prv_parse_common_resources(item_vb, res, filter_mask);

	if (filter_mask & DLS_PROP_MASK_BITRATE) {
		int_val = gupnp_didl_lite_resource_get_bitrate(res);
		prv_add_int_prop(item_vb, DLS_INTERFACE_PROP_BITRATE, int_val);
	}

	if (filter_mask & DLS_PROP_MASK_SAMPLE_RATE) {
		int_val = gupnp_didl_lite_resource_get_sample_freq(res);
		prv_add_int_prop(item_vb, DLS_INTERFACE_PROP_SAMPLE_RATE,
				 int_val);
	}

	if (filter_mask & DLS_PROP_MASK_BITS_PER_SAMPLE) {
		int_val = gupnp_didl_lite_resource_get_bits_per_sample(res);
		prv_add_int_prop(item_vb, DLS_INTERFACE_PROP_BITS_PER_SAMPLE,
				 int_val);
	}

	if (filter_mask & DLS_PROP_MASK_DURATION) {
		int_val = (int) gupnp_didl_lite_resource_get_duration(res);
		prv_add_int_prop(item_vb, DLS_INTERFACE_PROP_DURATION, int_val);
	}

	if (filter_mask & DLS_PROP_MASK_WIDTH) {
		int_val = (int) gupnp_didl_lite_resource_get_width(res);
		prv_add_int_prop(item_vb, DLS_INTERFACE_PROP_WIDTH, int_val);
	}

	if (filter_mask & DLS_PROP_MASK_HEIGHT) {
		int_val = (int) gupnp_didl_lite_resource_get_height(res);
		prv_add_int_prop(item_vb, DLS_INTERFACE_PROP_HEIGHT, int_val);
	}

	if (filter_mask & DLS_PROP_MASK_COLOR_DEPTH) {
		int_val = (int) gupnp_didl_lite_resource_get_color_depth(res);
		prv_add_int_prop(item_vb, DLS_INTERFACE_PROP_COLOR_DEPTH,
				 int_val);
	}
}

static GVariant *prv_compute_create_classes(GUPnPDIDLLiteContainer *container)
{
	GVariantBuilder create_classes_vb;
	GList *create_classes;
	GList *ptr;
	GUPnPDIDLLiteCreateClass *create_class;
	const char *content;
	gboolean inc_derived;

	g_variant_builder_init(&create_classes_vb, G_VARIANT_TYPE("a(sb)"));

	create_classes = gupnp_didl_lite_container_get_create_classes_full(
								 container);
	ptr = create_classes;
	while (ptr) {
		create_class = ptr->data;
		content = gupnp_didl_lite_create_class_get_content(
								 create_class);
		inc_derived = gupnp_didl_lite_create_class_get_include_derived(
								 create_class);
		g_variant_builder_add(&create_classes_vb,
				      "(sb)", content, inc_derived);
		g_object_unref(ptr->data);
		ptr = g_list_next(ptr);
	}
	g_list_free(create_classes);

	return g_variant_builder_end(&create_classes_vb);
}

const gchar *dls_props_media_spec_to_upnp_class(const gchar *m2spec_class)
{
	const gchar *retval = NULL;

	if (!m2spec_class)
		goto on_error;

	if (!strcmp(m2spec_class, gMediaSpec2AlbumPhoto))
		retval = gUPnPPhotoAlbum;
	else if (!strcmp(m2spec_class, gMediaSpec2AlbumMusic))
		retval = gUPnPMusicAlbum;
	else if (!strcmp(m2spec_class, gMediaSpec2Album))
		retval = gUPnPAlbum;
	else if (!strcmp(m2spec_class, gMediaSpec2PersonMusicArtist))
		retval = gUPnPMusicArtist;
	else if (!strcmp(m2spec_class, gMediaSpec2Person))
		retval = gUPnPPerson;
	else if (!strcmp(m2spec_class, gMediaSpec2GenreMovie))
		retval = gUPnPMovieGenre;
	else if (!strcmp(m2spec_class, gMediaSpec2GenreMusic))
		retval = gUPnPMusicGenre;
	else if (!strcmp(m2spec_class, gMediaSpec2Genre))
		retval = gUPnPGenre;
	else if (!strcmp(m2spec_class, gMediaSpec2Container))
		retval = gUPnPContainer;
	else if (!strcmp(m2spec_class, gMediaSpec2AudioMusic))
		retval = gUPnPMusicTrack;
	else if (!strcmp(m2spec_class, gMediaSpec2AudioBroadcast))
		retval = gUPnPAudioBroadcast;
	else if (!strcmp(m2spec_class, gMediaSpec2AudioBook))
		retval = gUPnPAudioBook;
	else if (!strcmp(m2spec_class, gMediaSpec2Audio))
		retval = gUPnPAudioItem;
	else if (!strcmp(m2spec_class, gMediaSpec2VideoMovie))
		retval = gUPnPMovie;
	else if (!strcmp(m2spec_class, gMediaSpec2VideoMusicClip))
		retval = gUPnPMusicVideoClip;
	else if (!strcmp(m2spec_class, gMediaSpec2VideoBroadcast))
		retval = gUPnPVideoBroadcast;
	else if (!strcmp(m2spec_class, gMediaSpec2Video))
		retval = gUPnPVideoItem;
	else if (!strcmp(m2spec_class, gMediaSpec2ImagePhoto))
		retval = gUPnPPhoto;
	else if (!strcmp(m2spec_class, gMediaSpec2Image))
		retval = gUPnPImageItem;
	else if (!strcmp(m2spec_class, gMediaSpec2Playlist))
		retval = gUPnPPlaylistItem;
	else if (!strcmp(m2spec_class, gMediaSpec2Item))
		retval = gUPnPItem;

on_error:

	return retval;
}

const gchar *dls_props_upnp_class_to_media_spec(const gchar *upnp_class)
{
	const gchar *retval = NULL;
	const gchar *ptr;

	if (!upnp_class)
		goto on_error;

	if (!strncmp(upnp_class, gUPnPAlbum, gUPnPAlbumLen)) {
		ptr = upnp_class + gUPnPAlbumLen;
		if (!strcmp(ptr, ".photoAlbum"))
			retval = gMediaSpec2AlbumPhoto;
		else if (!strcmp(ptr, ".musicAlbum"))
			retval = gMediaSpec2AlbumMusic;
		else
			retval = gMediaSpec2Album;
	} else if (!strncmp(upnp_class, gUPnPPerson, gUPnPPersonLen)) {
		ptr = upnp_class + gUPnPPersonLen;
		if (!strcmp(ptr, ".musicArtist"))
			retval = gMediaSpec2PersonMusicArtist;
		else
			retval = gMediaSpec2Person;
	} else if (!strncmp(upnp_class, gUPnPGenre, gUPnPGenreLen)) {
		ptr = upnp_class + gUPnPGenreLen;
		if (!strcmp(ptr, ".movieGenre"))
			retval = gMediaSpec2GenreMovie;
		else if (!strcmp(ptr, ".musicGenre"))
			retval = gMediaSpec2GenreMusic;
		else
			retval = gMediaSpec2Genre;
	} else if (!strncmp(upnp_class, gUPnPContainer, gUPnPContainerLen)) {
		ptr = upnp_class + gUPnPContainerLen;
		if (!*ptr || *ptr == '.')
			retval = gMediaSpec2Container;
	} else if (!strncmp(upnp_class, gUPnPAudioItem, gUPnPAudioItemLen)) {
		ptr = upnp_class + gUPnPAudioItemLen;
		if (!strcmp(ptr, ".musicTrack"))
			retval = gMediaSpec2AudioMusic;
		else if (!strcmp(ptr, ".audioBroadcast"))
			retval = gMediaSpec2AudioBroadcast;
		else if (!strcmp(ptr, ".audioBook"))
			retval = gMediaSpec2AudioBook;
		else
			retval = gMediaSpec2Audio;
	} else if (!strncmp(upnp_class, gUPnPVideoItem, gUPnPVideoItemLen)) {
		ptr = upnp_class + gUPnPVideoItemLen;
		if (!strcmp(ptr, ".movie"))
			retval = gMediaSpec2VideoMovie;
		else if (!strcmp(ptr, ".musicVideoClip"))
			retval = gMediaSpec2VideoMusicClip;
		else if (!strcmp(ptr, ".videoBroadcast"))
			retval = gMediaSpec2VideoBroadcast;
		else
			retval = gMediaSpec2Video;
	}  else if (!strncmp(upnp_class, gUPnPImageItem, gUPnPImageItemLen)) {
		ptr = upnp_class + gUPnPImageItemLen;
		if (!strcmp(ptr, ".photo"))
			retval = gMediaSpec2ImagePhoto;
		else
			retval = gMediaSpec2Image;
	}  else if (!strncmp(upnp_class, gUPnPPlaylistItem,
			     gUPnPPlaylistItemLen)) {
		retval = gMediaSpec2Playlist;
	} else if (!strncmp(upnp_class, gUPnPItem, gUPnPItemLen)) {
		ptr = upnp_class + gUPnPItemLen;
		if (!*ptr || *ptr == '.')
			retval = gMediaSpec2Item;
	}

on_error:

	return retval;
}

static GVariant *prv_compute_resources(GUPnPDIDLLiteObject *object,
				       dls_upnp_prop_mask filter_mask,
				       gboolean all_res)
{
	GUPnPDIDLLiteResource *res = NULL;
	GList *resources;
	GList *ptr;
	GVariantBuilder *res_array_vb;
	GVariantBuilder *res_vb;
	const char *str_val;
	GVariant *retval;

	res_array_vb = g_variant_builder_new(G_VARIANT_TYPE("aa{sv}"));

	resources = gupnp_didl_lite_object_get_resources(object);
	ptr = resources;

	while (ptr) {
		res = ptr->data;
		res_vb = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
		if (filter_mask & DLS_PROP_MASK_URL) {
			str_val = gupnp_didl_lite_resource_get_uri(res);
			prv_add_string_prop(res_vb, DLS_INTERFACE_PROP_URL,
					    str_val);
		}

		if (all_res)
			prv_parse_all_resources(res_vb, res, filter_mask);
		else
			prv_parse_common_resources(res_vb, res, filter_mask);

		g_variant_builder_add(res_array_vb, "@a{sv}",
				      g_variant_builder_end(res_vb));
		g_variant_builder_unref(res_vb);
		g_object_unref(ptr->data);
		ptr = g_list_next(ptr);
	}
	retval = g_variant_builder_end(res_array_vb);
	g_variant_builder_unref(res_array_vb);

	g_list_free(resources);

	return retval;
}

static void prv_add_resources(GVariantBuilder *item_vb,
			      GUPnPDIDLLiteObject *object,
			      dls_upnp_prop_mask filter_mask,
			      gboolean all_res)
{
	GVariant *val;

	val = prv_compute_resources(object, filter_mask, all_res);
	g_variant_builder_add(item_vb, "{sv}", DLS_INTERFACE_PROP_RES_RESOURCES,
			      val);
}

gboolean dls_props_add_object(GVariantBuilder *item_vb,
			      GUPnPDIDLLiteObject *object,
			      const char *root_path,
			      const gchar *parent_path,
			      dls_upnp_prop_mask filter_mask)
{
	gchar *path = NULL;
	const char *id;
	const char *title;
	const char *creator;
	const char *upnp_class;
	const char *media_spec_type;
	gboolean retval = FALSE;
	gboolean rest;
	GUPnPOCMFlags flags;
	guint uint_val;

	id = gupnp_didl_lite_object_get_id(object);
	if (!id)
		goto on_error;

	upnp_class = gupnp_didl_lite_object_get_upnp_class(object);
	media_spec_type = dls_props_upnp_class_to_media_spec(upnp_class);

	if (!media_spec_type)
		goto on_error;

	title = gupnp_didl_lite_object_get_title(object);
	creator = gupnp_didl_lite_object_get_creator(object);
	rest = gupnp_didl_lite_object_get_restricted(object);
	path = dls_path_from_id(root_path, id);

	if (filter_mask & DLS_PROP_MASK_DISPLAY_NAME)
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_DISPLAY_NAME,
				    title);

	if (filter_mask & DLS_PROP_MASK_CREATOR)
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_CREATOR,
				    creator);

	if (filter_mask & DLS_PROP_MASK_PATH)
		prv_add_path_prop(item_vb, DLS_INTERFACE_PROP_PATH, path);

	if (filter_mask & DLS_PROP_MASK_PARENT)
		prv_add_path_prop(item_vb, DLS_INTERFACE_PROP_PARENT,
				  parent_path);

	if (filter_mask & DLS_PROP_MASK_TYPE)
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_TYPE,
				    media_spec_type);

	if (filter_mask & DLS_PROP_MASK_RESTRICTED)
		prv_add_bool_prop(item_vb, DLS_INTERFACE_PROP_RESTRICTED, rest);

	if (filter_mask & DLS_PROP_MASK_DLNA_MANAGED) {
		flags = gupnp_didl_lite_object_get_dlna_managed(object);
		prv_add_variant_prop(item_vb,
				     DLS_INTERFACE_PROP_DLNA_MANAGED,
				     prv_get_dlna_flags_dict(flags,
							     g_prop_dlna_ocm));
	}

	if ((filter_mask & DLS_PROP_MASK_OBJECT_UID) &&
	    gupnp_didl_lite_object_update_id_is_set(object)) {
		uint_val = gupnp_didl_lite_object_get_update_id(object);
		prv_add_uint_prop(item_vb, DLS_INTERFACE_PROP_OBJECT_UPDATE_ID,
				  uint_val);
	}

	retval = TRUE;

on_error:

	g_free(path);

	return retval;
}

void dls_props_add_container(GVariantBuilder *item_vb,
			     GUPnPDIDLLiteContainer *object,
			     dls_upnp_prop_mask filter_mask,
			     const gchar *protocol_info,
			     gboolean *have_child_count)
{
	int child_count;
	gboolean searchable;
	guint uint_val;
	GUPnPDIDLLiteResource *res;
	const char *str_val;

	*have_child_count = FALSE;
	if (filter_mask & DLS_PROP_MASK_CHILD_COUNT) {
		child_count = gupnp_didl_lite_container_get_child_count(object);
		if (child_count >= 0) {
			prv_add_uint_prop(item_vb,
					  DLS_INTERFACE_PROP_CHILD_COUNT,
					  (unsigned int) child_count);
			*have_child_count = TRUE;
		}
	}

	if (filter_mask & DLS_PROP_MASK_SEARCHABLE) {
		searchable = gupnp_didl_lite_container_get_searchable(object);
		prv_add_bool_prop(item_vb, DLS_INTERFACE_PROP_SEARCHABLE,
				  searchable);
	}

	if (filter_mask & DLS_PROP_MASK_CREATE_CLASSES)
		prv_add_variant_prop(item_vb,
				     DLS_INTERFACE_PROP_CREATE_CLASSES,
				     prv_compute_create_classes(object));

	if ((filter_mask & DLS_PROP_MASK_CONTAINER_UID) &&
	    gupnp_didl_lite_container_container_update_id_is_set(object)) {
		uint_val = gupnp_didl_lite_container_get_container_update_id(
									object);
		prv_add_uint_prop(item_vb,
				  DLS_INTERFACE_PROP_CONTAINER_UPDATE_ID,
				  uint_val);
	}

	if ((filter_mask & DLS_PROP_MASK_TOTAL_DELETED_CC) &&
	    gupnp_didl_lite_container_total_deleted_child_count_is_set(
								object)) {
		uint_val =
		gupnp_didl_lite_container_get_total_deleted_child_count(object);

		prv_add_uint_prop(item_vb,
				  DLS_INTERFACE_PROP_TOTAL_DELETED_CHILD_COUNT,
				  uint_val);
	}

	if (filter_mask & DLS_PROP_MASK_RES_RESOURCES)
		prv_add_resources(item_vb, GUPNP_DIDL_LITE_OBJECT(object),
				  filter_mask, FALSE);

	res = prv_get_matching_resource(GUPNP_DIDL_LITE_OBJECT(object),
					protocol_info);
	if (res) {
		if (filter_mask & DLS_PROP_MASK_URLS) {
			str_val = gupnp_didl_lite_resource_get_uri(res);
			prv_add_strv_prop(item_vb, DLS_INTERFACE_PROP_URLS,
					  &str_val, 1);
		}
		prv_parse_common_resources(item_vb, res, filter_mask);
		g_object_unref(res);
	}
}

void dls_props_add_item(GVariantBuilder *item_vb,
			GUPnPDIDLLiteObject *object,
			const gchar *root_path,
			dls_upnp_prop_mask filter_mask,
			const gchar *protocol_info)
{
	int track_number;
	GUPnPDIDLLiteResource *res;
	const char *str_val;
	char *path;
	GList *list;

	if (filter_mask & DLS_PROP_MASK_ARTIST)
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_ARTIST,
				    gupnp_didl_lite_object_get_artist(object));

	if (filter_mask & DLS_PROP_MASK_ARTISTS) {
		list = gupnp_didl_lite_object_get_artists(object);
		prv_add_variant_prop(item_vb, DLS_INTERFACE_PROP_ARTISTS,
				     prv_get_artists_prop(list));
		g_list_free_full(list, g_object_unref);
	}

	if (filter_mask & DLS_PROP_MASK_ALBUM)
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_ALBUM,
				    gupnp_didl_lite_object_get_album(object));

	if (filter_mask & DLS_PROP_MASK_DATE)
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_DATE,
				    gupnp_didl_lite_object_get_date(object));

	if (filter_mask & DLS_PROP_MASK_GENRE)
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_GENRE,
				    gupnp_didl_lite_object_get_genre(object));

	if (filter_mask & DLS_PROP_MASK_TRACK_NUMBER) {
		track_number = gupnp_didl_lite_object_get_track_number(object);
		if (track_number >= 0)
			prv_add_int_prop(item_vb,
					 DLS_INTERFACE_PROP_TRACK_NUMBER,
					 track_number);
	}

	if (filter_mask & DLS_PROP_MASK_ALBUM_ART_URL)
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_ALBUM_ART_URL,
				    gupnp_didl_lite_object_get_album_art(
					    object));

	if (filter_mask & DLS_PROP_MASK_REFPATH) {
		str_val = gupnp_didl_lite_item_get_ref_id(
						GUPNP_DIDL_LITE_ITEM(object));
		if (str_val != NULL) {
			path = dls_path_from_id(root_path, str_val);
			prv_add_path_prop(item_vb, DLS_INTERFACE_PROP_REFPATH,
					  path);
			g_free(path);
		}
	}

	res = prv_get_matching_resource(object, protocol_info);
	if (res) {
		if (filter_mask & DLS_PROP_MASK_URLS) {
			str_val = gupnp_didl_lite_resource_get_uri(res);
			prv_add_strv_prop(item_vb, DLS_INTERFACE_PROP_URLS,
					  &str_val, 1);
		}
		prv_parse_all_resources(item_vb, res, filter_mask);
		g_object_unref(res);
	}

	if (filter_mask & DLS_PROP_MASK_RES_RESOURCES)
		prv_add_resources(item_vb, object, filter_mask, TRUE);
}

void dls_props_add_resource(GVariantBuilder *item_vb,
			    GUPnPDIDLLiteObject *object,
			    dls_upnp_prop_mask filter_mask,
			    const gchar *protocol_info)
{
	GUPnPDIDLLiteResource *res;
	const char *str_val;

	res = prv_get_matching_resource(object, protocol_info);
	if (res) {
		if (filter_mask & DLS_PROP_MASK_URL) {
			str_val = gupnp_didl_lite_resource_get_uri(res);
			prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_URL,
					    str_val);
		}

		if (GUPNP_IS_DIDL_LITE_CONTAINER(object))
			prv_parse_common_resources(item_vb, res, filter_mask);
		else
			prv_parse_all_resources(item_vb, res, filter_mask);

		g_object_unref(res);
	}
}


static GVariant *prv_get_common_resource_property(const gchar *prop,
						GUPnPDIDLLiteResource *res)
{
	gint64 int64_val;
	guint uint_val;
	const char *str_val;
	GVariant *retval = NULL;
	GUPnPProtocolInfo *protocol_info;
	GUPnPDLNAConversion conv;
	GUPnPDLNAOperation ope;
	GUPnPDLNAFlags flags;

	if (!strcmp(prop, DLS_INTERFACE_PROP_DLNA_PROFILE)) {
		protocol_info = gupnp_didl_lite_resource_get_protocol_info(res);
		if (!protocol_info)
			goto on_error;
		str_val = gupnp_protocol_info_get_dlna_profile(protocol_info);
		if (!str_val)
			goto on_error;
		retval = g_variant_ref_sink(g_variant_new_string(str_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_DLNA_CONVERSION)) {
		protocol_info = gupnp_didl_lite_resource_get_protocol_info(res);
		conv = gupnp_protocol_info_get_dlna_conversion(protocol_info);
		retval = prv_get_dlna_flags_dict(conv, g_prop_dlna_ci);
		if (retval)
			retval = g_variant_ref_sink(retval);
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_DLNA_OPERATION)) {
		protocol_info = gupnp_didl_lite_resource_get_protocol_info(res);
		ope = gupnp_protocol_info_get_dlna_operation(protocol_info);
		retval = prv_get_dlna_flags_dict(ope, g_prop_dlna_op);
		if (retval)
			retval = g_variant_ref_sink(retval);
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_DLNA_FLAGS)) {
		protocol_info = gupnp_didl_lite_resource_get_protocol_info(res);
		flags = gupnp_protocol_info_get_dlna_flags(protocol_info);
		retval =  prv_get_dlna_flags_dict(flags, g_prop_dlna_flags);
		if (retval)
			retval = g_variant_ref_sink(retval);
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_MIME_TYPE)) {
		protocol_info = gupnp_didl_lite_resource_get_protocol_info(res);
		if (!protocol_info)
			goto on_error;
		str_val = gupnp_protocol_info_get_mime_type(protocol_info);
		if (!str_val)
			goto on_error;
		retval = g_variant_ref_sink(g_variant_new_string(str_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_SIZE)) {
		int64_val = gupnp_didl_lite_resource_get_size64(res);
		if (int64_val == -1)
			goto on_error;
		retval = g_variant_ref_sink(g_variant_new_int64(int64_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_URLS)) {
		str_val = gupnp_didl_lite_resource_get_uri(res);
		if (str_val)
			retval = g_variant_ref_sink(g_variant_new_strv(&str_val,
								       1));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_UPDATE_COUNT) &&
		   gupnp_didl_lite_resource_update_count_is_set(res)) {
		uint_val = gupnp_didl_lite_resource_get_update_count(res);
		retval = g_variant_ref_sink(g_variant_new_uint32(uint_val));
	}

on_error:

	return retval;
}

static GVariant *prv_get_item_resource_property(const gchar *prop,
						GUPnPDIDLLiteResource *res)
{
	int int_val;
	GVariant *retval;

	retval = prv_get_common_resource_property(prop, res);

	if (retval)
		goto on_exit;

	if (!strcmp(prop, DLS_INTERFACE_PROP_DURATION)) {
		int_val = (int) gupnp_didl_lite_resource_get_duration(res);
		if (int_val == -1)
			goto on_exit;
		retval = g_variant_ref_sink(g_variant_new_int32(int_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_BITRATE)) {
		int_val = gupnp_didl_lite_resource_get_bitrate(res);
		if (int_val == -1)
			goto on_exit;
		retval = g_variant_ref_sink(g_variant_new_int32(int_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_SAMPLE_RATE)) {
		int_val = gupnp_didl_lite_resource_get_sample_freq(res);
		if (int_val == -1)
			goto on_exit;
		retval = g_variant_ref_sink(g_variant_new_int32(int_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_BITS_PER_SAMPLE)) {
		int_val = gupnp_didl_lite_resource_get_bits_per_sample(res);
		if (int_val == -1)
			goto on_exit;
		retval = g_variant_ref_sink(g_variant_new_int32(int_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_WIDTH)) {
		int_val = (int) gupnp_didl_lite_resource_get_width(res);
		if (int_val == -1)
			goto on_exit;
		retval = g_variant_ref_sink(g_variant_new_int32(int_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_HEIGHT)) {
		int_val = (int) gupnp_didl_lite_resource_get_height(res);
		if (int_val == -1)
			goto on_exit;
		retval = g_variant_ref_sink(g_variant_new_int32(int_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_COLOR_DEPTH)) {
		int_val = (int) gupnp_didl_lite_resource_get_color_depth(res);
		if (int_val == -1)
			goto on_exit;
		retval = g_variant_ref_sink(g_variant_new_int32(int_val));
	}

on_exit:

	return retval;
}

GVariant *dls_props_get_object_prop(const gchar *prop, const gchar *root_path,
				    GUPnPDIDLLiteObject *object)
{
	const char *id;
	gchar *path;
	const char *upnp_class;
	const char *media_spec_type;
	const char *title;
	gboolean rest;
	GVariant *retval = NULL;
	GUPnPOCMFlags dlna_managed;
	guint uint_val;

	if (!strcmp(prop, DLS_INTERFACE_PROP_PARENT)) {
		id = gupnp_didl_lite_object_get_parent_id(object);
		if (!id || !strcmp(id, "-1")) {
			DLEYNA_LOG_DEBUG("Prop %s = %s", prop, root_path);

			retval = g_variant_ref_sink(g_variant_new_string(
							    root_path));
		} else {
			path = dls_path_from_id(root_path, id);

			DLEYNA_LOG_DEBUG("Prop %s = %s", prop, path);

			retval = g_variant_ref_sink(g_variant_new_string(
							    path));
			g_free(path);
		}
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_PATH)) {
		id = gupnp_didl_lite_object_get_id(object);
		if (!id)
			goto on_error;

		path = dls_path_from_id(root_path, id);

		DLEYNA_LOG_DEBUG("Prop %s = %s", prop, path);

		retval = g_variant_ref_sink(g_variant_new_string(path));
		g_free(path);
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_TYPE)) {
		upnp_class = gupnp_didl_lite_object_get_upnp_class(object);
		media_spec_type =
			dls_props_upnp_class_to_media_spec(upnp_class);
		if (!media_spec_type)
			goto on_error;

		DLEYNA_LOG_DEBUG("Prop %s = %s", prop, media_spec_type);

		retval = g_variant_ref_sink(g_variant_new_string(
						    media_spec_type));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_DISPLAY_NAME)) {
		title = gupnp_didl_lite_object_get_title(object);
		if (!title)
			goto on_error;

		DLEYNA_LOG_DEBUG("Prop %s = %s", prop, title);

		retval = g_variant_ref_sink(g_variant_new_string(title));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_CREATOR)) {
		title = gupnp_didl_lite_object_get_creator(object);
		if (!title)
			goto on_error;

		DLEYNA_LOG_DEBUG("Prop %s = %s", prop, title);

		retval = g_variant_ref_sink(g_variant_new_string(title));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_RESTRICTED)) {
		rest = gupnp_didl_lite_object_get_restricted(object);

		DLEYNA_LOG_DEBUG("Prop %s = %d", prop, rest);

		retval = g_variant_ref_sink(g_variant_new_boolean(rest));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_DLNA_MANAGED)) {
		dlna_managed = gupnp_didl_lite_object_get_dlna_managed(object);

		DLEYNA_LOG_DEBUG("Prop %s = %0x", prop, dlna_managed);

		retval = g_variant_ref_sink(prv_get_dlna_flags_dict(
						dlna_managed, g_prop_dlna_ocm));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_OBJECT_UPDATE_ID) &&
		   gupnp_didl_lite_object_update_id_is_set(object)) {
		uint_val = gupnp_didl_lite_object_get_update_id(object);

		DLEYNA_LOG_DEBUG("Prop %s = %u", prop, uint_val);

		retval = g_variant_ref_sink(g_variant_new_uint32(uint_val));
	}

on_error:

	return retval;
}

GVariant *dls_props_get_item_prop(const gchar *prop, const gchar *root_path,
				  GUPnPDIDLLiteObject *object,
				  const gchar *protocol_info)
{
	const gchar *str;
	gchar *path;
	gint track_number;
	GUPnPDIDLLiteResource *res;
	GVariant *retval = NULL;
	GList *list;
#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
	gchar *prop_str;
#endif

	if (GUPNP_IS_DIDL_LITE_CONTAINER(object))
		goto on_error;

	if (!strcmp(prop, DLS_INTERFACE_PROP_ARTIST)) {
		str = gupnp_didl_lite_object_get_artist(object);
		if (!str)
			goto on_error;

		DLEYNA_LOG_DEBUG("Prop %s = %s", prop, str);

		retval = g_variant_ref_sink(g_variant_new_string(str));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_ARTISTS)) {
		list = gupnp_didl_lite_object_get_artists(object);
		if (!list)
			goto on_error;

		retval = g_variant_ref_sink(prv_get_artists_prop(list));
		g_list_free_full(list, g_object_unref);

#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
		prop_str = g_variant_print(retval, FALSE);
		DLEYNA_LOG_DEBUG("Prop %s = %s", prop, prop_str);
		g_free(prop_str);
#endif
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_ALBUM)) {
		str = gupnp_didl_lite_object_get_album(object);
		if (!str)
			goto on_error;

		DLEYNA_LOG_DEBUG("Prop %s = %s", prop, str);

		retval = g_variant_ref_sink(g_variant_new_string(str));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_DATE)) {
		str = gupnp_didl_lite_object_get_date(object);
		if (!str)
			goto on_error;

		DLEYNA_LOG_DEBUG("Prop %s = %s", prop, str);

		retval = g_variant_ref_sink(g_variant_new_string(str));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_GENRE)) {
		str = gupnp_didl_lite_object_get_genre(object);
		if (!str)
			goto on_error;

		DLEYNA_LOG_DEBUG("Prop %s = %s", prop, str);

		retval = g_variant_ref_sink(g_variant_new_string(str));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_TRACK_NUMBER)) {
		track_number = gupnp_didl_lite_object_get_track_number(object);
		if (track_number < 0)
			goto on_error;

		DLEYNA_LOG_DEBUG("Prop %s = %d", prop, track_number);

		retval = g_variant_ref_sink(
			g_variant_new_int32(track_number));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_ALBUM_ART_URL)) {
		str = gupnp_didl_lite_object_get_album_art(object);
		if (!str)
			goto on_error;

		DLEYNA_LOG_DEBUG("Prop %s = %s", prop, str);

		retval = g_variant_ref_sink(g_variant_new_string(str));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_REFPATH)) {
		str = gupnp_didl_lite_item_get_ref_id(
					GUPNP_DIDL_LITE_ITEM(object));
		if (!str)
			goto on_error;

		DLEYNA_LOG_DEBUG("Prop %s = %s", prop, str);

		path = dls_path_from_id(root_path, str);
		retval = g_variant_ref_sink(g_variant_new_string(path));
		g_free(path);
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_RES_RESOURCES)) {
		retval = g_variant_ref_sink(
			prv_compute_resources(object, DLS_PROP_MASK_ALL_PROPS,
					      TRUE));
#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
		prop_str = g_variant_print(retval, FALSE);
		DLEYNA_LOG_DEBUG("Prop %s = %s", prop, prop_str);
		g_free(prop_str);
#endif
	} else {
		res = prv_get_matching_resource(object, protocol_info);
		if (!res)
			goto on_error;

		retval = prv_get_item_resource_property(prop, res);

#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
		if (retval) {
			prop_str = g_variant_print(retval, FALSE);
			DLEYNA_LOG_DEBUG("Prop %s = %s", prop, prop_str);
			g_free(prop_str);
		}
#endif
		g_object_unref(res);
	}

on_error:

	return retval;
}

GVariant *dls_props_get_container_prop(const gchar *prop,
				       GUPnPDIDLLiteObject *object,
				       const gchar *protocol_info)
{
	gint child_count;
	gboolean searchable;
	GUPnPDIDLLiteContainer *container;
	GVariant *retval = NULL;
	guint uint_val;
	GUPnPDIDLLiteResource *res;
#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
	gchar *prop_str;
#endif
	if (!GUPNP_IS_DIDL_LITE_CONTAINER(object))
		goto on_error;

	container = (GUPnPDIDLLiteContainer *)object;
	if (!strcmp(prop, DLS_INTERFACE_PROP_CHILD_COUNT)) {
		child_count =
			gupnp_didl_lite_container_get_child_count(container);

		DLEYNA_LOG_DEBUG("Prop %s = %d", prop, child_count);

		if (child_count >= 0) {
			retval = g_variant_new_uint32((guint) child_count);
			retval = g_variant_ref_sink(retval);
		}
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_SEARCHABLE)) {
		searchable =
			gupnp_didl_lite_container_get_searchable(container);

		DLEYNA_LOG_DEBUG("Prop %s = %d", prop, searchable);

		retval = g_variant_ref_sink(
			g_variant_new_boolean(searchable));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_CREATE_CLASSES)) {
		retval = g_variant_ref_sink(
			prv_compute_create_classes(container));
#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
		prop_str = g_variant_print(retval, FALSE);
		DLEYNA_LOG_DEBUG("Prop %s = %s", prop, prop_str);
		g_free(prop_str);
#endif
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_CONTAINER_UPDATE_ID) &&
		gupnp_didl_lite_container_container_update_id_is_set(
								container)) {
		uint_val = gupnp_didl_lite_container_get_container_update_id(
								container);

		DLEYNA_LOG_DEBUG("Prop %s = %u", prop, uint_val);

		retval = g_variant_ref_sink(g_variant_new_uint32(uint_val));
	} else if (!strcmp(prop,
				DLS_INTERFACE_PROP_TOTAL_DELETED_CHILD_COUNT) &&
		   gupnp_didl_lite_container_total_deleted_child_count_is_set(
								container)) {
		uint_val =
			gupnp_didl_lite_container_get_total_deleted_child_count(
								container);

		DLEYNA_LOG_DEBUG("Prop %s = %u", prop, uint_val);

		retval = g_variant_ref_sink(g_variant_new_uint32(uint_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_RES_RESOURCES)) {
		retval = g_variant_ref_sink(
			prv_compute_resources(object, DLS_PROP_MASK_ALL_PROPS,
					      FALSE));
#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
		prop_str = g_variant_print(retval, FALSE);
		DLEYNA_LOG_DEBUG("Prop %s = %s", prop, prop_str);
		g_free(prop_str);
#endif
	} else {
		res = prv_get_matching_resource(object, protocol_info);
		if (!res)
			goto on_error;

		retval = prv_get_common_resource_property(prop, res);

#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
		if (retval) {
			prop_str = g_variant_print(retval, FALSE);
			DLEYNA_LOG_DEBUG("Prop %s = %s", prop, prop_str);
			g_free(prop_str);
		}
#endif
		g_object_unref(res);
	}

on_error:

	return retval;
}

static gint prv_mask_to_index(dls_upnp_prop_mask filter_mask)
{
	int x = -1;
	dls_upnp_prop_mask mask = filter_mask;

	while (mask) {
		mask = mask >> 1;
		x++;
	}

	DLEYNA_LOG_DEBUG("##### MASK %lx INDEX %d", filter_mask, x);

	return x;
}

GVariant *dls_props_get_prop_value(GUPnPDeviceInfo *proxy,
				   GUPnPDIDLLiteObject *object,
				   GUPnPDIDLLiteResource *res,
				   const char *root_path,
				   const gchar *parent_path,
				   const dls_device_t *device,
				   dls_props_index_t prop_index,
				   const gchar *protocol_info,
				   gboolean always)
{
	GVariant *retval = NULL;
	gint ivalue;
	guint uvalue;
	gint64 int64_val;
	gchar *str = NULL;
	const char* cstr = NULL;
	gboolean bvalue;
	GList *list;
	GUPnPOCMFlags ocm_flags;
	GUPnPDLNAConversion conv;
	GUPnPDLNAOperation ope;
	GUPnPDLNAFlags flags;
	GUPnPProtocolInfo *p_info;
#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
	dls_prop_map_t *prop_t;
#endif

	switch(prop_index) {
	case DLS_PROP_INDEX_CHILD_COUNT:
		ivalue = gupnp_didl_lite_container_get_child_count(
					GUPNP_DIDL_LITE_CONTAINER(object));
		if (ivalue >= 0)
			retval = g_variant_new_uint32((guint) ivalue);
		break;

	case DLS_PROP_INDEX_PATH:
		cstr = gupnp_didl_lite_object_get_id(object);
		if (!cstr)
			goto on_exit;

		str = dls_path_from_id(root_path, cstr);
		break;

	case DLS_PROP_INDEX_PARENT:
		cstr = gupnp_didl_lite_object_get_parent_id(object);
		if (!cstr || !strcmp(cstr, "-1") || !strcmp(cstr, ""))
			str = g_strdup(root_path);
		else
			str = dls_path_from_id(root_path, cstr);
		break;

	case DLS_PROP_INDEX_REFPATH:
		cstr = gupnp_didl_lite_item_get_ref_id(
						GUPNP_DIDL_LITE_ITEM(object));
		if (!cstr)
			goto on_exit;

		str = dls_path_from_id(root_path, cstr);
		break;

	case DLS_PROP_INDEX_RESTRICTED:
		bvalue = gupnp_didl_lite_object_get_restricted(object);
		retval = g_variant_new_boolean(bvalue);
		break;

	case DLS_PROP_INDEX_SEARCHABLE:
		bvalue = gupnp_didl_lite_container_get_searchable(
					GUPNP_DIDL_LITE_CONTAINER(object));
		retval = g_variant_new_boolean(bvalue);
		break;

	case DLS_PROP_INDEX_CREATOR:
		cstr = gupnp_didl_lite_object_get_creator(object);
		if (!cstr)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_DATE:
		cstr = gupnp_didl_lite_object_get_date(object);
		if (!cstr)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_DISPLAY_NAME:
		cstr = gupnp_didl_lite_object_get_title(object);
		if (!cstr)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_DLNA_MANAGED:
		ocm_flags = gupnp_didl_lite_object_get_dlna_managed(object);
		retval = prv_get_dlna_flags_dict(ocm_flags, g_prop_dlna_ocm);
		break;

	case DLS_PROP_INDEX_RES_RESOURCES:
	case DLS_PROP_INDEX_URL:
	case DLS_PROP_INDEX_URLS:
#warning "To implement"
	break;

	case DLS_PROP_INDEX_BITRATE:
		ivalue = gupnp_didl_lite_resource_get_bitrate(res);
		if (ivalue == -1)
			goto on_exit;
		retval = g_variant_new_int32(ivalue);
		break;

	case DLS_PROP_INDEX_BITS_PER_SAMPLE:
		ivalue = gupnp_didl_lite_resource_get_bits_per_sample(res);
		if (ivalue == -1)
			goto on_exit;
		retval = g_variant_new_int32(ivalue);
		break;

	case DLS_PROP_INDEX_COLOR_DEPTH:
		ivalue = (int) gupnp_didl_lite_resource_get_color_depth(res);
		if (ivalue == -1)
			goto on_exit;
		retval = g_variant_new_int32(ivalue);
		break;

	case DLS_PROP_INDEX_DURATION:
		ivalue = (int) gupnp_didl_lite_resource_get_duration(res);
		if (ivalue == -1)
			goto on_exit;
		retval = g_variant_new_int32(ivalue);
		break;

	case DLS_PROP_INDEX_DLNA_PROFILE:
		p_info = gupnp_didl_lite_resource_get_protocol_info(res);
		if (!p_info)
			goto on_exit;
		cstr = gupnp_protocol_info_get_dlna_profile(p_info);
		if (!cstr)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_MIME_TYPE:
		p_info = gupnp_didl_lite_resource_get_protocol_info(res);
		if (!p_info)
			goto on_exit;
		cstr = gupnp_protocol_info_get_mime_type(p_info);
		if (!cstr)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_HEIGHT:
		ivalue = (int) gupnp_didl_lite_resource_get_height(res);
		if (ivalue == -1)
			goto on_exit;
		retval = g_variant_new_int32(ivalue);
		break;

	case DLS_PROP_INDEX_WIDTH:
		ivalue = (int) gupnp_didl_lite_resource_get_width(res);
		if (ivalue == -1)
			goto on_exit;
		retval = g_variant_new_int32(ivalue);
		break;

	case DLS_PROP_INDEX_SAMPLE_RATE:
		ivalue = gupnp_didl_lite_resource_get_sample_freq(res);
		if (ivalue == -1)
			goto on_exit;
		retval = g_variant_new_int32(ivalue);
		break;

	case DLS_PROP_INDEX_SIZE:
		int64_val = gupnp_didl_lite_resource_get_size64(res);
		if (int64_val == -1)
			goto on_exit;
		retval = g_variant_new_int64(int64_val);
		break;

	case DLS_PROP_INDEX_UPDATE_COUNT:
		if (!gupnp_didl_lite_resource_update_count_is_set(res))
			goto on_exit;
		uvalue = gupnp_didl_lite_resource_get_update_count(res);
		retval = g_variant_new_uint32(uvalue);
		break;

	case DLS_PROP_INDEX_ALBUM:
		cstr = gupnp_didl_lite_object_get_album(object);
		if (!cstr)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_ALBUM_ART_URL:
		cstr = gupnp_didl_lite_object_get_album_art(object);
		if (!cstr)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_ARTIST:
		cstr = gupnp_didl_lite_object_get_artist(object);
		if (!cstr)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_ARTISTS:
		list = gupnp_didl_lite_object_get_artists(object);
		if (!list)
			goto on_exit;

		retval = prv_get_artists_prop(list);
		g_list_free_full(list, g_object_unref);
		break;

	case DLS_PROP_INDEX_TYPE:
		cstr = gupnp_didl_lite_object_get_upnp_class(object);
		cstr = dls_props_upnp_class_to_media_spec(cstr);
		if (!cstr)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_CONTAINER_UID:
		if (!gupnp_didl_lite_container_container_update_id_is_set(
					GUPNP_DIDL_LITE_CONTAINER(object)))
			goto on_exit;

		uvalue = gupnp_didl_lite_container_get_container_update_id(
					GUPNP_DIDL_LITE_CONTAINER(object));

		retval = g_variant_new_uint32(uvalue);
		break;

	case DLS_PROP_INDEX_CREATE_CLASSES:
		retval = prv_compute_create_classes(
					GUPNP_DIDL_LITE_CONTAINER(object));
		break;

	case DLS_PROP_INDEX_GENRE:
		cstr = gupnp_didl_lite_object_get_genre(object);
		if (!cstr)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_OBJECT_UID:
		if (!gupnp_didl_lite_object_update_id_is_set(object))
			goto on_exit;

		uvalue = gupnp_didl_lite_object_get_update_id(object);

		retval = g_variant_new_uint32(uvalue);
		break;

	case DLS_PROP_INDEX_TRACK_NUMBER:
		ivalue = gupnp_didl_lite_object_get_track_number(object);
		if (ivalue < 0)
			goto on_exit;

		retval = g_variant_new_int32(ivalue);
		break;

	case DLS_PROP_INDEX_TOTAL_DELETED_CC:
		if (!gupnp_didl_lite_container_total_deleted_child_count_is_set(
					GUPNP_DIDL_LITE_CONTAINER(object)))
			goto on_exit;

		uvalue =
			gupnp_didl_lite_container_get_total_deleted_child_count(
					GUPNP_DIDL_LITE_CONTAINER(object));
		retval = g_variant_new_uint32(uvalue);
		break;

	case DLS_PROP_INDEX_DLNA_CONVERSION:
		p_info = gupnp_didl_lite_resource_get_protocol_info(res);
		conv = gupnp_protocol_info_get_dlna_conversion(p_info);
		retval = prv_get_dlna_flags_dict(conv, g_prop_dlna_ci);
		break;

	case DLS_PROP_INDEX_DLNA_OPERATION:
		p_info = gupnp_didl_lite_resource_get_protocol_info(res);
		ope = gupnp_protocol_info_get_dlna_operation(p_info);
		retval = prv_get_dlna_flags_dict(ope, g_prop_dlna_op);
		break;

	case DLS_PROP_INDEX_DLNA_FLAGS:
		p_info = gupnp_didl_lite_resource_get_protocol_info(res);
		flags = gupnp_protocol_info_get_dlna_flags(p_info);
		retval =  prv_get_dlna_flags_dict(flags, g_prop_dlna_flags);

	case DLS_PROP_INDEX_LOCATION:
		cstr = gupnp_device_info_get_location(proxy);
		if (!cstr)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_UDN:
		cstr = gupnp_device_info_get_udn(proxy);
		if (!cstr)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_DEVICE_TYPE:
		cstr = gupnp_device_info_get_device_type(proxy);
		if (!cstr)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_FRIENDLY_NAME:
		str = gupnp_device_info_get_friendly_name(proxy);
		if (!str)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_MANUFACTURER:
		str = gupnp_device_info_get_manufacturer(proxy);
		if (!str)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_MANUFACTURER_URL:
		str = gupnp_device_info_get_manufacturer_url(proxy);
		if (!str)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_MODEL_DESC:
		str = gupnp_device_info_get_model_description(proxy);
		if (!str)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_MODEL_NAME:
		str = gupnp_device_info_get_model_name(proxy);
		if (!str)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_MODEL_NUMBER:
		str = gupnp_device_info_get_model_number(proxy);
		if (!str)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_MODEL_URL:
		str = gupnp_device_info_get_model_url(proxy);
		if (!str)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_SERIAL_NUMBER:
		str = gupnp_device_info_get_serial_number(proxy);
		if (!str)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_PRESENTATION_URL:
		str = gupnp_device_info_get_presentation_url(proxy);
		if (!str)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_ICON_URL:
		str = gupnp_device_info_get_icon_url(proxy, NULL, -1, -1, -1,
						     FALSE, NULL, NULL, NULL,
						     NULL);
		if (!str)
			goto on_exit;
		break;

	case DLS_PROP_INDEX_SV_DLNA_CAPS:
		list = gupnp_device_info_list_dlna_capabilities(proxy);
		if (list == NULL)
			goto on_exit;

		retval = prv_add_list_dlna_prop(list);
		g_list_free_full(list, g_free);
		break;

	case DLS_PROP_INDEX_SV_SEARCH_CAPS:
		if (device->search_caps != NULL)
			retval = device->search_caps;
		break;

	case DLS_PROP_INDEX_SV_SORT_CAPS:
		if (device->sort_caps != NULL)
			retval = device->sort_caps;
		break;

	case DLS_PROP_INDEX_SV_SORT_EXT_CAPS:
		if (device->sort_ext_caps != NULL)
			retval = device->sort_ext_caps;
		break;

	case DLS_PROP_INDEX_SV_FEATURE_LIST:
		if (device->feature_list != NULL)
			retval = device->feature_list;
		break;

/*	case DLS_PROP_INDEX_SV_SRT:
 * 	Managed in a dedicated task
 */
	case DLS_PROP_INDEX_ESV_SYSTEM_UID:
		uvalue = device->system_update_id;
		retval = g_variant_new_uint32(uvalue);
	break;

	default:
		break;
	};

	if (str != NULL)
		retval = g_variant_new_string(str);
	else if (cstr != NULL)
		retval = g_variant_new_string(cstr);

on_exit:
	g_free(str);

#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
	str = NULL;
	cstr = NULL;

	if (retval != NULL)
		str = g_variant_print(retval, FALSE);

	prop_t = dls_props_def_get();
	cstr = prop_t[prop_index].dls_prop_name;

	DLEYNA_LOG_DEBUG("Prop %s = %s", cstr, str);

	g_free(str);
#endif

	return retval;
}

