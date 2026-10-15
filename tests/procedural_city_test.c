/*
 * procedural_city_test.c
 * City config, generator lifecycle, and building footprint/facade.
 * Compiles procedural_city.c directly and does not link Engine.
 */

#include "procedural_city.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void expect_true(bool cond, const char *msg) {
    if (!cond) {
        fail(msg);
    }
}

static int near(float a, float b) {
    float d = a - b;
    if (d < 0.0f) {
        d = -d;
    }
    return d < 0.001f;
}

static void test_config(void) {
    CityConfig config;
    CityConfig preset;

    expect_true(!city_config_validate(NULL), "null config");
    expect_true(!city_config_from_preset(NULL, CITY_STYLE_MODERN, 100.0f), "null preset");

    memset(&config, 0, sizeof(config));
    expect_true(!city_config_validate(&config), "zero size");

    config.size_x = 10.0f;
    config.size_z = -1.0f;
    config.density = 0.5f;
    expect_true(!city_config_validate(&config), "negative depth");

    config.size_z = 10.0f;
    config.density = -0.01f;
    expect_true(!city_config_validate(&config), "negative density");

    config.density = 1.01f;
    expect_true(!city_config_validate(&config), "density above one");

    config.density = 0.0f;
    expect_true(city_config_validate(&config), "density zero");
    config.density = 1.0f;
    expect_true(city_config_validate(&config), "density one");

    expect_true(city_config_from_preset(&preset, CITY_STYLE_MEDIEVAL, 200.0f), "preset");
    expect_true(preset.style == CITY_STYLE_MEDIEVAL, "preset style");
    expect_true(preset.size_x == 200.0f && preset.size_z == 200.0f, "preset size");
    expect_true(preset.population_target == 400u, "preset population");
    expect_true(preset.density == 0.5f, "preset density");
    expect_true(preset.random_seed == 12345u, "preset seed");
    expect_true(preset.min_floors == 1u && preset.max_floors == 5u, "preset floors");
    expect_true(preset.floor_height == 3.5f, "preset floor height");
    expect_true(city_config_validate(&preset), "preset is valid");

    expect_true(city_config_from_preset(&preset, CITY_STYLE_FUTURISTIC, 0.0f), "zero size preset");
    expect_true(!city_config_validate(&preset), "zero size preset is invalid");
    expect_true(!city_config_serialize(&preset, NULL, 0), "serialize stub");
    expect_true(!city_config_deserialize(&preset, "{}"), "deserialize stub");
}

static void test_generator(void) {
    CityGenerator generator;
    CityConfig config;

    expect_true(city_config_from_preset(&config, CITY_STYLE_FANTASY, 80.0f), "gen preset");
    expect_true(!city_generator_init(NULL, &config), "null generator");
    expect_true(!city_generator_init(&generator, NULL), "null gen config");

    expect_true(city_generator_init(&generator, &config), "init");
    expect_true(generator.config.style == CITY_STYLE_FANTASY, "copied style");
    expect_true(generator.config.random_seed == 12345u, "copied seed");
    expect_true(!generator.is_generating, "not generating");
    expect_true(generator.generation_progress == 0.0f, "progress field");
    expect_true(city_generator_get_progress(&generator) == 0.0f, "progress api");
    expect_true(generator.block_count == 0u && generator.blocks == NULL, "no blocks yet");
    expect_true(generator.building_count == 0u && generator.buildings == NULL, "no buildings yet");
    expect_true(generator.roads.node_capacity == 1000u, "node capacity");
    expect_true(generator.roads.segment_capacity == 1000u, "segment capacity");
    expect_true(generator.roads.node_count == 0u && generator.roads.segment_count == 0u, "empty roads");
    expect_true(generator.roads.nodes != NULL && generator.roads.segments != NULL, "road storage");

    expect_true(!city_generator_generate(&generator), "generate stub");
    expect_true(!city_generator_generate_async(&generator), "async stub");
    city_generator_cancel(&generator);
    expect_true(!city_generator_preview(&generator, NULL), "preview stub");
    expect_true(!road_network_generate_grid(&generator.roads, &config), "grid stub");
    expect_true(!block_fill_with_buildings(NULL, &config), "fill stub");

    city_generator_shutdown(&generator);
    expect_true(generator.roads.nodes == NULL && generator.roads.segments == NULL, "roads freed");
    expect_true(generator.config.size_x == 0.0f, "generator cleared");
    city_generator_shutdown(&generator);
    city_generator_shutdown(NULL);
    city_generator_cancel(NULL);
}

static void test_buildings(void) {
    BuildingFootprint footprint;
    ProceduralBuilding building;
    CityBlock block;
    float max_area = 70.0f;
    float target = max_area * 0.7f;
    float side = sqrtf(target);
    ZoneType zone;

    expect_true(!building_generate_footprint(NULL, BUILDING_HOUSE, NULL, max_area), "null footprint");
    expect_true(building_generate_footprint(&footprint, BUILDING_SHOP, NULL, max_area), "footprint");
    expect_true(footprint.vertex_count == 4u, "quad footprint");
    expect_true(near(footprint.vertices[0][0], -side / 2.0f), "v0 x");
    expect_true(near(footprint.vertices[0][1], -side / 2.0f), "v0 z");
    expect_true(near(footprint.vertices[1][0], side / 2.0f), "v1 x");
    expect_true(near(footprint.vertices[1][1], -side / 2.0f), "v1 z");
    expect_true(near(footprint.vertices[2][0], side / 2.0f), "v2 x");
    expect_true(near(footprint.vertices[2][1], side / 2.0f), "v2 z");
    expect_true(near(footprint.vertices[3][0], -side / 2.0f), "v3 x");
    expect_true(near(footprint.vertices[3][1], side / 2.0f), "v3 z");
    expect_true(footprint.min_height == 0.0f && footprint.max_height == 10.0f, "footprint height");

    expect_true(!building_extrude_floors(NULL), "null extrude");
    memset(&building, 0, sizeof(building));
    building.footprint = footprint;
    building.floor_height = 3.5f;
    expect_true(building_extrude_floors(&building), "extrude empty");
    expect_true(building.floors == 1u, "default one floor");
    building.floors = 4u;
    expect_true(building_extrude_floors(&building), "extrude kept");
    expect_true(building.floors == 4u, "floor count kept");

    expect_true(!building_place_windows(NULL), "null windows");
    expect_true(!building_place_doors(NULL), "null doors");
    building.footprint.vertex_count = 1u;
    expect_true(!building_place_windows(&building), "windows need an edge");
    expect_true(!building_place_doors(&building), "doors need an edge");
    building.footprint.vertex_count = 4u;
    expect_true(building_place_windows(&building), "place windows");
    expect_true(building_place_doors(&building), "place doors");
    expect_true(!building_generate_roof(&building), "roof stub");
    expect_true(!building_place_balconies(&building), "balcony stub");

    memset(&block, 0, sizeof(block));
    zone = block_assign_zone(&block, NULL, NULL, 0);
    expect_true(zone == ZONE_RESIDENTIAL, "default zone");
    expect_true(!block_from_road_polygon(&block, NULL, 0), "polygon stub");
    expect_true(!block_subdivide(&block, 2), "subdivide stub");
}

int main(void) {
    test_config();
    test_generator();
    test_buildings();
    printf("procedural_city_test: ok\n");
    return 0;
}
