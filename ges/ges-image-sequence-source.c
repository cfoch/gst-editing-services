/* GStreamer Editing Services
 * Copyright (C) 2013 Lubosz Sarnecki <lubosz@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * SECTION:gesimagesequencesource
 * @short_description: outputs the video stream from a sequence of images.
 *
 * Outputs the video stream from a given image sequence.
 */
#include <stdlib.h>
#include <string.h>
#include "ges-internal.h"
#include "ges-track-element.h"
#include "ges-image-sequence-source.h"
#include "ges-extractable.h"
#include "ges-uri-asset.h"
#include "ges-internal.h"
#include "ges-uri-clip.h"

/* Extractable interface implementation */

static gchar *
ges_extractable_check_id (GType type, const gchar * id, GError ** error)
{
  return g_strdup (id);
}

static void
ges_extractable_interface_init (GESExtractableInterface * iface)
{
  iface->check_id = ges_extractable_check_id;
}

G_DEFINE_TYPE_WITH_CODE (GESImageSequenceSource, ges_image_sequence_source,
    GES_TYPE_VIDEO_SOURCE,
    G_IMPLEMENT_INTERFACE (GES_TYPE_EXTRACTABLE,
        ges_extractable_interface_init));

struct _GESImageSequenceSourcePrivate
{
  GstElement *src;
  GStrv filenames_list;
  gint fps_n, fps_d;
};

enum
{
  PROP_0,
  PROP_URI,
  PROP_FILENAMES
};

static void
ges_image_sequence_source_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GESImageSequenceSource *source = GES_IMAGE_SEQUENCE_SOURCE (object);

  switch (property_id) {
    case PROP_URI:
      g_value_set_string (value, source->uri);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_image_sequence_source_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GESImageSequenceSource *source = GES_IMAGE_SEQUENCE_SOURCE (object);

  switch (property_id) {
    case PROP_URI:
    {
      gchar *uri;

      uri = g_value_dup_string (value);

      if (source->priv->src) {
        if (g_strcmp0 (g_strdup (source->uri), uri) == 0) {

          GstDiscovererInfo *info;
          GstDiscoverer *discoverer;
          GESUriClipAsset *asset;
          GESUriClip *clip;
          GError *lerror = NULL;

          clip = GES_URI_CLIP (ges_timeline_element_get_parent
              (GES_TIMELINE_ELEMENT (source)));
          asset = GES_URI_CLIP_ASSET (ges_extractable_get_asset
              (GES_EXTRACTABLE (clip)));

          discoverer = GES_URI_CLIP_ASSET_GET_CLASS (asset)->sync_discoverer;
          info = gst_discoverer_discover_uri (discoverer, uri, &lerror);

          ges_uri_clip_asset_set_info (asset, info);

          g_object_set (source->priv->src, "uri", uri, NULL);
        }
      }
      source->uri = g_strdup (uri);
      g_free (uri);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

/**
 * ges_image_sequence_source_set_filenames:
 * @self: the #GESImageSequenceSource* to get text on
 * @filenames_list: list of filenames
 *
 * Sets the filenames this source will render.
 *
 */
void
ges_image_sequence_source_set_filenames (GESImageSequenceSource * self,
    GStrv filenames_list)
{
  self->priv->filenames_list = g_strdupv (filenames_list);
  if (self->priv->src)
    g_object_set (self->priv->src, "filenames-list", self->priv->filenames_list,
        NULL);
}

/**
 * ges_image_sequence_source_set_framerate:
 * @self: the #GESImageSequenceSource* to set the framerate on
 * @fps_n: the framerate numerator value
 * @fps_d: the framerate denominator value
 *
 * Sets the framerate to this source.
 *
 */
void
ges_image_sequence_source_set_framerate (GESImageSequenceSource * self,
    gint fps_n, gint fps_d)
{
  self->priv->fps_n = fps_n;
  self->priv->fps_d = fps_d;

  if (self->priv->src)
    g_object_set (self->priv->src, "framerate", self->priv->fps_n,
        self->priv->fps_d, NULL);
}

/**
 * ges_image_sequence_source_get_filenames:
 * @self: the #GESImageSequenceSource*
 *
 * Returns: (array zero-terminated=1) (element-type utf8) (transfer none): A list of filenames.
 *
 */
gchar **
ges_image_sequence_source_get_filenames (GESImageSequenceSource * self)
{
  return self->priv->filenames_list;
}

/**
 * ges_image_sequence_source_get_framerate:
 * @self: the #GESImageSequenceSource* to set the framerate on
 * @fps_n: (out): the framerate numerator value
 * @fps_d: (out): the framerate denominator value
 *
 * Sets the framerate to this source.
 *
 */
void
ges_image_sequence_source_get_framerate (GESImageSequenceSource * self,
    gint * fps_n, gint * fps_d)
{
  *fps_n = self->priv->fps_n;
  *fps_d = self->priv->fps_d;
}

static void
ges_image_sequence_source_dispose (GObject * object)
{
  GESImageSequenceSource *source = GES_IMAGE_SEQUENCE_SOURCE (object);

  if (source->uri)
    g_free (source->uri);
  g_strfreev (source->priv->filenames_list);
  G_OBJECT_CLASS (ges_image_sequence_source_parent_class)->dispose (object);
}

static void
pad_added_cb (GstElement * decodebin, GstPad * pad, GstElement * bin)
{
  GstPad *srcpad;

  srcpad = gst_ghost_pad_new (NULL, pad);

  gst_pad_set_active (srcpad, TRUE);
  gst_element_add_pad (bin, srcpad);
  gst_element_no_more_pads (bin);
}

static GstElement *
ges_image_sequence_source_create_source (GESTrackElement * track_element)
{
  GESImageSequenceSource *self;
  GstElement *bin, *decodebin;

  self = (GESImageSequenceSource *) track_element;

  bin = GST_ELEMENT (gst_bin_new ("multi-image-bin"));
  self->priv->src = gst_element_factory_make ("imagesequencesrc", NULL);

  if (self->uri)
    g_object_set (self->priv->src, "uri", self->uri, NULL);
  if (self->priv->filenames_list)
    g_object_set (self->priv->src, "filenames_list", self->priv->filenames_list,
        NULL);
  if ((self->priv->fps_n != -1) && (self->priv->fps_d != -1))
    ges_image_sequence_source_set_framerate (self, self->priv->fps_n,
        self->priv->fps_d);


  decodebin = gst_element_factory_make ("decodebin", NULL);

  gst_bin_add_many (GST_BIN (bin), self->priv->src, decodebin, NULL);
  gst_element_link_pads_full (self->priv->src, "src", decodebin, "sink",
      GST_PAD_LINK_CHECK_NOTHING);

  g_signal_connect (G_OBJECT (decodebin), "pad-added",
      G_CALLBACK (pad_added_cb), bin);

  return bin;
}

static void
ges_image_sequence_source_class_init (GESImageSequenceSourceClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GESVideoSourceClass *source_class = GES_VIDEO_SOURCE_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESImageSequenceSourcePrivate));

  object_class->get_property = ges_image_sequence_source_get_property;
  object_class->set_property = ges_image_sequence_source_set_property;
  object_class->dispose = ges_image_sequence_source_dispose;

  /**
   * GESImageSequenceSource:uri:
   *
   * The uri of the file/resource to use.
   *
   * This is the same URI that GstImageSequenceSrc handles.
   *
   */
  g_object_class_install_property (object_class, PROP_URI,
      g_param_spec_string ("uri", "URI", "image-sequence uri",
          NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_FILENAMES,
      g_param_spec_boxed ("filenames-list", "Filenames (path) List",
          "Set a list of filenames directly instead of a location pattern."
          "If you *get* the current list, you will obtain a copy of it.",
          G_TYPE_STRV, G_PARAM_READABLE));
  source_class->create_source = ges_image_sequence_source_create_source;
}

static void
ges_image_sequence_source_init (GESImageSequenceSource * self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_IMAGE_SEQUENCE_SOURCE, GESImageSequenceSourcePrivate);
  self->priv->fps_n = -1;
  self->priv->fps_d = -1;
}

/**
 * ges_image_sequence_source_new:
 * @uri: the URI the source should control
 *
 * Creates a new #GESImageSequenceSource for the provided @uri.
 *
 * Returns: A new #GESImageSequenceSource.
 */

GESImageSequenceSource *
ges_image_sequence_source_new (void)
{
  return g_object_new (GES_TYPE_IMAGE_SEQUENCE_SOURCE,
      "track-type", GES_TRACK_TYPE_VIDEO, NULL);
}


/**
 * ges_image_sequence_source_new_from_uri:
 * @uri: the URI the source should control
 *
 * Creates a new #GESImageSequenceSource for the provided @uri.
 *
 * Returns: A new #GESImageSequenceSource.
 */

GESImageSequenceSource *
ges_image_sequence_source_new_from_uri (gchar * uri)
{
  return g_object_new (GES_TYPE_IMAGE_SEQUENCE_SOURCE, "uri", uri,
      "track-type", GES_TRACK_TYPE_VIDEO, NULL);
}
