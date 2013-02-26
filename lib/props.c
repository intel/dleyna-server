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

#include <libdleyna/core/log.h>

#include "device.h"
#include "interface.h"
#include "path.h"
#include "props.h"

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

static dls_prop_map_t *prv_prop_map_new(const gchar *prop_name,
					    dls_upnp_prop_mask type,
					    gboolean filter,
					    gboolean searchable,
					    gboolean updateable)
{
	dls_prop_map_t *retval = g_new(dls_prop_map_t, 1);
	retval->upnp_prop_name = prop_name;
	retval->type = type;
	retval->filter = filter;
	retval->searchable = searchable;
	retval->updateable = updateable;
	return retval;
}

void dls_prop_maps_new(GHashTable **property_map, GHashTable **filter_map)
{
	dls_prop_map_t *prop_t;
	GHashTable *p_map;
	GHashTable *f_map;

	p_map = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	f_map = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);

	/* @childCount */
	prop_t = prv_prop_map_new("@childCount",
					DLS_UPNP_MASK_PROP_CHILD_COUNT,
					TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_CHILD_COUNT, prop_t);
	g_hash_table_insert(p_map, "@childCount",
			    DLS_INTERFACE_PROP_CHILD_COUNT);

	/* @id */
	prop_t = prv_prop_map_new("@id",
					DLS_UPNP_MASK_PROP_PATH,
					FALSE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_PATH, prop_t);
	g_hash_table_insert(p_map, "@id", DLS_INTERFACE_PROP_PATH);

	/* @parentID */
	prop_t = prv_prop_map_new("@parentID",
					DLS_UPNP_MASK_PROP_PARENT,
					FALSE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_PARENT, prop_t);
	g_hash_table_insert(p_map, "@parentID", DLS_INTERFACE_PROP_PARENT);

	/* @refID */
	prop_t = prv_prop_map_new("@refID",
					DLS_UPNP_MASK_PROP_REFPATH,
					TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_REFPATH, prop_t);
	g_hash_table_insert(p_map, "@refID", DLS_INTERFACE_PROP_REFPATH);

	/* @restricted */
	prop_t = prv_prop_map_new("@restricted",
					DLS_UPNP_MASK_PROP_RESTRICTED,
					TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_RESTRICTED, prop_t);
	g_hash_table_insert(p_map, "@restricted",
			    DLS_INTERFACE_PROP_RESTRICTED);

	/* @searchable */
	prop_t = prv_prop_map_new("@searchable",
					DLS_UPNP_MASK_PROP_SEARCHABLE,
					TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_SEARCHABLE, prop_t);
	g_hash_table_insert(p_map, "@searchable",
			    DLS_INTERFACE_PROP_SEARCHABLE);

	/* dc:creator */
	prop_t = prv_prop_map_new("dc:creator",
					DLS_UPNP_MASK_PROP_CREATOR,
					TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_CREATOR, prop_t);
	g_hash_table_insert(p_map, "dc:creator", DLS_INTERFACE_PROP_CREATOR);

	/* dc:date */
	prop_t = prv_prop_map_new("dc:date",
					DLS_UPNP_MASK_PROP_DATE,
					TRUE, TRUE, TRUE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_DATE, prop_t);
	g_hash_table_insert(p_map, "dc:date", DLS_INTERFACE_PROP_DATE);

	/* dc:title */
	prop_t = prv_prop_map_new("dc:title",
					DLS_UPNP_MASK_PROP_DISPLAY_NAME,
					FALSE, TRUE, TRUE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_DISPLAY_NAME, prop_t);
	g_hash_table_insert(p_map, "dc:title", DLS_INTERFACE_PROP_DISPLAY_NAME);

	/* dlna:dlnaManaged */
	prop_t = prv_prop_map_new("dlna:dlnaManaged",
					DLS_UPNP_MASK_PROP_DLNA_MANAGED,
					TRUE, FALSE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_DLNA_MANAGED, prop_t);
	g_hash_table_insert(p_map, "dlna:dlnaManaged",
			    DLS_INTERFACE_PROP_DLNA_MANAGED);

	/* res */
	/* res - RES */
	prop_t = prv_prop_map_new("res",
					DLS_UPNP_MASK_PROP_RESOURCES,
					TRUE, FALSE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_RESOURCES, prop_t);

	/* res - URL */
	prop_t = prv_prop_map_new("res",
					DLS_UPNP_MASK_PROP_URL,
					TRUE, FALSE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_URL, prop_t);

	/* res - URLS */
	prop_t = prv_prop_map_new("res",
					DLS_UPNP_MASK_PROP_URLS,
					TRUE, FALSE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_URLS, prop_t);

	/* res@bitrate */
	prop_t = prv_prop_map_new("res@bitrate",
					DLS_UPNP_MASK_PROP_BITRATE,
					TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_BITRATE, prop_t);
	g_hash_table_insert(p_map, "res@bitrate", DLS_INTERFACE_PROP_BITRATE);

	/* res@bitsPerSample */
	prop_t = prv_prop_map_new("res@bitsPerSample",
					DLS_UPNP_MASK_PROP_BITS_PER_SAMPLE,
					TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_BITS_PER_SAMPLE, prop_t);
	g_hash_table_insert(p_map, "res@bitsPerSample",
			    DLS_INTERFACE_PROP_BITS_PER_SAMPLE);

	/* res@colorDepth */
	prop_t = prv_prop_map_new("res@colorDepth",
					DLS_UPNP_MASK_PROP_COLOR_DEPTH,
					TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_COLOR_DEPTH, prop_t);
	g_hash_table_insert(p_map, "res@colorDepth",
			    DLS_INTERFACE_PROP_COLOR_DEPTH);

	/* res@duration */
	prop_t = prv_prop_map_new("res@duration",
					DLS_UPNP_MASK_PROP_DURATION,
					TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_DURATION, prop_t);
	g_hash_table_insert(p_map, "res@duration",
			    DLS_INTERFACE_PROP_DURATION);

	/* res@protocolInfo */
	/* res@protocolInfo - DLNA PROFILE*/
	prop_t = prv_prop_map_new("res@protocolInfo",
				       DLS_UPNP_MASK_PROP_DLNA_PROFILE,
				       TRUE, FALSE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_DLNA_PROFILE, prop_t);

	/* res@protocolInfo - MIME TYPES*/
	prop_t = prv_prop_map_new("res@protocolInfo",
					DLS_UPNP_MASK_PROP_MIME_TYPE,
					TRUE, FALSE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_MIME_TYPE, prop_t);

	/* res@resolution */
	/* res@resolution - HEIGH */
	prop_t = prv_prop_map_new("res@resolution",
					DLS_UPNP_MASK_PROP_HEIGHT,
					TRUE, FALSE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_HEIGHT, prop_t);

	/* res@resolution - WIDTH */
	prop_t = prv_prop_map_new("res@resolution",
					DLS_UPNP_MASK_PROP_WIDTH,
					TRUE, FALSE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_WIDTH, prop_t);

	/* res@sampleFrequency */
	prop_t = prv_prop_map_new("res@sampleFrequency",
					DLS_UPNP_MASK_PROP_SAMPLE_RATE,
					TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_SAMPLE_RATE, prop_t);
	g_hash_table_insert(p_map, "res@sampleFrequency",
			    DLS_INTERFACE_PROP_SAMPLE_RATE);

	/* res@size */
	prop_t = prv_prop_map_new("res@size",
					DLS_UPNP_MASK_PROP_SIZE,
				       TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_SIZE, prop_t);
	g_hash_table_insert(p_map, "res@size", DLS_INTERFACE_PROP_SIZE);

	/* res@updateCount */
	prop_t = prv_prop_map_new("res@updateCount",
				      DLS_UPNP_MASK_PROP_UPDATE_COUNT,
				      TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_UPDATE_COUNT, prop_t);
	g_hash_table_insert(p_map, "res@updateCount",
			    DLS_INTERFACE_PROP_UPDATE_COUNT);

	/* upnp:album */
	prop_t = prv_prop_map_new("upnp:album",
					DLS_UPNP_MASK_PROP_ALBUM,
				       TRUE, TRUE, TRUE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_ALBUM, prop_t);
	g_hash_table_insert(p_map, "upnp:album", DLS_INTERFACE_PROP_ALBUM);

	/* upnp:albumArtURI */
	prop_t = prv_prop_map_new("upnp:albumArtURI",
					DLS_UPNP_MASK_PROP_ALBUM_ART_URL,
					TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_ALBUM_ART_URL, prop_t);
	g_hash_table_insert(p_map, "upnp:albumArtURI",
			    DLS_INTERFACE_PROP_ALBUM_ART_URL);

	/* upnp:artist */
	/* upnp:artist - ARTIST*/
	prop_t = prv_prop_map_new("upnp:artist",
					DLS_UPNP_MASK_PROP_ARTIST,
					TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_ARTIST, prop_t);
	g_hash_table_insert(p_map, "upnp:artist", DLS_INTERFACE_PROP_ARTIST);

	/* upnp:artist - ARTISTS*/
	prop_t = prv_prop_map_new("upnp:artist",
					DLS_UPNP_MASK_PROP_ARTISTS,
					TRUE, FALSE, TRUE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_ARTISTS, prop_t);

	/* upnp:class */
	prop_t = prv_prop_map_new("upnp:class",
					DLS_UPNP_MASK_PROP_TYPE,
					FALSE, TRUE, TRUE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_TYPE, prop_t);
	g_hash_table_insert(p_map, "upnp:class", DLS_INTERFACE_PROP_TYPE);

	/* upnp:containerUpdateID */
	prop_t = prv_prop_map_new("upnp:containerUpdateID",
				      DLS_UPNP_MASK_PROP_CONTAINER_UPDATE_ID,
				      TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_CONTAINER_UPDATE_ID,
			    prop_t);
	g_hash_table_insert(p_map, "upnp:containerUpdateID",
			    DLS_INTERFACE_PROP_CONTAINER_UPDATE_ID);

	/* upnp:createClass */
	prop_t = prv_prop_map_new("upnp:createClass",
					DLS_UPNP_MASK_PROP_CREATE_CLASSES,
					TRUE, FALSE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_CREATE_CLASSES, prop_t);

	/* upnp:genre */
	prop_t = prv_prop_map_new("upnp:genre",
					DLS_UPNP_MASK_PROP_GENRE,
					TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_GENRE, prop_t);
	g_hash_table_insert(p_map, "upnp:genre", DLS_INTERFACE_PROP_GENRE);

	/* upnp:objectUpdateID */
	prop_t = prv_prop_map_new("upnp:objectUpdateID",
				      DLS_UPNP_MASK_PROP_OBJECT_UPDATE_ID,
				      TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_OBJECT_UPDATE_ID, prop_t);
	g_hash_table_insert(p_map, "upnp:objectUpdateID",
			    DLS_INTERFACE_PROP_OBJECT_UPDATE_ID);

	/* upnp:originalTrackNumber */
	prop_t = prv_prop_map_new("upnp:originalTrackNumber",
					DLS_UPNP_MASK_PROP_TRACK_NUMBER,
					TRUE, TRUE, TRUE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_TRACK_NUMBER, prop_t);
	g_hash_table_insert(p_map, "upnp:originalTrackNumber",
			    DLS_INTERFACE_PROP_TRACK_NUMBER);

	/* upnp:totalDeletedChildCount */
	prop_t = prv_prop_map_new("upnp:totalDeletedChildCount",
				DLS_UPNP_MASK_PROP_TOTAL_DELETED_CHILD_COUNT,
				TRUE, TRUE, FALSE);
	g_hash_table_insert(f_map, DLS_INTERFACE_PROP_TOTAL_DELETED_CHILD_COUNT,
			    prop_t);
	g_hash_table_insert(p_map, "upnp:totalDeletedChildCount",
			    DLS_INTERFACE_PROP_TOTAL_DELETED_CHILD_COUNT);

	*filter_map = f_map;
	*property_map = p_map;
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
		mask = DLS_UPNP_MASK_ALL_PROPS;
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
	if (len > 0)
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

static void prv_parse_resources(GVariantBuilder *item_vb,
				GUPnPDIDLLiteResource *res,
				dls_upnp_prop_mask filter_mask)
{
	GUPnPProtocolInfo *protocol_info;
	int int_val;
	gint64 int64_val;
	const char *str_val;
	guint uint_val;

	if (filter_mask & DLS_UPNP_MASK_PROP_SIZE) {
		int64_val = gupnp_didl_lite_resource_get_size64(res);
		prv_add_int64_prop(item_vb, DLS_INTERFACE_PROP_SIZE, int64_val);
	}

	if (filter_mask & DLS_UPNP_MASK_PROP_BITRATE) {
		int_val = gupnp_didl_lite_resource_get_bitrate(res);
		prv_add_int_prop(item_vb, DLS_INTERFACE_PROP_BITRATE, int_val);
	}

	if (filter_mask & DLS_UPNP_MASK_PROP_SAMPLE_RATE) {
		int_val = gupnp_didl_lite_resource_get_sample_freq(res);
		prv_add_int_prop(item_vb, DLS_INTERFACE_PROP_SAMPLE_RATE,
				 int_val);
	}

	if (filter_mask & DLS_UPNP_MASK_PROP_BITS_PER_SAMPLE) {
		int_val = gupnp_didl_lite_resource_get_bits_per_sample(res);
		prv_add_int_prop(item_vb, DLS_INTERFACE_PROP_BITS_PER_SAMPLE,
				 int_val);
	}

	if (filter_mask & DLS_UPNP_MASK_PROP_DURATION) {
		int_val = (int) gupnp_didl_lite_resource_get_duration(res);
		prv_add_int_prop(item_vb, DLS_INTERFACE_PROP_DURATION, int_val);
	}

	if (filter_mask & DLS_UPNP_MASK_PROP_WIDTH) {
		int_val = (int) gupnp_didl_lite_resource_get_width(res);
		prv_add_int_prop(item_vb, DLS_INTERFACE_PROP_WIDTH, int_val);
	}

	if (filter_mask & DLS_UPNP_MASK_PROP_HEIGHT) {
		int_val = (int) gupnp_didl_lite_resource_get_height(res);
		prv_add_int_prop(item_vb, DLS_INTERFACE_PROP_HEIGHT, int_val);
	}

	if (filter_mask & DLS_UPNP_MASK_PROP_COLOR_DEPTH) {
		int_val = (int) gupnp_didl_lite_resource_get_color_depth(res);
		prv_add_int_prop(item_vb, DLS_INTERFACE_PROP_COLOR_DEPTH,
				 int_val);
	}

	if (filter_mask & DLS_UPNP_MASK_PROP_UPDATE_COUNT) {
		uint_val = gupnp_didl_lite_resource_get_update_count(res);
		prv_add_uint_prop(item_vb, DLS_INTERFACE_PROP_UPDATE_COUNT,
				  uint_val);
	}

	protocol_info = gupnp_didl_lite_resource_get_protocol_info(res);

	if (filter_mask & DLS_UPNP_MASK_PROP_DLNA_PROFILE) {
		str_val = gupnp_protocol_info_get_dlna_profile(protocol_info);
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_DLNA_PROFILE,
				    str_val);
	}

	if (filter_mask & DLS_UPNP_MASK_PROP_MIME_TYPE) {
		str_val = gupnp_protocol_info_get_mime_type(protocol_info);
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_MIME_TYPE,
				    str_val);
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

	return retval;
}

const gchar *dls_props_upnp_class_to_media_spec(const gchar *upnp_class)
{
	const gchar *retval = NULL;
	const gchar *ptr;

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

	return retval;
}

static GVariant *prv_props_get_dlna_managed_dict(GUPnPOCMFlags flags)
{
	GVariantBuilder builder;
	gboolean managed;

	g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sb}"));

	managed = (flags & GUPNP_OCM_FLAGS_UPLOAD);
	g_variant_builder_add(&builder, "{sb}", "Upload", managed);

	managed = (flags & GUPNP_OCM_FLAGS_CREATE_CONTAINER);
	g_variant_builder_add(&builder, "{sb}", "CreateContainer", managed);

	managed = (flags & GUPNP_OCM_FLAGS_DESTROYABLE);
	g_variant_builder_add(&builder, "{sb}", "Delete", managed);

	managed = (flags & GUPNP_OCM_FLAGS_UPLOAD_DESTROYABLE);
	g_variant_builder_add(&builder, "{sb}", "UploadDelete", managed);

	managed = (flags & GUPNP_OCM_FLAGS_CHANGE_METADATA);
	g_variant_builder_add(&builder, "{sb}", "ChangeMeta", managed);

	return g_variant_builder_end(&builder);
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

	if (filter_mask & DLS_UPNP_MASK_PROP_DISPLAY_NAME)
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_DISPLAY_NAME,
				    title);

	if (filter_mask & DLS_UPNP_MASK_PROP_CREATOR)
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_CREATOR,
				    creator);

	if (filter_mask & DLS_UPNP_MASK_PROP_PATH)
		prv_add_path_prop(item_vb, DLS_INTERFACE_PROP_PATH, path);

	if (filter_mask & DLS_UPNP_MASK_PROP_PARENT)
		prv_add_path_prop(item_vb, DLS_INTERFACE_PROP_PARENT,
				  parent_path);

	if (filter_mask & DLS_UPNP_MASK_PROP_TYPE)
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_TYPE,
				    media_spec_type);

	if (filter_mask & DLS_UPNP_MASK_PROP_RESTRICTED)
		prv_add_bool_prop(item_vb, DLS_INTERFACE_PROP_RESTRICTED, rest);

	if (filter_mask & DLS_UPNP_MASK_PROP_DLNA_MANAGED) {
		flags = gupnp_didl_lite_object_get_dlna_managed(object);
		prv_add_variant_prop(item_vb,
				     DLS_INTERFACE_PROP_DLNA_MANAGED,
				     prv_props_get_dlna_managed_dict(flags));
	}

	if (filter_mask & DLS_UPNP_MASK_PROP_OBJECT_UPDATE_ID) {
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
			     gboolean *have_child_count)
{
	int child_count;
	gboolean searchable;
	guint uint_val;

	*have_child_count = FALSE;
	if (filter_mask & DLS_UPNP_MASK_PROP_CHILD_COUNT) {
		child_count = gupnp_didl_lite_container_get_child_count(object);
		if (child_count >= 0) {
			prv_add_uint_prop(item_vb,
					  DLS_INTERFACE_PROP_CHILD_COUNT,
					  (unsigned int) child_count);
			*have_child_count = TRUE;
		}
	}

	if (filter_mask & DLS_UPNP_MASK_PROP_SEARCHABLE) {
		searchable = gupnp_didl_lite_container_get_searchable(object);
		prv_add_bool_prop(item_vb, DLS_INTERFACE_PROP_SEARCHABLE,
				  searchable);
	}

	if (filter_mask & DLS_UPNP_MASK_PROP_CREATE_CLASSES)
		prv_add_variant_prop(item_vb,
				     DLS_INTERFACE_PROP_CREATE_CLASSES,
				     prv_compute_create_classes(object));

	if (filter_mask & DLS_UPNP_MASK_PROP_CONTAINER_UPDATE_ID) {
		uint_val = gupnp_didl_lite_container_get_container_update_id(
									object);
		prv_add_uint_prop(item_vb,
				  DLS_INTERFACE_PROP_CONTAINER_UPDATE_ID,
				  uint_val);
	}

	if (filter_mask & DLS_UPNP_MASK_PROP_TOTAL_DELETED_CHILD_COUNT) {
		uint_val =
		gupnp_didl_lite_container_get_total_deleted_child_count(object);
		prv_add_uint_prop(item_vb,
				  DLS_INTERFACE_PROP_TOTAL_DELETED_CHILD_COUNT,
				  uint_val);
	}
}

static GVariant *prv_compute_resources(GUPnPDIDLLiteObject *object,
				       dls_upnp_prop_mask filter_mask)
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
		if (filter_mask & DLS_UPNP_MASK_PROP_URL) {
			str_val = gupnp_didl_lite_resource_get_uri(res);
			if (str_val)
				prv_add_string_prop(res_vb,
						    DLS_INTERFACE_PROP_URL,
						    str_val);
		}
		prv_parse_resources(res_vb, res, filter_mask);
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
			      dls_upnp_prop_mask filter_mask)
{
	GVariant *val;

	val = prv_compute_resources(object, filter_mask);
	g_variant_builder_add(item_vb, "{sv}", DLS_INTERFACE_PROP_RESOURCES,
			      val);
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

	if (filter_mask & DLS_UPNP_MASK_PROP_ARTIST)
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_ARTIST,
				    gupnp_didl_lite_object_get_artist(object));

	if (filter_mask & DLS_UPNP_MASK_PROP_ARTISTS) {
		list = gupnp_didl_lite_object_get_artists(object);
		prv_add_variant_prop(item_vb, DLS_INTERFACE_PROP_ARTISTS,
				     prv_get_artists_prop(list));
		g_list_free_full(list, g_object_unref);
	}

	if (filter_mask & DLS_UPNP_MASK_PROP_ALBUM)
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_ALBUM,
				    gupnp_didl_lite_object_get_album(object));

	if (filter_mask & DLS_UPNP_MASK_PROP_DATE)
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_DATE,
				    gupnp_didl_lite_object_get_date(object));

	if (filter_mask & DLS_UPNP_MASK_PROP_GENRE)
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_GENRE,
				    gupnp_didl_lite_object_get_genre(object));

	if (filter_mask & DLS_UPNP_MASK_PROP_TRACK_NUMBER) {
		track_number = gupnp_didl_lite_object_get_track_number(object);
		if (track_number >= 0)
			prv_add_int_prop(item_vb,
					 DLS_INTERFACE_PROP_TRACK_NUMBER,
					 track_number);
	}

	if (filter_mask & DLS_UPNP_MASK_PROP_ALBUM_ART_URL)
		prv_add_string_prop(item_vb, DLS_INTERFACE_PROP_ALBUM_ART_URL,
				    gupnp_didl_lite_object_get_album_art(
					    object));

	if (filter_mask & DLS_UPNP_MASK_PROP_REFPATH) {
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
		if (filter_mask & DLS_UPNP_MASK_PROP_URLS) {
			str_val = gupnp_didl_lite_resource_get_uri(res);
			if (str_val)
				prv_add_strv_prop(item_vb,
						  DLS_INTERFACE_PROP_URLS,
						  &str_val, 1);
		}
		prv_parse_resources(item_vb, res, filter_mask);
		g_object_unref(res);
	}

	if (filter_mask & DLS_UPNP_MASK_PROP_RESOURCES)
		prv_add_resources(item_vb, object, filter_mask);
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
		if (filter_mask & DLS_UPNP_MASK_PROP_URL) {
			str_val = gupnp_didl_lite_resource_get_uri(res);
			if (str_val)
				prv_add_string_prop(item_vb,
						    DLS_INTERFACE_PROP_URL,
						    str_val);
		}
		prv_parse_resources(item_vb, res, filter_mask);
		g_object_unref(res);
	}
}


static GVariant *prv_get_resource_property(const gchar *prop,
					   GUPnPDIDLLiteResource *res)
{
	int int_val;
	gint64 int64_val;
	const char *str_val;
	GVariant *retval = NULL;
	GUPnPProtocolInfo *protocol_info;

	if (!strcmp(prop, DLS_INTERFACE_PROP_DLNA_PROFILE)) {
		protocol_info = gupnp_didl_lite_resource_get_protocol_info(res);
		if (!protocol_info)
			goto on_error;
		str_val = gupnp_protocol_info_get_dlna_profile(protocol_info);
		if (!str_val)
			goto on_error;
		retval = g_variant_ref_sink(g_variant_new_string(str_val));
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
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_DURATION)) {
		int_val = (int) gupnp_didl_lite_resource_get_duration(res);
		if (int_val == -1)
			goto on_error;
		retval = g_variant_ref_sink(g_variant_new_int32(int_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_BITRATE)) {
		int_val = gupnp_didl_lite_resource_get_bitrate(res);
		if (int_val == -1)
			goto on_error;
		retval = g_variant_ref_sink(g_variant_new_int32(int_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_SAMPLE_RATE)) {
		int_val = gupnp_didl_lite_resource_get_sample_freq(res);
		if (int_val == -1)
			goto on_error;
		retval = g_variant_ref_sink(g_variant_new_int32(int_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_BITS_PER_SAMPLE)) {
		int_val = gupnp_didl_lite_resource_get_bits_per_sample(res);
		if (int_val == -1)
			goto on_error;
		retval = g_variant_ref_sink(g_variant_new_int32(int_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_WIDTH)) {
		int_val = (int) gupnp_didl_lite_resource_get_width(res);
		if (int_val == -1)
			goto on_error;
		retval = g_variant_ref_sink(g_variant_new_int32(int_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_HEIGHT)) {
		int_val = (int) gupnp_didl_lite_resource_get_height(res);
		if (int_val == -1)
			goto on_error;
		retval = g_variant_ref_sink(g_variant_new_int32(int_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_COLOR_DEPTH)) {
		int_val = (int) gupnp_didl_lite_resource_get_color_depth(res);
		if (int_val == -1)
			goto on_error;
		retval = g_variant_ref_sink(g_variant_new_int32(int_val));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_URLS)) {
		str_val = gupnp_didl_lite_resource_get_uri(res);
		if (str_val)
			retval = g_variant_new_strv(&str_val, 1);
	}

on_error:

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

		retval = g_variant_ref_sink(
				prv_props_get_dlna_managed_dict(dlna_managed));
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_OBJECT_UPDATE_ID)) {
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
		path = g_variant_print(retval, FALSE);
		DLEYNA_LOG_DEBUG("Prop %s = %s", prop, path);
		g_free(path);
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
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_RESOURCES)) {
		retval = g_variant_ref_sink(
			prv_compute_resources(object, DLS_UPNP_MASK_ALL_PROPS));
	} else {
		res = prv_get_matching_resource(object, protocol_info);
		if (!res)
			goto on_error;

		retval = prv_get_resource_property(prop, res);

		g_object_unref(res);
	}

on_error:

	return retval;
}

GVariant *dls_props_get_container_prop(const gchar *prop,
				       GUPnPDIDLLiteObject *object)
{
	gint child_count;
	gboolean searchable;
	GUPnPDIDLLiteContainer *container;
	GVariant *retval = NULL;
	guint uint_val;
#if DLEYNA_LOG_LEVEL & DLEYNA_LOG_LEVEL_DEBUG
	gchar *create_classes;
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
		create_classes = g_variant_print(retval, FALSE);
		DLEYNA_LOG_DEBUG("Prop %s = %s", prop, create_classes);
		g_free(create_classes);
#endif
	} else if (!strcmp(prop, DLS_INTERFACE_PROP_CONTAINER_UPDATE_ID)) {
		uint_val = gupnp_didl_lite_container_get_container_update_id(
								container);

		DLEYNA_LOG_DEBUG("Prop %s = %u", prop, uint_val);

		retval = g_variant_ref_sink(g_variant_new_uint32(uint_val));
	} else if (!strcmp(prop,
				DLS_INTERFACE_PROP_TOTAL_DELETED_CHILD_COUNT)) {
		uint_val =
			gupnp_didl_lite_container_get_total_deleted_child_count(
								container);

		DLEYNA_LOG_DEBUG("Prop %s = %u", prop, uint_val);

		retval = g_variant_ref_sink(g_variant_new_uint32(uint_val));
	}

on_error:

	return retval;
}
