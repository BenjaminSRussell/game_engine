/*
 * procedural_foliage_test.c
 * Placement, density, terrain constraints, scale, and yaw for
 * procedural_foliage.c. Compiles that file directly and does not link Engine.
 */

#include "environment/procedural_foliage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_failures = 0;

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    g_failures++;
}

static void expect_true(int cond, const char *msg) {
    if (!cond) {
        fail(msg);
    }
}

#define MAX_CAPTURED 64

typedef struct {
    char mesh[128];
    Vec3 pos;
    Vec3 scale;
    f32 rot;
} Captured;

static Captured g_captured[MAX_CAPTURED];
static int g_captured_count;
static int g_callback_count;

static void reset_capture(void) {
    memset(g_captured, 0, sizeof(g_captured));
    g_captured_count = 0;
    g_callback_count = 0;
}

static void capture_cb(const char *mesh, Vec3 pos, Vec3 scale, f32 rot) {
    g_callback_count++;
    if (g_captured_count >= MAX_CAPTURED) {
        return;
    }
    if (mesh) {
        snprintf(g_captured[g_captured_count].mesh,
                 sizeof(g_captured[g_captured_count].mesh), "%s", mesh);
    }
    g_captured[g_captured_count].pos = pos;
    g_captured[g_captured_count].scale = scale;
    g_captured[g_captured_count].rot = rot;
    g_captured_count++;
}

static FoliageType make_type(const char *mesh, f32 density) {
    FoliageType type;
    memset(&type, 0, sizeof(type));
    snprintf(type.mesh_path, sizeof(type.mesh_path), "%s", mesh);
    type.density = density;
    type.min_scale = 1.0f;
    type.max_scale = 1.0f;
    type.lock_scale_xy = true;
    type.random_yaw = false;
    type.min_slope = 0.0f;
    type.max_slope = 90.0f;
    type.min_height = -1000.0f;
    type.max_height = 1000.0f;
    return type;
}

static ProceduralFoliageSpawner *make_spawner(f32 width, f32 length, u32 seed) {
    ProceduralFoliageSpawner *spawner = foliage_spawner_create();
    if (!spawner) {
        fail("create");
        exit(1);
    }
    spawner->width = width;
    spawner->length = length;
    spawner->random_seed = seed;
    return spawner;
}

static int nearly_equal(f32 a, f32 b) {
    f32 d = a - b;
    if (d < 0.0f) {
        d = -d;
    }
    return d <= 0.0001f;
}

static f32 height_constant(f32 x, f32 z, void *user_data) {
    (void)x;
    (void)z;
    return *(const f32 *)user_data;
}

static void normal_up(f32 x, f32 z, Vec3 *out_normal, void *user_data) {
    (void)x;
    (void)z;
    (void)user_data;
    out_normal->x = 0.0f;
    out_normal->y = 1.0f;
    out_normal->z = 0.0f;
}

static void normal_flat_60(f32 x, f32 z, Vec3 *out_normal, void *user_data) {
    (void)x;
    (void)z;
    (void)user_data;
    /* cos(60 deg) = 0.5, so the slope angle is about 60 degrees. */
    out_normal->x = 0.0f;
    out_normal->y = 0.5f;
    out_normal->z = 0.8660254f;
}

static void normal_horizontal(f32 x, f32 z, Vec3 *out_normal, void *user_data) {
    (void)x;
    (void)z;
    (void)user_data;
    out_normal->x = 0.0f;
    out_normal->y = 0.0f;
    out_normal->z = 1.0f;
}

static void expect_inside_rect(const ProceduralFoliageSpawner *spawner, Vec3 pos,
                               const char *msg) {
    expect_true(pos.x >= spawner->origin.x &&
                    pos.x <= spawner->origin.x + spawner->width,
                msg);
    expect_true(pos.z >= spawner->origin.z &&
                    pos.z <= spawner->origin.z + spawner->length,
                msg);
}

static void test_create_defaults(void) {
    ProceduralFoliageSpawner *spawner = foliage_spawner_create();
    expect_true(spawner != NULL, "create returns spawner");
    if (!spawner) {
        return;
    }
    expect_true(spawner->type_count == 0, "type_count starts at 0");
    expect_true(spawner->random_seed == 12345, "default seed");
    expect_true(spawner->tile_size == 64, "default tile size");
    expect_true(spawner->width == 100.0f, "default width");
    expect_true(spawner->length == 100.0f, "default length");
    expect_true(spawner->origin.x == 0.0f && spawner->origin.y == 0.0f &&
                    spawner->origin.z == 0.0f,
                "origin zeroed");
    expect_true(spawner->get_height == NULL && spawner->get_normal == NULL,
                "no terrain callbacks");
    free(spawner);
}

static void test_add_type_cap_and_copy(void) {
    ProceduralFoliageSpawner *spawner = make_spawner(8.0f, 4.0f, 7);
    FoliageType type = make_type("oak", 0.5f);
    FoliageType extra;
    u32 i;

    foliage_spawner_add_type(NULL, type);
    foliage_spawner_add_type(spawner, type);
    expect_true(spawner->type_count == 1, "one type stored");
    snprintf(type.mesh_path, sizeof(type.mesh_path), "changed");
    expect_true(strcmp(spawner->types[0].mesh_path, "oak") == 0,
                "type is copied by value");

    for (i = 1; i < MAX_FOLIAGE_TYPES; i++) {
        char name[32];
        snprintf(name, sizeof(name), "fill-%u", i);
        foliage_spawner_add_type(spawner, make_type(name, 0.0f));
    }
    expect_true(spawner->type_count == MAX_FOLIAGE_TYPES, "filled to cap");
    extra = make_type("overflow", 10.0f);
    foliage_spawner_add_type(spawner, extra);
    expect_true(spawner->type_count == MAX_FOLIAGE_TYPES, "cap rejects extra type");
    expect_true(strcmp(spawner->types[MAX_FOLIAGE_TYPES - 1].mesh_path,
                       "overflow") != 0,
                "overflow type is not stored");
    free(spawner);
}

static void test_null_and_empty(void) {
    ProceduralFoliageSpawner *spawner = make_spawner(8.0f, 4.0f, 1);
    reset_capture();
    foliage_spawner_simulate(NULL, 4, capture_cb);
    foliage_spawner_simulate(spawner, 4, NULL);
    foliage_spawner_simulate(spawner, 4, capture_cb);
    expect_true(g_callback_count == 0, "no types places nothing");
    free(spawner);
}

static void test_flat_density_and_determinism(void) {
    ProceduralFoliageSpawner *a;
    ProceduralFoliageSpawner *b;
    Captured first[16];
    int i;
    const int expect_count = 16; /* 8 * 4 * 0.5 */

    a = make_spawner(8.0f, 4.0f, 99);
    a->origin.x = 3.0f;
    a->origin.y = 50.0f;
    a->origin.z = -2.0f;
    foliage_spawner_add_type(a, make_type("grass", 0.5f));
    reset_capture();
    foliage_spawner_simulate(a, 3, capture_cb);
    expect_true(g_callback_count == expect_count, "density sets instance count");
    expect_true(g_captured_count == expect_count, "all instances captured");
    for (i = 0; i < g_captured_count; i++) {
        expect_true(strcmp(g_captured[i].mesh, "grass") == 0, "mesh path");
        expect_true(nearly_equal(g_captured[i].pos.y, 0.0f), "flat height is 0");
        expect_inside_rect(a, g_captured[i].pos, "instance inside area");
        expect_true(nearly_equal(g_captured[i].scale.x, 1.0f) &&
                        nearly_equal(g_captured[i].scale.y, 1.0f) &&
                        nearly_equal(g_captured[i].scale.z, 1.0f),
                    "fixed scale");
        expect_true(nearly_equal(g_captured[i].rot, 0.0f), "yaw locked at 0");
    }
    memcpy(first, g_captured, sizeof(first));

    reset_capture();
    foliage_spawner_simulate(a, 9, capture_cb);
    expect_true(g_callback_count == expect_count, "reseeded rerun count");
    expect_true(memcmp(first, g_captured, sizeof(first)) == 0, "same seed is deterministic");

    b = make_spawner(8.0f, 4.0f, 100);
    b->origin = a->origin;
    foliage_spawner_add_type(b, make_type("grass", 0.5f));
    reset_capture();
    foliage_spawner_simulate(b, 3, capture_cb);
    expect_true(g_callback_count == expect_count, "other seed count");
    expect_true(memcmp(first, g_captured, sizeof(first)) != 0, "different seed differs");

    free(a);
    free(b);
}

static void test_height_and_slope_constraints(void) {
    ProceduralFoliageSpawner *spawner = make_spawner(8.0f, 4.0f, 5);
    FoliageType type = make_type("pine", 0.5f);
    f32 height = 12.0f;

    type.min_height = 1.0f;
    type.max_height = 10.0f;
    foliage_spawner_add_type(spawner, type);
    reset_capture();
    foliage_spawner_simulate(spawner, 2, capture_cb);
    expect_true(g_callback_count == 0, "flat y=0 outside height range");

    spawner->types[0].min_height = -1.0f;
    spawner->types[0].max_height = 1.0f;
    height = 40.0f;
    spawner->get_height = height_constant;
    spawner->terrain_user_data = &height;
    reset_capture();
    foliage_spawner_simulate(spawner, 2, capture_cb);
    expect_true(g_callback_count == 16, "one terrain callback falls back to flat");
    expect_true(nearly_equal(g_captured[0].pos.y, 0.0f), "fallback height is 0");

    spawner->get_normal = normal_up;
    reset_capture();
    foliage_spawner_simulate(spawner, 2, capture_cb);
    expect_true(g_callback_count == 0, "terrain height outside range");

    height = 0.25f;
    spawner->types[0].min_height = 0.0f;
    spawner->types[0].max_height = 1.0f;
    spawner->types[0].min_slope = 0.0f;
    spawner->types[0].max_slope = 45.0f;
    reset_capture();
    foliage_spawner_simulate(spawner, 2, capture_cb);
    expect_true(g_callback_count == 16, "flat terrain slope accepted");
    expect_true(nearly_equal(g_captured[0].pos.y, 0.25f), "uses terrain height");
    expect_true(nearly_equal(g_captured[15].pos.y, 0.25f), "every sample uses terrain height");

    spawner->types[0].min_slope = 10.0f;
    reset_capture();
    foliage_spawner_simulate(spawner, 2, capture_cb);
    expect_true(g_callback_count == 0, "slope below minimum rejected");

    spawner->types[0].min_slope = 0.0f;
    spawner->types[0].max_slope = 30.0f;
    spawner->get_normal = normal_flat_60;
    reset_capture();
    foliage_spawner_simulate(spawner, 2, capture_cb);
    expect_true(g_callback_count == 0, "steep slope rejected");

    spawner->get_normal = normal_horizontal;
    spawner->types[0].max_slope = 89.0f;
    reset_capture();
    foliage_spawner_simulate(spawner, 2, capture_cb);
    expect_true(g_callback_count == 0, "vertical surface rejected");

    spawner->get_normal = normal_up;
    spawner->types[0].max_slope = 90.0f;
    reset_capture();
    foliage_spawner_simulate(spawner, 2, capture_cb);
    expect_true(g_callback_count == 16, "up normal accepted again");

    free(spawner);
}

static void test_scale_and_yaw(void) {
    ProceduralFoliageSpawner *spawner = make_spawner(8.0f, 4.0f, 42);
    FoliageType type = make_type("bush", 0.5f);
    int i;
    int unlocked_y = 0;
    int yawed = 0;

    type.min_scale = 1.0f;
    type.max_scale = 3.0f;
    type.lock_scale_xy = true;
    type.random_yaw = false;
    foliage_spawner_add_type(spawner, type);
    reset_capture();
    foliage_spawner_simulate(spawner, 1, capture_cb);
    expect_true(g_callback_count == 16, "scale test count");
    for (i = 0; i < g_captured_count; i++) {
        expect_true(g_captured[i].scale.x >= 1.0f && g_captured[i].scale.x <= 3.0f,
                    "locked scale in range");
        expect_true(g_captured[i].scale.x == g_captured[i].scale.y &&
                        g_captured[i].scale.y == g_captured[i].scale.z,
                    "lock_scale_xy keeps axes equal");
        expect_true(g_captured[i].rot == 0.0f, "random_yaw off");
    }

    spawner->types[0].lock_scale_xy = false;
    spawner->types[0].random_yaw = true;
    reset_capture();
    foliage_spawner_simulate(spawner, 1, capture_cb);
    for (i = 0; i < g_captured_count; i++) {
        expect_true(g_captured[i].scale.x == g_captured[i].scale.z, "xz share base scale");
        expect_true(g_captured[i].scale.y >= 1.0f && g_captured[i].scale.y <= 3.0f,
                    "unlocked y in range");
        expect_true(g_captured[i].rot >= 0.0f && g_captured[i].rot <= 360.0f, "yaw range");
        if (g_captured[i].scale.y != g_captured[i].scale.x) {
            unlocked_y = 1;
        }
        if (g_captured[i].rot != 0.0f) {
            yawed = 1;
        }
    }
    expect_true(unlocked_y, "unlocked y is sampled separately");
    expect_true(yawed, "random yaw is applied");
    free(spawner);
}

static void test_multiple_types_and_zero_density(void) {
    ProceduralFoliageSpawner *spawner = make_spawner(8.0f, 4.0f, 3);
    int ferns = 0;
    int rocks = 0;
    int i;

    foliage_spawner_add_type(spawner, make_type("none", 0.0f));
    foliage_spawner_add_type(spawner, make_type("fern", 0.25f)); /* 8 * 4 * 0.25 = 8 */
    foliage_spawner_add_type(spawner, make_type("rock", 0.5f));  /* 16 */
    reset_capture();
    foliage_spawner_simulate(spawner, 6, capture_cb);
    expect_true(g_callback_count == 24, "per-type density sums");
    for (i = 0; i < g_captured_count; i++) {
        if (strcmp(g_captured[i].mesh, "fern") == 0) {
            ferns++;
        } else if (strcmp(g_captured[i].mesh, "rock") == 0) {
            rocks++;
        } else {
            fail("unexpected mesh");
        }
    }
    expect_true(ferns == 8, "fern count");
    expect_true(rocks == 16, "rock count");
    expect_true(strcmp(g_captured[0].mesh, "fern") == 0, "types simulate in order");
    expect_true(strcmp(g_captured[8].mesh, "rock") == 0, "second type follows first");
    free(spawner);
}

static void test_placement_cap(void) {
    ProceduralFoliageSpawner *spawner = make_spawner(250.0f, 400.0f, 11);
    FoliageType type = make_type("cap", 2.0f); /* area 100000 * 2 = 200000 */
    foliage_spawner_add_type(spawner, type);
    reset_capture();
    foliage_spawner_simulate(spawner, 1, capture_cb);
    expect_true(g_callback_count == 100000, "target count is capped at 100000");
    free(spawner);
}

int main(void) {
    test_create_defaults();
    test_add_type_cap_and_copy();
    test_null_and_empty();
    test_flat_density_and_determinism();
    test_height_and_slope_constraints();
    test_scale_and_yaw();
    test_multiple_types_and_zero_density();
    test_placement_cap();
    if (g_failures != 0) {
        fprintf(stderr, "%d assertion(s) failed\n", g_failures);
        return 1;
    }
    printf("procedural foliage tests passed\n");
    return 0;
}
