/* GStreamer Editing Services
 * Copyright (C) 2014 Fabian Orccon <cfoch.fabian@gmail.com>
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

#ifndef _GES_IMAGE_SEQUENCE_SOURCE_CLIP
#define _GES_IMAGE_SEQUENCE_SOURCE_CLIP

#include <glib-object.h>
#include <ges/ges-types.h>
#include <ges/ges-source-clip.h>
#include <ges/ges-track.h>

G_BEGIN_DECLS

#define GES_TYPE_IMAGE_SEQUENCE_CLIP ges_image_sequence_clip_get_type()

#define GES_IMAGE_SEQUENCE_CLIP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_IMAGE_SEQUENCE_CLIP, GESImageSequenceClip))

#define GES_IMAGE_SEQUENCE_CLIP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_IMAGE_SEQUENCE_CLIP, GESImageSequenceClipClass))

#define GES_IS_IMAGE_SEQUENCE_CLIP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_IMAGE_SEQUENCE_CLIP))

#define GES_IS_IMAGE_SEQUENCE_CLIP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_IMAGE_SEQUENCE_CLIP))

#define GES_IMAGE_SEQUENCE_CLIP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_IMAGE_SEQUENCE_CLIP, GESImageSequenceClipClass))

typedef struct _GESImageSequenceClipPrivate GESImageSequenceClipPrivate;

/**
 * GESImageSequenceClip:
 *
 * Render stand-alone image-sequence in GESLayer.
 */

struct _GESImageSequenceClip {
  GESSourceClip parent;

  /*< private >*/
  GESImageSequenceClipPrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

struct _GESImageSequenceClipClass {
  /*< private >*/
  GESSourceClipClass parent_class;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

GType ges_image_sequence_clip_get_type (void);

void ges_image_sequence_clip_set_filenames (GESImageSequenceClip * self, GStrv filenames_list);
void ges_image_sequence_clip_set_framerate (GESImageSequenceClip * self, gint fps_n, gint fps_d);

gchar ** ges_image_sequence_clip_get_filenames (GESImageSequenceClip * self);
void ges_image_sequence_clip_get_framerate (GESImageSequenceClip * self, gint * fps_n, gint * fps_d);

GESImageSequenceClip* ges_image_sequence_clip_new (void);


G_END_DECLS

#endif /* _GES_IMAGE_SEQUENCE_SOURCE_CLIP */

