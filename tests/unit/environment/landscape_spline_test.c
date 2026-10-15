/* Issue #249: unit test for src/engine/environment/landscape_spline.c.
 *
 * Compiled directly against the unit, because the Engine target does not
 * build on main (see tests/registry/test_landscape_spline.cmake). Checks are
 * explicit rather than assert(), so they still run when NDEBUG is defined.
 */
#include <math.h>
#include <stdio.h>

#include "environment/landscape_spline.h"

#define TOLERANCE 1e-5f

static int g_failures = 0;

static void expect(const char *what, bool ok) {
  if (!ok) {
    fprintf(stderr, "FAIL: %s\n", what);
    g_failures++;
  }
}

static void expect_f32(const char *what, f32 got, f32 want) {
  if (fabsf(got - want) > TOLERANCE) {
    fprintf(stderr, "FAIL: %s: got %g, want %g\n", what, got, want);
    g_failures++;
  }
}

static void expect_vec3(const char *what, Vec3 got, Vec3 want) {
  if (fabsf(got.x - want.x) > TOLERANCE ||
      fabsf(got.y - want.y) > TOLERANCE || fabsf(got.z - want.z) > TOLERANCE) {
    fprintf(stderr, "FAIL: %s: got (%g, %g, %g), want (%g, %g, %g)\n", what,
            got.x, got.y, got.z, want.x, want.y, want.z);
    g_failures++;
  }
}

/* Builds a spline with the given control points, each with width 1. */
static LandscapeSpline *spline_through(const Vec3 *points, u32 count) {
  LandscapeSpline *spline = landscape_spline_create();
  if (!spline)
    return NULL;
  for (u32 i = 0; i < count; i++)
    landscape_spline_add_point(spline, points[i], 1.0f);
  return spline;
}

static void expect_position(const char *what, const Vec3 *points, u32 count,
                            f32 t, Vec3 want) {
  LandscapeSpline *spline = spline_through(points, count);
  if (!spline) {
    expect(what, false);
    return;
  }
  expect_vec3(what, landscape_spline_eval_position(spline, t), want);
  landscape_spline_destroy(spline);
}

static void expect_tangent(const char *what, const Vec3 *points, u32 count,
                           f32 t, Vec3 want) {
  LandscapeSpline *spline = spline_through(points, count);
  if (!spline) {
    expect(what, false);
    return;
  }
  expect_vec3(what, landscape_spline_eval_tangent(spline, t), want);
  landscape_spline_destroy(spline);
}

static void test_create_destroy_and_null_safety(void) {
  LandscapeSpline *spline = landscape_spline_create();
  expect("create returns a spline", spline != NULL);
  if (spline) {
    expect("new spline has no points", spline->point_count == 0);
    expect_f32("new spline segment_length", spline->segment_length, 1.0f);
  }
  landscape_spline_add_point(NULL, vec3_create(1, 2, 3), 1.0f);
  landscape_spline_destroy(spline);
  landscape_spline_destroy(NULL);
}

static void test_add_point_stores_points_and_caps_count(void) {
  LandscapeSpline *spline = landscape_spline_create();
  if (!spline) {
    expect("create for add_point", false);
    return;
  }
  for (u32 i = 0; i < SPLINE_MAX_POINTS + 5; i++)
    landscape_spline_add_point(spline, vec3_create((f32)i, 0.0f, 0.0f), 2.0f);
  expect("add_point caps point_count at SPLINE_MAX_POINTS",
         spline->point_count == SPLINE_MAX_POINTS);
  expect_f32("first stored point x", spline->points[0].position.x, 0.0f);
  expect_f32("last stored point x", spline->points[SPLINE_MAX_POINTS - 1]
                                        .position.x,
             (f32)(SPLINE_MAX_POINTS - 1));
  expect_f32("stored width", spline->points[3].width, 2.0f);
  landscape_spline_destroy(spline);
}

static void test_eval_position_without_two_points_is_origin(void) {
  Vec3 origin = vec3_create(0, 0, 0);
  expect_vec3("eval_position(NULL) is origin",
              landscape_spline_eval_position(NULL, 0.5f), origin);
  Vec3 one[1] = {vec3_create(4, 5, 6)};
  expect_position("eval_position with one point is origin", one, 1, 0.5f,
                  origin);
}

static void test_eval_position_two_points(void) {
  Vec3 line[2] = {vec3_create(0, 0, 0), vec3_create(10, 0, 0)};
  expect_position("t=0 is the first point", line, 2, 0.0f,
                  vec3_create(0, 0, 0));
  expect_position("t=0.25 lerps", line, 2, 0.25f, vec3_create(2.5f, 0, 0));
  expect_position("t=0.5 lerps", line, 2, 0.5f, vec3_create(5, 0, 0));
  expect_position("t=1 is the last point", line, 2, 1.0f,
                  vec3_create(10, 0, 0));
  expect_position("t>1 clamps to the last point", line, 2, 1.5f,
                  vec3_create(10, 0, 0));
  expect_position("t<0 clamps to the first point", line, 2, -0.5f,
                  vec3_create(0, 0, 0));
}

static void test_eval_position_three_points_crosses_segments(void) {
  Vec3 bent[3] = {vec3_create(0, 0, 0), vec3_create(10, 0, 0),
                  vec3_create(10, 10, 0)};
  expect_position("t=0.25 is midway on segment 0", bent, 3, 0.25f,
                  vec3_create(5, 0, 0));
  expect_position("t=0.5 is the middle point", bent, 3, 0.5f,
                  vec3_create(10, 0, 0));
  expect_position("t=0.75 is midway on segment 1", bent, 3, 0.75f,
                  vec3_create(10, 5, 0));
  expect_position("t=1 is the last point", bent, 3, 1.0f,
                  vec3_create(10, 10, 0));
}

static void test_eval_tangent_follows_segment_direction(void) {
  expect_vec3("tangent(NULL) falls back to up",
              landscape_spline_eval_tangent(NULL, 0.5f), vec3_create(0, 1, 0));

  Vec3 along_x[2] = {vec3_create(0, 0, 0), vec3_create(10, 0, 0)};
  expect_tangent("tangent along +X", along_x, 2, 0.5f, vec3_create(1, 0, 0));

  Vec3 along_z[2] = {vec3_create(0, 0, 0), vec3_create(0, 0, 10)};
  expect_tangent("tangent along +Z", along_z, 2, 0.5f, vec3_create(0, 0, 1));
  expect_tangent("tangent at t=1 along +Z", along_z, 2, 1.0f,
                 vec3_create(0, 0, 1));

  Vec3 diagonal[2] = {vec3_create(0, 0, 0), vec3_create(3, 4, 0)};
  expect_tangent("tangent is unit length", diagonal, 2, 0.5f,
                 vec3_create(0.6f, 0.8f, 0));

  Vec3 bent[3] = {vec3_create(0, 0, 0), vec3_create(10, 0, 0),
                  vec3_create(10, 10, 0)};
  expect_tangent("tangent on segment 0", bent, 3, 0.25f, vec3_create(1, 0, 0));
  expect_tangent("tangent on segment 1", bent, 3, 0.75f, vec3_create(0, 1, 0));
}

int main(void) {
  test_create_destroy_and_null_safety();
  test_add_point_stores_points_and_caps_count();
  test_eval_position_without_two_points_is_origin();
  test_eval_position_two_points();
  test_eval_position_three_points_crosses_segments();
  test_eval_tangent_follows_segment_direction();

  if (g_failures != 0) {
    fprintf(stderr, "test_landscape_spline: %d check(s) failed\n", g_failures);
    return 1;
  }
  printf("test_landscape_spline: all checks passed\n");
  return 0;
}
