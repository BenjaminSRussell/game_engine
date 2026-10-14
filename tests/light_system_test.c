/*
 * light_system_test.c
 * Create, look up, destroy, and enumerate lights. Compiles light_system.c
 * directly and does not link Engine.
 */

#include "lighting/light_system.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_LIGHTS 1024

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

static void expect_inactive(const char *label) {
    uint32_t count = 99;
    if (light_system_create_light(LIGHT_TYPE_POINT) != 0) {
        fprintf(stderr, "FAIL: %s create\n", label);
        exit(1);
    }
    if (light_system_get_light(1) != NULL) {
        fprintf(stderr, "FAIL: %s get\n", label);
        exit(1);
    }
    if (light_system_get_all_lights(&count) != NULL || count != 0) {
        fprintf(stderr, "FAIL: %s get_all\n", label);
        exit(1);
    }
}

int main(void) {
    uint32_t dir_id;
    uint32_t point_id;
    uint32_t spot_id;
    uint32_t area_id;
    uint32_t again_id;
    uint32_t count;
    uint32_t i;
    uint32_t created;
    const Light *all;
    Light *directional;
    Light *point;
    Light *spot;
    int saw_point;

    expect_inactive("before init");
    light_system_update(0.016f);
    light_system_destroy_light(1);
    light_system_prune();
    light_system_shutdown();

    if (!light_system_init() || !light_system_init()) {
        fail("init");
    }

    dir_id = light_system_create_light(LIGHT_TYPE_DIRECTIONAL);
    point_id = light_system_create_light(LIGHT_TYPE_POINT);
    spot_id = light_system_create_light(LIGHT_TYPE_SPOT);
    area_id = light_system_create_light(LIGHT_TYPE_AREA);
    if (dir_id == 0 || point_id == 0 || spot_id == 0 || area_id == 0) {
        fail("create");
    }
    if (dir_id == point_id || point_id == spot_id || spot_id == area_id ||
        dir_id == area_id) {
        fail("ids not unique");
    }
    if (!light_system_init() || light_system_get_light(dir_id) == NULL) {
        fail("reinit cleared state");
    }

    directional = light_system_get_light(dir_id);
    if (directional->base.id != dir_id ||
        directional->base.type != LIGHT_TYPE_DIRECTIONAL ||
        !directional->base.enabled || directional->base.cast_shadows ||
        directional->base.shadow_map_resolution != 1024 ||
        !near(directional->base.intensity, 1.0f) ||
        !near(directional->base.color.x, 1.0f) ||
        !near(directional->base.color.y, 1.0f) ||
        !near(directional->base.color.z, 1.0f) ||
        !near(directional->base.shadow_bias, 0.005f) ||
        !near(directional->directional.direction.x, 0.0f) ||
        !near(directional->directional.direction.y, -1.0f) ||
        !near(directional->directional.direction.z, 0.0f)) {
        fail("directional defaults");
    }

    point = light_system_get_light(point_id);
    if (point == NULL || point->base.type != LIGHT_TYPE_POINT ||
        !near(point->point.range, 10.0f) ||
        !near(point->point.constant_attenuation, 1.0f) ||
        !near(point->point.linear_attenuation, 0.09f) ||
        !near(point->point.quadratic_attenuation, 0.032f)) {
        fail("point defaults");
    }

    spot = light_system_get_light(spot_id);
    if (spot == NULL || spot->base.type != LIGHT_TYPE_SPOT ||
        !near(spot->spot.range, 10.0f) ||
        !near(spot->spot.inner_cone_angle, 0.5f) ||
        !near(spot->spot.outer_cone_angle, 0.7f) ||
        !near(spot->spot.direction.y, -1.0f) ||
        !near(spot->spot.constant_attenuation, 1.0f)) {
        fail("spot defaults");
    }
    if (light_system_get_light(area_id) == NULL ||
        light_system_get_light(area_id)->base.type != LIGHT_TYPE_AREA) {
        fail("area type");
    }

    point->base.intensity = 3.5f;
    point->base.enabled = false;
    point->point.position = (Vec3){1.0f, 2.0f, 3.0f};
    point = light_system_get_light(point_id);
    if (point == NULL || point->base.enabled || !near(point->base.intensity, 3.5f) ||
        !near(point->point.position.x, 1.0f) ||
        !near(point->point.position.y, 2.0f) ||
        !near(point->point.position.z, 3.0f)) {
        fail("mutation");
    }

    if (light_system_get_light(0) != NULL || light_system_get_light(99999) != NULL) {
        fail("missing id");
    }

    all = light_system_get_all_lights(&count);
    if (all == NULL || count != 4) {
        fail("packed count");
    }
    saw_point = 0;
    for (i = 0; i < count; i++) {
        if (all[i].base.id == point_id) {
            saw_point = 1;
            if (all[i].base.enabled || !near(all[i].base.intensity, 3.5f)) {
                fail("packed point");
            }
        }
        if (all[i].base.id == 0) {
            fail("packed zero id");
        }
    }
    if (!saw_point) {
        fail("packed missing point");
    }
    if (light_system_get_all_lights(NULL) == NULL) {
        fail("null out_count");
    }

    light_system_update(1.0f);
    light_system_prune();
    all = light_system_get_all_lights(&count);
    if (all == NULL || count != 4) {
        fail("prune dropped a live light");
    }

    light_system_destroy_light(point_id);
    light_system_destroy_light(point_id);
    light_system_destroy_light(0);
    if (light_system_get_light(point_id) != NULL) {
        fail("destroyed light still live");
    }
    all = light_system_get_all_lights(&count);
    if (all == NULL || count != 3) {
        fail("count after destroy");
    }
    for (i = 0; i < count; i++) {
        if (all[i].base.id == point_id) {
            fail("destroyed light still packed");
        }
    }

    again_id = light_system_create_light(LIGHT_TYPE_SKY);
    if (again_id == 0 || again_id == point_id ||
        light_system_get_light(again_id) == NULL ||
        light_system_get_light(again_id)->base.type != LIGHT_TYPE_SKY) {
        fail("recreate in free slot");
    }
    if (light_system_get_light(dir_id) == NULL) {
        fail("other lights lost");
    }

    light_system_shutdown();
    expect_inactive("after shutdown");
    if (!light_system_init()) {
        fail("init after shutdown");
    }

    for (created = 0; created < MAX_LIGHTS; created++) {
        if (light_system_create_light(LIGHT_TYPE_POINT) == 0) {
            fail("fill");
        }
    }
    if (light_system_create_light(LIGHT_TYPE_POINT) != 0) {
        fail("over capacity");
    }
    all = light_system_get_all_lights(&count);
    if (all == NULL || count != MAX_LIGHTS) {
        fail("full count");
    }
    light_system_destroy_light(all[0].base.id);
    if (light_system_create_light(LIGHT_TYPE_DIRECTIONAL) == 0) {
        fail("create after a slot frees");
    }
    if (light_system_create_light(LIGHT_TYPE_POINT) != 0) {
        fail("still full");
    }

    light_system_shutdown();
    light_system_shutdown();
    printf("PASS: light_system core\n");
    return 0;
}
