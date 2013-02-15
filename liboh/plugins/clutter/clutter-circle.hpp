#ifndef _HAVE_CLUTTER_CIRCLE_H
#define _HAVE_CLUTTER_CIRCLE_H

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define CLUTTER_TYPE_CIRCLE clutter_circle_get_type()

#define CLUTTER_CIRCLE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CLUTTER_TYPE_CIRCLE, ClutterCircle))

#define CLUTTER_CIRCLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CLUTTER_TYPE_CIRCLE, ClutterCircleClass))

#define CLUTTER_IS_CIRCLE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CLUTTER_TYPE_CIRCLE))

#define CLUTTER_IS_CIRCLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CLUTTER_TYPE_CIRCLE))

#define CLUTTER_CIRCLE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CLUTTER_TYPE_CIRCLE, ClutterCircleClass))

typedef struct _ClutterCircle        ClutterCircle;
typedef struct _ClutterCircleClass   ClutterCircleClass;
typedef struct _ClutterCirclePrivate ClutterCirclePrivate;

struct _ClutterCircle
{
  ClutterActor           parent;

  /*< private >*/
  ClutterCirclePrivate *priv;
};

struct _ClutterCircleClass
{
  ClutterActorClass parent_class;

  /* padding for future expansion */
  void (*_clutter_circle1) (void);
  void (*_clutter_circle2) (void);
  void (*_clutter_circle3) (void);
  void (*_clutter_circle4) (void);
};

GType clutter_circle_get_type (void) G_GNUC_CONST;

ClutterActor *clutter_circle_new              (void);
ClutterActor *clutter_circle_new_with_color   (const ClutterColor *color);

void          clutter_circle_get_color        (ClutterCircle   *circle,
                                                  ClutterColor       *color);
void          clutter_circle_set_color        (ClutterCircle   *circle,
						  const ClutterColor *color);

void          clutter_circle_get_border_color        (ClutterCircle   *circle,
                                                  ClutterColor       *color);
void          clutter_circle_set_border_color        (ClutterCircle   *circle,
						  const ClutterColor *color);

void clutter_circle_set_angle_start (ClutterCircle *circle,
                                guint          angle);
void clutter_circle_set_angle_stop (ClutterCircle *circle,
                                guint          angle);
void clutter_circle_set_border_width (ClutterCircle *circle,
                                guint          width);
void clutter_circle_set_radius (ClutterCircle *circle,
                                guint          radius);

G_END_DECLS

#endif
