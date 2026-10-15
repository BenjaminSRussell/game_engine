/*
 * sky_light_test.c
 * Sky light create defaults and rotation/contrast setters.
 * Compiles sky_light.c directly and does not link Engine.
 */

#include "lighting/sources/sky_light.h"

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

static int nearf(float a, float b) {
    float d = a - b;
    if (d < 0.0f) {
        d = -d;
    }
    return d <= 0.00001f;
}

static void expect_vec(Vec3 v, float x, float y, float z, const char *msg) {
    if (!nearf(v.x, x) || !nearf(v.y, y) || !nearf(v.z, z)) {
        fprintf(stderr, "FAIL: %s got (%f, %f, %f)\n", msg, v.x, v.y, v.z);
        exit(1);
    }
}

int main(void) {
    Vec3 tint;
    SkyLight light;
    SkyLight unshadowed;

    tint = vec3_create(0.2f, 0.4f, 0.8f);
    light = sky_light_create(42u, tint, 1.5f, true);

    expect_true(light.base.type == LIGHT_TYPE_SKY, "type is sky");
    expect_true(light.base.enabled, "created enabled");
    expect_true(light.base.cast_shadows, "cast shadows");
    expect_true(light.base.id == 0u, "id left unset");
    expect_true(nearf(light.base.intensity, 1.5f), "intensity");
    expect_vec(light.base.color, 0.2f, 0.4f, 0.8f, "base color");
    expect_true(light.cubemap_texture_id == 42u, "cubemap id");
    expect_vec(light.tint_color, 0.2f, 0.4f, 0.8f, "tint");
    expect_true(nearf(light.rotation, 0.0f), "default rotation");
    expect_true(nearf(light.contrast, 1.0f), "default contrast");
    expect_true(!light.real_time_capture, "capture starts off");
    expect_true(nearf(light.base.shadow_bias, 0.0f), "shadow bias cleared");
    expect_true(light.base.shadow_map_resolution == 0u, "shadow map cleared");
    printf("PASS: create sky light\n");

    unshadowed = sky_light_create(7u, vec3_create(1.0f, 1.0f, 1.0f), 0.0f, false);
    expect_true(!unshadowed.base.cast_shadows, "shadows off");
    expect_true(unshadowed.base.enabled, "still enabled");
    expect_true(unshadowed.base.type == LIGHT_TYPE_SKY, "unshadowed type");
    expect_true(nearf(unshadowed.base.intensity, 0.0f), "zero intensity");
    expect_true(unshadowed.cubemap_texture_id == 7u, "cubemap 7");
    expect_vec(unshadowed.tint_color, 1.0f, 1.0f, 1.0f, "white tint");
    expect_vec(unshadowed.base.color, 1.0f, 1.0f, 1.0f, "white base color");
    printf("PASS: create without shadows\n");

    sky_light_set_rotation(&light, 90.0f);
    expect_true(nearf(light.rotation, 90.0f), "rotation set");
    expect_true(nearf(light.contrast, 1.0f), "contrast unchanged by rotation");
    expect_true(light.cubemap_texture_id == 42u, "cubemap unchanged");
    expect_true(nearf(light.base.intensity, 1.5f), "intensity unchanged");
    printf("PASS: set rotation\n");

    sky_light_set_contrast(&light, 2.5f);
    expect_true(nearf(light.contrast, 2.5f), "contrast set");
    expect_true(nearf(light.rotation, 90.0f), "rotation unchanged by contrast");
    expect_vec(light.tint_color, 0.2f, 0.4f, 0.8f, "tint unchanged");
    printf("PASS: set contrast\n");

    sky_light_set_rotation(NULL, 12.0f);
    sky_light_set_contrast(NULL, 4.0f);
    expect_true(nearf(light.rotation, 90.0f), "null rotation ignored");
    expect_true(nearf(light.contrast, 2.5f), "null contrast ignored");
    printf("PASS: null setters\n");

    sky_light_set_rotation(&light, -15.0f);
    expect_true(nearf(light.rotation, -15.0f), "negative rotation stored");
    sky_light_set_contrast(&light, 0.0f);
    expect_true(nearf(light.contrast, 0.0f), "zero contrast stored");
    printf("PASS: sky light core\n");
    return 0;
}
