/*
 * landscape_spline_test.c
 * Create, capacity, linear segment evaluation, tangent fallback, and mesh safety.
 * Compiles src/engine/environment/landscape_spline.c directly and does not link Engine.
 */

#include "environment/landscape_spline.h"

#include <stdio.h>
#include <stdlib.h>

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void expect_true(int cond, const char *msg) {
    if (!cond) {
        fail(msg);
    }
}

static int near_f(f32 a, f32 b) {
    f32 d = a - b;
    if (d < 0.0f) {
        d = -d;
    }
    return d <= 0.0001f;
}

static void expect_vec(Vec3 v, f32 x, f32 y, f32 z, const char *msg) {
    if (!near_f(v.x, x) || !near_f(v.y, y) || !near_f(v.z, z)) {
        fprintf(stderr, "FAIL: %s (got %g %g %g expected %g %g %g)\n", msg,
                (double)v.x, (double)v.y, (double)v.z, (double)x, (double)y,
                (double)z);
        exit(1);
    }
}

static Vec3 v3(f32 x, f32 y, f32 z) {
    Vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v._w = 0.0f;
    return v;
}

static int g_mesh_calls;

static void mesh_callback(Vec3 p1, Vec3 p2, Vec3 p3, Vec3 normal) {
    (void)p1;
    (void)p2;
    (void)p3;
    (void)normal;
    g_mesh_calls++;
}

static void test_null_and_short(void) {
    Vec3 pos;
    Vec3 tangent;

    landscape_spline_destroy(NULL);
    landscape_spline_add_point(NULL, v3(1.0f, 2.0f, 3.0f), 4.0f);
    landscape_spline_generate_mesh(NULL, mesh_callback);
    expect_true(g_mesh_calls == 0, "null spline must not emit mesh triangles");

    pos = landscape_spline_eval_position(NULL, 0.5f);
    expect_vec(pos, 0.0f, 0.0f, 0.0f, "null position");
    tangent = landscape_spline_eval_tangent(NULL, 0.5f);
    expect_vec(tangent, 0.0f, 1.0f, 0.0f, "null tangent");

    LandscapeSpline *spline = landscape_spline_create();
    expect_true(spline != NULL, "create");
    expect_true(spline->point_count == 0, "empty count");
    expect_true(near_f(spline->segment_length, 1.0f), "default segment length");
    expect_true(spline->closed_loop == false, "not closed");
    expect_true(spline->mesh_profile_path[0] == '\0', "empty profile path");

    pos = landscape_spline_eval_position(spline, 0.3f);
    expect_vec(pos, 0.0f, 0.0f, 0.0f, "no points");
    tangent = landscape_spline_eval_tangent(spline, 0.3f);
    expect_vec(tangent, 0.0f, 1.0f, 0.0f, "no points tangent");

    landscape_spline_add_point(spline, v3(4.0f, 5.0f, 6.0f), 1.5f);
    expect_true(spline->point_count == 1, "one point");
    expect_vec(spline->points[0].position, 4.0f, 5.0f, 6.0f, "stored position");
    expect_true(near_f(spline->points[0].width, 1.5f), "stored width");
    expect_vec(spline->points[0].tangent_in, 0.0f, 0.0f, 0.0f, "tangent in stays zero");
    expect_vec(spline->points[0].tangent_out, 0.0f, 0.0f, 0.0f, "tangent out stays zero");

    pos = landscape_spline_eval_position(spline, 0.5f);
    expect_vec(pos, 0.0f, 0.0f, 0.0f, "one point is not a segment");
    tangent = landscape_spline_eval_tangent(spline, 0.0f);
    expect_vec(tangent, 0.0f, 1.0f, 0.0f, "one point tangent");

    landscape_spline_generate_mesh(spline, NULL);
    landscape_spline_generate_mesh(spline, mesh_callback);
    expect_true(g_mesh_calls == 0, "mesh stub emits nothing");

    landscape_spline_destroy(spline);
}

static void test_linear_segments(void) {
    LandscapeSpline *spline = landscape_spline_create();
    Vec3 pos;
    Vec3 tangent;

    expect_true(spline != NULL, "create line");
    landscape_spline_add_point(spline, v3(1.0f, 2.0f, 3.0f), 2.0f);
    landscape_spline_add_point(spline, v3(5.0f, 6.0f, 9.0f), 4.0f);
    expect_true(spline->point_count == 2, "two points");

    pos = landscape_spline_eval_position(spline, 0.0f);
    expect_vec(pos, 1.0f, 2.0f, 3.0f, "t=0 is the first point");
    pos = landscape_spline_eval_position(spline, 1.0f);
    expect_vec(pos, 5.0f, 6.0f, 9.0f, "t=1 is the last point");
    pos = landscape_spline_eval_position(spline, 0.5f);
    expect_vec(pos, 3.0f, 4.0f, 6.0f, "t=0.5 is the midpoint");
    pos = landscape_spline_eval_position(spline, 0.25f);
    expect_vec(pos, 2.0f, 3.0f, 4.5f, "t=0.25 is linear, not hermite");
    pos = landscape_spline_eval_position(spline, 1.5f);
    expect_vec(pos, 5.0f, 6.0f, 9.0f, "t>1 clamps to the last point");

    tangent = landscape_spline_eval_tangent(spline, 0.0f);
    expect_vec(tangent, 1.0f, 0.0f, 0.0f, "stub tangent at start");
    tangent = landscape_spline_eval_tangent(spline, 0.4f);
    expect_vec(tangent, 1.0f, 0.0f, 0.0f, "stub tangent mid");
    tangent = landscape_spline_eval_tangent(spline, 2.0f);
    expect_vec(tangent, 1.0f, 0.0f, 0.0f, "stub tangent past the end");

    landscape_spline_add_point(spline, v3(6.0f, 4.0f, 2.0f), 1.0f);
    expect_true(spline->point_count == 3, "three points");
    pos = landscape_spline_eval_position(spline, 0.0f);
    expect_vec(pos, 1.0f, 2.0f, 3.0f, "three-point start");
    pos = landscape_spline_eval_position(spline, 0.25f);
    expect_vec(pos, 3.0f, 4.0f, 6.0f, "first segment midpoint");
    pos = landscape_spline_eval_position(spline, 0.5f);
    expect_vec(pos, 5.0f, 6.0f, 9.0f, "middle control point");
    pos = landscape_spline_eval_position(spline, 0.75f);
    expect_vec(pos, 5.5f, 5.0f, 5.5f, "second segment midpoint");
    pos = landscape_spline_eval_position(spline, 1.0f);
    expect_vec(pos, 6.0f, 4.0f, 2.0f, "three-point end");

    landscape_spline_destroy(spline);
}

static void test_capacity(void) {
    LandscapeSpline *spline = landscape_spline_create();
    u32 i;

    expect_true(spline != NULL, "create capacity");
    for (i = 0; i < SPLINE_MAX_POINTS; i++) {
        landscape_spline_add_point(spline, v3((f32)i, 1.0f, -1.0f), 0.5f);
    }
    expect_true(spline->point_count == SPLINE_MAX_POINTS, "filled to the cap");
    landscape_spline_add_point(spline, v3(999.0f, 999.0f, 999.0f), 9.0f);
    expect_true(spline->point_count == SPLINE_MAX_POINTS, "extra point is dropped");
    expect_vec(spline->points[0].position, 0.0f, 1.0f, -1.0f, "first kept");
    expect_vec(spline->points[SPLINE_MAX_POINTS - 1].position,
               (f32)(SPLINE_MAX_POINTS - 1), 1.0f, -1.0f, "last kept");
    expect_true(near_f(spline->points[SPLINE_MAX_POINTS - 1].width, 0.5f),
                "last width unchanged");

    landscape_spline_destroy(spline);
}

int main(void) {
    test_null_and_short();
    test_linear_segments();
    test_capacity();
    printf("landscape_spline_test passed\n");
    return 0;
}
