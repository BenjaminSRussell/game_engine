/*
 * spot_light_test.c
 * Spot light create, IES profile, and source radius.
 * Compiles spot_light.c directly and does not link Engine.
 */

#include "lighting/sources/spot_light.h"

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
    return d <= 0.0001f;
}

static simd_float3 v3(float x, float y, float z) {
    simd_float3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

static void expect_v3(simd_float3 v, float x, float y, float z, const char *msg) {
    if (!near(v.x, x) || !near(v.y, y) || !near(v.z, z)) {
        fail(msg);
    }
}

int main(void) {
    light_t light;
    light_t unshadowed;

    light = spot_light_create(v3(1.0f, 2.0f, 3.0f), v3(0.0f, -1.0f, 0.0f),
                              v3(0.2f, 0.4f, 0.6f), 4.5f, 12.0f, 0.3f, 0.8f,
                              true);

    expect_v3(light.position, 1.0f, 2.0f, 3.0f, "position");
    expect_v3(light.direction, 0.0f, -1.0f, 0.0f, "direction");
    expect_v3(light.color, 0.2f, 0.4f, 0.6f, "color");
    if (!near(light.intensity, 4.5f)) {
        fail("intensity");
    }
    if (!near(light.radius, 12.0f)) {
        fail("radius");
    }
    if (!near(light.inner_cone, 0.3f)) {
        fail("inner_cone");
    }
    if (!near(light.outer_cone, 0.8f)) {
        fail("outer_cone");
    }
    if (!light.cast_shadows) {
        fail("cast_shadows");
    }
    if (light.type != LIGHT_TYPE_SPOT) {
        fail("type");
    }
    if (light.shadow_map_index != 0) {
        fail("shadow_map_index");
    }
    if (light.ies_texture_id != 0) {
        fail("ies default");
    }
    if (!near(light.source_radius, 0.0f)) {
        fail("source_radius default");
    }
    if (!near(light.source_length, 0.0f)) {
        fail("source_length default");
    }

    spot_light_set_ies_profile(NULL, 9);
    spot_light_set_source_radius(NULL, 3.0f);
    if (light.ies_texture_id != 0 || !near(light.source_radius, 0.0f)) {
        fail("null setters");
    }

    spot_light_set_ies_profile(&light, 42);
    if (light.ies_texture_id != 42 || !near(light.source_radius, 0.0f) ||
        light.type != LIGHT_TYPE_SPOT) {
        fail("ies profile");
    }

    spot_light_set_source_radius(&light, 1.25f);
    if (!near(light.source_radius, 1.25f) || light.ies_texture_id != 42) {
        fail("source radius");
    }
    expect_v3(light.position, 1.0f, 2.0f, 3.0f, "position stable");

    spot_light_set_source_radius(&light, 0.0f);
    if (!near(light.source_radius, 0.0f)) {
        fail("zero radius");
    }

    unshadowed = spot_light_create(v3(0.0f, 0.0f, 0.0f), v3(0.0f, 0.0f, 1.0f),
                                   v3(1.0f, 1.0f, 1.0f), 1.0f, 1.0f, 0.1f, 0.2f,
                                   false);
    if (unshadowed.cast_shadows || unshadowed.type != LIGHT_TYPE_SPOT) {
        fail("unshadowed");
    }

    printf("ok\n");
    return 0;
}
