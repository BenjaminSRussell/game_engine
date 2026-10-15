/*
 * mat4_transform_test.c
 * Translation, scale, and rotation matrices from mat4_transform.c.
 * Compiles that file directly and does not link Engine.
 */

#include "include/math/math_all.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void expect_eq(float actual, float expected, const char *msg) {
    float delta = actual - expected;
    if (delta < 0.0f) {
        delta = -delta;
    }
    if (delta > 1.0e-5f) {
        fprintf(stderr, "FAIL: %s (got %g expected %g)\n", msg, actual, expected);
        exit(1);
    }
}

static Vec3 apply_point(Mat4 m, Vec3 v) {
    Vec3 out;
    out.x = m.m00 * v.x + m.m10 * v.y + m.m20 * v.z + m.m30;
    out.y = m.m01 * v.x + m.m11 * v.y + m.m21 * v.z + m.m31;
    out.z = m.m02 * v.x + m.m12 * v.y + m.m22 * v.z + m.m32;
    out._w = 0.0f;
    return out;
}

static void expect_point(Vec3 actual, float x, float y, float z, const char *msg) {
    expect_eq(actual.x, x, msg);
    expect_eq(actual.y, y, msg);
    expect_eq(actual.z, z, msg);
}

static void expect_identity(Mat4 m, const char *msg) {
    expect_eq(m.m00, 1.0f, msg);
    expect_eq(m.m11, 1.0f, msg);
    expect_eq(m.m22, 1.0f, msg);
    expect_eq(m.m33, 1.0f, msg);
    expect_eq(m.m01, 0.0f, msg);
    expect_eq(m.m02, 0.0f, msg);
    expect_eq(m.m03, 0.0f, msg);
    expect_eq(m.m10, 0.0f, msg);
    expect_eq(m.m12, 0.0f, msg);
    expect_eq(m.m13, 0.0f, msg);
    expect_eq(m.m20, 0.0f, msg);
    expect_eq(m.m21, 0.0f, msg);
    expect_eq(m.m23, 0.0f, msg);
    expect_eq(m.m30, 0.0f, msg);
    expect_eq(m.m31, 0.0f, msg);
    expect_eq(m.m32, 0.0f, msg);
}

int main(void) {
    Vec3 origin = vec3_create(0.0f, 0.0f, 0.0f);
    Vec3 unit = vec3_create(1.0f, 1.0f, 1.0f);
    Vec3 x_axis = vec3_create(1.0f, 0.0f, 0.0f);
    Vec3 y_axis = vec3_create(0.0f, 1.0f, 0.0f);
    Vec3 z_axis = vec3_create(0.0f, 0.0f, 1.0f);
    Mat4 translated;
    Mat4 scaled;
    Mat4 turned;
    Mat4 same;
    const float half_pi = PI * 0.5f;

    translated = mat4_translate(origin);
    expect_identity(translated, "translate zero");

    translated = mat4_translate(vec3_create(10.0f, -2.0f, 0.5f));
    expect_eq(translated.m00, 1.0f, "translate basis x");
    expect_eq(translated.m11, 1.0f, "translate basis y");
    expect_eq(translated.m22, 1.0f, "translate basis z");
    expect_eq(translated.m33, 1.0f, "translate w");
    expect_eq(translated.m30, 10.0f, "translate x");
    expect_eq(translated.m31, -2.0f, "translate y");
    expect_eq(translated.m32, 0.5f, "translate z");
    expect_eq(translated.m03, 0.0f, "translate m03");
    expect_point(apply_point(translated, unit), 11.0f, -1.0f, 1.5f, "translate point");

    scaled = mat4_scale(vec3_create(2.0f, 3.0f, 4.0f));
    expect_eq(scaled.m00, 2.0f, "scale x");
    expect_eq(scaled.m11, 3.0f, "scale y");
    expect_eq(scaled.m22, 4.0f, "scale z");
    expect_eq(scaled.m33, 1.0f, "scale w");
    expect_eq(scaled.m30, 0.0f, "scale translation");
    expect_point(apply_point(scaled, unit), 2.0f, 3.0f, 4.0f, "scale point");

    scaled = mat4_scale(vec3_create(-1.0f, 1.0f, 1.0f));
    expect_point(apply_point(scaled, x_axis), -1.0f, 0.0f, 0.0f, "negative scale");

    expect_identity(mat4_rotate_x(0.0f), "rotate_x zero");
    expect_identity(mat4_rotate_y(0.0f), "rotate_y zero");
    expect_identity(mat4_rotate_z(0.0f), "rotate_z zero");
    expect_identity(mat4_rotate(z_axis, 0.0f), "rotate zero angle");
    expect_identity(mat4_rotate(origin, half_pi), "rotate zero axis");
    expect_identity(mat4_rotate(vec3_create(1.0e-5f, 0.0f, 0.0f), half_pi),
                    "rotate tiny axis");

    turned = mat4_rotate_x(half_pi);
    expect_eq(turned.m00, 1.0f, "rotate_x m00");
    expect_eq(turned.m33, 1.0f, "rotate_x m33");
    expect_point(apply_point(turned, y_axis), 0.0f, 0.0f, -1.0f, "rotate_x y");
    expect_point(apply_point(turned, z_axis), 0.0f, 1.0f, 0.0f, "rotate_x z");

    turned = mat4_rotate_y(half_pi);
    expect_eq(turned.m11, 1.0f, "rotate_y m11");
    expect_point(apply_point(turned, x_axis), 0.0f, 0.0f, 1.0f, "rotate_y x");
    expect_point(apply_point(turned, z_axis), -1.0f, 0.0f, 0.0f, "rotate_y z");

    turned = mat4_rotate_z(half_pi);
    expect_eq(turned.m22, 1.0f, "rotate_z m22");
    expect_point(apply_point(turned, x_axis), 0.0f, -1.0f, 0.0f, "rotate_z x");
    expect_point(apply_point(turned, y_axis), 1.0f, 0.0f, 0.0f, "rotate_z y");

    turned = mat4_rotate(z_axis, half_pi);
    same = mat4_rotate(vec3_create(0.0f, 0.0f, 5.0f), half_pi);
    expect_eq(turned.m00, same.m00, "axis length m00");
    expect_eq(turned.m01, same.m01, "axis length m01");
    expect_eq(turned.m10, same.m10, "axis length m10");
    expect_eq(turned.m11, same.m11, "axis length m11");
    expect_eq(turned.m22, same.m22, "axis length m22");
    expect_point(apply_point(turned, x_axis), 0.0f, 1.0f, 0.0f, "rotate z axis x");
    expect_point(apply_point(turned, y_axis), -1.0f, 0.0f, 0.0f, "rotate z axis y");
    expect_point(apply_point(same, x_axis), 0.0f, 1.0f, 0.0f, "long z axis");

    turned = mat4_rotate(vec3_create(1.0f, 1.0f, 0.0f), PI);
    expect_point(apply_point(turned, z_axis), 0.0f, 0.0f, -1.0f, "rotate 180 z");
    expect_point(apply_point(turned, x_axis), 0.0f, 1.0f, 0.0f, "rotate 180 x onto y");
    expect_point(apply_point(turned, y_axis), 1.0f, 0.0f, 0.0f, "rotate 180 y onto x");

    printf("ok\n");
    return 0;
}
