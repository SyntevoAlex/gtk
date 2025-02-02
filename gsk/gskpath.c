/*
 * Copyright © 2020 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "gskpathprivate.h"

#include "gskcurveprivate.h"
#include "gskpathbuilder.h"
#include "gskpathpointprivate.h"
#include "gsksplineprivate.h"

/**
 * GskPath:
 *
 * A `GskPath` describes lines and curves that are more complex
 * than simple rectangles.
 *
 * Paths can used for rendering (filling or stroking) and for animations
 * (e.g. as trajectories).
 *
 * `GskPath` is an immutable, opaque, reference-counted struct.
 * After creation, you cannot change the types it represents. Instead,
 * new `GskPath` objects have to be created. The [struct@Gsk.PathBuilder]
 * structure is meant to help in this endeavor.
 *
 * Conceptually, a path consists of zero or more contours (continous, connected
 * curves), each of which may or may not be closed. Contours are typically
 * constructed from Bézier segments.
 *
 * <picture>
 *   <source srcset="path-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="A Path" src="path-light.png">
 * </picture>
 *
 * Since: 4.14
 */

struct _GskPath
{
  /*< private >*/
  guint ref_count;

  GskPathFlags flags;

  gsize n_contours;
  GskContour *contours[];
  /* followed by the contours data */
};

G_DEFINE_BOXED_TYPE (GskPath, gsk_path, gsk_path_ref, gsk_path_unref)

GskPath *
gsk_path_new_from_contours (const GSList *contours)
{
  GskPath *path;
  const GSList *l;
  gsize size;
  gsize n_contours;
  guint8 *contour_data;
  GskPathFlags flags;

  flags = GSK_PATH_CLOSED | GSK_PATH_FLAT;
  size = 0;
  n_contours = 0;
  for (l = contours; l; l = l->next)
    {
      GskContour *contour = l->data;

      n_contours++;
      size += sizeof (GskContour *);
      size += gsk_contour_get_size (contour);
      flags &= gsk_contour_get_flags (contour);
    }

  path = g_malloc0 (sizeof (GskPath) + size);
  path->ref_count = 1;
  path->flags = flags;
  path->n_contours = n_contours;
  contour_data = (guint8 *) &path->contours[n_contours];
  n_contours = 0;

  for (l = contours; l; l = l->next)
    {
      GskContour *contour = l->data;

      path->contours[n_contours] = (GskContour *) contour_data;
      gsk_contour_copy ((GskContour *) contour_data, contour);
      size = gsk_contour_get_size (contour);
      contour_data += size;
      n_contours++;
    }

  return path;
}

/**
 * gsk_path_ref:
 * @self: a `GskPath`
 *
 * Increases the reference count of a `GskPath` by one.
 *
 * Returns: the passed in `GskPath`.
 *
 * Since: 4.14
 */
GskPath *
gsk_path_ref (GskPath *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  self->ref_count++;

  return self;
}

/**
 * gsk_path_unref:
 * @self: a `GskPath`
 *
 * Decreases the reference count of a `GskPath` by one.
 *
 * If the resulting reference count is zero, frees the path.
 *
 * Since: 4.14
 */
void
gsk_path_unref (GskPath *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ref_count > 0);

  self->ref_count--;
  if (self->ref_count > 0)
    return;

  g_free (self);
}

const GskContour *
gsk_path_get_contour (const GskPath *self,
                      gsize          i)
{
  if (i < self->n_contours)
    return self->contours[i];
  else
    return NULL;
}

GskPathFlags
gsk_path_get_flags (const GskPath *self)
{
  return self->flags;
}

/**
 * gsk_path_print:
 * @self: a `GskPath`
 * @string:  The string to print into
 *
 * Converts @self into a human-readable string representation suitable
 * for printing.
 *
 * The string is compatible with
 * [SVG path syntax](https://www.w3.org/TR/SVG11/paths.html#PathData),
 * see [func@Gsk.Path.parse] for a summary of the syntax.
 *
 * Since: 4.14
 */
void
gsk_path_print (GskPath *self,
                GString *string)
{
  gsize i;

  g_return_if_fail (self != NULL);
  g_return_if_fail (string != NULL);

  for (i = 0; i < self->n_contours; i++)
    {
      if (i > 0)
        g_string_append_c (string, ' ');
      gsk_contour_print (self->contours[i], string);
    }
}

/**
 * gsk_path_to_string:
 * @self: a `GskPath`
 *
 * Converts the path into a string that is suitable for printing.
 *
 * You can use this function in a debugger to get a quick overview
 * of the path.
 *
 * This is a wrapper around [method@Gsk.Path.print], see that function
 * for details.
 *
 * Returns: A new string for @self
 *
 * Since: 4.14
 */
char *
gsk_path_to_string (GskPath *self)
{
  GString *string;

  g_return_val_if_fail (self != NULL, NULL);

  string = g_string_new ("");

  gsk_path_print (self, string);

  return g_string_free (string, FALSE);
}

static gboolean
gsk_path_to_cairo_add_op (GskPathOperation        op,
                          const graphene_point_t *pts,
                          gsize                   n_pts,
                          gpointer                cr)
{
  switch (op)
  {
    case GSK_PATH_MOVE:
      cairo_move_to (cr, pts[0].x, pts[0].y);
      break;

    case GSK_PATH_CLOSE:
      cairo_close_path (cr);
      break;

    case GSK_PATH_LINE:
      cairo_line_to (cr, pts[1].x, pts[1].y);
      break;

    case GSK_PATH_CUBIC:
      cairo_curve_to (cr, pts[1].x, pts[1].y, pts[2].x, pts[2].y, pts[3].x, pts[3].y);
      break;

    case GSK_PATH_QUAD:
    case GSK_PATH_ARC:
    default:
      g_assert_not_reached ();
      return FALSE;
  }

  return TRUE;
}

/**
 * gsk_path_to_cairo:
 * @self: a `GskPath`
 * @cr: a cairo context
 *
 * Appends the given @path to the given cairo context for drawing
 * with Cairo.
 *
 * This may cause some suboptimal conversions to be performed as
 * Cairo does not support all features of `GskPath`.
 *
 * This function does not clear the existing Cairo path. Call
 * cairo_new_path() if you want this.
 *
 * Since: 4.14
 */
void
gsk_path_to_cairo (GskPath *self,
                   cairo_t *cr)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (cr != NULL);

  gsk_path_foreach_with_tolerance (self,
                                   GSK_PATH_FOREACH_ALLOW_CUBIC,
                                   cairo_get_tolerance (cr),
                                   gsk_path_to_cairo_add_op,
                                   cr);
}

/*< private >
 * gsk_path_get_n_contours:
 * @path: a `GskPath`
 *
 * Gets the number of contours @path is composed out of.
 *
 * Returns: the number of contours in @path
 */
gsize
gsk_path_get_n_contours (const GskPath *self)
{
  return self->n_contours;
}

/**
 * gsk_path_is_empty:
 * @self: a `GskPath`
 *
 * Checks if the path is empty, i.e. contains no lines or curves.
 *
 * Returns: `TRUE` if the path is empty
 *
 * Since: 4.14
 */
gboolean
gsk_path_is_empty (GskPath *self)
{
  g_return_val_if_fail (self != NULL, FALSE);

  return self->n_contours == 0;
}

/**
 * gsk_path_is_closed:
 * @self: a `GskPath`
 *
 * Returns if the path represents a single closed
 * contour.
 *
 * Returns: `TRUE` if the path is closed
 *
 * Since: 4.14
 */
gboolean
gsk_path_is_closed (GskPath *self)
{
  g_return_val_if_fail (self != NULL, FALSE);

  /* XXX: is the empty path closed? Currently it's not */
  if (self->n_contours != 1)
    return FALSE;

  return gsk_contour_get_flags (self->contours[0]) & GSK_PATH_CLOSED ? TRUE : FALSE;
}

/**
 * gsk_path_get_bounds:
 * @self: a `GskPath`
 * @bounds: (out caller-allocates): the bounds of the given path
 *
 * Computes the bounds of the given path.
 *
 * The returned bounds may be larger than necessary, because this
 * function aims to be fast, not accurate. The bounds are guaranteed
 * to contain the path.
 *
 * It is possible that the returned rectangle has 0 width and/or height.
 * This can happen when the path only describes a point or an
 * axis-aligned line.
 *
 * If the path is empty, `FALSE` is returned and @bounds are set to
 * graphene_rect_zero(). This is different from the case where the path
 * is a single point at the origin, where the @bounds will also be set to
 * the zero rectangle but 0 will be returned.
 *
 * Returns: `TRUE` if the path has bounds, `FALSE` if the path is known
 *   to be empty and have no bounds.
 *
 * Since: 4.14
 */
gboolean
gsk_path_get_bounds (GskPath         *self,
                     graphene_rect_t *bounds)
{
  GskBoundingBox b;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (bounds != NULL, FALSE);

  if (self->n_contours == 0)
    {
      graphene_rect_init_from_rect (bounds, graphene_rect_zero ());
      return FALSE;
    }

  gsk_contour_get_bounds (self->contours[0], &b);

  for (gsize i = 1; i < self->n_contours; i++)
    {
      GskBoundingBox tmp;

      gsk_contour_get_bounds (self->contours[i], &tmp);
      gsk_bounding_box_union (&b, &tmp, &b);
    }

  gsk_bounding_box_to_rect (&b, bounds);

  return TRUE;
}

/**
 * gsk_path_get_stroke_bounds:
 * @self: a #GtkPath
 * @stroke: stroke parameters
 * @bounds: (out caller-allocates): the bounds to fill in
 *
 * Computes the bounds for stroking the given path with the
 * parameters in @stroke.
 *
 * The returned bounds may be larger than necessary, because this
 * function aims to be fast, not accurate. The bounds are guaranteed
 * to contain the area affected by the stroke, including protrusions
 * like miters.
 *
 * Returns: `TRUE` if the path has bounds, `FALSE` if the path is known
 *   to be empty and have no bounds.
 * Since: 4.14
 */
gboolean
gsk_path_get_stroke_bounds (GskPath         *self,
                            const GskStroke *stroke,
                            graphene_rect_t *bounds)
{
  GskBoundingBox b;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (bounds != NULL, FALSE);

  if (self->n_contours == 0)
    {
      graphene_rect_init_from_rect (bounds, graphene_rect_zero ());
      return FALSE;
    }

  gsk_contour_get_stroke_bounds (self->contours[0], stroke, &b);

  for (gsize i = 1; i < self->n_contours; i++)
    {
      GskBoundingBox tmp;

      if (gsk_contour_get_stroke_bounds (self->contours[i], stroke, &tmp))
        gsk_bounding_box_union (&b, &tmp, &b);
    }

  gsk_bounding_box_to_rect (&b, bounds);

  return TRUE;
}

/**
 * gsk_path_in_fill:
 * @self: a `GskPath`
 * @point: the point to test
 * @fill_rule: the fill rule to follow
 *
 * Returns whether the given point is inside the area
 * that would be affected if the path was filled according
 * to @fill_rule.
 *
 * Note that this function assumes that filling a contour
 * implicitly closes it.
 *
 * Returns: `TRUE` if @point is inside
 *
 * Since: 4.14
 */
gboolean
gsk_path_in_fill (GskPath                *self,
                  const graphene_point_t *point,
                  GskFillRule             fill_rule)
{
  int winding = 0;

  for (int i = 0; i < self->n_contours; i++)
    winding += gsk_contour_get_winding (self->contours[i], point);

  switch (fill_rule)
    {
    case GSK_FILL_RULE_EVEN_ODD:
      return winding & 1;
    case GSK_FILL_RULE_WINDING:
      return winding != 0;
    default:
      g_assert_not_reached ();
    }
}

/**
 * gsk_path_get_start_point:
 * @self: a `GskPath`
 * @result: (out caller-allocates): return location for point
 *
 * Gets the start point of the path.
 *
 * An empty path has no points, so `FALSE`
 * is returned in this case.
 *
 * Returns: `TRUE` if @result was filled
 *
 * Since: 4.14
 */
gboolean
gsk_path_get_start_point (GskPath      *self,
                          GskPathPoint *result)
{
  GskRealPathPoint *res = (GskRealPathPoint *) result;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (result != NULL, FALSE);

  if (self->n_contours == 0)
    return FALSE;

  res->contour = 0;
  res->idx = 1;
  res->t = 0;

  return TRUE;
}

/**
 * gsk_path_get_end_point:
 * @self: a `GskPath`
 * @result: (out caller-allocates): return location for point
 *
 * Gets the end point of the path.
 *
 * An empty path has no points, so `FALSE`
 * is returned in this case.
 *
 * Returns: `TRUE` if @result was filled
 *
 * Since: 4.14
 */
gboolean
gsk_path_get_end_point (GskPath      *self,
                        GskPathPoint *result)
{
  GskRealPathPoint *res = (GskRealPathPoint *) result;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (result != NULL, FALSE);

  if (self->n_contours == 0)
    return FALSE;

  res->contour = self->n_contours - 1;
  res->idx = gsk_contour_get_n_ops (self->contours[self->n_contours - 1]) - 1;
  res->t = 1;

  return TRUE;
}

/**
 * gsk_path_get_closest_point:
 * @self: a `GskPath`
 * @point: the point
 * @threshold: maximum allowed distance
 * @result: (out caller-allocates): return location for the closest point
 *
 * Computes the closest point on the path to the given point
 * and sets the @result to it.
 *
 * If there is no point closer than the given threshold,
 * `FALSE` is returned.
 *
 * Returns: `TRUE` if @point was set to the closest point
 *   on @self, `FALSE` if no point is closer than @threshold
 *
 * Since: 4.14
 */
gboolean
gsk_path_get_closest_point (GskPath                *self,
                            const graphene_point_t *point,
                            float                   threshold,
                            GskPathPoint           *result)
{
  GskRealPathPoint *res = (GskRealPathPoint *) result;
  gboolean found;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (point != NULL, FALSE);
  g_return_val_if_fail (threshold >= 0, FALSE);
  g_return_val_if_fail (result != NULL, FALSE);

  found = FALSE;

  for (int i = 0; i < self->n_contours; i++)
    {
      float distance;

      if (gsk_contour_get_closest_point (self->contours[i], point, threshold, res, &distance))
        {
          found = TRUE;
          res->contour = i;
          threshold = distance;
        }
    }

  return found;
}

/**
 * gsk_path_foreach:
 * @self: a `GskPath`
 * @flags: flags to pass to the foreach function. See [flags@Gsk.PathForeachFlags]
 *   for details about flags
 * @func: (scope call) (closure user_data): the function to call for operations
 * @user_data: (nullable): user data passed to @func
 *
 * Calls @func for every operation of the path.
 *
 * Note that this only approximates @self, because paths can contain
 * optimizations for various specialized contours, and depending on
 * the @flags, the path may be decomposed into simpler curves than
 * the ones that it contained originally.
 *
 * Returns: `FALSE` if @func returned FALSE`, `TRUE` otherwise.
 *
 * Since: 4.14
 */
gboolean
gsk_path_foreach (GskPath             *self,
                  GskPathForeachFlags  flags,
                  GskPathForeachFunc   func,
                  gpointer             user_data)
{
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (func, FALSE);

  return gsk_path_foreach_with_tolerance (self,
                                          flags,
                                          GSK_PATH_TOLERANCE_DEFAULT,
                                          func,
                                          user_data);
}

typedef struct _GskPathForeachTrampoline GskPathForeachTrampoline;
struct _GskPathForeachTrampoline
{
  GskPathForeachFlags flags;
  double tolerance;
  GskPathForeachFunc func;
  gpointer user_data;
};

static gboolean
gsk_path_foreach_trampoline_add_line (const graphene_point_t *from,
                                      const graphene_point_t *to,
                                      float                   from_progress,
                                      float                   to_progress,
                                      GskCurveLineReason      reason,
                                      gpointer                data)
{
  GskPathForeachTrampoline *trampoline = data;

  return trampoline->func (GSK_PATH_LINE,
                           (graphene_point_t[2]) { *from, *to },
                           2,
                           trampoline->user_data);
}

static gboolean
gsk_path_foreach_trampoline_add_curve (GskPathOperation        op,
                                       const graphene_point_t *pts,
                                       gsize                   n_pts,
                                       gpointer                data)
{
  GskPathForeachTrampoline *trampoline = data;

  return trampoline->func (op, pts, n_pts, trampoline->user_data);
}

static gboolean
gsk_path_foreach_trampoline (GskPathOperation        op,
                             const graphene_point_t *pts,
                             gsize                   n_pts,
                             gpointer                data)
{
  GskPathForeachTrampoline *trampoline = data;

  switch (op)
    {
    case GSK_PATH_MOVE:
    case GSK_PATH_CLOSE:
    case GSK_PATH_LINE:
      return trampoline->func (op, pts, n_pts, trampoline->user_data);

    case GSK_PATH_QUAD:
      {
        GskCurve curve;

        if (trampoline->flags & GSK_PATH_FOREACH_ALLOW_QUAD)
          return trampoline->func (op, pts, n_pts, trampoline->user_data);
        else if (trampoline->flags & GSK_PATH_FOREACH_ALLOW_CUBIC)
          {
            return trampoline->func (GSK_PATH_CUBIC,
                                     (graphene_point_t[4]) {
                                       pts[0],
                                       GRAPHENE_POINT_INIT ((pts[0].x + 2 * pts[1].x) / 3,
                                                            (pts[0].y + 2 * pts[1].y) / 3),
                                       GRAPHENE_POINT_INIT ((pts[2].x + 2 * pts[1].x) / 3,
                                                            (pts[2].y + 2 * pts[1].y) / 3),
                                       pts[2],
                                     },
                                     4,
                                     trampoline->user_data);
          }

        gsk_curve_init (&curve, gsk_pathop_encode (GSK_PATH_QUAD, pts));
        return gsk_curve_decompose (&curve,
                                    trampoline->tolerance,
                                    gsk_path_foreach_trampoline_add_line,
                                    trampoline);
      }

    case GSK_PATH_CUBIC:
      {
        GskCurve curve;

        if (trampoline->flags & GSK_PATH_FOREACH_ALLOW_CUBIC)
          return trampoline->func (op, pts, n_pts, trampoline->user_data);

        gsk_curve_init (&curve, gsk_pathop_encode (GSK_PATH_CUBIC, pts));
        if (trampoline->flags & GSK_PATH_FOREACH_ALLOW_QUAD)
          return gsk_curve_decompose_curve (&curve,
                                            trampoline->flags,
                                            trampoline->tolerance,
                                            gsk_path_foreach_trampoline_add_curve,
                                            trampoline);

        return gsk_curve_decompose (&curve,
                                    trampoline->tolerance,
                                    gsk_path_foreach_trampoline_add_line,
                                    trampoline);
      }

    case GSK_PATH_ARC:
      {
        GskCurve curve;

        if (trampoline->flags & GSK_PATH_FOREACH_ALLOW_ARC)
          return trampoline->func (op, pts, n_pts, trampoline->user_data);

        gsk_curve_init (&curve, gsk_pathop_encode (GSK_PATH_ARC, pts));
        if (trampoline->flags & (GSK_PATH_FOREACH_ALLOW_CUBIC|GSK_PATH_FOREACH_ALLOW_QUAD))
          return gsk_curve_decompose_curve (&curve,
                                            trampoline->flags,
                                            trampoline->tolerance,
                                            gsk_path_foreach_trampoline_add_curve,
                                            trampoline);

        return gsk_curve_decompose (&curve,
                                    trampoline->tolerance,
                                    gsk_path_foreach_trampoline_add_line,
                                    trampoline);
      }

    default:
      g_assert_not_reached ();
      return FALSE;
    }
}

#define ALLOW_ANY (GSK_PATH_FOREACH_ALLOW_QUAD  | \
                   GSK_PATH_FOREACH_ALLOW_CUBIC | \
                   GSK_PATH_FOREACH_ALLOW_ARC)

gboolean
gsk_path_foreach_with_tolerance (GskPath             *self,
                                 GskPathForeachFlags  flags,
                                 double               tolerance,
                                 GskPathForeachFunc   func,
                                 gpointer             user_data)
{
  GskPathForeachTrampoline trampoline;
  gsize i;

  /* If we need to massage the data, set up a trampoline here */
  if ((flags & ALLOW_ANY) != ALLOW_ANY)
    {
      trampoline = (GskPathForeachTrampoline) { flags, tolerance, func, user_data };
      func = gsk_path_foreach_trampoline;
      user_data = &trampoline;
    }

  for (i = 0; i < self->n_contours; i++)
    {
      if (!gsk_contour_foreach (self->contours[i], tolerance, func, user_data))
        return FALSE;
    }

  return TRUE;
}

/* path parser and utilities */

static void
skip_whitespace (const char **p)
{
  while (g_ascii_isspace (**p))
    (*p)++;
}

static void
skip_optional_comma (const char **p)
{
  skip_whitespace (p);
  if (**p == ',')
    (*p)++;
}

static gboolean
parse_number (const char **p,
              double      *c)
{
  char *e;
  *c = g_ascii_strtod (*p, &e);
  if (e == *p)
    return FALSE;
  *p = e;
  skip_optional_comma (p);
  return TRUE;
}

static gboolean
parse_coordinate (const char **p,
                  double      *c)
{
  return parse_number (p, c);
}

static gboolean
parse_coordinate_pair (const char **p,
                       double      *x,
                       double      *y)
{
  double xx, yy;
  const char *o = *p;

  if (!parse_coordinate (p, &xx))
    {
      *p = o;
      return FALSE;
    }
  if (!parse_coordinate (p, &yy))
    {
      *p = o;
      return FALSE;
    }

  *x = xx;
  *y = yy;

  return TRUE;
}

static gboolean
parse_nonnegative_number (const char **p,
                          double      *x)
{
  const char *o = *p;
  double n;

  if (!parse_number (p, &n))
    return FALSE;

  if (n < 0)
    {
      *p = o;
      return FALSE;
    }

  *x = n;

  return TRUE;
}

/* This fixes a flaw in our use of strchr() below:
 *
 * If p already points at the end of the string,
 * we misinterpret strchr ("xyz", *p) returning
 * non-NULL to mean that we can increment p.
 *
 * But strchr() will return a pointer to the
 * final NUL byte in this case, and we walk off
 * the end of the string. Oops
 */
static inline char *
_strchr (const char *str,
           int         c)
{
  if (c == 0)
    return NULL;
  else
    return strchr (str, c);
}

static gboolean
parse_flag (const char **p,
            gboolean    *f)
{
  skip_whitespace (p);
  if (_strchr ("01", **p))
    {
      *f = **p == '1';
      (*p)++;
      skip_optional_comma (p);
      return TRUE;
    }

  return FALSE;
}

static gboolean
parse_command (const char **p,
               char        *cmd)
{
  char *s;
  const char *allowed;

  if (*cmd == 'X')
    allowed = "mM";
  else
    allowed = "mMhHvVzZlLcCsStTqQaAeE";

  skip_whitespace (p);
  s = _strchr (allowed, **p);
  if (s)
    {
      *cmd = *s;
      (*p)++;
      return TRUE;
    }
  return FALSE;
}

/**
 * gsk_path_parse:
 * @string: a string
 *
 * This is a convenience function that constructs a `GskPath`
 * from a serialized form.
 *
 * The string is expected to be in (a superset of)
 * [SVG path syntax](https://www.w3.org/TR/SVG11/paths.html#PathData),
 * as e.g. produced by [method@Gsk.Path.to_string].
 *
 * A high-level summary of the syntax:
 *
 * - `M x y` Move to `(x, y)`
 * - `L x y` Add a line from the current point to `(x, y)`
 * - `Q x1 y1 x2 y2` Add a quadratic Bézier from the current point to `(x2, y2)`, with control point `(x1, y1)`
 * - `C x1 y1 x2 y2 x3 y3` Add a cubic Bézier from the current point to `(x3, y3)`, with control points `(x1, y1)` and `(x2, y2)`
 * - `Z` Close the contour by drawing a line back to the start point
 * - `H x` Add a horizontal line from the current point to the given x value
 * - `V y` Add a vertical line from the current point to the given y value
 * - `T x2 y2` Add a quadratic Bézier, using the reflection of the previous segments' control point as control point
 * - `S x2 y2 x3 y3` Add a cubic Bézier, using the reflection of the previous segments' second control point as first control point
 * - `A rx ry r l s x y` Add an elliptical arc from the current point to `(x, y)` with radii rx and ry. See the SVG documentation for how the other parameters influence the arc.
 * - `E x1 y1 x2 y2` Add an elliptical arc from the current point to `(x2, y2)` with tangents that are dermined by the point `(x1, y1)`.
 *
 * All the commands have lowercase variants that interpret coordinates
 * relative to the current point.
 *
 * The `E` command is an extension that is not supported in SVG.
 *
 * Returns: (nullable): a new `GskPath`, or `NULL`
 *   if @string could not be parsed
 *
 * Since: 4.14
 */
GskPath *
gsk_path_parse (const char *string)
{
  GskPathBuilder *builder;
  double x, y;
  double prev_x1, prev_y1;
  double path_x, path_y;
  const char *p;
  char cmd;
  char prev_cmd;
  gboolean after_comma;
  gboolean repeat;

  builder = gsk_path_builder_new ();

  cmd = 'X';
  path_x = path_y = 0;
  x = y = 0;
  prev_x1 = prev_y1 = 0;
  after_comma = FALSE;

  p = string;
  while (*p)
    {
      prev_cmd = cmd;
      repeat = !parse_command (&p, &cmd);

      if (after_comma && !repeat)
        goto error;

      switch (cmd)
        {
        case 'X':
          goto error;

        case 'Z':
        case 'z':
          if (repeat)
            goto error;
          else
            {
              gsk_path_builder_close (builder);
              x = path_x;
              y = path_y;
            }
          break;

        case 'M':
        case 'm':
          {
            double x1, y1;

            if (parse_coordinate_pair (&p, &x1, &y1))
              {
                if (cmd == 'm')
                  {
                    x1 += x;
                    y1 += y;
                  }
                if (repeat)
                  gsk_path_builder_line_to (builder, x1, y1);
                else
                  {
                    gsk_path_builder_move_to (builder, x1, y1);
                    if (_strchr ("zZX", prev_cmd))
                      {
                        path_x = x1;
                        path_y = y1;
                      }
                  }
                x = x1;
                y = y1;
              }
            else
              goto error;
          }
          break;

        case 'L':
        case 'l':
          {
            double x1, y1;

            if (parse_coordinate_pair (&p, &x1, &y1))
              {
                if (cmd == 'l')
                  {
                    x1 += x;
                    y1 += y;
                  }

                if (_strchr ("zZ", prev_cmd))
                  {
                    gsk_path_builder_move_to (builder, x, y);
                    path_x = x;
                    path_y = y;
                  }
                gsk_path_builder_line_to (builder, x1, y1);
                x = x1;
                y = y1;
              }
            else
              goto error;
          }
          break;

        case 'H':
        case 'h':
          {
            double x1;

            if (parse_coordinate (&p, &x1))
              {
                if (cmd == 'h')
                  x1 += x;
                if (_strchr ("zZ", prev_cmd))
                  {
                    gsk_path_builder_move_to (builder, x, y);
                    path_x = x;
                    path_y = y;
                  }
                gsk_path_builder_line_to (builder, x1, y);
                x = x1;
              }
            else
              goto error;
          }
          break;

        case 'V':
        case 'v':
          {
            double y1;

            if (parse_coordinate (&p, &y1))
              {
                if (cmd == 'v')
                  y1 += y;
                if (_strchr ("zZ", prev_cmd))
                  {
                    gsk_path_builder_move_to (builder, x, y);
                    path_x = x;
                    path_y = y;
                  }
                gsk_path_builder_line_to (builder, x, y1);
                y = y1;
              }
            else
              goto error;
          }
          break;

        case 'C':
        case 'c':
          {
            double x0, y0, x1, y1, x2, y2;

            if (parse_coordinate_pair (&p, &x0, &y0) &&
                parse_coordinate_pair (&p, &x1, &y1) &&
                parse_coordinate_pair (&p, &x2, &y2))
              {
                if (cmd == 'c')
                  {
                    x0 += x;
                    y0 += y;
                    x1 += x;
                    y1 += y;
                    x2 += x;
                    y2 += y;
                  }
                if (_strchr ("zZ", prev_cmd))
                  {
                    gsk_path_builder_move_to (builder, x, y);
                    path_x = x;
                    path_y = y;
                  }
                gsk_path_builder_cubic_to (builder, x0, y0, x1, y1, x2, y2);
                prev_x1 = x1;
                prev_y1 = y1;
                x = x2;
                y = y2;
              }
            else
              goto error;
          }
          break;

        case 'S':
        case 's':
          {
            double x0, y0, x1, y1, x2, y2;

            if (parse_coordinate_pair (&p, &x1, &y1) &&
                parse_coordinate_pair (&p, &x2, &y2))
              {
                if (cmd == 's')
                  {
                    x1 += x;
                    y1 += y;
                    x2 += x;
                    y2 += y;
                  }
                if (_strchr ("CcSs", prev_cmd))
                  {
                    x0 = 2 * x - prev_x1;
                    y0 = 2 * y - prev_y1;
                  }
                else
                  {
                    x0 = x;
                    y0 = y;
                  }
                if (_strchr ("zZ", prev_cmd))
                  {
                    gsk_path_builder_move_to (builder, x, y);
                    path_x = x;
                    path_y = y;
                  }
                gsk_path_builder_cubic_to (builder, x0, y0, x1, y1, x2, y2);
                prev_x1 = x1;
                prev_y1 = y1;
                x = x2;
                y = y2;
              }
            else
              goto error;
          }
          break;

        case 'Q':
        case 'q':
          {
            double x1, y1, x2, y2;

            if (parse_coordinate_pair (&p, &x1, &y1) &&
                parse_coordinate_pair (&p, &x2, &y2))
              {
                if (cmd == 'q')
                  {
                    x1 += x;
                    y1 += y;
                    x2 += x;
                    y2 += y;
                  }
                if (_strchr ("zZ", prev_cmd))
                  {
                    gsk_path_builder_move_to (builder, x, y);
                    path_x = x;
                    path_y = y;
                  }
                gsk_path_builder_quad_to (builder, x1, y1, x2, y2);
                prev_x1 = x1;
                prev_y1 = y1;
                x = x2;
                y = y2;
              }
            else
              goto error;
          }
          break;

        case 'T':
        case 't':
          {
            double x1, y1, x2, y2;

            if (parse_coordinate_pair (&p, &x2, &y2))
              {
                if (cmd == 't')
                  {
                    x2 += x;
                    y2 += y;
                  }
                if (_strchr ("QqTt", prev_cmd))
                  {
                    x1 = 2 * x - prev_x1;
                    y1 = 2 * y - prev_y1;
                  }
                else
                  {
                    x1 = x;
                    y1 = y;
                  }
                if (_strchr ("zZ", prev_cmd))
                  {
                    gsk_path_builder_move_to (builder, x, y);
                    path_x = x;
                    path_y = y;
                  }
                gsk_path_builder_quad_to (builder, x1, y1, x2, y2);
                prev_x1 = x1;
                prev_y1 = y1;
                x = x2;
                y = y2;
              }
            else
              goto error;
          }
          break;

        case 'A':
        case 'a':
          {
            double rx, ry;
            double x_axis_rotation;
            int large_arc, sweep;
            double x1, y1;

            if (parse_nonnegative_number (&p, &rx) &&
                parse_nonnegative_number (&p, &ry) &&
                parse_number (&p, &x_axis_rotation) &&
                parse_flag (&p, &large_arc) &&
                parse_flag (&p, &sweep) &&
                parse_coordinate_pair (&p, &x1, &y1))
              {
                if (cmd == 'a')
                  {
                    x1 += x;
                    y1 += y;
                  }

                if (_strchr ("zZ", prev_cmd))
                  {
                    gsk_path_builder_move_to (builder, x, y);
                    path_x = x;
                    path_y = y;
                  }
                gsk_path_builder_svg_arc_to (builder,
                                             rx, ry, x_axis_rotation,
                                             large_arc, sweep,
                                             x1, y1);
                x = x1;
                y = y1;
              }
            else
              goto error;
          }
          break;

        case 'E':
        case 'e':
          {
            double x1, y1, x2, y2;

            if (parse_coordinate_pair (&p, &x1, &y1) &&
                parse_coordinate_pair (&p, &x2, &y2))
              {
                if (cmd == 'e')
                  {
                    x1 += x;
                    y1 += y;
                    x2 += x;
                    y2 += y;
                  }
                if (_strchr ("zZ", prev_cmd))
                  {
                    gsk_path_builder_move_to (builder, x, y);
                    path_x = x;
                    path_y = y;
                  }
                gsk_path_builder_arc_to (builder, x1, y1, x2, y2);
                prev_x1 = x1;
                prev_y1 = y1;
                x = x2;
                y = y2;
              }
            else
              goto error;
          }
          break;

        default:
          goto error;
        }

      after_comma = (p > string) && p[-1] == ',';
    }

  if (after_comma)
    goto error;

  return gsk_path_builder_free_to_path (builder);

error:
  //g_warning ("Can't parse string '%s' as GskPath, error at %ld", string, p - string);
  gsk_path_builder_unref (builder);

  return NULL;
}
