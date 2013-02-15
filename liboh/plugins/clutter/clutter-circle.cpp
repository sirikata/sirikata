#include <clutter/clutter.h>
#include "clutter-circle.hpp"
#include <cmath>

#ifndef CLUTTER_PARAM_READWRITE
#define CLUTTER_PARAM_READWRITE \
    (G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |G_PARAM_STATIC_BLURB)
#endif


G_DEFINE_TYPE (ClutterCircle, clutter_circle, CLUTTER_TYPE_ACTOR);

enum
{
  PROP_0,
  PROP_COLOR,
  PROP_BORDER_COLOR,
  PROP_ANGLE_START,
  PROP_ANGLE_STOP,
  PROP_BORDER_WIDTH,
  PROP_RADIUS,
};

#define CLUTTER_CIRCLE_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), CLUTTER_TYPE_CIRCLE, ClutterCirclePrivate))

struct _ClutterCirclePrivate
{
  ClutterColor color;
    ClutterColor border_color;
  CoglFixed angle_start;
  CoglFixed angle_stop;
  CoglFixed border_width;
  CoglFixed radius;
};

static void
cc_cogl_path_arc (CoglFixed center_x,
    gfloat center_y,
    gfloat radius_x,
    gfloat radius_y,
		guint angle_1,
		guint angle_2,
		guint angle_step,
		guint        move_first)
{
    guint a     = 0x0;
        gfloat cosa  = 0;
	gfloat sina  = 0x0;
	gfloat px    = 0x0;
	gfloat py    = 0x0;

	/* Fix invalid angles */

	if (angle_1 == angle_2 || angle_step == 0x0)
		return;

	if (angle_step < 0x0)
		angle_step = -angle_step;

	/* Walk the arc by given step */

	a = angle_1;
	while (a != angle_2)
	{
		cosa = cos (a*3.14598/180);
		sina = sin (a*3.14598/180);

		px = center_x + cosa * radius_x;
		py = center_y + sina * radius_y;

		if (a == angle_1 && move_first)
			cogl_path_move_to (px, py);
		else
			cogl_path_line_to (px, py);

		if (G_LIKELY (angle_2 > angle_1))
		{
			a += angle_step;
			if (a > angle_2)
				a = angle_2;
		}
		else
		{
			a -= angle_step;
			if (a < angle_2)
				a = angle_2;
		}
	}

	/* Make sure the final point is drawn */

	cosa = cos (angle_2*3.14598/180);
	sina = sin (angle_2*3.14598/180);

	px = center_x + cosa * radius_x;
	py = center_y + sina * radius_y;

	cogl_path_line_to (px, py);
}

static void
clutter_circle_paint (ClutterActor *self)
{
	ClutterCircle				*circle = CLUTTER_CIRCLE(self);
	ClutterCirclePrivate		*priv;
	ClutterGeometry				geom;
	CoglColor				tmp_col;
	guint				precision = 2;
        guint8 tmp_alpha;
        ClutterActorBox allocation = { 0, };

	circle = CLUTTER_CIRCLE(self);
	priv = circle->priv;

	clutter_actor_get_allocation_geometry (self, &geom);

        /* compute the composited opacity of the actor taking into
       * account the opacity of the color set by the user
       */
      tmp_alpha = clutter_actor_get_paint_opacity (self)
                * priv->color.alpha
                / 255;

      /* paint the border */
      cogl_set_source_color4ub (priv->color.red,
                                priv->color.green,
                                priv->color.blue,
                                tmp_alpha);

	if ( priv->radius == 0 )
		clutter_circle_set_radius(circle, geom.width);

        gfloat width, height;
        clutter_actor_get_allocation_box (self, &allocation);
        clutter_actor_box_get_size (&allocation, &width, &height);


        // The fill
	cogl_path_move_to(width/2, height/2);
	cc_cogl_path_arc(width/2, height/2,
            priv->radius,
		priv->radius,
		priv->angle_start + 270,
		priv->angle_stop + 270,
		precision, 1
	);
	cogl_path_close();
	cogl_path_fill();



        // The border
	if ( priv->border_width != 0 )
	{

            tmp_alpha = clutter_actor_get_paint_opacity (self)
                * priv->border_color.alpha
                / 255;
            cogl_set_source_color4ub (priv->border_color.red,
                priv->border_color.green,
                priv->border_color.blue,
                tmp_alpha);
            cc_cogl_path_arc(width/2, height/2,
                priv->radius + priv->border_width/2,
                priv->radius + priv->border_width/2,
                priv->angle_stop + 270,
                priv->angle_start + 270,
                precision, 1
            );
            glEnable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
            glLineWidth(priv->border_width);

            cogl_path_close();
            cogl_path_stroke();

            glDisable(GL_LINE_SMOOTH);
	}
}

void
clutter_circle_set_angle_start (ClutterCircle *circle,
                                guint          angle)
{
	ClutterCirclePrivate *priv;

	g_return_if_fail (CLUTTER_IS_CIRCLE (circle));
	priv = circle->priv;

	if (priv->angle_start != angle)
	{
		g_object_ref (circle);

		priv->angle_start = angle;

		if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (circle)))
			clutter_actor_queue_redraw (CLUTTER_ACTOR (circle));

		g_object_notify (G_OBJECT (circle), "angle-start");
		g_object_unref (circle);
	}
}

void
clutter_circle_set_angle_stop (ClutterCircle *circle,
                                guint          angle)
{
	ClutterCirclePrivate *priv;

	g_return_if_fail (CLUTTER_IS_CIRCLE (circle));
	priv = circle->priv;

	if (priv->angle_stop != angle)
	{
		g_object_ref (circle);

		priv->angle_stop = angle;

		if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (circle)))
			clutter_actor_queue_redraw (CLUTTER_ACTOR (circle));

		g_object_notify (G_OBJECT (circle), "angle-stop");
		g_object_unref (circle);
	}
}

void
clutter_circle_set_border_width (ClutterCircle *circle,
                                guint          width)
{
	ClutterCirclePrivate *priv;

	g_return_if_fail (CLUTTER_IS_CIRCLE (circle));
	priv = circle->priv;

	if (priv->border_width != width)
	{
		g_object_ref (circle);

		priv->border_width = width;

		if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (circle)))
			clutter_actor_queue_redraw (CLUTTER_ACTOR (circle));

		g_object_notify (G_OBJECT (circle), "width");
		g_object_unref (circle);
	}
}

void
clutter_circle_set_radius (ClutterCircle *circle,
                                guint          radius)
{
	ClutterCirclePrivate *priv;

	g_return_if_fail (CLUTTER_IS_CIRCLE (circle));
	priv = circle->priv;

	if (priv->radius != radius)
	{
		g_object_ref (circle);

		priv->radius = radius;

		if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (circle)))
			clutter_actor_queue_redraw (CLUTTER_ACTOR (circle));

		g_object_notify (G_OBJECT (circle), "radius");
		g_object_unref (circle);
	}
}

static void
clutter_circle_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
  ClutterCircle *circle = CLUTTER_CIRCLE(object);

  switch (prop_id)
    {
    case PROP_COLOR:
      clutter_circle_set_color (circle, (ClutterColor*)g_value_get_boxed (value));
      break;
    case PROP_BORDER_COLOR:
      clutter_circle_set_border_color (circle, (ClutterColor*)g_value_get_boxed (value));
      break;
	case PROP_ANGLE_START:
	  clutter_circle_set_angle_start (circle, g_value_get_uint (value));
	  break;
	case PROP_ANGLE_STOP:
	  clutter_circle_set_angle_stop (circle, g_value_get_uint (value));
	  break;
	case PROP_BORDER_WIDTH:
	  clutter_circle_set_border_width (circle, g_value_get_uint (value));
	  break;
	case PROP_RADIUS:
	  clutter_circle_set_radius (circle, g_value_get_uint (value));
	  break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
clutter_circle_get_property (GObject    *object,
				guint       prop_id,
				GValue     *value,
				GParamSpec *pspec)
{
  ClutterCircle *circle = CLUTTER_CIRCLE(object);
  ClutterColor      color;

  switch (prop_id)
    {
    case PROP_COLOR:
      clutter_circle_get_color (circle, &color);
      g_value_set_boxed (value, &color);
      break;
    case PROP_BORDER_COLOR:
      clutter_circle_get_border_color (circle, &color);
      g_value_set_boxed (value, &color);
      break;
    case PROP_ANGLE_START:
      g_value_set_uint (value, circle->priv->angle_start);
	  break;
    case PROP_ANGLE_STOP:
      g_value_set_uint (value, circle->priv->angle_stop);
	  break;
    case PROP_BORDER_WIDTH:
      g_value_set_uint (value, circle->priv->border_width);
	  break;
    case PROP_RADIUS:
      g_value_set_uint (value, circle->priv->radius);
	  break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
clutter_circle_finalize (GObject *object)
{
  G_OBJECT_CLASS (clutter_circle_parent_class)->finalize (object);
}

static void
clutter_circle_dispose (GObject *object)
{
  G_OBJECT_CLASS (clutter_circle_parent_class)->dispose (object);
}


static void
clutter_circle_class_init (ClutterCircleClass *klass)
{
  GObjectClass        *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  actor_class->paint        = clutter_circle_paint;

  gobject_class->finalize     = clutter_circle_finalize;
  gobject_class->dispose      = clutter_circle_dispose;
  gobject_class->set_property = clutter_circle_set_property;
  gobject_class->get_property = clutter_circle_get_property;

  /**
   * ClutterCircle:color:
   *
   * The color of the circle.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR,
                                   g_param_spec_boxed ("color",
                                                       "Color",
                                                       "The color of the circle",
                                                       CLUTTER_TYPE_COLOR,
                                       (GParamFlags)CLUTTER_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_BORDER_COLOR,
                                   g_param_spec_boxed ("border-color",
                                                       "border color",
                                                       "The color of the circle's border",
                                                       CLUTTER_TYPE_COLOR,
                                       (GParamFlags)CLUTTER_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_ANGLE_START,
                                   g_param_spec_uint ("angle-start",
                                                       "Start Angle",
                                                       "The start of angle",
													   0, G_MAXUINT,
													   0,
                                       (GParamFlags)CLUTTER_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_ANGLE_STOP,
                                   g_param_spec_uint ("angle-stop",
                                                       "End Angle",
                                                       "The end of angle",
													   0, G_MAXUINT,
													   0,
                                       (GParamFlags)CLUTTER_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_BORDER_WIDTH,
                                   g_param_spec_uint ("border-width",
                                                       "Border Width",
                                                       "Border Width",
													   0, G_MAXUINT,
													   0,
                                       (GParamFlags)CLUTTER_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_RADIUS,
                                   g_param_spec_uint ("radius",
                                                       "Radius",
                                                       "Radius",
													   0, G_MAXUINT,
													   0,
                                       (GParamFlags)CLUTTER_PARAM_READWRITE));
  g_type_class_add_private (gobject_class, sizeof (ClutterCirclePrivate));
}

static void
clutter_circle_init (ClutterCircle *self)
{
  ClutterCirclePrivate *priv;

  self->priv = priv = CLUTTER_CIRCLE_GET_PRIVATE (self);

  priv->color.red = 0xff;
  priv->color.green = 0xff;
  priv->color.blue = 0xff;
  priv->color.alpha = 0xff;

  priv->border_color.red = 0xff;
  priv->border_color.green = 0xff;
  priv->border_color.blue = 0xff;
  priv->border_color.alpha = 0xff;

  priv->angle_start = 0;
  priv->angle_stop = 0;
  priv->border_width = 0;
  priv->radius = 0;
}

/**
 * clutter_circle_new:
 *
 * Creates a new #ClutterActor with a rectangular shape.
 *
 * Return value: a new #ClutterActor
 */
ClutterActor*
clutter_circle_new (void)
{
    return (ClutterActor*)g_object_new (CLUTTER_TYPE_CIRCLE, NULL);
}

/**
 * clutter_circle_new_with_color:
 * @color: a #ClutterColor
 *
 * Creates a new #ClutterActor with a rectangular shape
 * and of the given @color.
 *
 * Return value: a new #ClutterActor
 */
ClutterActor *
clutter_circle_new_with_color (const ClutterColor *color)
{
  return (ClutterActor*)g_object_new (CLUTTER_TYPE_CIRCLE,
		       "color", color,
		       NULL);
}

/**
 * clutter_circle_get_color:
 * @circle: a #ClutterCircle
 * @color: return location for a #ClutterColor
 *
 * Retrieves the color of @circle.
 */
void
clutter_circle_get_color (ClutterCircle *circle,
			     ClutterColor     *color)
{
  ClutterCirclePrivate *priv;

  g_return_if_fail (CLUTTER_IS_CIRCLE (circle));
  g_return_if_fail (color != NULL);

  priv = circle->priv;

  color->red = priv->color.red;
  color->green = priv->color.green;
  color->blue = priv->color.blue;
  color->alpha = priv->color.alpha;
}

/**
 * clutter_circle_set_color:
 * @circle: a #ClutterCircle
 * @color: a #ClutterColor
 *
 * Sets the color of @circle.
 */
void
clutter_circle_set_color (ClutterCircle   *circle,
			     const ClutterColor *color)
{
  ClutterCirclePrivate *priv;

  g_return_if_fail (CLUTTER_IS_CIRCLE (circle));
  g_return_if_fail (color != NULL);

  g_object_ref (circle);

  priv = circle->priv;

  priv->color.red = color->red;
  priv->color.green = color->green;
  priv->color.blue = color->blue;
  priv->color.alpha = color->alpha;

  if (CLUTTER_ACTOR_IS_VISIBLE (circle))
    clutter_actor_queue_redraw (CLUTTER_ACTOR (circle));

  g_object_notify (G_OBJECT (circle), "color");
  g_object_unref (circle);
}



/**
 * clutter_circle_get_border_color:
 * @circle: a #ClutterCircle
 * @color: return location for a #ClutterColor
 *
 * Retrieves the border color of @circle.
 */
void
clutter_circle_get_border_color (ClutterCircle *circle,
			     ClutterColor     *color)
{
  ClutterCirclePrivate *priv;

  g_return_if_fail (CLUTTER_IS_CIRCLE (circle));
  g_return_if_fail (color != NULL);

  priv = circle->priv;

  color->red = priv->border_color.red;
  color->green = priv->border_color.green;
  color->blue = priv->border_color.blue;
  color->alpha = priv->border_color.alpha;
}

/**
 * clutter_circle_set_border_color:
 * @circle: a #ClutterCircle
 * @color: a #ClutterColor
 *
 * Sets the border color of @circle.
 */
void
clutter_circle_set_border_color (ClutterCircle   *circle,
			     const ClutterColor *color)
{
  ClutterCirclePrivate *priv;

  g_return_if_fail (CLUTTER_IS_CIRCLE (circle));
  g_return_if_fail (color != NULL);

  g_object_ref (circle);

  priv = circle->priv;

  priv->border_color.red = color->red;
  priv->border_color.green = color->green;
  priv->border_color.blue = color->blue;
  priv->border_color.alpha = color->alpha;

  if (CLUTTER_ACTOR_IS_VISIBLE (circle))
    clutter_actor_queue_redraw (CLUTTER_ACTOR (circle));

  g_object_notify (G_OBJECT (circle), "color");
  g_object_unref (circle);
}
