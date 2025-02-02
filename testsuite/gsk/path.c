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

#include <gtk/gtk.h>

static GskPath *
create_random_degenerate_path (guint max_contours)
{
#define N_DEGENERATE_PATHS 14
  GskPathBuilder *builder;
  guint i;

  builder = gsk_path_builder_new ();

  switch (g_test_rand_int_range (0, N_DEGENERATE_PATHS))
  {
    case 0:
      /* empty path */
      break;

    case 1:
      /* a single point */
      gsk_path_builder_move_to (builder, 
                                g_test_rand_double_range (-1000, 1000),
                                g_test_rand_double_range (-1000, 1000));
      break;

    case 2:
      /* N points */
      for (i = 0; i < MIN (10, max_contours); i++)
        {
          gsk_path_builder_move_to (builder, 
                                    g_test_rand_double_range (-1000, 1000),
                                    g_test_rand_double_range (-1000, 1000));
        }
      break;

    case 3:
      /* 1 closed point */
      gsk_path_builder_move_to (builder, 
                                g_test_rand_double_range (-1000, 1000),
                                g_test_rand_double_range (-1000, 1000));
      gsk_path_builder_close (builder);
      break;

    case 4:
      /* the same point closed N times */
      gsk_path_builder_move_to (builder, 
                                g_test_rand_double_range (-1000, 1000),
                                g_test_rand_double_range (-1000, 1000));
      for (i = 0; i < MIN (10, max_contours); i++)
        {
          gsk_path_builder_close (builder);
        }
      break;

    case 5:
      /* a zero-width and zero-height rect */
      gsk_path_builder_add_rect (builder,
                                 &GRAPHENE_RECT_INIT (g_test_rand_double_range (-1000, 1000),
                                                      g_test_rand_double_range (-1000, 1000),
                                                      0, 0));
      break;

    case 6:
      /* a zero-width rect */
      gsk_path_builder_add_rect (builder,
                                 &GRAPHENE_RECT_INIT (g_test_rand_double_range (-1000, 1000),
                                                      g_test_rand_double_range (-1000, 1000),
                                                      0,
                                                      g_test_rand_double_range (-1000, 1000)));
      break;

    case 7:
      /* a zero-height rect */
      gsk_path_builder_add_rect (builder,
                                 &GRAPHENE_RECT_INIT (g_test_rand_double_range (-1000, 1000),
                                                      g_test_rand_double_range (-1000, 1000),
                                                      g_test_rand_double_range (-1000, 1000),
                                                      0));
      break;

    case 8:
      /* a negative-size rect */
      gsk_path_builder_add_rect (builder,
                                 &GRAPHENE_RECT_INIT (g_test_rand_double_range (-1000, 1000),
                                                      g_test_rand_double_range (-1000, 1000),
                                                      g_test_rand_double_range (-1000, 0),
                                                      g_test_rand_double_range (-1000, 0)));
      break;

    case 9:
      /* an absolutely random rect */
      gsk_path_builder_add_rect (builder,
                                 &GRAPHENE_RECT_INIT (g_test_rand_double_range (-1000, 1000),
                                                      g_test_rand_double_range (-1000, 1000),
                                                      g_test_rand_double_range (-1000, 1000),
                                                      g_test_rand_double_range (-1000, 1000)));
      break;

    case 10:
      /* an absolutely random rect */
      gsk_path_builder_add_rect (builder,
                                 &GRAPHENE_RECT_INIT (g_test_rand_double_range (-1000, 1000),
                                                      g_test_rand_double_range (-1000, 1000),
                                                      g_test_rand_double_range (-1000, 1000),
                                                      g_test_rand_double_range (-1000, 1000)));
      break;

    case 11:
      /* an absolutely random circle */
      gsk_path_builder_add_circle (builder,
                                   &GRAPHENE_POINT_INIT (g_test_rand_double_range (-1000, 1000),
                                                         g_test_rand_double_range (-1000, 1000)),
                                   g_test_rand_double_range (1, 1000));
      break;

    case 12:
      /* a zero-length line */
      {
        graphene_point_t point = GRAPHENE_POINT_INIT (g_test_rand_double_range (-1000, 1000),
                                                      g_test_rand_double_range (-1000, 1000));
        gsk_path_builder_move_to (builder, point.x, point.y);
        gsk_path_builder_line_to (builder, point.x, point.y);
      }
      break;

    case 13:
      /* a cubic with start == end */
      {
        graphene_point_t point = GRAPHENE_POINT_INIT (g_test_rand_double_range (-1000, 1000),
                                                      g_test_rand_double_range (-1000, 1000));
        gsk_path_builder_move_to (builder, point.x, point.y);
        gsk_path_builder_cubic_to (builder,
                                   g_test_rand_double_range (-1000, 1000),
                                   g_test_rand_double_range (-1000, 1000),
                                   g_test_rand_double_range (-1000, 1000),
                                   g_test_rand_double_range (-1000, 1000),
                                   point.x, point.y);
      }
      break;

    case N_DEGENERATE_PATHS:
    default:
      g_assert_not_reached ();
  }

  return gsk_path_builder_free_to_path (builder);
}

static GskPath *
create_random_path (guint max_contours);

static void
add_shape_contour (GskPathBuilder *builder)
{
#define N_SHAPE_CONTOURS 3
  switch (g_test_rand_int_range (0, N_SHAPE_CONTOURS))
  {
    case 0:
      gsk_path_builder_add_rect (builder,
                                 &GRAPHENE_RECT_INIT (g_test_rand_double_range (-1000, 1000),
                                                      g_test_rand_double_range (-1000, 1000),
                                                      g_test_rand_double_range (1, 1000),
                                                      g_test_rand_double_range (1, 1000)));
      break;

    case 1:
      gsk_path_builder_add_circle (builder,
                                   &GRAPHENE_POINT_INIT (g_test_rand_double_range (-1000, 1000),
                                                         g_test_rand_double_range (-1000, 1000)),
                                   g_test_rand_double_range (1, 1000));
      break;

    case 2:
      {
        GskPath *path = create_random_path (1);
        gsk_path_builder_add_path (builder, path);
        gsk_path_unref (path);
      }
      break;

    case N_SHAPE_CONTOURS:
    default:
      g_assert_not_reached ();
      break;
  }
}

static void
add_standard_contour (GskPathBuilder *builder)
{
  guint i, n;

  if (g_test_rand_bit ())
    {
      if (g_test_rand_bit ())
        gsk_path_builder_move_to (builder,
                                  g_test_rand_double_range (-1000, 1000),
                                  g_test_rand_double_range (-1000, 1000));
      else
        gsk_path_builder_rel_move_to (builder,
                                      g_test_rand_double_range (-1000, 1000),
                                      g_test_rand_double_range (-1000, 1000));
    }

  /* that 20 is random, but should be enough to get some
   * crazy self-intersecting shapes */
  n = g_test_rand_int_range (1, 20);
  for (i = 0; i < n; i++)
    {
      switch (g_test_rand_int_range (0, 8))
      {
        case 0:
          gsk_path_builder_line_to (builder,
                                    g_test_rand_double_range (-1000, 1000),
                                    g_test_rand_double_range (-1000, 1000));
          break;

        case 1:
          gsk_path_builder_rel_line_to (builder,
                                        g_test_rand_double_range (-1000, 1000),
                                        g_test_rand_double_range (-1000, 1000));
          break;

        case 2:
          gsk_path_builder_quad_to (builder,
                                    g_test_rand_double_range (-1000, 1000),
                                    g_test_rand_double_range (-1000, 1000),
                                    g_test_rand_double_range (-1000, 1000),
                                    g_test_rand_double_range (-1000, 1000));
          break;

        case 3:
          gsk_path_builder_rel_quad_to (builder,
                                        g_test_rand_double_range (-1000, 1000),
                                        g_test_rand_double_range (-1000, 1000),
                                        g_test_rand_double_range (-1000, 1000),
                                        g_test_rand_double_range (-1000, 1000));
          break;

        case 4:
          gsk_path_builder_cubic_to (builder,
                                     g_test_rand_double_range (-1000, 1000),
                                     g_test_rand_double_range (-1000, 1000),
                                     g_test_rand_double_range (-1000, 1000),
                                     g_test_rand_double_range (-1000, 1000),
                                     g_test_rand_double_range (-1000, 1000),
                                     g_test_rand_double_range (-1000, 1000));
          break;

        case 5:
          gsk_path_builder_rel_cubic_to (builder,
                                         g_test_rand_double_range (-1000, 1000),
                                         g_test_rand_double_range (-1000, 1000),
                                         g_test_rand_double_range (-1000, 1000),
                                         g_test_rand_double_range (-1000, 1000),
                                         g_test_rand_double_range (-1000, 1000),
                                         g_test_rand_double_range (-1000, 1000));
          break;

        case 6:
          gsk_path_builder_arc_to (builder,
                                   g_test_rand_double_range (-1000, 1000),
                                   g_test_rand_double_range (-1000, 1000),
                                   g_test_rand_double_range (-1000, 1000),
                                   g_test_rand_double_range (-1000, 1000));
          break;

        case 7:
          gsk_path_builder_rel_arc_to (builder,
                                       g_test_rand_double_range (-1000, 1000),
                                       g_test_rand_double_range (-1000, 1000),
                                       g_test_rand_double_range (-1000, 1000),
                                       g_test_rand_double_range (-1000, 1000));
          break;

        default:
          g_assert_not_reached();
          break;
      }
    }

  if (g_test_rand_bit ())
    gsk_path_builder_close (builder);
}

static GskPath *
create_random_path (guint max_contours)
{
  GskPathBuilder *builder;
  guint i, n;

  /* 5% chance for a weird shape */
  if (!g_test_rand_int_range (0, 20))
    return create_random_degenerate_path (max_contours);

  builder = gsk_path_builder_new ();
  n = g_test_rand_int_range (1, 10);
  n = MIN (n, max_contours);

  for (i = 0; i < n; i++)
    {
      /* 2/3 of shapes are standard contours */
      if (g_test_rand_int_range (0, 3))
        add_standard_contour (builder);
      else
        add_shape_contour (builder);
    }

  return gsk_path_builder_free_to_path (builder);
}

typedef struct {
  GskPathOperation op;
  graphene_point_t pts[4];
} PathOperation;

static void
_g_string_append_double (GString *string,
                         double   d)
{
  char buf[G_ASCII_DTOSTR_BUF_SIZE];

  g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, d);
  g_string_append (string, buf);
}

static void
_g_string_append_point (GString                *string,
                        const graphene_point_t *pt)
{
  _g_string_append_double (string, pt->x);
  g_string_append_c (string, ' ');
  _g_string_append_double (string, pt->y);
}

static void
path_operation_print (const PathOperation *p,
                      GString             *string)
{
  switch (p->op)
  {
    case GSK_PATH_MOVE:
      g_string_append (string, "M ");
      _g_string_append_point (string, &p->pts[0]);
      break;

    case GSK_PATH_CLOSE:
      g_string_append (string, " Z");
      break;

    case GSK_PATH_LINE:
      g_string_append (string, " L ");
      _g_string_append_point (string, &p->pts[1]);
      break;

    case GSK_PATH_QUAD:
      g_string_append (string, " Q ");
      _g_string_append_point (string, &p->pts[1]);
      g_string_append (string, ", ");
      _g_string_append_point (string, &p->pts[2]);
      break;

    case GSK_PATH_CUBIC:
      g_string_append (string, " C ");
      _g_string_append_point (string, &p->pts[1]);
      g_string_append (string, ", ");
      _g_string_append_point (string, &p->pts[2]);
      g_string_append (string, ", ");
      _g_string_append_point (string, &p->pts[3]);
      break;

    case GSK_PATH_ARC:
      g_string_append (string, " E ");
      _g_string_append_point (string, &p->pts[1]);
      g_string_append (string, ", ");
      _g_string_append_point (string, &p->pts[2]);
      break;

    default:
      g_assert_not_reached();
      return;
  }
}

static gboolean
path_operation_equal (const PathOperation *p1,
                      const PathOperation *p2,
                      float                epsilon)
{
  if (p1->op != p2->op)
    return FALSE;

  /* No need to compare pts[0] for most ops, that's just
   * duplicate work. */
  switch (p1->op)
    {
      case GSK_PATH_MOVE:
        return graphene_point_near (&p1->pts[0], &p2->pts[0], epsilon);

      case GSK_PATH_LINE:
      case GSK_PATH_CLOSE:
        return graphene_point_near (&p1->pts[1], &p2->pts[1], epsilon);

      case GSK_PATH_QUAD:
        return graphene_point_near (&p1->pts[1], &p2->pts[1], epsilon)
            && graphene_point_near (&p1->pts[2], &p2->pts[2], epsilon);

      case GSK_PATH_CUBIC:
        return graphene_point_near (&p1->pts[1], &p2->pts[1], epsilon)
            && graphene_point_near (&p1->pts[2], &p2->pts[2], epsilon)
            && graphene_point_near (&p1->pts[3], &p2->pts[3], epsilon);

      case GSK_PATH_ARC:
        return graphene_point_near (&p1->pts[1], &p2->pts[1], epsilon)
            && graphene_point_near (&p1->pts[2], &p2->pts[2], epsilon);

      default:
        g_return_val_if_reached (FALSE);
    }
}

static gboolean 
collect_path_operation_cb (GskPathOperation        op,
                           const graphene_point_t *pts,
                           gsize                   n_pts,
                           gpointer                user_data)
{
  g_array_append_vals (user_data,
                       (PathOperation[1])  { {
                         op,
                         {
                           GRAPHENE_POINT_INIT(pts[0].x, pts[0].y),
                           GRAPHENE_POINT_INIT(n_pts > 1 ? pts[1].x : 0,
                                               n_pts > 1 ? pts[1].y : 0),
                           GRAPHENE_POINT_INIT(n_pts > 2 ? pts[2].x : 0,
                                               n_pts > 2 ? pts[2].y : 0),
                           GRAPHENE_POINT_INIT(n_pts > 3 ? pts[3].x : 0,
                                               n_pts > 3 ? pts[3].y : 0)
                         },
                       } },
                       1);
  return TRUE;
}

static GArray *
collect_path (GskPath *path)
{
  GArray *array = g_array_new (FALSE, FALSE, sizeof (PathOperation));

  /* Use -1 here because we want all the flags, even future additions */
  gsk_path_foreach (path, -1, collect_path_operation_cb, array);

  return array;
}

static void
assert_path_equal_func (const char *domain,
                        const char *file,
                        int         line,
                        const char *func,
                        GskPath    *path1,
                        GskPath    *path2,
                        float       epsilon)
{
  GArray *ops1, *ops2;
  guint i;

  ops1 = collect_path (path1);
  ops2 = collect_path (path2);

  for (i = 0; i < MAX (ops1->len, ops2->len); i++)
    {
      PathOperation *op1 = i < ops1->len ? &g_array_index (ops1, PathOperation, i) : NULL;
      PathOperation *op2 = i < ops2->len ? &g_array_index (ops2, PathOperation, i) : NULL;

      if (op1 == NULL || op2 == NULL || !path_operation_equal (op1, op2, epsilon))
        {
          GString *string;
          guint j;

          /* Find the operation we start to print */
          for (j = i; j-- > 0; )
            {
              PathOperation *op = &g_array_index (ops1, PathOperation, j);
              if (op->op == GSK_PATH_MOVE)
                break;
              if (j + 3 == i)
                {
                  j = i - 1;
                  break;
                }
            }

          string = g_string_new (j == 0 ? "" : "... ");
          for (; j < i; j++)
            {
              PathOperation *op = &g_array_index (ops1, PathOperation, j);
              path_operation_print (op, string);
              g_string_append_c (string, ' ');
            }

          g_string_append (string, "\\\n    ");
          if (op1)
            {
              path_operation_print (op1, string);
              if (ops1->len > i + 1)
                g_string_append (string, " ...");
            }
          g_string_append (string, "\n    ");
          if (op1)
            {
              path_operation_print (op2, string);
              if (ops2->len > i + 1)
                g_string_append (string, " ...");
            }

          g_assertion_message (domain, file, line, func, string->str);

          g_string_free (string, TRUE);
        }
    }

  g_array_free (ops1, TRUE);
  g_array_free (ops2, TRUE);
}
#define assert_path_equal(p1,p2) assert_path_equal_func(G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, (p1),(p2), FLOAT_EPSILON)
#define assert_path_equal_with_epsilon(p1,p2, epsilon) \
    assert_path_equal_func(G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, (p1),(p2), (epsilon))

static void
test_create (void)
{
  GskPath *path1, *path2, *built;
  GskPathBuilder *builder;
  guint i;
  char *s;
  GString *str;

  for (i = 0; i < 1000; i++)
    {
      builder = gsk_path_builder_new ();
      path1 = create_random_path (G_MAXUINT);
      gsk_path_builder_add_path (builder, path1);
      path2 = create_random_path (G_MAXUINT);
      gsk_path_builder_add_path (builder, path2);
      built = gsk_path_builder_free_to_path (builder);

      str = g_string_new (NULL);
      gsk_path_print (path1, str);
      if (!gsk_path_is_empty (path1) && !gsk_path_is_empty (path2))
        g_string_append_c (str, ' ');
      gsk_path_print (path2, str);

      s = gsk_path_to_string (built);

      g_assert_cmpstr (s, ==, str->str);
      g_string_free (str, TRUE);
      g_free (s);

      gsk_path_unref (built);
      gsk_path_unref (path2);
      gsk_path_unref (path1);
    }
}

static void
test_parse (void)
{
  int i;

  for (i = 0; i < 1000; i++)
    {
      GskPath *path1, *path2;
      char *string1, *string2;

      path1 = create_random_path (G_MAXUINT);
      string1 = gsk_path_to_string (path1);
      g_assert_nonnull (string1);

      path2 = gsk_path_parse (string1);
      g_assert_nonnull (path2);
      string2 = gsk_path_to_string (path2);
      g_assert_nonnull (string2);

      assert_path_equal_with_epsilon (path1, path2, 1.f / 1024);

      gsk_path_unref (path2);
      gsk_path_unref (path1);
      g_free (string2);
      g_free (string1);
    }
}

#define N_PATHS 3
static void
test_in_fill_union (void)
{
  GskPath *path, *paths[N_PATHS];
  GskPathBuilder *builder;
  guint i, j, k;

  for (i = 0; i < 100; i++)
    {
      builder = gsk_path_builder_new ();

      for (k = 0; k < N_PATHS; k++)
        {
          paths[k] = create_random_path (G_MAXUINT);
          gsk_path_builder_add_path (builder, paths[k]);
        }

      path = gsk_path_builder_free_to_path (builder);

      for (j = 0; j < 100; j++)
        {
          graphene_point_t test = GRAPHENE_POINT_INIT (g_test_rand_double_range (-1000, 1000),
                                                       g_test_rand_double_range (-1000, 1000));
          GskFillRule fill_rule;

          for (fill_rule = GSK_FILL_RULE_WINDING; fill_rule <= GSK_FILL_RULE_EVEN_ODD; fill_rule++)
            {
              guint n_in_fill = 0;
              gboolean in_fill;

              for (k = 0; k < N_PATHS; k++)
                {
                  if (gsk_path_in_fill (paths[k], &test, GSK_FILL_RULE_EVEN_ODD))
                    n_in_fill++;
                }

              in_fill = gsk_path_in_fill (path, &test, GSK_FILL_RULE_EVEN_ODD);
              switch (fill_rule)
                {
                case GSK_FILL_RULE_WINDING:
                  if (n_in_fill == 0)
                    g_assert_false (in_fill);
                  else if (n_in_fill == 1)
                    g_assert_true (in_fill);
                  /* else we can't say anything because the winding rule doesn't give enough info */
                  break;

                case GSK_FILL_RULE_EVEN_ODD:
                  g_assert_cmpint (in_fill, ==, n_in_fill & 1);
                  break;

                default:
                  g_assert_not_reached ();
                  break;
                }
            }
        }

      gsk_path_unref (path);

      for (k = 0; k < N_PATHS; k++)
        gsk_path_unref (paths[k]);
    }
}
#undef N_PATHS

/* This is somewhat sucky because using foreach breaks up the contours
 * (like rects and circles) and replaces everything with the standard
 * contour.
 * But at least it extensively tests the standard contour.
 */
static gboolean
rotate_path_cb (GskPathOperation        op,
                const graphene_point_t *pts,
                gsize                   n_pts,
                gpointer                user_data)
{
  GskPathBuilder **builders = user_data;

  switch (op)
  {
    case GSK_PATH_MOVE:
      gsk_path_builder_move_to (builders[0], pts[0].x, pts[0].y);
      gsk_path_builder_move_to (builders[1], pts[0].y, -pts[0].x);
      break;

    case GSK_PATH_CLOSE:
      gsk_path_builder_close (builders[0]);
      gsk_path_builder_close (builders[1]);
      break;

    case GSK_PATH_LINE:
      gsk_path_builder_line_to (builders[0], pts[1].x, pts[1].y);
      gsk_path_builder_line_to (builders[1], pts[1].y, -pts[1].x);
      break;

    case GSK_PATH_QUAD:
      gsk_path_builder_quad_to (builders[0], pts[1].x, pts[1].y, pts[2].x, pts[2].y);
      gsk_path_builder_quad_to (builders[1], pts[1].y, -pts[1].x, pts[2].y, -pts[2].x);
      break;

    case GSK_PATH_CUBIC:
      gsk_path_builder_cubic_to (builders[0], pts[1].x, pts[1].y, pts[2].x, pts[2].y, pts[3].x, pts[3].y);
      gsk_path_builder_cubic_to (builders[1], pts[1].y, -pts[1].x, pts[2].y, -pts[2].x, pts[3].y, -pts[3].x);
      break;

    case GSK_PATH_ARC:
      gsk_path_builder_arc_to (builders[0], pts[1].x, pts[1].y, pts[2].x, pts[2].y);
      gsk_path_builder_arc_to (builders[1], pts[1].y, -pts[1].x, pts[2].y, -pts[2].x);
      break;

    default:
      g_assert_not_reached ();
      return FALSE;
  }

  return TRUE;
}

static void
test_in_fill_rotated (void)
{
  GskPath *path;
  GskPathBuilder *builders[2];
  GskPath *paths[2];
  guint i, j;

#define N_FILL_RULES 2
  /* if this triggers, you added a new enum value to GskFillRule, so the define above needs
   * an update */
  g_assert_null (g_enum_get_value (g_type_class_ref (GSK_TYPE_FILL_RULE), N_FILL_RULES));

  for (i = 0; i < 100; i++)
    {
      path = create_random_path (G_MAXUINT);
      builders[0] = gsk_path_builder_new ();
      builders[1] = gsk_path_builder_new ();
      /* Use -1 here because we want all the flags, even future additions */
      gsk_path_foreach (path, -1, rotate_path_cb, builders);
      gsk_path_unref (path);

      paths[0] = gsk_path_builder_free_to_path (builders[0]);
      paths[1] = gsk_path_builder_free_to_path (builders[1]);

      for (j = 0; j < 100; j++)
        {
          GskFillRule fill_rule = g_random_int_range (0, N_FILL_RULES);
          float x = g_test_rand_double_range (-1000, 1000);
          float y = g_test_rand_double_range (-1000, 1000);

          g_assert_cmpint (gsk_path_in_fill (paths[0], &GRAPHENE_POINT_INIT (x, y), fill_rule),
                           ==,
                           gsk_path_in_fill (paths[1], &GRAPHENE_POINT_INIT (y, -x), fill_rule));
          g_assert_cmpint (gsk_path_in_fill (paths[0], &GRAPHENE_POINT_INIT (y, x), fill_rule),
                           ==,
                           gsk_path_in_fill (paths[1], &GRAPHENE_POINT_INIT (x, -y), fill_rule));
        }

      gsk_path_unref (paths[0]);
      gsk_path_unref (paths[1]);
    }
#undef N_FILL_RULES
}

int
main (int   argc,
      char *argv[])
{
  gtk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/path/create", test_create);
  g_test_add_func ("/path/parse", test_parse);
  g_test_add_func ("/path/in-fill-union", test_in_fill_union);
  g_test_add_func ("/path/in-fill-rotated", test_in_fill_rotated);

  return g_test_run ();
}
