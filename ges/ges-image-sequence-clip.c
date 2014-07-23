/* GStreamer Editing Services
 * Copyright (C) 2009 Fabian Orccon <cfoch.fabian@gmail.com>
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
 * SECTION:gesImageSequenceClip
 * @short_description: Render stand-alone titles in  GESLayer.
 *
 * Renders the given filenames-list with the given framerate as a sequence
 * of images.
 */

#include "ges-internal.h"
#include "ges-image-sequence-clip.h"
#include "ges-source-clip.h"
#include "ges-track-element.h"
#include "ges-image-sequence-source.h"
#include <string.h>

G_DEFINE_TYPE (GESImageSequenceClip, ges_image_sequence_clip,
    GES_TYPE_SOURCE_CLIP);

struct _GESImageSequenceClipPrivate
{
  gchar **filenames_list;
  gint fps_n, fps_d;
  GSList *track_titles;
};

enum
{
  PROP_0,
  PROP_FILENAMES,
  PROP_FRAMERATE
};

static GESTrackElement *ges_image_sequence_clip_create_track_element
    (GESClip * clip, GESTrackType type);

static void _child_added (GESContainer * container,
    GESTimelineElement * element);
static void _child_removed (GESContainer * container,
    GESTimelineElement * element);
static void _update_max_duration (GESImageSequenceClip * self);

static void
ges_image_sequence_clip_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GESImageSequenceClipPrivate *priv = GES_IMAGE_SEQUENCE_CLIP (object)->priv;

  switch (property_id) {
    case PROP_FILENAMES:
      g_value_set_boxed (value, priv->filenames_list);
      break;
    case PROP_FRAMERATE:
    {
      gst_value_set_fraction (value, priv->fps_n, priv->fps_d);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_image_sequence_clip_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GESImageSequenceClip *clip = GES_IMAGE_SEQUENCE_CLIP (object);

  switch (property_id) {
    case PROP_FILENAMES:
      ges_image_sequence_clip_set_filenames (clip, g_value_get_boxed (value));
      break;
    case PROP_FRAMERATE:
    {
      gint fps_n, fps_d;
      fps_n = gst_value_get_fraction_numerator (value);
      fps_d = gst_value_get_fraction_denominator (value);
      ges_image_sequence_clip_set_framerate (clip, fps_n, fps_d);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_image_sequence_clip_dispose (GObject * object)
{
  GESImageSequenceClip *self = GES_IMAGE_SEQUENCE_CLIP (object);

  g_strfreev (self->priv->filenames_list);

  G_OBJECT_CLASS (ges_image_sequence_clip_parent_class)->dispose (object);
}

static void
ges_image_sequence_clip_class_init (GESImageSequenceClipClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GESClipClass *timobj_class = GES_CLIP_CLASS (klass);
  GESContainerClass *container_class = GES_CONTAINER_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESImageSequenceClipPrivate));

  object_class->get_property = ges_image_sequence_clip_get_property;
  object_class->set_property = ges_image_sequence_clip_set_property;
  object_class->dispose = ges_image_sequence_clip_dispose;


  timobj_class->create_track_element =
      ges_image_sequence_clip_create_track_element;

  container_class->child_added = _child_added;
  container_class->child_removed = _child_removed;

  g_object_class_install_property (object_class, PROP_FILENAMES,
      g_param_spec_boxed ("filenames-list", "Filenames (path) List",
          "Set a list of filenames directly instead of a location pattern."
          "If you *get* the current list, you will obtain a copy of it.",
          G_TYPE_STRV, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_FRAMERATE,
      gst_param_spec_fraction ("framerate", "Framerate",
          "Set the framerate to the clip.",
          1, 1, G_MAXINT, 1, -1, -1,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
ges_image_sequence_clip_init (GESImageSequenceClip * self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_IMAGE_SEQUENCE_CLIP, GESImageSequenceClipPrivate);

  GES_TIMELINE_ELEMENT (self)->duration = 0;
}


/**
 * ges_image_sequence_clip_set_framerate:
 * @self: the #GESImageSequenceClip* to set the framerate
 * @fps_n: the framerate numerator value
 * @fps_d: the framerate denominator value
 *
 * Sets the framerate to this source.
 *
 */
void
ges_image_sequence_clip_set_framerate (GESImageSequenceClip * self, gint fps_n,
    gint fps_d)
{
  GSList *tmp;

  GST_DEBUG_OBJECT (self, "Setting framerate (%d:%d)", fps_n, fps_d);

  self->priv->fps_n = fps_n;
  self->priv->fps_d = fps_d;

  for (tmp = self->priv->track_titles; tmp; tmp = tmp->next)
    ges_image_sequence_source_set_framerate (GES_IMAGE_SEQUENCE_SOURCE
        (tmp->data), fps_n, fps_d);
  _update_max_duration (self);
}

/**
 * ges_image_sequence_clip_set_filenames:
 * @self: the #GESImageSequenceClip* to get text on
 * @filenames_list: list of filenames
 *
 * Sets the filenames this clip will render.
 *
 */
void
ges_image_sequence_clip_set_filenames (GESImageSequenceClip * self,
    GStrv filenames_list)
{
  GSList *tmp;

  GST_DEBUG_OBJECT (self, "Setting filenames. First filename: %s",
      filenames_list[0]);

  if (self->priv->filenames_list)
    g_strfreev (self->priv->filenames_list);

  self->priv->filenames_list = g_strdupv (filenames_list);

  for (tmp = self->priv->track_titles; tmp; tmp = tmp->next)
    ges_image_sequence_source_set_filenames (GES_IMAGE_SEQUENCE_SOURCE
        (tmp->data), self->priv->filenames_list);

  _update_max_duration (self);
}

/**
 * ges_image_sequence_clip_get_filenames:
 * @self: the #GESImageSequenceClip*
 *
 * Returns: (array zero-terminated=1) (element-type utf8) (transfer none): A list of filenames.
 *
 */
gchar **
ges_image_sequence_clip_get_filenames (GESImageSequenceClip * self)
{
  return self->priv->filenames_list;
}

/**
 * ges_image_sequence_clip_get_framerate:
 * @self: the #GESImageSequenceClip* to set the framerate on
 * @fps_n: (out): the framerate numerator value
 * @fps_d: (out): the framerate denominator value
 *
 * Sets the framerate to this source.
 *
 */
void
ges_image_sequence_clip_get_framerate (GESImageSequenceClip * self,
    gint * fps_n, gint * fps_d)
{
  *fps_n = self->priv->fps_n;
  *fps_d = self->priv->fps_d;
}

static void
_child_removed (GESContainer * container, GESTimelineElement * element)
{
  GESImageSequenceClipPrivate *priv = GES_IMAGE_SEQUENCE_CLIP (container)->priv;

  /* If this is called, we should be sure the element exists */
  if (GES_IS_IMAGE_SEQUENCE_SOURCE (element)) {
    GST_DEBUG_OBJECT (container, "%" GST_PTR_FORMAT " removed", element);
    priv->track_titles = g_slist_remove (priv->track_titles, element);
    gst_object_unref (element);
  }
}

static void
_child_added (GESContainer * container, GESTimelineElement * element)
{
  GESImageSequenceClipPrivate *priv = GES_IMAGE_SEQUENCE_CLIP (container)->priv;

  if (GES_IS_IMAGE_SEQUENCE_SOURCE (element)) {
    GST_DEBUG_OBJECT (container, "%" GST_PTR_FORMAT " added", element);
    priv->track_titles = g_slist_prepend (priv->track_titles,
        gst_object_ref (element));
  }
}

static GESTrackElement *
ges_image_sequence_clip_create_track_element (GESClip * clip, GESTrackType type)
{

  GESImageSequenceClipPrivate *priv = GES_IMAGE_SEQUENCE_CLIP (clip)->priv;
  GESTrackElement *res = NULL;

  GST_DEBUG_OBJECT (clip, "a GESImageSequenceClip");

  if (type == GES_TRACK_TYPE_VIDEO) {
    res = (GESTrackElement *) ges_image_sequence_source_new ();
    ges_image_sequence_source_set_filenames ((GESImageSequenceSource *) res,
        priv->filenames_list);
    ges_image_sequence_source_set_framerate ((GESImageSequenceSource *) res,
        priv->fps_n, priv->fps_d);
  }

  return res;
}


static void
_update_max_duration (GESImageSequenceClip * self)
{
  /* Calculating duration */
  guint frames;
  GstClockTime duration;
  GESTimelineElement *clip;

  frames = g_strv_length (self->priv->filenames_list);
  duration = gst_util_uint64_scale (GST_SECOND * frames,
      self->priv->fps_d, self->priv->fps_n);

  clip = GES_TIMELINE_ELEMENT (self);
  GES_TIMELINE_ELEMENT_GET_CLASS (clip)->set_max_duration (clip, duration);
  GES_TIMELINE_ELEMENT (clip)->maxduration = duration;
}

/**
 * ges_image_sequence_clip_new:
 *
 * Creates a new #GESImageSequenceClip
 *
 * Returns: The newly created #GESImageSequenceClip, or NULL if there was an
 * error.
 */
GESImageSequenceClip *
ges_image_sequence_clip_new (void)
{
  GESImageSequenceClip *new_clip;
  GESAsset *asset = ges_asset_request (GES_TYPE_IMAGE_SEQUENCE_CLIP, NULL,
      NULL);

  new_clip = GES_IMAGE_SEQUENCE_CLIP (ges_asset_extract (asset, NULL));
  gst_object_unref (asset);

  return new_clip;
}
