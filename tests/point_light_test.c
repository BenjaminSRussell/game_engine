/*
 * point_light_test.c
 * point_light_create and the source-parameter setters.
 * Compiles point_light.c directly and does not link Engine.
 */

#include "lighting/sources/point_light.h"

#include <stdio.h>
#include <stdlib.h>

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static int near(float a, float b) {
    float d = a - b;
    if (d < 0.0f) {
        d = -d;
    }
    return d < 0.00001f;
}

static void expect_vec(simd_float3 v, float x, float y, float z, const char *label) {
    if (!near(v.x, x) || !near(v.y, y) || !near(v.z, z)) {
        fprintf(stderr, "FAIL: %s (%.6f, %.6f, %.6f)\n", label, v.x, v.y, v.z);
        exit(1);
    }
}

int main(void) {
    simd_float3 position = (simd_float3){1.5f, -2.0f, 3.25f};
    simd_float3 color = (simd_float3){0.25f, 0.5f, 0.75f};
    light_t light;
    light_t shadowed;

    light = point_light_create(position, color, 4.5f, 12.0f, false);
    if (light.type != LIGHT_TYPE_POINT) {
        fail("type");
    }
    expect_vec(light.position, 1.5f, -2.0f, 3.25f, "position");
    expect_vec(light.color, 0.25f, 0.5f, 0.75f, "color");
    if (!near(light.intensity, 4.5f)) {
        fail("intensity");
    }
    if (!near(light.radius, 12.0f)) {
        fail("radius");
    }
    if (light.cast_shadows) {
        fail("cast_shadows false");
    }
    expect_vec(light.direction, 0.0f, -1.0f, 0.0f, "default direction");
    if (!near(light.inner_cone, 0.0f) || !near(light.outer_cone, 0.0f)) {
        fail("default cones");
    }
    if (light.shadow_map_index != 0) {
        fail("default shadow map");
    }
    if (light.ies_texture_id != 0 || !near(light.source_radius, 0.0f) ||
        !near(light.source_length, 0.0f)) {
        fail("default source parameters");
    }

    shadowed = point_light_create(position, color, 1.0f, 2.0f, true);
    if (!shadowed.cast_shadows || shadowed.type != LIGHT_TYPE_POINT) {
        fail("cast_shadows true");
    }

    point_light_set_ies_profile(NULL, 9);
    point_light_set_source_radius(NULL, 1.0f);
    point_light_set_source_length(NULL, 2.0f);

    point_light_set_ies_profile(&light, 42);
    point_light_set_source_radius(&light, 0.25f);
    point_light_set_source_length(&light, 1.5f);
    if (light.ies_texture_id != 42) {
        fail("ies profile");
    }
    if (!near(light.source_radius, 0.25f)) {
        fail("source radius");
    }
    if (!near(light.source_length, 1.5f)) {
        fail("source length");
    }
    if (light.type != LIGHT_TYPE_POINT || !near(light.intensity, 4.5f) ||
        !near(light.radius, 12.0f)) {
        fail("setters changed unrelated fields");
    }
    expect_vec(light.position, 1.5f, -2.0f, 3.25f, "position after setters");

    printf("PASS: point light create and source parameters\n");
    return 0;
}
