/* GTK - The GIMP Toolkit
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <math.h>
#include <gtk/gtk.h>

#include <gtk/gtkthemingengine.h>
#include <gtk/gtkstylecontext.h>
#include <gtk/gtkintl.h>

#include "gtkprivate.h"
#include "gtkmodulesprivate.h"
#include "gtkborderimageprivate.h"
#include "gtkpango.h"
#include "gtkshadowprivate.h"
#include "gtkcsstypesprivate.h"
#include "gtkthemingengineprivate.h"
#include "gtkroundedboxprivate.h"
#include "gtkthemingbackgroundprivate.h"

/**
 * SECTION:gtkthemingengine
 * @Short_description: Theming renderers
 * @Title: GtkThemingEngine
 * @See_also: #GtkStyleContext
 *
 * #GtkThemingEngine is the object used for rendering themed content
 * in GTK+ widgets. Even though GTK+ has a default implementation,
 * it can be overridden in CSS files by enforcing a #GtkThemingEngine
 * object to be loaded as a module.
 *
 * In order to implement a theming engine, a #GtkThemingEngine subclass
 * must be created, alongside the CSS file that will reference it, the
 * theming engine would be created as an .so library, and installed in
 * $(gtk-modules-dir)/theming-engines/.
 *
 * #GtkThemingEngine<!-- -->s have limited access to the object they are
 * rendering, the #GtkThemingEngine API has read-only accessors to the
 * style information contained in the rendered object's #GtkStyleContext.
 */

enum {
  PROP_0,
  PROP_NAME
};

struct GtkThemingEnginePrivate
{
  GtkStyleContext *context;
  gchar *name;
};

#define GTK_THEMING_ENGINE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTK_TYPE_THEMING_ENGINE, GtkThemingEnginePrivate))

static void gtk_theming_engine_finalize          (GObject      *object);
static void gtk_theming_engine_impl_set_property (GObject      *object,
                                                  guint         prop_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void gtk_theming_engine_impl_get_property (GObject      *object,
                                                  guint         prop_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);

static void gtk_theming_engine_render_check (GtkThemingEngine *engine,
                                             cairo_t          *cr,
                                             gdouble           x,
                                             gdouble           y,
                                             gdouble           width,
                                             gdouble           height);
static void gtk_theming_engine_render_option (GtkThemingEngine *engine,
                                              cairo_t          *cr,
                                              gdouble           x,
                                              gdouble           y,
                                              gdouble           width,
                                              gdouble           height);
static void gtk_theming_engine_render_arrow  (GtkThemingEngine *engine,
                                              cairo_t          *cr,
                                              gdouble           angle,
                                              gdouble           x,
                                              gdouble           y,
                                              gdouble           size);
static void gtk_theming_engine_render_background (GtkThemingEngine *engine,
                                                  cairo_t          *cr,
                                                  gdouble           x,
                                                  gdouble           y,
                                                  gdouble           width,
                                                  gdouble           height);
static void gtk_theming_engine_render_frame  (GtkThemingEngine *engine,
                                              cairo_t          *cr,
                                              gdouble           x,
                                              gdouble           y,
                                              gdouble           width,
                                              gdouble           height);
static void gtk_theming_engine_render_expander (GtkThemingEngine *engine,
                                                cairo_t          *cr,
                                                gdouble           x,
                                                gdouble           y,
                                                gdouble           width,
                                                gdouble           height);
static void gtk_theming_engine_render_focus    (GtkThemingEngine *engine,
                                                cairo_t          *cr,
                                                gdouble           x,
                                                gdouble           y,
                                                gdouble           width,
                                                gdouble           height);
static void gtk_theming_engine_render_layout   (GtkThemingEngine *engine,
                                                cairo_t          *cr,
                                                gdouble           x,
                                                gdouble           y,
                                                PangoLayout      *layout);
static void gtk_theming_engine_render_line     (GtkThemingEngine *engine,
                                                cairo_t          *cr,
                                                gdouble           x0,
                                                gdouble           y0,
                                                gdouble           x1,
                                                gdouble           y1);
static void gtk_theming_engine_render_slider   (GtkThemingEngine *engine,
                                                cairo_t          *cr,
                                                gdouble           x,
                                                gdouble           y,
                                                gdouble           width,
                                                gdouble           height,
                                                GtkOrientation    orientation);
static void gtk_theming_engine_render_frame_gap (GtkThemingEngine *engine,
                                                 cairo_t          *cr,
                                                 gdouble           x,
                                                 gdouble           y,
                                                 gdouble           width,
                                                 gdouble           height,
                                                 GtkPositionType   gap_side,
                                                 gdouble           xy0_gap,
                                                 gdouble           xy1_gap);
static void gtk_theming_engine_render_extension (GtkThemingEngine *engine,
                                                 cairo_t          *cr,
                                                 gdouble           x,
                                                 gdouble           y,
                                                 gdouble           width,
                                                 gdouble           height,
                                                 GtkPositionType   gap_side);
static void gtk_theming_engine_render_handle    (GtkThemingEngine *engine,
                                                 cairo_t          *cr,
                                                 gdouble           x,
                                                 gdouble           y,
                                                 gdouble           width,
                                                 gdouble           height);
static void gtk_theming_engine_render_activity  (GtkThemingEngine *engine,
                                                 cairo_t          *cr,
                                                 gdouble           x,
                                                 gdouble           y,
                                                 gdouble           width,
                                                 gdouble           height);
static GdkPixbuf * gtk_theming_engine_render_icon_pixbuf (GtkThemingEngine    *engine,
                                                          const GtkIconSource *source,
                                                          GtkIconSize          size);
static void gtk_theming_engine_render_icon (GtkThemingEngine *engine,
                                            cairo_t *cr,
					    GdkPixbuf *pixbuf,
                                            gdouble x,
                                            gdouble y);

G_DEFINE_TYPE (GtkThemingEngine, gtk_theming_engine, G_TYPE_OBJECT)


typedef struct GtkThemingModule GtkThemingModule;
typedef struct GtkThemingModuleClass GtkThemingModuleClass;

struct GtkThemingModule
{
  GTypeModule parent_instance;
  GModule *module;
  gchar *name;

  void (*init) (GTypeModule *module);
  void (*exit) (void);
  GtkThemingEngine * (*create_engine) (void);
};

struct GtkThemingModuleClass
{
  GTypeModuleClass parent_class;
};

#define GTK_TYPE_THEMING_MODULE  (gtk_theming_module_get_type ())
#define GTK_THEMING_MODULE(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GTK_TYPE_THEMING_MODULE, GtkThemingModule))
#define GTK_IS_THEMING_MODULE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTK_TYPE_THEMING_MODULE))

G_DEFINE_TYPE (GtkThemingModule, gtk_theming_module, G_TYPE_TYPE_MODULE);

static void
gtk_theming_engine_class_init (GtkThemingEngineClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gtk_theming_engine_finalize;
  object_class->set_property = gtk_theming_engine_impl_set_property;
  object_class->get_property = gtk_theming_engine_impl_get_property;

  klass->render_icon = gtk_theming_engine_render_icon;
  klass->render_check = gtk_theming_engine_render_check;
  klass->render_option = gtk_theming_engine_render_option;
  klass->render_arrow = gtk_theming_engine_render_arrow;
  klass->render_background = gtk_theming_engine_render_background;
  klass->render_frame = gtk_theming_engine_render_frame;
  klass->render_expander = gtk_theming_engine_render_expander;
  klass->render_focus = gtk_theming_engine_render_focus;
  klass->render_layout = gtk_theming_engine_render_layout;
  klass->render_line = gtk_theming_engine_render_line;
  klass->render_slider = gtk_theming_engine_render_slider;
  klass->render_frame_gap = gtk_theming_engine_render_frame_gap;
  klass->render_extension = gtk_theming_engine_render_extension;
  klass->render_handle = gtk_theming_engine_render_handle;
  klass->render_activity = gtk_theming_engine_render_activity;
  klass->render_icon_pixbuf = gtk_theming_engine_render_icon_pixbuf;

  /**
   * GtkThemingEngine:name:
   *
   * The theming engine name, this name will be used when registering
   * custom properties, for a theming engine named "Clearlooks" registering
   * a "glossy" custom property, it could be referenced in the CSS file as
   *
   * <programlisting>
   * -Clearlooks-glossy: true;
   * </programlisting>
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
				   PROP_NAME,
				   g_param_spec_string ("name",
                                                        P_("Name"),
                                                        P_("Theming engine name"),
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY | GTK_PARAM_READWRITE));

  g_type_class_add_private (object_class, sizeof (GtkThemingEnginePrivate));
}

static void
gtk_theming_engine_init (GtkThemingEngine *engine)
{
  engine->priv = GTK_THEMING_ENGINE_GET_PRIVATE (engine);
}

static void
gtk_theming_engine_finalize (GObject *object)
{
  GtkThemingEnginePrivate *priv;

  priv = GTK_THEMING_ENGINE (object)->priv;
  g_free (priv->name);

  G_OBJECT_GET_CLASS (gtk_theming_engine_parent_class)->finalize (object);
}

static void
gtk_theming_engine_impl_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  GtkThemingEnginePrivate *priv;

  priv = GTK_THEMING_ENGINE (object)->priv;

  switch (prop_id)
    {
    case PROP_NAME:
      if (priv->name)
        g_free (priv->name);

      priv->name = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_theming_engine_impl_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  GtkThemingEnginePrivate *priv;

  priv = GTK_THEMING_ENGINE (object)->priv;

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

void
_gtk_theming_engine_set_context (GtkThemingEngine *engine,
                                 GtkStyleContext  *context)
{
  GtkThemingEnginePrivate *priv;

  g_return_if_fail (GTK_IS_THEMING_ENGINE (engine));
  g_return_if_fail (GTK_IS_STYLE_CONTEXT (context));

  priv = engine->priv;
  priv->context = context;
}

const GValue *
_gtk_theming_engine_peek_property (GtkThemingEngine *engine,
                                   const char       *property_name)
{
  g_return_val_if_fail (GTK_IS_THEMING_ENGINE (engine), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  return _gtk_style_context_peek_property (engine->priv->context, property_name);
}

double
_gtk_theming_engine_get_number (GtkThemingEngine *engine,
                                const char       *property_name,
                                double            one_hundred_percent)
{
  g_return_val_if_fail (GTK_IS_THEMING_ENGINE (engine), 0.0);
  g_return_val_if_fail (property_name != NULL, 0.0);

  return _gtk_style_context_get_number (engine->priv->context, property_name, one_hundred_percent);
}

/**
 * gtk_theming_engine_get_property:
 * @engine: a #GtkThemingEngine
 * @property: the property name
 * @state: state to retrieve the value for
 * @value: (out) (transfer full): return location for the property value,
 *         you must free this memory using g_value_unset() once you are
 *         done with it.
 *
 * Gets a property value as retrieved from the style settings that apply
 * to the currently rendered element.
 *
 * Since: 3.0
 **/
void
gtk_theming_engine_get_property (GtkThemingEngine *engine,
                                 const gchar      *property,
                                 GtkStateFlags     state,
                                 GValue           *value)
{
  GtkThemingEnginePrivate *priv;

  g_return_if_fail (GTK_IS_THEMING_ENGINE (engine));
  g_return_if_fail (property != NULL);
  g_return_if_fail (value != NULL);

  priv = engine->priv;
  gtk_style_context_get_property (priv->context, property, state, value);
}

/**
 * gtk_theming_engine_get_valist:
 * @engine: a #GtkThemingEngine
 * @state: state to retrieve values for
 * @args: va_list of property name/return location pairs, followed by %NULL
 *
 * Retrieves several style property values that apply to the currently
 * rendered element.
 *
 * Since: 3.0
 **/
void
gtk_theming_engine_get_valist (GtkThemingEngine *engine,
                               GtkStateFlags     state,
                               va_list           args)
{
  GtkThemingEnginePrivate *priv;

  g_return_if_fail (GTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;
  gtk_style_context_get_valist (priv->context, state, args);
}

/**
 * gtk_theming_engine_get:
 * @engine: a #GtkThemingEngine
 * @state: state to retrieve values for
 * @...: property name /return value pairs, followed by %NULL
 *
 * Retrieves several style property values that apply to the currently
 * rendered element.
 *
 * Since: 3.0
 **/
void
gtk_theming_engine_get (GtkThemingEngine *engine,
                        GtkStateFlags     state,
                        ...)
{
  GtkThemingEnginePrivate *priv;
  va_list args;

  g_return_if_fail (GTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;

  va_start (args, state);
  gtk_style_context_get_valist (priv->context, state, args);
  va_end (args);
}

/**
 * gtk_theming_engine_get_style_property:
 * @engine: a #GtkThemingEngine
 * @property_name: the name of the widget style property
 * @value: Return location for the property value, free with
 *         g_value_unset() after use.
 *
 * Gets the value for a widget style property.
 *
 * Since: 3.0
 **/
void
gtk_theming_engine_get_style_property (GtkThemingEngine *engine,
                                       const gchar      *property_name,
                                       GValue           *value)
{
  GtkThemingEnginePrivate *priv;

  g_return_if_fail (GTK_IS_THEMING_ENGINE (engine));
  g_return_if_fail (property_name != NULL);

  priv = engine->priv;
  gtk_style_context_get_style_property (priv->context, property_name, value);
}

/**
 * gtk_theming_engine_get_style_valist:
 * @engine: a #GtkThemingEngine
 * @args: va_list of property name/return location pairs, followed by %NULL
 *
 * Retrieves several widget style properties from @engine according to the
 * currently rendered content's style.
 *
 * Since: 3.0
 **/
void
gtk_theming_engine_get_style_valist (GtkThemingEngine *engine,
                                     va_list           args)
{
  GtkThemingEnginePrivate *priv;

  g_return_if_fail (GTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;
  gtk_style_context_get_style_valist (priv->context, args);
}

/**
 * gtk_theming_engine_get_style:
 * @engine: a #GtkThemingEngine
 * @...: property name /return value pairs, followed by %NULL
 *
 * Retrieves several widget style properties from @engine according
 * to the currently rendered content's style.
 *
 * Since: 3.0
 **/
void
gtk_theming_engine_get_style (GtkThemingEngine *engine,
                              ...)
{
  GtkThemingEnginePrivate *priv;
  va_list args;

  g_return_if_fail (GTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;

  va_start (args, engine);
  gtk_style_context_get_style_valist (priv->context, args);
  va_end (args);
}

/**
 * gtk_theming_engine_lookup_color:
 * @engine: a #GtkThemingEngine
 * @color_name: color name to lookup
 * @color: (out): Return location for the looked up color
 *
 * Looks up and resolves a color name in the current style's color map.
 *
 * Returns: %TRUE if @color_name was found and resolved, %FALSE otherwise
 **/
gboolean
gtk_theming_engine_lookup_color (GtkThemingEngine *engine,
                                 const gchar      *color_name,
                                 GdkRGBA          *color)
{
  GtkThemingEnginePrivate *priv;

  g_return_val_if_fail (GTK_IS_THEMING_ENGINE (engine), FALSE);
  g_return_val_if_fail (color_name != NULL, FALSE);

  priv = engine->priv;
  return gtk_style_context_lookup_color (priv->context, color_name, color);
}

/**
 * gtk_theming_engine_get_state:
 * @engine: a #GtkThemingEngine
 *
 * returns the state used when rendering.
 *
 * Returns: the state flags
 *
 * Since: 3.0
 **/
GtkStateFlags
gtk_theming_engine_get_state (GtkThemingEngine *engine)
{
  GtkThemingEnginePrivate *priv;

  g_return_val_if_fail (GTK_IS_THEMING_ENGINE (engine), 0);

  priv = engine->priv;
  return gtk_style_context_get_state (priv->context);
}

/**
 * gtk_theming_engine_state_is_running:
 * @engine: a #GtkThemingEngine
 * @state: a widget state
 * @progress: (out): return location for the transition progress
 *
 * Returns %TRUE if there is a transition animation running for the
 * current region (see gtk_style_context_push_animatable_region()).
 *
 * If @progress is not %NULL, the animation progress will be returned
 * there, 0.0 means the state is closest to being %FALSE, while 1.0 means
 * it's closest to being %TRUE. This means transition animations will
 * run from 0 to 1 when @state is being set to %TRUE and from 1 to 0 when
 * it's being set to %FALSE.
 *
 * Returns: %TRUE if there is a running transition animation for @state.
 *
 * Since: 3.0
 **/
gboolean
gtk_theming_engine_state_is_running (GtkThemingEngine *engine,
                                     GtkStateType      state,
                                     gdouble          *progress)
{
  GtkThemingEnginePrivate *priv;

  g_return_val_if_fail (GTK_IS_THEMING_ENGINE (engine), FALSE);

  priv = engine->priv;
  return gtk_style_context_state_is_running (priv->context, state, progress);
}

/**
 * gtk_theming_engine_get_path:
 * @engine: a #GtkThemingEngine
 *
 * Returns the widget path used for style matching.
 *
 * Returns: (transfer none): A #GtkWidgetPath
 *
 * Since: 3.0
 **/
const GtkWidgetPath *
gtk_theming_engine_get_path (GtkThemingEngine *engine)
{
  GtkThemingEnginePrivate *priv;

  g_return_val_if_fail (GTK_IS_THEMING_ENGINE (engine), NULL);

  priv = engine->priv;
  return gtk_style_context_get_path (priv->context);
}

/**
 * gtk_theming_engine_has_class:
 * @engine: a #GtkThemingEngine
 * @style_class: class name to look up
 *
 * Returns %TRUE if the currently rendered contents have
 * defined the given class name.
 *
 * Returns: %TRUE if @engine has @class_name defined
 *
 * Since: 3.0
 **/
gboolean
gtk_theming_engine_has_class (GtkThemingEngine *engine,
                              const gchar      *style_class)
{
  GtkThemingEnginePrivate *priv;

  g_return_val_if_fail (GTK_IS_THEMING_ENGINE (engine), FALSE);

  priv = engine->priv;
  return gtk_style_context_has_class (priv->context, style_class);
}

/**
 * gtk_theming_engine_has_region:
 * @engine: a #GtkThemingEngine
 * @style_region: a region name
 * @flags: (out) (allow-none): return location for region flags
 *
 * Returns %TRUE if the currently rendered contents have the
 * region defined. If @flags_return is not %NULL, it is set
 * to the flags affecting the region.
 *
 * Returns: %TRUE if region is defined
 *
 * Since: 3.0
 **/
gboolean
gtk_theming_engine_has_region (GtkThemingEngine *engine,
                               const gchar      *style_region,
                               GtkRegionFlags   *flags)
{
  GtkThemingEnginePrivate *priv;

  if (flags)
    *flags = 0;

  g_return_val_if_fail (GTK_IS_THEMING_ENGINE (engine), FALSE);

  priv = engine->priv;
  return gtk_style_context_has_region (priv->context, style_region, flags);
}

/**
 * gtk_theming_engine_get_direction:
 * @engine: a #GtkThemingEngine
 *
 * Returns the widget direction used for rendering.
 *
 * Returns: the widget direction
 *
 * Since: 3.0
 **/
GtkTextDirection
gtk_theming_engine_get_direction (GtkThemingEngine *engine)
{
  GtkThemingEnginePrivate *priv;

  g_return_val_if_fail (GTK_IS_THEMING_ENGINE (engine), GTK_TEXT_DIR_LTR);

  priv = engine->priv;
  return gtk_style_context_get_direction (priv->context);
}

/**
 * gtk_theming_engine_get_junction_sides:
 * @engine: a #GtkThemingEngine
 *
 * Returns the widget direction used for rendering.
 *
 * Returns: the widget direction
 *
 * Since: 3.0
 **/
GtkJunctionSides
gtk_theming_engine_get_junction_sides (GtkThemingEngine *engine)
{
  GtkThemingEnginePrivate *priv;

  g_return_val_if_fail (GTK_IS_THEMING_ENGINE (engine), 0);

  priv = engine->priv;
  return gtk_style_context_get_junction_sides (priv->context);
}

/**
 * gtk_theming_engine_get_color:
 * @engine: a #GtkThemingEngine
 * @state: state to retrieve the color for
 * @color: (out): return value for the foreground color
 *
 * Gets the foreground color for a given state.
 *
 * Since: 3.0
 **/
void
gtk_theming_engine_get_color (GtkThemingEngine *engine,
                              GtkStateFlags     state,
                              GdkRGBA          *color)
{
  GtkThemingEnginePrivate *priv;

  g_return_if_fail (GTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;
  gtk_style_context_get_color (priv->context, state, color);
}

/**
 * gtk_theming_engine_get_background_color:
 * @engine: a #GtkThemingEngine
 * @state: state to retrieve the color for
 * @color: (out): return value for the background color
 *
 * Gets the background color for a given state.
 *
 * Since: 3.0
 **/
void
gtk_theming_engine_get_background_color (GtkThemingEngine *engine,
                                         GtkStateFlags     state,
                                         GdkRGBA          *color)
{
  GtkThemingEnginePrivate *priv;

  g_return_if_fail (GTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;
  gtk_style_context_get_background_color (priv->context, state, color);
}

/**
 * gtk_theming_engine_get_border_color:
 * @engine: a #GtkThemingEngine
 * @state: state to retrieve the color for
 * @color: (out): return value for the border color
 *
 * Gets the border color for a given state.
 *
 * Since: 3.0
 **/
void
gtk_theming_engine_get_border_color (GtkThemingEngine *engine,
                                     GtkStateFlags     state,
                                     GdkRGBA          *color)
{
  GtkThemingEnginePrivate *priv;

  g_return_if_fail (GTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;
  gtk_style_context_get_border_color (priv->context, state, color);
}

/**
 * gtk_theming_engine_get_border:
 * @engine: a #GtkthemingEngine
 * @state: state to retrieve the border for
 * @border: (out): return value for the border settings
 *
 * Gets the border for a given state as a #GtkBorder.
 *
 * Since: 3.0
 **/
void
gtk_theming_engine_get_border (GtkThemingEngine *engine,
                               GtkStateFlags     state,
                               GtkBorder        *border)
{
  GtkThemingEnginePrivate *priv;

  g_return_if_fail (GTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;
  gtk_style_context_get_border (priv->context, state, border);
}

/**
 * gtk_theming_engine_get_padding:
 * @engine: a #GtkthemingEngine
 * @state: state to retrieve the padding for
 * @padding: (out): return value for the padding settings
 *
 * Gets the padding for a given state as a #GtkBorder.
 *
 * Since: 3.0
 **/
void
gtk_theming_engine_get_padding (GtkThemingEngine *engine,
                                GtkStateFlags     state,
                                GtkBorder        *padding)
{
  GtkThemingEnginePrivate *priv;

  g_return_if_fail (GTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;
  gtk_style_context_get_padding (priv->context, state, padding);
}

/**
 * gtk_theming_engine_get_margin:
 * @engine: a #GtkThemingEngine
 * @state: state to retrieve the border for
 * @margin: (out): return value for the margin settings
 *
 * Gets the margin for a given state as a #GtkBorder.
 *
 * Since: 3.0
 **/
void
gtk_theming_engine_get_margin (GtkThemingEngine *engine,
                               GtkStateFlags     state,
                               GtkBorder        *margin)
{
  GtkThemingEnginePrivate *priv;

  g_return_if_fail (GTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;
  gtk_style_context_get_margin (priv->context, state, margin);
}

/**
 * gtk_theming_engine_get_font:
 * @engine: a #GtkThemingEngine
 * @state: state to retrieve the font for
 *
 * Returns the font description for a given state.
 *
 * Returns: (transfer none): the #PangoFontDescription for the given
 *          state. This object is owned by GTK+ and should not be
 *          freed.
 *
 * Since: 3.0
 **/
const PangoFontDescription *
gtk_theming_engine_get_font (GtkThemingEngine *engine,
                             GtkStateFlags     state)
{
  GtkThemingEnginePrivate *priv;

  g_return_val_if_fail (GTK_IS_THEMING_ENGINE (engine), NULL);

  priv = engine->priv;
  return gtk_style_context_get_font (priv->context, state);
}

/* GtkThemingModule */

static gboolean
gtk_theming_module_load (GTypeModule *type_module)
{
  GtkThemingModule *theming_module;
  GModule *module;
  gchar *name, *module_path;

  theming_module = GTK_THEMING_MODULE (type_module);
  name = theming_module->name;
  module_path = _gtk_find_module (name, "theming-engines");

  if (!module_path)
    return FALSE;

  module = g_module_open (module_path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
  g_free (module_path);

  if (!module)
    return FALSE;

  if (!g_module_symbol (module, "theme_init",
                        (gpointer *) &theming_module->init) ||
      !g_module_symbol (module, "theme_exit",
                        (gpointer *) &theming_module->exit) ||
      !g_module_symbol (module, "create_engine",
                        (gpointer *) &theming_module->create_engine))
    {
      g_module_close (module);

      return FALSE;
    }

  theming_module->module = module;

  theming_module->init (G_TYPE_MODULE (theming_module));

  return TRUE;
}

static void
gtk_theming_module_unload (GTypeModule *type_module)
{
  GtkThemingModule *theming_module;

  theming_module = GTK_THEMING_MODULE (type_module);

  theming_module->exit ();

  g_module_close (theming_module->module);

  theming_module->module = NULL;
  theming_module->init = NULL;
  theming_module->exit = NULL;
  theming_module->create_engine = NULL;
}

static void
gtk_theming_module_class_init (GtkThemingModuleClass *klass)
{
  GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (klass);

  module_class->load = gtk_theming_module_load;
  module_class->unload = gtk_theming_module_unload;
}

static void
gtk_theming_module_init (GtkThemingModule *module)
{
}

/**
 * gtk_theming_engine_load:
 * @name: Theme engine name to load
 *
 * Loads and initializes a theming engine module from the
 * standard directories.
 *
 * Returns: (transfer none): A theming engine, or %NULL if
 * the engine @name doesn't exist.
 **/
GtkThemingEngine *
gtk_theming_engine_load (const gchar *name)
{
  static GHashTable *engines = NULL;
  static GtkThemingEngine *default_engine;
  GtkThemingEngine *engine = NULL;

  if (name)
    {
      if (!engines)
        engines = g_hash_table_new (g_str_hash, g_str_equal);

      engine = g_hash_table_lookup (engines, name);

      if (!engine)
        {
          GtkThemingModule *module;

          module = g_object_new (GTK_TYPE_THEMING_MODULE, NULL);
          g_type_module_set_name (G_TYPE_MODULE (module), name);
          module->name = g_strdup (name);

          if (module && g_type_module_use (G_TYPE_MODULE (module)))
            {
              engine = (module->create_engine) ();

              if (engine)
                g_hash_table_insert (engines, module->name, engine);
            }
        }
    }
  else
    {
      if (G_UNLIKELY (!default_engine))
        default_engine = g_object_new (GTK_TYPE_THEMING_ENGINE, NULL);

      engine = default_engine;
    }

  return engine;
}

/**
 * gtk_theming_engine_get_screen:
 * @engine: a #GtkThemingEngine
 *
 * Returns the #GdkScreen to which @engine currently rendering to.
 *
 * Returns: (transfer none): a #GdkScreen, or %NULL.
 **/
GdkScreen *
gtk_theming_engine_get_screen (GtkThemingEngine *engine)
{
  GtkThemingEnginePrivate *priv;

  g_return_val_if_fail (GTK_IS_THEMING_ENGINE (engine), NULL);

  priv = engine->priv;
  return gtk_style_context_get_screen (priv->context);
}

/* Paint method implementations */
static void
gtk_theming_engine_render_check (GtkThemingEngine *engine,
                                 cairo_t          *cr,
                                 gdouble           x,
                                 gdouble           y,
                                 gdouble           width,
                                 gdouble           height)
{
  GdkRGBA fg_color, bg_color;
  GtkStateFlags flags;
  gint exterior_size, interior_size, thickness, pad;
  GtkBorderStyle border_style;
  GtkBorder border;
  gint border_width;
  GtkThemingBackground bg;

  _gtk_theming_background_init (&bg, engine, 
                                x, y,
                                width, height,
                                gtk_theming_engine_get_junction_sides (engine));

  if (_gtk_theming_background_has_background_image (&bg))
    {
      _gtk_theming_background_render (&bg, cr);
      return;
    }

  flags = gtk_theming_engine_get_state (engine);
  cairo_save (cr);

  gtk_theming_engine_get_color (engine, flags, &fg_color);
  gtk_theming_engine_get_background_color (engine, flags, &bg_color);
  gtk_theming_engine_get_border (engine, flags, &border);

  gtk_theming_engine_get (engine, flags,
                          "border-style", &border_style,
                          NULL);

  border_width = MIN (MIN (border.top, border.bottom),
                      MIN (border.left, border.right));
  exterior_size = MIN (width, height);

  if (exterior_size % 2 == 0) /* Ensure odd */
    exterior_size -= 1;

  /* FIXME: thickness */
  thickness = 1;
  pad = thickness + MAX (1, (exterior_size - 2 * thickness) / 9);
  interior_size = MAX (1, exterior_size - 2 * pad);

  if (interior_size < 7)
    {
      interior_size = 7;
      pad = MAX (0, (exterior_size - interior_size) / 2);
    }

  x -= (1 + exterior_size - (gint) width) / 2;
  y -= (1 + exterior_size - (gint) height) / 2;

  if (border_style == GTK_BORDER_STYLE_SOLID)
    {
      GdkRGBA border_color;

      cairo_set_line_width (cr, border_width);
      gtk_theming_engine_get_border_color (engine, flags, &border_color);

      cairo_rectangle (cr, x + 0.5, y + 0.5, exterior_size - 1, exterior_size - 1);
      gdk_cairo_set_source_rgba (cr, &bg_color);
      cairo_fill_preserve (cr);

      gdk_cairo_set_source_rgba (cr, &border_color);
      cairo_stroke (cr);
    }

  gdk_cairo_set_source_rgba (cr, &fg_color);

  if (flags & GTK_STATE_FLAG_INCONSISTENT)
    {
      int line_thickness = MAX (1, (3 + interior_size * 2) / 7);

      cairo_rectangle (cr,
		       x + pad,
		       y + pad + (1 + interior_size - line_thickness) / 2,
		       interior_size,
		       line_thickness);
      cairo_fill (cr);
    }
  else
    {
      gdouble progress;
      gboolean running;

      running = gtk_theming_engine_state_is_running (engine, GTK_STATE_ACTIVE, &progress);

      if ((flags & GTK_STATE_FLAG_ACTIVE) || running)
        {
          if (!running)
            progress = 1;

          cairo_translate (cr,
                           x + pad, y + pad);

          cairo_scale (cr, interior_size / 7., interior_size / 7.);

          cairo_rectangle (cr, 0, 0, 7 * progress, 7);
          cairo_clip (cr);

          cairo_move_to  (cr, 7.0, 0.0);
          cairo_line_to  (cr, 7.5, 1.0);
          cairo_curve_to (cr, 5.3, 2.0,
                          4.3, 4.0,
                          3.5, 7.0);
          cairo_curve_to (cr, 3.0, 5.7,
                          1.3, 4.7,
                          0.0, 4.7);
          cairo_line_to  (cr, 0.2, 3.5);
          cairo_curve_to (cr, 1.1, 3.5,
                          2.3, 4.3,
                          3.0, 5.0);
          cairo_curve_to (cr, 1.0, 3.9,
                          2.4, 4.1,
                          3.2, 4.9);
          cairo_curve_to (cr, 3.5, 3.1,
                          5.2, 2.0,
                          7.0, 0.0);

          cairo_fill (cr);
        }
    }

  cairo_restore (cr);
}

static void
gtk_theming_engine_render_option (GtkThemingEngine *engine,
                                  cairo_t          *cr,
                                  gdouble           x,
                                  gdouble           y,
                                  gdouble           width,
                                  gdouble           height)
{
  GtkStateFlags flags;
  GdkRGBA fg_color, bg_color;
  gint exterior_size, interior_size, pad, thickness, border_width;
  GtkBorderStyle border_style;
  GtkBorder border;
  GtkThemingBackground bg;

  _gtk_theming_background_init (&bg, engine, 
                                x, y,
                                width, height,
                                gtk_theming_engine_get_junction_sides (engine));

  if (_gtk_theming_background_has_background_image (&bg))
    {
      _gtk_theming_background_render (&bg, cr);
      return;
    }

  flags = gtk_theming_engine_get_state (engine);

  cairo_save (cr);

  gtk_theming_engine_get_color (engine, flags, &fg_color);
  gtk_theming_engine_get_background_color (engine, flags, &bg_color);
  gtk_theming_engine_get_border (engine, flags, &border);

  gtk_theming_engine_get (engine, flags,
                          "border-style", &border_style,
                          NULL);

  exterior_size = MIN (width, height);
  border_width = MIN (MIN (border.top, border.bottom),
                      MIN (border.left, border.right));

  if (exterior_size % 2 == 0) /* Ensure odd */
    exterior_size -= 1;

  x -= (1 + exterior_size - width) / 2;
  y -= (1 + exterior_size - height) / 2;

  if (border_style == GTK_BORDER_STYLE_SOLID)
    {
      GdkRGBA border_color;

      cairo_set_line_width (cr, border_width);
      gtk_theming_engine_get_border_color (engine, flags, &border_color);

      cairo_new_sub_path (cr);
      cairo_arc (cr,
                 x + exterior_size / 2.,
                 y + exterior_size / 2.,
                 (exterior_size - 1) / 2.,
                 0, 2 * G_PI);

      gdk_cairo_set_source_rgba (cr, &bg_color);
      cairo_fill_preserve (cr);

      gdk_cairo_set_source_rgba (cr, &border_color);
      cairo_stroke (cr);
    }

  gdk_cairo_set_source_rgba (cr, &fg_color);

  /* FIXME: thickness */
  thickness = 1;

  if (flags & GTK_STATE_FLAG_INCONSISTENT)
    {
      gint line_thickness;

      pad = thickness + MAX (1, (exterior_size - 2 * thickness) / 9);
      interior_size = MAX (1, exterior_size - 2 * pad);

      if (interior_size < 7)
        {
          interior_size = 7;
          pad = MAX (0, (exterior_size - interior_size) / 2);
        }

      line_thickness = MAX (1, (3 + interior_size * 2) / 7);

      cairo_rectangle (cr,
                       x + pad,
                       y + pad + (interior_size - line_thickness) / 2.,
                       interior_size,
                       line_thickness);
      cairo_fill (cr);
    }
  if (flags & GTK_STATE_FLAG_ACTIVE)
    {
      pad = thickness + MAX (1, 2 * (exterior_size - 2 * thickness) / 9);
      interior_size = MAX (1, exterior_size - 2 * pad);

      if (interior_size < 5)
        {
          interior_size = 7;
          pad = MAX (0, (exterior_size - interior_size) / 2);
        }

      cairo_new_sub_path (cr);
      cairo_arc (cr,
                 x + pad + interior_size / 2.,
                 y + pad + interior_size / 2.,
                 interior_size / 2.,
                 0, 2 * G_PI);
      cairo_fill (cr);
    }

  cairo_restore (cr);
}

static void
add_path_arrow (cairo_t *cr,
                gdouble  angle,
                gdouble  x,
                gdouble  y,
                gdouble  size)
{
  cairo_save (cr);

  cairo_translate (cr, x + (size / 2), y + (size / 2));
  cairo_rotate (cr, angle);

  cairo_move_to (cr, 0, - (size / 4));
  cairo_line_to (cr, - (size / 2), (size / 4));
  cairo_line_to (cr, (size / 2), (size / 4));
  cairo_close_path (cr);

  cairo_restore (cr);
}

static void
gtk_theming_engine_render_arrow (GtkThemingEngine *engine,
                                 cairo_t          *cr,
                                 gdouble           angle,
                                 gdouble           x,
                                 gdouble           y,
                                 gdouble           size)
{
  GtkStateFlags flags;
  GdkRGBA fg_color;

  cairo_save (cr);

  flags = gtk_theming_engine_get_state (engine);
  gtk_theming_engine_get_color (engine, flags, &fg_color);

  if (flags & GTK_STATE_FLAG_INSENSITIVE)
    {
      add_path_arrow (cr, angle, x + 1, y + 1, size);
      cairo_set_source_rgb (cr, 1, 1, 1);
      cairo_fill (cr);
    }

  add_path_arrow (cr, angle, x, y, size);
  gdk_cairo_set_source_rgba (cr, &fg_color);
  cairo_fill (cr);

  cairo_restore (cr);
}

static void
add_path_line (cairo_t        *cr,
               gdouble         x1,
               gdouble         y1,
               gdouble         x2,
               gdouble         y2)
{
  /* Adjust endpoints */
  if (y1 == y2)
    {
      y1 += 0.5;
      y2 += 0.5;
      x2 += 1;
    }
  else if (x1 == x2)
    {
      x1 += 0.5;
      x2 += 0.5;
      y2 += 1;
    }

  cairo_move_to (cr, x1, y1);
  cairo_line_to (cr, x2, y2);
}

static void
color_shade (const GdkRGBA *color,
             gdouble        factor,
             GdkRGBA       *color_return)
{
  GtkSymbolicColor *literal, *shade;

  literal = gtk_symbolic_color_new_literal (color);
  shade = gtk_symbolic_color_new_shade (literal, factor);
  gtk_symbolic_color_unref (literal);

  gtk_symbolic_color_resolve (shade, NULL, color_return);
  gtk_symbolic_color_unref (shade);
}

static void
gtk_theming_engine_render_background (GtkThemingEngine *engine,
                                      cairo_t          *cr,
                                      gdouble           x,
                                      gdouble           y,
                                      gdouble           width,
                                      gdouble           height)
{
  GtkThemingBackground bg;

  _gtk_theming_background_init (&bg, engine,
                                x, y,
                                width, height,
                                gtk_theming_engine_get_junction_sides (engine));

  _gtk_theming_background_render (&bg, cr);
}

static void
gtk_theming_engine_hide_border_sides (GtkBorder *border,
                                      guint      hidden_side)
{
  if (hidden_side & (1 << GTK_CSS_TOP))
    border->top = 0;
  if (hidden_side & (1 << GTK_CSS_RIGHT))
    border->right = 0;
  if (hidden_side & (1 << GTK_CSS_BOTTOM))
    border->bottom = 0;
  if (hidden_side & (1 << GTK_CSS_LEFT))
    border->left = 0;
}

static void
render_frame_fill (cairo_t       *cr,
                   GtkRoundedBox *border_box,
                   GtkBorder     *border,
                   GdkRGBA        colors[4],
                   guint          hidden_side)
{
  GtkRoundedBox padding_box;
  guint i, j;

  padding_box = *border_box;
  _gtk_rounded_box_shrink (&padding_box, border->top, border->right, border->bottom, border->left);

  if (hidden_side == 0 &&
      gdk_rgba_equal (&colors[0], &colors[1]) &&
      gdk_rgba_equal (&colors[0], &colors[2]) &&
      gdk_rgba_equal (&colors[0], &colors[3]))
    {
      gdk_cairo_set_source_rgba (cr, &colors[0]);

      _gtk_rounded_box_path (border_box, cr);
      _gtk_rounded_box_path (&padding_box, cr);
      cairo_fill (cr);
    }
  else
    {
      for (i = 0; i < 4; i++) 
        {
          if (hidden_side & (1 << i))
            continue;

          for (j = 0; j < 4; j++)
            { 
              if (hidden_side & (1 << j))
                continue;

              if (i == j || 
                  (gdk_rgba_equal (&colors[i], &colors[j])))
                {
                  /* We were already painted when i == j */
                  if (i > j)
                    break;

                  if (j == 0)
                    _gtk_rounded_box_path_top (border_box, &padding_box, cr);
                  else if (j == 1)
                    _gtk_rounded_box_path_right (border_box, &padding_box, cr);
                  else if (j == 2)
                    _gtk_rounded_box_path_bottom (border_box, &padding_box, cr);
                  else if (j == 3)
                    _gtk_rounded_box_path_left (border_box, &padding_box, cr);
                }
            }
          /* We were already painted when i == j */
          if (i > j)
            continue;

          gdk_cairo_set_source_rgba (cr, &colors[i]);

          cairo_fill (cr);
        }
    }
}

static void
set_stroke_style (cairo_t        *cr,
                  double          line_width,
                  GtkBorderStyle  style,
                  double          length)
{
  double segments[2];
  double n;

  cairo_set_line_width (cr, line_width);

  if (style == GTK_BORDER_STYLE_DOTTED)
    {
      n = round (0.5 * length / line_width);

      segments[0] = 0;
      segments[1] = n ? length / n : 2;
      cairo_set_dash (cr, segments, G_N_ELEMENTS (segments), 0);

      cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
    }
  else
    {
      n = length / line_width;
      /* Optimize the common case of an integer-sized rectangle
       * Again, we care about focus rectangles.
       */
      if (n == nearbyint (n))
        {
          segments[0] = 1;
          segments[1] = 2;
        }
      else
        {
          n = round ((1. / 3) * n);

          segments[0] = n ? (1. / 3) * length / n : 1;
          segments[1] = 2 * segments[1];
        }
      cairo_set_dash (cr, segments, G_N_ELEMENTS (segments), 0);

      cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
      cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);
    }
}

static int
get_border_side (GtkBorder  *border,
                 GtkCssSide  side)
{
  switch (side)
    {
    case GTK_CSS_TOP:
      return border->top;
    case GTK_CSS_RIGHT:
      return border->right;
    case GTK_CSS_BOTTOM:
      return border->bottom;
    case GTK_CSS_LEFT:
      return border->left;
    default:
      g_assert_not_reached ();
      return 0;
    }
}

static void
render_frame_stroke (cairo_t       *cr,
                     GtkRoundedBox *border_box,
                     GtkBorder     *border,
                     GdkRGBA        colors[4],
                     guint          hidden_side,
                     GtkBorderStyle stroke_style)
{
  gboolean different_colors, different_borders;
  GtkRoundedBox stroke_box;
  guint i;

  different_colors = !gdk_rgba_equal (&colors[0], &colors[1]) ||
                     !gdk_rgba_equal (&colors[0], &colors[2]) ||
                     !gdk_rgba_equal (&colors[0], &colors[3]);
  different_borders = border->top != border->right ||
                      border->top != border->bottom ||
                      border->top != border->left;

  stroke_box = *border_box;
  _gtk_rounded_box_shrink (&stroke_box,
                           border->top / 2.0,
                           border->right / 2.0,
                           border->bottom / 2.0,
                           border->left / 2.0);

  if (!different_colors && !different_borders && hidden_side == 0)
    {
      double length = 0;

      /* FAST PATH:
       * Mostly expected to trigger for focus rectangles */
      for (i = 0; i < 4; i++) 
        {
          length += _gtk_rounded_box_guess_length (&stroke_box, i);
          _gtk_rounded_box_path_side (&stroke_box, cr, i);
        }

      gdk_cairo_set_source_rgba (cr, &colors[0]);
      set_stroke_style (cr, border->top, stroke_style, length);
      cairo_stroke (cr);
    }
  else
    {
      GtkRoundedBox padding_box;

      padding_box = *border_box;
      _gtk_rounded_box_path (&padding_box, cr);
      _gtk_rounded_box_shrink (&padding_box,
                               border->top,
                               border->right,
                               border->bottom,
                               border->left);

      for (i = 0; i < 4; i++) 
        {
          if (hidden_side & (1 << i))
            continue;

          cairo_save (cr);

          if (i == 0)
            _gtk_rounded_box_path_top (border_box, &padding_box, cr);
          else if (i == 1)
            _gtk_rounded_box_path_right (border_box, &padding_box, cr);
          else if (i == 2)
            _gtk_rounded_box_path_bottom (border_box, &padding_box, cr);
          else if (i == 3)
            _gtk_rounded_box_path_left (border_box, &padding_box, cr);
          cairo_clip (cr);

          _gtk_rounded_box_path_side (&stroke_box, cr, i);

          gdk_cairo_set_source_rgba (cr, &colors[i]);
          set_stroke_style (cr,
                            get_border_side (border, i),
                            stroke_style,
                            _gtk_rounded_box_guess_length (&stroke_box, i));
          cairo_stroke (cr);

          cairo_restore (cr);
        }
    }
}

static void
render_border (cairo_t       *cr,
               GtkRoundedBox *border_box,
               GtkBorder     *border,
               guint          hidden_side,
               GdkRGBA        colors[4],
               GtkBorderStyle border_style[4])
{
  guint i, j;

  cairo_save (cr);

  cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);

  for (i = 0; i < 4; i++)
    {
      if (hidden_side & (1 << i))
        continue;

      switch (border_style[i])
        {
        case GTK_BORDER_STYLE_NONE:
        case GTK_BORDER_STYLE_HIDDEN:
        case GTK_BORDER_STYLE_SOLID:
          break;
        case GTK_BORDER_STYLE_INSET:
          if (i == 1 || i == 2)
            color_shade (&colors[i], 1.8, &colors[i]);
          break;
        case GTK_BORDER_STYLE_OUTSET:
          if (i == 0 || i == 3)
            color_shade (&colors[i], 1.8, &colors[i]);
          break;
        case GTK_BORDER_STYLE_DOTTED:
        case GTK_BORDER_STYLE_DASHED:
          {
            guint dont_draw = hidden_side;

            for (j = 0; j < 4; j++)
              {
                if (border_style[j] == border_style[i])
                  hidden_side |= (1 << j);
                else
                  dont_draw |= (1 << j);
              }
            
            render_frame_stroke (cr, border_box, border, colors, dont_draw, border_style[i]);
          }
          break;
        case GTK_BORDER_STYLE_DOUBLE:
          {
            GtkRoundedBox other_box;
            GtkBorder other_border;
            guint dont_draw = hidden_side;

            for (j = 0; j < 4; j++)
              {
                if (border_style[j] == GTK_BORDER_STYLE_DOUBLE)
                  hidden_side |= (1 << j);
                else
                  dont_draw |= (1 << j);
              }
            other_border.top = (border->top + 2) / 3;
            other_border.right = (border->right + 2) / 3;
            other_border.bottom = (border->bottom + 2) / 3;
            other_border.left = (border->left + 2) / 3;
            
            render_frame_fill (cr, border_box, &other_border, colors, dont_draw);
            
            other_box = *border_box;
            _gtk_rounded_box_shrink (&other_box,
                                     border->top - other_border.top,
                                     border->right - other_border.right,
                                     border->bottom - other_border.bottom,
                                     border->left - other_border.left);
            render_frame_fill (cr, &other_box, &other_border, colors, dont_draw);
          }
        case GTK_BORDER_STYLE_GROOVE:
        case GTK_BORDER_STYLE_RIDGE:
          {
            GtkRoundedBox other_box;
            GdkRGBA other_colors[4];
            guint dont_draw = hidden_side;
            GtkBorder other_border;

            for (j = 0; j < 4; j++)
              {
                other_colors[j] = colors[j];
                if ((j == 0 || j == 3) ^ (border_style[j] == GTK_BORDER_STYLE_RIDGE))
                  color_shade (&other_colors[j], 1.8, &other_colors[j]);
                else
                  color_shade (&colors[j], 1.8, &colors[j]);
                if (border_style[j] == GTK_BORDER_STYLE_GROOVE ||
                    border_style[j] == GTK_BORDER_STYLE_RIDGE)
                  hidden_side |= (1 << j);
                else
                  dont_draw |= (1 << j);
              }
            other_border.top = border->top / 2;
            other_border.right = border->right / 2;
            other_border.bottom = border->bottom / 2;
            other_border.left = border->left / 2;
            
            render_frame_fill (cr, border_box, &other_border, colors, dont_draw);
            
            other_box = *border_box;
            _gtk_rounded_box_shrink (&other_box,
                                     other_border.top, other_border.right,
                                     other_border.bottom, other_border.left);
            other_border.top = border->top - other_border.top;
            other_border.right = border->right - other_border.right;
            other_border.bottom = border->bottom - other_border.bottom;
            other_border.left = border->left - other_border.left;
            render_frame_fill (cr, &other_box, &other_border, other_colors, dont_draw);
          }
          break;
        default:
          g_assert_not_reached ();
          break;
        }
    }
  
  render_frame_fill (cr, border_box, border, colors, hidden_side);

  cairo_restore (cr);
}

static void
render_frame_internal (GtkThemingEngine *engine,
                       cairo_t          *cr,
                       gdouble           x,
                       gdouble           y,
                       gdouble           width,
                       gdouble           height,
                       guint             hidden_side,
                       GtkJunctionSides  junction)
{
  GtkBorderImage border_image;
  GtkStateFlags state;
  GtkBorderStyle border_style[4];
  GtkRoundedBox border_box;
  gdouble progress;
  gboolean running;
  GtkBorder border;
  GdkRGBA *alloc_colors[4];
  GdkRGBA colors[4];
  guint i;

  state = gtk_theming_engine_get_state (engine);

  gtk_theming_engine_get_border (engine, state, &border);
  gtk_theming_engine_hide_border_sides (&border, hidden_side);

  if (_gtk_border_image_init (&border_image, engine))
    _gtk_border_image_render (&border_image, &border, cr, x, y, width, height);
  else
    {
      gtk_theming_engine_get (engine, state,
                              "border-top-style", &border_style[0],
                              "border-right-style", &border_style[1],
                              "border-bottom-style", &border_style[2],
                              "border-left-style", &border_style[3],
                              "border-top-color", &alloc_colors[0],
                              "border-right-color", &alloc_colors[1],
                              "border-bottom-color", &alloc_colors[2],
                              "border-left-color", &alloc_colors[3],
                              NULL);

      running = gtk_theming_engine_state_is_running (engine, GTK_STATE_PRELIGHT, &progress);

      if (running)
        {
          GtkStateFlags other_state;
          GdkRGBA *other_colors[4];

          if (state & GTK_STATE_FLAG_PRELIGHT)
            {
              other_state = state & ~(GTK_STATE_FLAG_PRELIGHT);
              progress = 1 - progress;
            }
          else
            other_state = state | GTK_STATE_FLAG_PRELIGHT;

          gtk_theming_engine_get (engine, other_state,
                                  "border-top-color", &other_colors[0],
                                  "border-right-color", &other_colors[1],
                                  "border-bottom-color", &other_colors[2],
                                  "border-left-color", &other_colors[3],
                                  NULL);

          for (i = 0; i < 4; i++)
            {
              colors[i].red = CLAMP (alloc_colors[i]->red + ((other_colors[i]->red - alloc_colors[i]->red) * progress), 0, 1);
              colors[i].green = CLAMP (alloc_colors[i]->green + ((other_colors[i]->green - alloc_colors[i]->green) * progress), 0, 1);
              colors[i].blue = CLAMP (alloc_colors[i]->blue + ((other_colors[i]->blue - alloc_colors[i]->blue) * progress), 0, 1);
              colors[i].alpha = CLAMP (alloc_colors[i]->alpha + ((other_colors[i]->alpha - alloc_colors[i]->alpha) * progress), 0, 1);
              gdk_rgba_free (other_colors[i]);
              gdk_rgba_free (alloc_colors[i]);
            }
        }
      else
        {
          for (i = 0; i < 4; i++)
            {
              colors[i] = *alloc_colors[i];
              gdk_rgba_free (alloc_colors[i]);
            }
        }

      _gtk_rounded_box_init_rect (&border_box, x, y, width, height);
      _gtk_rounded_box_apply_border_radius (&border_box, engine, state, junction);

      render_border (cr, &border_box, &border, hidden_side, colors, border_style);
    }

  border_style[0] = g_value_get_enum (_gtk_theming_engine_peek_property (engine, "outline-style"));
  if (border_style[0] != GTK_BORDER_STYLE_NONE)
    {
      int offset;

      border_style[1] = border_style[2] = border_style[3] = border_style[0];
      border.top = g_value_get_int (_gtk_theming_engine_peek_property (engine, "outline-width"));
      border.left = border.right = border.bottom = border.top;
      colors[0] = *(GdkRGBA *) g_value_get_boxed (_gtk_theming_engine_peek_property (engine, "outline-color"));
      colors[3] = colors[2] = colors[1] = colors[0];
      offset = g_value_get_int (_gtk_theming_engine_peek_property (engine, "outline-offset"));
      
      /* reinit box here - outlines don't have a border radius */
      _gtk_rounded_box_init_rect (&border_box, x, y, width, height);
      _gtk_rounded_box_shrink (&border_box,
                               - border.top - offset,
                               - border.right - offset,
                               - border.left - offset,
                               - border.bottom - offset);
      
      render_border (cr, &border_box, &border, hidden_side, colors, border_style);
    }
}

static void
gtk_theming_engine_render_frame (GtkThemingEngine *engine,
                                 cairo_t          *cr,
                                 gdouble           x,
                                 gdouble           y,
                                 gdouble           width,
                                 gdouble           height)
{
  GtkJunctionSides junction;

  junction = gtk_theming_engine_get_junction_sides (engine);

  render_frame_internal (engine, cr,
                         x, y, width, height,
                         0, junction);
}

static void
gtk_theming_engine_render_expander (GtkThemingEngine *engine,
                                    cairo_t          *cr,
                                    gdouble           x,
                                    gdouble           y,
                                    gdouble           width,
                                    gdouble           height)
{
  GtkStateFlags flags;
  GdkRGBA outline_color, fg_color;
  double vertical_overshoot;
  int diameter;
  double radius;
  double interp;		/* interpolation factor for center position */
  double x_double_horz, y_double_horz;
  double x_double_vert, y_double_vert;
  double x_double, y_double;
  gdouble angle;
  gint line_width;
  gboolean running, is_rtl;
  gdouble progress;

  cairo_save (cr);
  flags = gtk_theming_engine_get_state (engine);

  gtk_theming_engine_get_color (engine, flags, &fg_color);
  gtk_theming_engine_get_border_color (engine, flags, &outline_color);

  running = gtk_theming_engine_state_is_running (engine, GTK_STATE_ACTIVE, &progress);
  is_rtl = (gtk_theming_engine_get_direction (engine) == GTK_TEXT_DIR_RTL);
  line_width = 1;

  if (!running)
    progress = (flags & GTK_STATE_FLAG_ACTIVE) ? 1 : 0;

  if (!gtk_theming_engine_has_class (engine, GTK_STYLE_CLASS_HORIZONTAL))
    {
      if (is_rtl)
        angle = (G_PI) - ((G_PI / 2) * progress);
      else
        angle = (G_PI / 2) * progress;
    }
  else
    {
      if (is_rtl)
        angle = (G_PI / 2) + ((G_PI / 2) * progress);
      else
        angle = (G_PI / 2) - ((G_PI / 2) * progress);
    }

  interp = progress;

  /* Compute distance that the stroke extends beyonds the end
   * of the triangle we draw.
   */
  vertical_overshoot = line_width / 2.0 * (1. / tan (G_PI / 8));

  /* For odd line widths, we end the vertical line of the triangle
   * at a half pixel, so we round differently.
   */
  if (line_width % 2 == 1)
    vertical_overshoot = ceil (0.5 + vertical_overshoot) - 0.5;
  else
    vertical_overshoot = ceil (vertical_overshoot);

  /* Adjust the size of the triangle we draw so that the entire stroke fits
   */
  diameter = (gint) MAX (3, width - 2 * vertical_overshoot);

  /* If the line width is odd, we want the diameter to be even,
   * and vice versa, so force the sum to be odd. This relationship
   * makes the point of the triangle look right.
   */
  diameter -= (1 - (diameter + line_width) % 2);

  radius = diameter / 2.;

  /* Adjust the center so that the stroke is properly aligned with
   * the pixel grid. The center adjustment is different for the
   * horizontal and vertical orientations. For intermediate positions
   * we interpolate between the two.
   */
  x_double_vert = floor ((x + width / 2) - (radius + line_width) / 2.) + (radius + line_width) / 2.;
  y_double_vert = (y + height / 2) - 0.5;

  x_double_horz = (x + width / 2) - 0.5;
  y_double_horz = floor ((y + height / 2) - (radius + line_width) / 2.) + (radius + line_width) / 2.;

  x_double = x_double_vert * (1 - interp) + x_double_horz * interp;
  y_double = y_double_vert * (1 - interp) + y_double_horz * interp;

  cairo_translate (cr, x_double, y_double);
  cairo_rotate (cr, angle);

  cairo_move_to (cr, - radius / 2., - radius);
  cairo_line_to (cr,   radius / 2.,   0);
  cairo_line_to (cr, - radius / 2.,   radius);
  cairo_close_path (cr);

  cairo_set_line_width (cr, line_width);

  gdk_cairo_set_source_rgba (cr, &fg_color);

  cairo_fill_preserve (cr);

  gdk_cairo_set_source_rgba (cr, &outline_color);
  cairo_stroke (cr);

  cairo_restore (cr);
}

static void
gtk_theming_engine_render_focus (GtkThemingEngine *engine,
                                 cairo_t          *cr,
                                 gdouble           x,
                                 gdouble           y,
                                 gdouble           width,
                                 gdouble           height)
{
  GtkStateFlags flags;
  GdkRGBA color;
  gint line_width;
  gint8 *dash_list;

  cairo_save (cr);
  flags = gtk_theming_engine_get_state (engine);

  gtk_theming_engine_get_color (engine, flags, &color);

  gtk_theming_engine_get_style (engine,
				"focus-line-width", &line_width,
				"focus-line-pattern", (gchar *) &dash_list,
				NULL);

  cairo_set_line_width (cr, (gdouble) line_width);

  if (dash_list[0])
    {
      gint n_dashes = strlen ((const gchar *) dash_list);
      gdouble *dashes = g_new (gdouble, n_dashes);
      gdouble total_length = 0;
      gdouble dash_offset;
      gint i;

      for (i = 0; i < n_dashes; i++)
	{
	  dashes[i] = dash_list[i];
	  total_length += dash_list[i];
	}

      /* The dash offset here aligns the pattern to integer pixels
       * by starting the dash at the right side of the left border
       * Negative dash offsets in cairo don't work
       * (https://bugs.freedesktop.org/show_bug.cgi?id=2729)
       */
      dash_offset = - line_width / 2.;

      while (dash_offset < 0)
	dash_offset += total_length;

      cairo_set_dash (cr, dashes, n_dashes, dash_offset);
      g_free (dashes);
    }

  cairo_rectangle (cr,
                   x + line_width / 2.,
                   y + line_width / 2.,
                   width - line_width,
                   height - line_width);

  gdk_cairo_set_source_rgba (cr, &color);
  cairo_stroke (cr);

  cairo_restore (cr);

  g_free (dash_list);
}

static void
gtk_theming_engine_render_line (GtkThemingEngine *engine,
                                cairo_t          *cr,
                                gdouble           x0,
                                gdouble           y0,
                                gdouble           x1,
                                gdouble           y1)
{
  GdkRGBA color;
  GtkStateFlags flags;

  flags = gtk_theming_engine_get_state (engine);
  cairo_save (cr);

  gtk_theming_engine_get_color (engine, flags, &color);

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
  cairo_set_line_width (cr, 1);

  cairo_move_to (cr, x0 + 0.5, y0 + 0.5);
  cairo_line_to (cr, x1 + 0.5, y1 + 0.5);

  gdk_cairo_set_source_rgba (cr, &color);
  cairo_stroke (cr);

  cairo_restore (cr);
}

static void
prepare_context_for_layout (cairo_t *cr,
                            gdouble x,
                            gdouble y,
                            PangoLayout *layout)
{
  const PangoMatrix *matrix;

  matrix = pango_context_get_matrix (pango_layout_get_context (layout));

  cairo_move_to (cr, x, y);

  if (matrix)
    {
      cairo_matrix_t cairo_matrix;

      cairo_matrix_init (&cairo_matrix,
                         matrix->xx, matrix->yx,
                         matrix->xy, matrix->yy,
                         matrix->x0, matrix->y0);

      cairo_transform (cr, &cairo_matrix);
    }
}

static void
gtk_theming_engine_render_layout (GtkThemingEngine *engine,
                                  cairo_t          *cr,
                                  gdouble           x,
                                  gdouble           y,
                                  PangoLayout      *layout)
{
  GdkRGBA fg_color;
  GtkShadow *text_shadow = NULL;
  GtkStateFlags flags;
  gdouble progress;
  gboolean running;

  cairo_save (cr);
  flags = gtk_theming_engine_get_state (engine);
  gtk_theming_engine_get_color (engine, flags, &fg_color);

  running = gtk_theming_engine_state_is_running (engine, GTK_STATE_PRELIGHT, &progress);

  if (running)
    {
      GtkStateFlags other_flags;
      GdkRGBA other_fg;

      if (flags & GTK_STATE_FLAG_PRELIGHT)
        {
          other_flags = flags & ~(GTK_STATE_FLAG_PRELIGHT);
          progress = 1 - progress;
        }
      else
        other_flags = flags | GTK_STATE_FLAG_PRELIGHT;

      gtk_theming_engine_get_color (engine, other_flags, &other_fg);

      fg_color.red = CLAMP (fg_color.red + ((other_fg.red - fg_color.red) * progress), 0, 1);
      fg_color.green = CLAMP (fg_color.green + ((other_fg.green - fg_color.green) * progress), 0, 1);
      fg_color.blue = CLAMP (fg_color.blue + ((other_fg.blue - fg_color.blue) * progress), 0, 1);
      fg_color.alpha = CLAMP (fg_color.alpha + ((other_fg.alpha - fg_color.alpha) * progress), 0, 1);
    }

  gtk_theming_engine_get (engine, flags,
                          "text-shadow", &text_shadow,
                          NULL);

  prepare_context_for_layout (cr, x, y, layout);

  if (text_shadow != NULL)
    {
      _gtk_text_shadow_paint_layout (text_shadow, cr, layout);
      _gtk_shadow_unref (text_shadow);
    }

  gdk_cairo_set_source_rgba (cr, &fg_color);
  pango_cairo_show_layout (cr, layout);

  cairo_restore (cr);
}

static void
gtk_theming_engine_render_slider (GtkThemingEngine *engine,
                                  cairo_t          *cr,
                                  gdouble           x,
                                  gdouble           y,
                                  gdouble           width,
                                  gdouble           height,
                                  GtkOrientation    orientation)
{
  const GtkWidgetPath *path;
  gint thickness;

  path = gtk_theming_engine_get_path (engine);

  gtk_theming_engine_render_background (engine, cr, x, y, width, height);
  gtk_theming_engine_render_frame (engine, cr, x, y, width, height);

  /* FIXME: thickness */
  thickness = 2;

  if (gtk_widget_path_is_type (path, GTK_TYPE_SCALE))
    {
      if (orientation == GTK_ORIENTATION_VERTICAL)
        gtk_theming_engine_render_line (engine, cr,
                                        x + thickness,
                                        y + (gint) height / 2,
                                        x + width - thickness - 1,
                                        y + (gint) height / 2);
      else
        gtk_theming_engine_render_line (engine, cr,
                                        x + (gint) width / 2,
                                        y + thickness,
                                        x + (gint) width / 2,
                                        y + height - thickness - 1);
    }
}

static void
gtk_theming_engine_render_frame_gap (GtkThemingEngine *engine,
                                     cairo_t          *cr,
                                     gdouble           x,
                                     gdouble           y,
                                     gdouble           width,
                                     gdouble           height,
                                     GtkPositionType   gap_side,
                                     gdouble           xy0_gap,
                                     gdouble           xy1_gap)
{
  GtkJunctionSides junction;
  GtkStateFlags state;
  gint border_width;
  GtkCssBorderCornerRadius *top_left_radius, *top_right_radius;
  GtkCssBorderCornerRadius *bottom_left_radius, *bottom_right_radius;
  gdouble x0, y0, x1, y1, xc, yc, wc, hc;
  GtkBorder border;

  xc = yc = wc = hc = 0;
  state = gtk_theming_engine_get_state (engine);
  junction = gtk_theming_engine_get_junction_sides (engine);

  gtk_theming_engine_get_border (engine, state, &border);
  gtk_theming_engine_get (engine, state,
			  "border-top-left-radius", &top_left_radius,
			  "border-top-right-radius", &top_right_radius,
			  "border-bottom-right-radius", &bottom_right_radius,
			  "border-bottom-left-radius", &bottom_left_radius,
			  NULL);

  border_width = MIN (MIN (border.top, border.bottom),
                      MIN (border.left, border.right));

  cairo_save (cr);

  switch (gap_side)
    {
    case GTK_POS_TOP:
      xc = x + xy0_gap + border_width;
      yc = y;
      wc = MAX (xy1_gap - xy0_gap - 2 * border_width, 0);
      hc = border_width;

      if (xy0_gap < _gtk_css_number_get (&top_left_radius->horizontal, width))
        junction |= GTK_JUNCTION_CORNER_TOPLEFT;

      if (xy1_gap > width - _gtk_css_number_get (&top_right_radius->horizontal, width))
        junction |= GTK_JUNCTION_CORNER_TOPRIGHT;
      break;
    case GTK_POS_BOTTOM:
      xc = x + xy0_gap + border_width;
      yc = y + height - border_width;
      wc = MAX (xy1_gap - xy0_gap - 2 * border_width, 0);
      hc = border_width;

      if (xy0_gap < _gtk_css_number_get (&bottom_left_radius->horizontal, width))
        junction |= GTK_JUNCTION_CORNER_BOTTOMLEFT;

      if (xy1_gap > width - _gtk_css_number_get (&bottom_right_radius->horizontal, width))
        junction |= GTK_JUNCTION_CORNER_BOTTOMRIGHT;

      break;
    case GTK_POS_LEFT:
      xc = x;
      yc = y + xy0_gap + border_width;
      wc = border_width;
      hc = MAX (xy1_gap - xy0_gap - 2 * border_width, 0);

      if (xy0_gap < _gtk_css_number_get (&top_left_radius->vertical, height))
        junction |= GTK_JUNCTION_CORNER_TOPLEFT;

      if (xy1_gap > height - _gtk_css_number_get (&bottom_left_radius->vertical, height))
        junction |= GTK_JUNCTION_CORNER_BOTTOMLEFT;

      break;
    case GTK_POS_RIGHT:
      xc = x + width - border_width;
      yc = y + xy0_gap + border_width;
      wc = border_width;
      hc = MAX (xy1_gap - xy0_gap - 2 * border_width, 0);

      if (xy0_gap < _gtk_css_number_get (&top_right_radius->vertical, height))
        junction |= GTK_JUNCTION_CORNER_TOPRIGHT;

      if (xy1_gap > height - _gtk_css_number_get (&bottom_right_radius->vertical, height))
        junction |= GTK_JUNCTION_CORNER_BOTTOMRIGHT;

      break;
    }

  cairo_clip_extents (cr, &x0, &y0, &x1, &y1);
  cairo_rectangle (cr, x0, y0, x1 - x0, yc - y0);
  cairo_rectangle (cr, x0, yc, xc - x0, hc);
  cairo_rectangle (cr, xc + wc, yc, x1 - (xc + wc), hc);
  cairo_rectangle (cr, x0, yc + hc, x1 - x0, y1 - (yc + hc));
  cairo_clip (cr);

  render_frame_internal (engine, cr,
                         x, y, width, height,
                         0, junction);

  cairo_restore (cr);

  g_free (top_left_radius);
  g_free (top_right_radius);
  g_free (bottom_right_radius);
  g_free (bottom_left_radius);
}

static void
gtk_theming_engine_render_extension (GtkThemingEngine *engine,
                                     cairo_t          *cr,
                                     gdouble           x,
                                     gdouble           y,
                                     gdouble           width,
                                     gdouble           height,
                                     GtkPositionType   gap_side)
{
  GtkThemingBackground bg;
  GtkJunctionSides junction = 0;
  guint hidden_side = 0;

  cairo_save (cr);

  switch (gap_side)
    {
    case GTK_POS_LEFT:
      junction = GTK_JUNCTION_LEFT;
      hidden_side = (1 << GTK_CSS_LEFT);

      cairo_translate (cr, x + width, y);
      cairo_rotate (cr, G_PI / 2);
      break;
    case GTK_POS_RIGHT:
      junction = GTK_JUNCTION_RIGHT;
      hidden_side = (1 << GTK_CSS_RIGHT);

      cairo_translate (cr, x, y + height);
      cairo_rotate (cr, - G_PI / 2);
      break;
    case GTK_POS_TOP:
      junction = GTK_JUNCTION_TOP;
      hidden_side = (1 << GTK_CSS_TOP);

      cairo_translate (cr, x + width, y + height);
      cairo_rotate (cr, G_PI);
      break;
    case GTK_POS_BOTTOM:
      junction = GTK_JUNCTION_BOTTOM;
      hidden_side = (1 << GTK_CSS_BOTTOM);

      cairo_translate (cr, x, y);
      break;
    }

  if (gap_side == GTK_POS_TOP ||
      gap_side == GTK_POS_BOTTOM)
    _gtk_theming_background_init (&bg, engine, 
                                  0, 0,
                                  width, height,
                                  GTK_JUNCTION_BOTTOM);
  else
    _gtk_theming_background_init (&bg, engine, 
                                  0, 0,
                                  height, width,
                                  GTK_JUNCTION_BOTTOM);

  _gtk_theming_background_render (&bg, cr);

  cairo_restore (cr);

  cairo_save (cr);

  render_frame_internal (engine, cr,
                         x, y, width, height,
                         hidden_side, junction);

  cairo_restore (cr);
}

static void
render_dot (cairo_t       *cr,
            const GdkRGBA *lighter,
            const GdkRGBA *darker,
            gdouble        x,
            gdouble        y,
            gdouble        size)
{
  size = CLAMP ((gint) size, 2, 3);

  if (size == 2)
    {
      gdk_cairo_set_source_rgba (cr, lighter);
      cairo_rectangle (cr, x, y, 1, 1);
      cairo_rectangle (cr, x + 1, y + 1, 1, 1);
      cairo_fill (cr);
    }
  else if (size == 3)
    {
      gdk_cairo_set_source_rgba (cr, lighter);
      cairo_rectangle (cr, x, y, 2, 1);
      cairo_rectangle (cr, x, y, 1, 2);
      cairo_fill (cr);

      gdk_cairo_set_source_rgba (cr, darker);
      cairo_rectangle (cr, x + 1, y + 1, 2, 1);
      cairo_rectangle (cr, x + 2, y, 1, 2);
      cairo_fill (cr);
    }
}

static void
gtk_theming_engine_render_handle (GtkThemingEngine *engine,
                                  cairo_t          *cr,
                                  gdouble           x,
                                  gdouble           y,
                                  gdouble           width,
                                  gdouble           height)
{
  GtkStateFlags flags;
  GdkRGBA bg_color, lighter, darker;
  GtkJunctionSides sides;
  GtkThemingBackground bg;
  gint xx, yy;

  cairo_save (cr);
  flags = gtk_theming_engine_get_state (engine);

  cairo_set_line_width (cr, 1.0);
  sides = gtk_theming_engine_get_junction_sides (engine);
  gtk_theming_engine_get_background_color (engine, flags, &bg_color);

  color_shade (&bg_color, 0.7, &darker);
  color_shade (&bg_color, 1.3, &lighter);

  _gtk_theming_background_init (&bg, engine, x, y, width, height, sides);
  _gtk_theming_background_render (&bg, cr);

  if (gtk_theming_engine_has_class (engine, GTK_STYLE_CLASS_GRIP))
    {
      /* reduce confusing values to a meaningful state */
      if ((sides & (GTK_JUNCTION_CORNER_TOPLEFT | GTK_JUNCTION_CORNER_BOTTOMRIGHT)) == (GTK_JUNCTION_CORNER_TOPLEFT | GTK_JUNCTION_CORNER_BOTTOMRIGHT))
        sides &= ~GTK_JUNCTION_CORNER_TOPLEFT;

      if ((sides & (GTK_JUNCTION_CORNER_TOPRIGHT | GTK_JUNCTION_CORNER_BOTTOMLEFT)) == (GTK_JUNCTION_CORNER_TOPRIGHT | GTK_JUNCTION_CORNER_BOTTOMLEFT))
        sides &= ~GTK_JUNCTION_CORNER_TOPRIGHT;

      if (sides == 0)
        sides = GTK_JUNCTION_CORNER_BOTTOMRIGHT;

      /* align drawing area to the connected side */
      if (sides == GTK_JUNCTION_LEFT)
        {
          if (height < width)
            width = height;
        }
      else if (sides == GTK_JUNCTION_CORNER_TOPLEFT)
        {
          if (width < height)
            height = width;
          else if (height < width)
            width = height;
        }
      else if (sides == GTK_JUNCTION_CORNER_BOTTOMLEFT)
        {
          /* make it square, aligning to bottom left */
          if (width < height)
            {
              y += (height - width);
              height = width;
            }
          else if (height < width)
            width = height;
        }
      else if (sides == GTK_JUNCTION_RIGHT)
        {
          /* aligning to right */
          if (height < width)
            {
              x += (width - height);
              width = height;
            }
        }
      else if (sides == GTK_JUNCTION_CORNER_TOPRIGHT)
        {
          if (width < height)
            height = width;
          else if (height < width)
            {
              x += (width - height);
              width = height;
            }
        }
      else if (sides == GTK_JUNCTION_CORNER_BOTTOMRIGHT)
        {
          /* make it square, aligning to bottom right */
          if (width < height)
            {
              y += (height - width);
              height = width;
            }
          else if (height < width)
            {
              x += (width - height);
              width = height;
            }
        }
      else if (sides == GTK_JUNCTION_TOP)
        {
          if (width < height)
            height = width;
        }
      else if (sides == GTK_JUNCTION_BOTTOM)
        {
          /* align to bottom */
          if (width < height)
            {
              y += (height - width);
              height = width;
            }
        }
      else
        g_assert_not_reached ();

      if (sides == GTK_JUNCTION_LEFT ||
          sides == GTK_JUNCTION_RIGHT)
        {
          gint xi;

          xi = x;

          while (xi < x + width)
            {
              gdk_cairo_set_source_rgba (cr, &lighter);
              add_path_line (cr, x, y, x, y + height);
              cairo_stroke (cr);
              xi++;

              gdk_cairo_set_source_rgba (cr, &darker);
              add_path_line (cr, xi, y, xi, y + height);
              cairo_stroke (cr);
              xi += 2;
            }
        }
      else if (sides == GTK_JUNCTION_TOP ||
               sides == GTK_JUNCTION_BOTTOM)
        {
          gint yi;

          yi = y;

          while (yi < y + height)
            {
              gdk_cairo_set_source_rgba (cr, &lighter);
              add_path_line (cr, x, yi, x + width, yi);
              cairo_stroke (cr);
              yi++;

              gdk_cairo_set_source_rgba (cr, &darker);
              add_path_line (cr, x, yi, x + width, yi);
              cairo_stroke (cr);
              yi += 2;
            }
        }
      else if (sides == GTK_JUNCTION_CORNER_TOPLEFT)
        {
          gint xi, yi;

          xi = x + width;
          yi = y + height;

          while (xi > x + 3)
            {
              gdk_cairo_set_source_rgba (cr, &darker);
              add_path_line (cr, xi, y, x, yi);
              cairo_stroke (cr);

              --xi;
              --yi;

              add_path_line (cr, xi, y, x, yi);
              cairo_stroke (cr);

              --xi;
              --yi;

              gdk_cairo_set_source_rgba (cr, &lighter);
              add_path_line (cr, xi, y, x, yi);
              cairo_stroke (cr);

              xi -= 3;
              yi -= 3;
            }
        }
      else if (sides == GTK_JUNCTION_CORNER_TOPRIGHT)
        {
          gint xi, yi;

          xi = x;
          yi = y + height;

          while (xi < (x + width - 3))
            {
              gdk_cairo_set_source_rgba (cr, &lighter);
              add_path_line (cr, xi, y, x + width, yi);
              cairo_stroke (cr);

              ++xi;
              --yi;

              gdk_cairo_set_source_rgba (cr, &darker);
              add_path_line (cr, xi, y, x + width, yi);
              cairo_stroke (cr);

              ++xi;
              --yi;

              add_path_line (cr, xi, y, x + width, yi);
              cairo_stroke (cr);

              xi += 3;
              yi -= 3;
            }
        }
      else if (sides == GTK_JUNCTION_CORNER_BOTTOMLEFT)
        {
          gint xi, yi;

          xi = x + width;
          yi = y;

          while (xi > x + 3)
            {
              gdk_cairo_set_source_rgba (cr, &darker);
              add_path_line (cr, x, yi, xi, y + height);
              cairo_stroke (cr);

              --xi;
              ++yi;

              add_path_line (cr, x, yi, xi, y + height);
              cairo_stroke (cr);

              --xi;
              ++yi;

              gdk_cairo_set_source_rgba (cr, &lighter);
              add_path_line (cr, x, yi, xi, y + height);
              cairo_stroke (cr);

              xi -= 3;
              yi += 3;
            }
        }
      else if (sides == GTK_JUNCTION_CORNER_BOTTOMRIGHT)
        {
          gint xi, yi;

          xi = x;
          yi = y;

          while (xi < (x + width - 3))
            {
              gdk_cairo_set_source_rgba (cr, &lighter);
              add_path_line (cr, xi, y + height, x + width, yi);
              cairo_stroke (cr);

              ++xi;
              ++yi;

              gdk_cairo_set_source_rgba (cr, &darker);
              add_path_line (cr, xi, y + height, x + width, yi);
              cairo_stroke (cr);

              ++xi;
              ++yi;

              add_path_line (cr, xi, y + height, x + width, yi);
              cairo_stroke (cr);

              xi += 3;
              yi += 3;
            }
        }
    }
  else if (gtk_theming_engine_has_class (engine, GTK_STYLE_CLASS_PANE_SEPARATOR))
    {
      if (width > height)
        for (xx = x + width / 2 - 15; xx <= x + width / 2 + 15; xx += 5)
          render_dot (cr, &lighter, &darker, xx, y + height / 2 - 1, 3);
      else
        for (yy = y + height / 2 - 15; yy <= y + height / 2 + 15; yy += 5)
          render_dot (cr, &lighter, &darker, x + width / 2 - 1, yy, 3);
    }
  else
    {
      for (yy = y; yy < y + height; yy += 3)
        for (xx = x; xx < x + width; xx += 6)
          {
            render_dot (cr, &lighter, &darker, xx, yy, 2);
            render_dot (cr, &lighter, &darker, xx + 3, yy + 1, 2);
          }
    }

  cairo_restore (cr);
}

void
_gtk_theming_engine_paint_spinner (cairo_t *cr,
                                   gdouble  radius,
                                   gdouble  progress,
                                   GdkRGBA *color)
{
  guint num_steps, step;
  gdouble half;
  gint i;

  num_steps = 12;

  if (progress >= 0)
    step = (guint) (progress * num_steps);
  else
    step = 0;

  cairo_save (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_line_width (cr, 2.0);

  half = num_steps / 2;

  for (i = 0; i < num_steps; i++)
    {
      gint inset = 0.7 * radius;

      /* transparency is a function of time and intial value */
      gdouble t = 1.0 - (gdouble) ((i + step) % num_steps) / num_steps;
      gdouble xscale = - sin (i * G_PI / half);
      gdouble yscale = - cos (i * G_PI / half);

      cairo_set_source_rgba (cr,
                             color->red,
                             color->green,
                             color->blue,
                             color->alpha * t);

      cairo_move_to (cr,
                     (radius - inset) * xscale,
                     (radius - inset) * yscale);
      cairo_line_to (cr,
                     radius * xscale,
                     radius * yscale);

      cairo_stroke (cr);
    }

  cairo_restore (cr);
}

static void
render_spinner (GtkThemingEngine *engine,
                cairo_t          *cr,
                gdouble           x,
                gdouble           y,
                gdouble           width,
                gdouble           height)
{
  GtkStateFlags state;
  GtkShadow *shadow;
  GdkRGBA color;
  gdouble progress;
  gdouble radius;

  state = gtk_theming_engine_get_state (engine);

  if (!gtk_theming_engine_state_is_running (engine, GTK_STATE_ACTIVE, &progress))
    progress = -1;

  radius = MIN (width / 2, height / 2);

  gtk_theming_engine_get_color (engine, state, &color);
  gtk_theming_engine_get (engine, state,
                          "icon-shadow", &shadow,
                          NULL);

  cairo_save (cr);
  cairo_translate (cr, x + width / 2, y + height / 2);

  if (shadow != NULL)
    {
      _gtk_icon_shadow_paint_spinner (shadow, cr,
                                      radius,
                                      progress);
      _gtk_shadow_unref (shadow);
    }

  _gtk_theming_engine_paint_spinner (cr,
                                     radius,
                                     progress,
                                     &color);

  cairo_restore (cr);
}

static void
gtk_theming_engine_render_activity (GtkThemingEngine *engine,
                                    cairo_t          *cr,
                                    gdouble           x,
                                    gdouble           y,
                                    gdouble           width,
                                    gdouble           height)
{
  if (gtk_theming_engine_has_class (engine, GTK_STYLE_CLASS_SPINNER))
    {
      render_spinner (engine, cr, x, y, width, height);
    }
  else
    {
      gtk_theming_engine_render_background (engine, cr, x, y, width, height);
      gtk_theming_engine_render_frame (engine, cr, x, y, width, height);
    }
}

static GdkPixbuf *
scale_or_ref (GdkPixbuf *src,
              gint       width,
              gint       height)
{
  if (width == gdk_pixbuf_get_width (src) &&
      height == gdk_pixbuf_get_height (src))
    return g_object_ref (src);
  else
    return gdk_pixbuf_scale_simple (src,
                                    width, height,
                                    GDK_INTERP_BILINEAR);
}

static gboolean
lookup_icon_size (GtkThemingEngine *engine,
		  GtkIconSize       size,
		  gint             *width,
		  gint             *height)
{
  GdkScreen *screen;
  GtkSettings *settings;

  screen = gtk_theming_engine_get_screen (engine);
  settings = gtk_settings_get_for_screen (screen);

  return gtk_icon_size_lookup_for_settings (settings, size, width, height);
}

static void
colorshift_source (cairo_t *cr,
		   gdouble shift)
{
  cairo_pattern_t *source;

  cairo_save (cr);
  cairo_paint (cr);

  source = cairo_pattern_reference (cairo_get_source (cr));

  cairo_set_source_rgb (cr, shift, shift, shift);
  cairo_set_operator (cr, CAIRO_OPERATOR_COLOR_DODGE);

  cairo_mask (cr, source);

  cairo_pattern_destroy (source);
  cairo_restore (cr);
}

static GdkPixbuf *
gtk_theming_engine_render_icon_pixbuf (GtkThemingEngine    *engine,
                                       const GtkIconSource *source,
                                       GtkIconSize          size)
{
  GdkPixbuf *scaled;
  GdkPixbuf *stated;
  GdkPixbuf *base_pixbuf;
  GtkStateFlags state;
  gint width = 1;
  gint height = 1;
  cairo_t *cr;
  cairo_surface_t *surface;

  base_pixbuf = gtk_icon_source_get_pixbuf (source);
  state = gtk_theming_engine_get_state (engine);

  g_return_val_if_fail (base_pixbuf != NULL, NULL);

  if (size != (GtkIconSize) -1 &&
      !lookup_icon_size (engine, size, &width, &height))
    {
      g_warning (G_STRLOC ": invalid icon size '%d'", size);
      return NULL;
    }

  /* If the size was wildcarded, and we're allowed to scale, then scale; otherwise,
   * leave it alone.
   */
  if (size != (GtkIconSize) -1 &&
      gtk_icon_source_get_size_wildcarded (source))
    scaled = scale_or_ref (base_pixbuf, width, height);
  else
    scaled = g_object_ref (base_pixbuf);

  /* If the state was wildcarded, then generate a state. */
  if (gtk_icon_source_get_state_wildcarded (source))
    {
      if (state & GTK_STATE_FLAG_INSENSITIVE)
        {
	  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						gdk_pixbuf_get_width (scaled),
						gdk_pixbuf_get_height (scaled));
	  cr = cairo_create (surface);
	  gdk_cairo_set_source_pixbuf (cr, scaled, 0, 0);
	  cairo_paint_with_alpha (cr, 0.5);

	  cairo_destroy (cr);

	  g_object_unref (scaled);
	  stated = gdk_pixbuf_get_from_surface (surface, 0, 0,
						cairo_image_surface_get_width (surface),
						cairo_image_surface_get_height (surface));
	  cairo_surface_destroy (surface);
        }
      else if (state & GTK_STATE_FLAG_PRELIGHT)
        {
	  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						gdk_pixbuf_get_width (scaled),
						gdk_pixbuf_get_height (scaled));

	  cr = cairo_create (surface);
	  gdk_cairo_set_source_pixbuf (cr, scaled, 0, 0);
	  colorshift_source (cr, 0.10);

	  cairo_destroy (cr);

	  g_object_unref (scaled);
	  stated = gdk_pixbuf_get_from_surface (surface, 0, 0,
						cairo_image_surface_get_width (surface),
						cairo_image_surface_get_height (surface));
	  cairo_surface_destroy (surface);
        }
      else
        stated = scaled;
    }
  else
    stated = scaled;

  return stated;
}

static void
gtk_theming_engine_render_icon (GtkThemingEngine *engine,
                                cairo_t *cr,
				GdkPixbuf *pixbuf,
                                gdouble x,
                                gdouble y)
{
  GtkStateFlags state;
  GtkShadow *icon_shadow;

  state = gtk_theming_engine_get_state (engine);

  cairo_save (cr);

  gdk_cairo_set_source_pixbuf (cr, pixbuf, x, y);

  gtk_theming_engine_get (engine, state,
                          "icon-shadow", &icon_shadow,
                          NULL);

  if (icon_shadow != NULL)
    {
      _gtk_icon_shadow_paint (icon_shadow, cr);
      _gtk_shadow_unref (icon_shadow);
    }

  cairo_paint (cr);

  cairo_restore (cr);
}

