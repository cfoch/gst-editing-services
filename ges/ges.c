/* GStreamer Editing Services
 * Copyright (C) 2009 Edward Hervey <edward.hervey@collabora.co.uk>
 *               2009 Nokia Corporation
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

#include <ges/ges.h>
#include "ges/gstframepositionner.h"
#include "ges-internal.h"
#include <stdlib.h>
#include "ges/ges-image-sequence-clip.h"

#define GES_GNONLIN_VERSION_NEEDED_MAJOR 1
#define GES_GNONLIN_VERSION_NEEDED_MINOR 2
#define GES_GNONLIN_VERSION_NEEDED_MICRO 0

GST_DEBUG_CATEGORY (_ges_debug);

static gboolean ges_initialized = FALSE;

/**
 * SECTION:ges-common
 * @short_description: Initialization.
 */

static gboolean
ges_check_gnonlin_availability (void)
{
  gboolean ret = TRUE;
  if (!gst_registry_check_feature_version (gst_registry_get (),
          "gnlcomposition", GES_GNONLIN_VERSION_NEEDED_MAJOR,
          GES_GNONLIN_VERSION_NEEDED_MINOR, GES_GNONLIN_VERSION_NEEDED_MICRO)) {
    GST_ERROR ("GNonLin plugins not found, or not at least version %u.%u.%u",
        GES_GNONLIN_VERSION_NEEDED_MAJOR, GES_GNONLIN_VERSION_NEEDED_MINOR,
        GES_GNONLIN_VERSION_NEEDED_MICRO);
    ret = FALSE;
  }
  return ret;
}

/* Serialization utils for ges-formatter */
static gchar *
_serialize_g_strv (const GValue * value)
{
  gchar **filenames;
  gchar *string, *ret;

  filenames = g_value_get_boxed (value);
  string = g_strjoinv (";", filenames);
  ret = g_strconcat ("\"", string, "\"", NULL);
  g_free (string);
  return ret;
}

static gboolean
_deserialize_g_strv (GValue * dest, const gchar * string)
{
  gchar **filenames;
  gboolean ret = FALSE;

  filenames = g_strsplit (string, ";", 0);
  g_value_set_boxed (dest, filenames);
  if (filenames)
    ret = TRUE;
  return ret;
}

static void
_register_serialization (void)
{
  {
    static GstValueTable value = {
      0,
      NULL,
      _serialize_g_strv,
      _deserialize_g_strv,
    };

    value.type = G_TYPE_STRV;
    gst_value_register (&value);
  }
}

/**
 * ges_init:
 *
 * Initialize the GStreamer Editing Service. Call this before any usage of
 * GES. You should take care of initilizing GStreamer before calling this
 * function.
 */

gboolean
ges_init (void)
{
  /* initialize debugging category */
  GST_DEBUG_CATEGORY_INIT (_ges_debug, "ges", GST_DEBUG_FG_YELLOW,
      "GStreamer Editing Services");

  if (ges_initialized) {
    GST_DEBUG ("already initialized ges");
    return TRUE;
  }

  /* register clip classes with the system */

  GES_TYPE_TEST_CLIP;
  GES_TYPE_URI_CLIP;
  GES_TYPE_TITLE_CLIP;
  GES_TYPE_IMAGE_SEQUENCE_CLIP;
  GES_TYPE_TRANSITION_CLIP;
  GES_TYPE_OVERLAY_CLIP;

  GES_TYPE_GROUP;

  /* register formatter types with the system */
  GES_TYPE_PITIVI_FORMATTER;
  GES_TYPE_XML_FORMATTER;
  /* Setting serializer to G_TYPE_STRV */
  _register_serialization ();

  /* Register track elements */
  GES_TYPE_EFFECT;

  /* Register interfaces */
  GES_TYPE_META_CONTAINER;

  ges_asset_cache_init ();

  /* check the gnonlin elements are available */
  if (!ges_check_gnonlin_availability ())
    return FALSE;

  gst_element_register (NULL, "framepositionner", 0,
      GST_TYPE_FRAME_POSITIONNER);
  gst_element_register (NULL, "gespipeline", 0, GES_TYPE_PIPELINE);

  /* TODO: user-defined types? */
  ges_initialized = TRUE;

  GST_DEBUG ("GStreamer Editing Services initialized");

  return TRUE;
}


/**
 * ges_version:
 * @major: (out): pointer to a guint to store the major version number
 * @minor: (out): pointer to a guint to store the minor version number
 * @micro: (out): pointer to a guint to store the micro version number
 * @nano:  (out): pointer to a guint to store the nano version number
 *
 * Gets the version number of the GStreamer Editing Services library.
 */
void
ges_version (guint * major, guint * minor, guint * micro, guint * nano)
{
  g_return_if_fail (major);
  g_return_if_fail (minor);
  g_return_if_fail (micro);
  g_return_if_fail (nano);

  *major = GES_VERSION_MAJOR;
  *minor = GES_VERSION_MINOR;
  *micro = GES_VERSION_MICRO;
  *nano = GES_VERSION_NANO;
}
