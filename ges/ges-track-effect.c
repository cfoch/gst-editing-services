/* GStreamer Editing Services
 * Copyright (C) 2010 Thibault Saunier <tsaunier@gnome.org>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:ges-track-effect
 * @short_description: adds an effect to a stream in a #GESTimelineSource or a
 * #GESTimelineLayer
 *
 */

#include "ges-internal.h"
#include "ges-track-object.h"
#include "ges-track-effect.h"

G_DEFINE_TYPE (GESTrackEffect, ges_track_effect, GES_TYPE_TRACK_OPERATION);

static void ges_track_effect_dispose (GObject * object);
static void ges_track_effect_finalize (GObject * object);
static GstElement *ges_track_effect_create_element (GESTrackObject * self);
static GHashTable *ges_track_effect_get_props_hashtable (GESTrackObject * self);
static GHashTable
    * ges_track_effect_get_props_hashtable_from_bin_desc (GESTrackObject *
    self);

struct _GESTrackEffectPrivate
{
  gchar *bin_description;
};

enum
{
  PROP_0,
  PROP_BIN_DESCRIPTION,
};

static void
ges_track_effect_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_track_effect_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  GESTrackEffect *self = GES_TRACK_EFFECT (object);

  switch (property_id) {
    case PROP_BIN_DESCRIPTION:
      self->priv->bin_description = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_track_effect_class_init (GESTrackEffectClass * klass)
{
  GObjectClass *object_class;
  GESTrackObjectClass *obj_bg_class;

  object_class = G_OBJECT_CLASS (klass);
  obj_bg_class = GES_TRACK_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESTrackEffectPrivate));

  object_class->get_property = ges_track_effect_get_property;
  object_class->set_property = ges_track_effect_set_property;
  object_class->dispose = ges_track_effect_dispose;
  object_class->finalize = ges_track_effect_finalize;

  obj_bg_class->create_element = ges_track_effect_create_element;
  obj_bg_class->get_props_hastable = ges_track_effect_get_props_hashtable;

  /**
   * GESTrackEffect:bin_description:
   *
   * The description of the effect bin with a gst-launch-style
   * pipeline description.
   * exemple: videobalance saturation=1.5 hue=+0.5
   */
  g_object_class_install_property (object_class, PROP_BIN_DESCRIPTION,
      g_param_spec_string ("bin-description",
          "bin description",
          "Bin description of the effect",
          NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
ges_track_effect_init (GESTrackEffect * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, GES_TYPE_TRACK_EFFECT,
      GESTrackEffectPrivate);
}

static void
ges_track_effect_dispose (GObject * object)
{
  G_OBJECT_CLASS (ges_track_effect_parent_class)->dispose (object);
}

static void
ges_track_effect_finalize (GObject * object)
{
  GESTrackEffect *self = GES_TRACK_EFFECT (object);

  if (self->priv->bin_description)
    g_free (self->priv->bin_description);

  G_OBJECT_CLASS (ges_track_effect_parent_class)->finalize (object);
}

/* This function is more for testing puposes */
static GHashTable *
ges_track_effect_get_props_hashtable_from_bin_desc (GESTrackObject * self)
{
  gpointer data;
  GstIterator *it;
  GParamSpec **parray;
  GObjectClass *class;
  guint i, nb_specs;
  const gchar *name, *klass;
  GstElementFactory *factory;
  GstElement *child, *element;
  gchar **categories, *categorie;

  gboolean done = FALSE;
  GHashTable *ret = NULL;

  element = ges_track_object_get_element (self);

  ret = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

  /*  We go over child elements recursivly, and add writable properties to the
   *  hashtable
   *  FIXME: Add a blacklist of properties */
  it = gst_bin_iterate_recurse (GST_BIN (element));
  while (!done) {
    switch (gst_iterator_next (it, &data)) {
      case GST_ITERATOR_OK:
        child = GST_ELEMENT_CAST (data);
        factory = gst_element_get_factory (child);
        klass = gst_element_factory_get_klass (factory);
        categories = g_strsplit (klass, "/", 0);

        i = 0;
        for (categorie = categories[0]; categorie;) {
          if (g_strcmp0 (categorie, "Effect") == 0) {

            class = G_OBJECT_GET_CLASS (child);
            parray = g_object_class_list_properties (class, &nb_specs);
            for (i = 0; i < nb_specs; i++) {
              if (parray[i]->flags & G_PARAM_WRITABLE) {
                name = g_param_spec_get_name (parray[i]);
                g_hash_table_insert (ret,
                    g_strconcat (G_OBJECT_CLASS_NAME (class),
                        "-", name, NULL), g_object_ref (child));
              }
            }
            GST_DEBUG ("%i configurable properties added to %p", child,
                nb_specs);
            gst_object_unref (child);
            break;
          }
          i++;
          categorie = categories[i];
        }
        g_strfreev (categories);
        break;

      case GST_ITERATOR_RESYNC:
        GST_DEBUG ("iterator resync");
        gst_iterator_resync (it);
        break;

      case GST_ITERATOR_DONE:
        GST_DEBUG ("iterator done");
        done = TRUE;
        break;

      default:
        break;
    }
  }
  gst_iterator_free (it);

  return ret;
}

/*  Virtual methods */
static GHashTable *
ges_track_effect_get_props_hashtable (GESTrackObject * self)
{

  if (GES_TRACK_EFFECT (self)->priv->bin_description)
    return ges_track_effect_get_props_hashtable_from_bin_desc (self);

  return NULL;
}

static GstElement *
ges_track_effect_create_element (GESTrackObject * object)
{
  GstElement *csp, *ret, *effect;
  GstPad *src_target, *sink_target;
  GstPad *src, *sink;

  GError *error = NULL;
  GESTrackEffect *self = GES_TRACK_EFFECT (object);

  effect =
      gst_parse_bin_from_description (self->priv->bin_description, TRUE,
      &error);
  if (error != NULL)
    return NULL;

  csp = gst_element_factory_make ("ffmpegcolorspace", NULL);

  ret = gst_bin_new ("effect-bin");
  gst_bin_add_many (GST_BIN (ret), effect, csp, NULL);
  gst_element_link (csp, ret);

  src_target = gst_element_get_static_pad (effect, "src");
  sink_target = gst_element_get_static_pad (csp, "sink");

  src = gst_ghost_pad_new ("src", src_target);
  sink = gst_ghost_pad_new ("video_sink", sink_target);

  gst_object_unref (src_target);
  gst_object_unref (sink_target);

  gst_element_add_pad (ret, src);
  gst_element_add_pad (ret, sink);

  GST_DEBUG ("Created %p", ret);

  return ret;
}

/**
* ges_track_effect_new_from_bin_desc:
* @bin_description: The gst-launch like bin description of the effect
*
* Creates a new #GESTrackEffect from the description of the bin. This is
* a convenience method for testing puposes.
*
* Returns: a newly created #GESTrackEffect, or %NULL if something went
* wrong.
*/
GESTrackEffect *
ges_track_effect_new_from_bin_desc (const gchar * bin_description)
{
  return g_object_new (GES_TYPE_TRACK_EFFECT, "bin-description",
      bin_description, NULL);
}