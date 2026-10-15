/* Unit tests for src/engine/environment/landscape/landscape_spline.c (issue #340).
 * Compiles the unit directly. add_engine_test would link Engine, which does not
 * build on origin/main (missing GLFW, then gpu_profiler.c:80).
 *
 * CHECK does not depend on NDEBUG, so these run meaningfully in Release too.
 */

#include "environment/landscape_spline.h"

#include <stdio.h>

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    g_checks++;                                                                \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
      g_failures++;                                                            \
    }                                                                          \
  } while (0)

static int approx(f32 a, f32 b) {
  f32 d = a - b;
  return d > -1e-5f && d < 1e-5f;
}

static int vec3_is(Vec3 v, f32 x, f32 y, f32 z) {
  return approx(v.x, x) && approx(v.y, y) && approx(v.z, z);
}

static void add_pt(LandscapeSpline *s, f32 x, f32 y, f32 z) {
  landscape_spline_add_point(s, (Vec3){x, y, z}, 1.0f);
}

static void test_create_returns_defaults(void) {
  LandscapeSpline *s = landscape_spline_create();
  CHECK(s != NULL);
  if (!s)
    return;
  CHECK(s->point_count == 0);
  CHECK(s->segment_length == 1.0f);
  CHECK(s->closed_loop == false);
  CHECK(s->mesh_profile_path[0] == '\0');
  landscape_spline_destroy(s);
}

/* NULL must be a no-op; a crash fails the test. */
static void test_destroy_null_is_safe(void) {
  landscape_spline_destroy(NULL);
}

static void test_add_point_stores_position_and_width(void) {
  LandscapeSpline *s = landscape_spline_create();
  CHECK(s != NULL);
  if (!s)
    return;
  landscape_spline_add_point(s, (Vec3){1.0f, 2.0f, 3.0f}, 0.5f);
  CHECK(s->point_count == 1);
  CHECK(vec3_is(s->points[0].position, 1.0f, 2.0f, 3.0f));
  CHECK(approx(s->points[0].width, 0.5f));
  landscape_spline_destroy(s);
}

/* NULL spline must be ignored; a crash fails the test. */
static void test_add_point_null_spline_is_noop(void) {
  landscape_spline_add_point(NULL, (Vec3){1.0f, 2.0f, 3.0f}, 1.0f);
}

/* The third point takes the tangent branch; it must not disturb earlier points. */
static void test_add_point_keeps_earlier_points(void) {
  LandscapeSpline *s = landscape_spline_create();
  CHECK(s != NULL);
  if (!s)
    return;
  add_pt(s, 0.0f, 0.0f, 0.0f);
  add_pt(s, 10.0f, 0.0f, 0.0f);
  add_pt(s, 10.0f, 20.0f, 0.0f);
  CHECK(s->point_count == 3);
  CHECK(vec3_is(s->points[0].position, 0.0f, 0.0f, 0.0f));
  CHECK(vec3_is(s->points[1].position, 10.0f, 0.0f, 0.0f));
  CHECK(vec3_is(s->points[2].position, 10.0f, 20.0f, 0.0f));
  landscape_spline_destroy(s);
}

static void test_add_point_caps_at_max_points(void) {
  LandscapeSpline *s = landscape_spline_create();
  CHECK(s != NULL);
  if (!s)
    return;
  for (u32 i = 0; i < (u32)SPLINE_MAX_POINTS + 5u; i++)
    landscape_spline_add_point(s, (Vec3){(f32)i, 0.0f, 0.0f}, 1.0f);
  CHECK(s->point_count == SPLINE_MAX_POINTS);
  CHECK(vec3_is(s->points[SPLINE_MAX_POINTS - 1].position,
                (f32)(SPLINE_MAX_POINTS - 1), 0.0f, 0.0f));
  landscape_spline_destroy(s);
}

/* Fewer than two control points evaluate to the origin. */
static void test_eval_position_degenerate_returns_origin(void) {
  LandscapeSpline *s = landscape_spline_create();
  CHECK(vec3_is(landscape_spline_eval_position(NULL, 0.5f), 0.0f, 0.0f, 0.0f));
  CHECK(s != NULL);
  if (!s)
    return;
  CHECK(vec3_is(landscape_spline_eval_position(s, 0.5f), 0.0f, 0.0f, 0.0f));
  add_pt(s, 5.0f, 5.0f, 5.0f);
  CHECK(vec3_is(landscape_spline_eval_position(s, 0.5f), 0.0f, 0.0f, 0.0f));
  landscape_spline_destroy(s);
}

static void test_eval_position_two_points_lerps(void) {
  LandscapeSpline *s = landscape_spline_create();
  CHECK(s != NULL);
  if (!s)
    return;
  add_pt(s, 0.0f, 0.0f, 0.0f);
  add_pt(s, 4.0f, 8.0f, -2.0f);
  CHECK(vec3_is(landscape_spline_eval_position(s, 0.0f), 0.0f, 0.0f, 0.0f));
  CHECK(vec3_is(landscape_spline_eval_position(s, 0.25f), 1.0f, 2.0f, -0.5f));
  CHECK(vec3_is(landscape_spline_eval_position(s, 0.5f), 2.0f, 4.0f, -1.0f));
  CHECK(vec3_is(landscape_spline_eval_position(s, 1.0f), 4.0f, 8.0f, -2.0f));
  landscape_spline_destroy(s);
}

/* t maps onto two segments, so t = 0.5 lands exactly on the middle knot. */
static void test_eval_position_three_points_hits_knots(void) {
  LandscapeSpline *s = landscape_spline_create();
  CHECK(s != NULL);
  if (!s)
    return;
  add_pt(s, 0.0f, 0.0f, 0.0f);
  add_pt(s, 10.0f, 0.0f, 0.0f);
  add_pt(s, 10.0f, 20.0f, 0.0f);
  CHECK(vec3_is(landscape_spline_eval_position(s, 0.25f), 5.0f, 0.0f, 0.0f));
  CHECK(vec3_is(landscape_spline_eval_position(s, 0.5f), 10.0f, 0.0f, 0.0f));
  CHECK(vec3_is(landscape_spline_eval_position(s, 0.75f), 10.0f, 10.0f, 0.0f));
  CHECK(vec3_is(landscape_spline_eval_position(s, 1.0f), 10.0f, 20.0f, 0.0f));
  landscape_spline_destroy(s);
}

/* t past 1.0 returns the last control point. */
static void test_eval_position_clamps_past_end(void) {
  LandscapeSpline *s = landscape_spline_create();
  CHECK(s != NULL);
  if (!s)
    return;
  add_pt(s, 0.0f, 0.0f, 0.0f);
  add_pt(s, 10.0f, 20.0f, 0.0f);
  add_pt(s, 10.0f, 40.0f, 0.0f);
  CHECK(vec3_is(landscape_spline_eval_position(s, 1.5f), 10.0f, 40.0f, 0.0f));
  landscape_spline_destroy(s);
}

/* Only the degenerate fallback is asserted; the valid-spline tangent is a stub. */
static void test_eval_tangent_degenerate_returns_up(void) {
  LandscapeSpline *s = landscape_spline_create();
  CHECK(vec3_is(landscape_spline_eval_tangent(NULL, 0.5f), 0.0f, 1.0f, 0.0f));
  CHECK(s != NULL);
  if (!s)
    return;
  CHECK(vec3_is(landscape_spline_eval_tangent(s, 0.5f), 0.0f, 1.0f, 0.0f));
  add_pt(s, 0.0f, 0.0f, 0.0f);
  CHECK(vec3_is(landscape_spline_eval_tangent(s, 0.5f), 0.0f, 1.0f, 0.0f));
  landscape_spline_destroy(s);
}

static int g_mesh_callbacks = 0;

static void count_callback(Vec3 p1, Vec3 p2, Vec3 p3, Vec3 normal) {
  (void)p1;
  (void)p2;
  (void)p3;
  (void)normal;
  g_mesh_callbacks++;
}

static void test_generate_mesh_null_args_are_noop(void) {
  LandscapeSpline *s = landscape_spline_create();
  CHECK(s != NULL);
  landscape_spline_generate_mesh(NULL, count_callback);
  CHECK(g_mesh_callbacks == 0);
  landscape_spline_generate_mesh(s, NULL);
  landscape_spline_destroy(s);
}

int main(void) {
  test_create_returns_defaults();
  test_destroy_null_is_safe();
  test_add_point_stores_position_and_width();
  test_add_point_null_spline_is_noop();
  test_add_point_keeps_earlier_points();
  test_add_point_caps_at_max_points();
  test_eval_position_degenerate_returns_origin();
  test_eval_position_two_points_lerps();
  test_eval_position_three_points_hits_knots();
  test_eval_position_clamps_past_end();
  test_eval_tangent_degenerate_returns_up();
  test_generate_mesh_null_args_are_noop();

  if (g_failures) {
    fprintf(stderr, "test_landscape_spline: %d of %d checks failed\n",
            g_failures, g_checks);
    return 1;
  }
  printf("test_landscape_spline: %d checks passed\n", g_checks);
  return 0;
}
