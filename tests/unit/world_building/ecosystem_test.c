/*
 * Unit test for the ecosystem simulation (src/engine/scene/world_building/ecosystem/ecosystem.c).
 * Issue #401.
 *
 * ecosystem.c is compiled directly into this test (no Engine link), so the test runs on hosts
 * where Engine does not build. It covers the species database, the vegetation lifecycle, creature
 * needs and fleeing, the manager (time, chunk update, population balance, initial spawn) and
 * serialization.
 *
 * Not covered: the TODO stubs in ecosystem.c (creature AI, species reintroduction and seasonal
 * shelter seeking; issues #135 to #137), and the placeholder water and shelter searches, which are
 * checked only against their contract (a returned point lies within the search radius).
 *
 * Vegetation types are registered once, at the start of main, so their ids are 1 (grass) and 2
 * (weed). The registry assigns ids sequentially from 1 and does not return them.
 */

#include "ecosystem.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_failures = 0;

#define CHECK(cond)                                                                 \
    do {                                                                            \
        if (!(cond)) {                                                              \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            g_failures++;                                                           \
        }                                                                           \
    } while (0)

#define CHECK_NEAR(actual, expected, eps) \
    CHECK(fabsf((float)(actual) - (float)(expected)) <= (float)(eps))

static void set_name(char *dst, size_t dst_size, const char *src) {
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

static WildlifeSpecies make_species(const char *name, uint32_t biome) {
    WildlifeSpecies s;
    memset(&s, 0, sizeof s);
    set_name(s.name, sizeof s.name, name);
    s.behavior = BEHAVIOR_NEUTRAL;
    s.fear_radius = 10.0f;
    s.chase_radius = 15.0f;
    s.flee_speed = 8.0f;
    s.walk_speed = 2.0f;
    s.run_speed = 6.0f;
    s.health = 100.0f;
    s.hunger_rate = 0.1f;
    s.thirst_rate = 0.2f;
    s.stamina = 100.0f;
    s.lifespan = 10.0f;
    s.preferred_biomes[0] = biome;
    s.biome_count = 1;
    return s;
}

static void setup_vegetation_types(void) {
    VegetationType grass;
    memset(&grass, 0, sizeof grass);
    set_name(grass.name, sizeof grass.name, "grass");
    grass.growth_rate = 1.0f;
    grass.max_age = 100.0f;
    grass.scale_min = 1.0f;
    grass.scale_max = 2.0f;
    grass.is_harvestable = true;
    grass.harvest_item = 42;
    grass.harvest_amount = 3.0f;
    grass.regrow_time = 2.0f;
    grass.density = 0.5f;

    VegetationType weed;
    memset(&weed, 0, sizeof weed);
    set_name(weed.name, sizeof weed.name, "weed");
    weed.growth_rate = 1.0f;
    weed.max_age = 100.0f;
    weed.scale_min = 1.0f;
    weed.scale_max = 1.0f;
    weed.spread_chance = 1000.0f; /* always passes the spread roll */
    weed.spread_radius = 0.0f;    /* seeds at the parent's own position */
    weed.density = 0.5f;

    CHECK(vegetation_type_register(&grass)); /* id 1 */
    CHECK(vegetation_type_register(&weed));  /* id 2 */

    const float origin[3] = {0.0f, 0.0f, 0.0f};
    VegetationInstance probe;
    CHECK(vegetation_spawn(&probe, 1, origin, 1.0f));
    CHECK(vegetation_spawn(&probe, 2, origin, 1.0f));
    CHECK(!vegetation_spawn(&probe, 3, origin, 1.0f));
}

static void test_species_database(void) {
    CHECK(species_database_init(0));

    WildlifeSpecies nameless = make_species("", 3);
    CHECK(!species_register(&nameless));

    WildlifeSpecies wolf = make_species("wolf", 3);
    WildlifeSpecies deer = make_species("deer", 3);
    WildlifeSpecies owl = make_species("owl", 5);
    CHECK(species_register(&wolf));
    CHECK(species_register(&deer));
    CHECK(species_register(&owl));
    CHECK(!species_register(&wolf)); /* duplicate name */

    const WildlifeSpecies *found[8];
    CHECK(species_find_by_biome(3, found, 8) == 2);
    CHECK(found[0] != NULL && strcmp(found[0]->name, "wolf") == 0);
    CHECK(found[1] != NULL && strcmp(found[1]->name, "deer") == 0);
    uint32_t wolf_id = found[0] != NULL ? found[0]->id : 0;
    CHECK(wolf_id != 0);
    CHECK(species_get(wolf_id) != NULL && strcmp(species_get(wolf_id)->name, "wolf") == 0);
    CHECK(species_get(0xFFFFFFFFu) == NULL);

    CHECK(species_find_by_biome(3, found, 1) == 1); /* max_results is honoured */
    CHECK(species_find_by_biome(99, found, 8) == 0);
    CHECK(species_find_by_biome(3, NULL, 8) == 0);

    const char *json = "{\"species\": [{\"name\": \"hare\", \"model_path\": \"models/hare.glb\"}]}";
    CHECK(species_load_json(json));
    CHECK(!species_load_json(json)); /* duplicate name */
    CHECK(!species_load_json("{\"creatures\": []}"));
    CHECK(!species_load_json(NULL));

    const WildlifeSpecies *hare[2];
    CHECK(species_find_by_biome(1, hare, 2) == 1); /* loaded species default to biome 1 */
    CHECK(hare[0] != NULL && strcmp(hare[0]->name, "hare") == 0);
    CHECK(hare[0] != NULL && strcmp(hare[0]->model_path, "models/hare.glb") == 0);
    CHECK(hare[0] != NULL && hare[0]->behavior == BEHAVIOR_NEUTRAL);
    CHECK(hare[0] != NULL && hare[0]->health == 100.0f);
}

static void test_vegetation(void) {
    const float pos[3] = {4.0f, 0.0f, 4.0f};

    /* spawn copies position and scale, and starts healthy and unharvested */
    VegetationInstance v;
    CHECK(!vegetation_spawn(NULL, 1, pos, 1.0f));
    CHECK(!vegetation_spawn(&v, 99, pos, 1.0f)); /* unknown type */
    CHECK(vegetation_spawn(&v, 1, pos, 1.5f));
    CHECK(v.type_id == 1);
    CHECK_NEAR(v.position[0], 4.0f, 1e-6f);
    CHECK_NEAR(v.position[2], 4.0f, 1e-6f);
    CHECK_NEAR(v.scale, 1.5f, 1e-6f);
    CHECK_NEAR(v.health, 100.0f, 1e-6f);
    CHECK(!v.is_harvested);
    CHECK_NEAR(v.age, 0.0f, 1e-6f);

    /* harvest yields the type's item once, and only for harvestable types */
    ItemID item = 0;
    float amount = 0.0f;
    CHECK(vegetation_harvest(&v, &item, &amount));
    CHECK(item == 42);
    CHECK_NEAR(amount, 3.0f, 1e-6f);
    CHECK(v.is_harvested);
    CHECK_NEAR(v.regrow_timer, 2.0f, 1e-6f);
    CHECK(!vegetation_harvest(&v, &item, &amount)); /* already harvested */

    VegetationInstance weed;
    CHECK(vegetation_spawn(&weed, 2, pos, 1.0f));
    CHECK(!vegetation_harvest(&weed, NULL, NULL)); /* weed is not harvestable */
    CHECK(!weed.is_harvested);

    VegetationInstance grass2;
    CHECK(vegetation_spawn(&grass2, 1, pos, 1.0f));
    CHECK(vegetation_harvest(&grass2, NULL, NULL)); /* output pointers are optional */

    /* harvested grass regrows once regrow_time has elapsed */
    v.health = 40.0f;
    CHECK(vegetation_grow(&v, 1.0f));
    CHECK(v.is_harvested);
    CHECK_NEAR(v.regrow_timer, 1.0f, 1e-6f);
    CHECK(vegetation_grow(&v, 1.0f));
    CHECK(!v.is_harvested);
    CHECK_NEAR(v.health, 100.0f, 1e-6f);

    /* growth: age advances, scale grows (capped at scale_max), health rises (capped at 100) */
    VegetationInstance g;
    CHECK(vegetation_spawn(&g, 1, pos, 1.0f));
    g.health = 50.0f;
    CHECK(vegetation_grow(&g, 1.0f));
    CHECK_NEAR(g.age, 1.0f, 1e-6f);
    CHECK_NEAR(g.scale, 1.01f, 1e-4f);
    CHECK_NEAR(g.health, 51.0f, 1e-4f);
    g.scale = 1.999f;
    CHECK(vegetation_grow(&g, 1.0f));
    CHECK_NEAR(g.scale, 2.0f, 1e-5f);
    g.health = 99.5f;
    CHECK(vegetation_grow(&g, 1.0f));
    CHECK_NEAR(g.health, 100.0f, 1e-5f);
    CHECK(!vegetation_grow(NULL, 1.0f));
    CHECK(!vegetation_grow(&g, 0.0f));

    /* death sets health to zero */
    CHECK(vegetation_die(&g));
    CHECK_NEAR(g.health, 0.0f, 1e-6f);
    CHECK(!vegetation_die(NULL));

    /* seasonal modifiers: spring 1.2, summer 1.0, autumn 0.8, winter 0.3, health floor 10 */
    VegetationInstance s;
    CHECK(vegetation_spawn(&s, 1, pos, 1.0f));
    s.health = 50.0f;
    CHECK(vegetation_seasonal_change(&s, 0.10f));
    CHECK_NEAR(s.health, 60.0f, 1e-4f);
    s.health = 50.0f;
    CHECK(vegetation_seasonal_change(&s, 0.40f));
    CHECK_NEAR(s.health, 50.0f, 1e-4f);
    s.health = 50.0f;
    CHECK(vegetation_seasonal_change(&s, 0.60f));
    CHECK_NEAR(s.health, 40.0f, 1e-4f);
    s.health = 50.0f;
    CHECK(vegetation_seasonal_change(&s, 0.90f));
    CHECK_NEAR(s.health, 15.0f, 1e-4f);
    s.health = 20.0f;
    CHECK(vegetation_seasonal_change(&s, 0.90f)); /* 20 * 0.3 = 6, floored to 10 */
    CHECK_NEAR(s.health, 10.0f, 1e-4f);
    CHECK(!vegetation_seasonal_change(&s, -0.1f));
    CHECK(!vegetation_seasonal_change(&s, 1.1f));

    /* spread: a parent seeds one instance in the chunk its spread point falls in */
    EcosystemManager sm;
    CHECK(ecosystem_manager_init(&sm, 2));
    sm.chunks[0].chunk_x = 0;
    sm.chunks[0].chunk_z = 0;
    sm.chunks[0].vegetation = calloc(1, sizeof(VegetationInstance));
    sm.chunks[0].vegetation_capacity = 1;
    sm.chunks[1].chunk_x = 1; /* no capacity, so it is always full */
    sm.chunks[1].chunk_z = 0;

    VegetationInstance parent;
    CHECK(vegetation_spawn(&parent, 2, pos, 1.0f));
    CHECK(vegetation_spread(&sm, &parent, 1.0f));
    CHECK(sm.chunks[0].vegetation_count == 1);
    CHECK(sm.chunks[0].vegetation[0].type_id == 2);
    CHECK_NEAR(sm.chunks[0].vegetation[0].position[0], 4.0f, 1e-5f);
    CHECK(!vegetation_spread(&sm, &parent, 1.0f)); /* chunk 0 is full */
    parent.position[0] = 20.0f;                    /* lands in chunk x=1, which has no room */
    CHECK(!vegetation_spread(&sm, &parent, 1.0f));
    parent.position[0] = 100.0f;                   /* lands in a chunk the manager does not have */
    parent.position[2] = 100.0f;
    CHECK(!vegetation_spread(&sm, &parent, 1.0f));
    ecosystem_manager_shutdown(&sm);

    /* fire: burns vegetation inside the radius, leaves the rest alone */
    EcosystemManager fm;
    CHECK(ecosystem_manager_init(&fm, 2));
    fm.chunks[0].chunk_x = 0;
    fm.chunks[0].chunk_z = 0;
    fm.chunks[0].vegetation = calloc(2, sizeof(VegetationInstance));
    fm.chunks[0].vegetation_capacity = 2;
    fm.chunks[0].vegetation_count = 2;
    fm.chunks[1].chunk_x = 3; /* centre (56, 56) is far outside the fire's reach */
    fm.chunks[1].chunk_z = 3;
    fm.chunks[1].vegetation = calloc(1, sizeof(VegetationInstance));
    fm.chunks[1].vegetation_capacity = 1;
    fm.chunks[1].vegetation_count = 1;

    const float at_fire[3] = {8.0f, 0.0f, 8.0f};
    const float just_outside[3] = {8.0f, 0.0f, 14.0f}; /* 6 units away, radius is 5 */
    const float far_away[3] = {56.0f, 0.0f, 56.0f};
    CHECK(vegetation_spawn(&fm.chunks[0].vegetation[0], 2, at_fire, 1.0f));
    CHECK(vegetation_spawn(&fm.chunks[0].vegetation[1], 2, just_outside, 1.0f));
    CHECK(vegetation_spawn(&fm.chunks[1].vegetation[0], 2, far_away, 1.0f));

    float fire[3] = {8.0f, 0.0f, 8.0f};
    CHECK(vegetation_fire_spread(&fm, fire, 5.0f, 1.0f));
    CHECK_NEAR(fm.chunks[0].vegetation[0].health, 0.0f, 1e-6f);   /* at the fire: burn chance 1 */
    CHECK_NEAR(fm.chunks[0].vegetation[1].health, 100.0f, 1e-6f); /* outside the radius */
    CHECK_NEAR(fm.chunks[1].vegetation[0].health, 100.0f, 1e-6f); /* chunk out of range */
    CHECK(!vegetation_fire_spread(&fm, fire, 0.0f, 1.0f));
    ecosystem_manager_shutdown(&fm);

    /* density map: instances inside the radius are counted into their grid cell */
    EcosystemManager dm;
    CHECK(ecosystem_manager_init(&dm, 1));
    dm.chunks[0].vegetation = calloc(3, sizeof(VegetationInstance));
    dm.chunks[0].vegetation_capacity = 3;
    dm.chunks[0].vegetation_count = 3;
    const float p_low[3] = {-2.0f, 0.0f, -2.0f}; /* cell (0, 0) */
    const float p_high[3] = {2.0f, 0.0f, 2.0f};  /* cell (1, 1) */
    const float p_far[3] = {10.0f, 0.0f, 0.0f};  /* outside radius 4 */
    CHECK(vegetation_spawn(&dm.chunks[0].vegetation[0], 1, p_low, 1.0f));
    CHECK(vegetation_spawn(&dm.chunks[0].vegetation[1], 1, p_high, 1.0f));
    CHECK(vegetation_spawn(&dm.chunks[0].vegetation[2], 1, p_far, 1.0f));

    float centre[3] = {0.0f, 0.0f, 0.0f}; /* not const: the API takes float center[3] */
    float grid[4] = {9.0f, 9.0f, 9.0f, 9.0f}; /* must be cleared first */
    CHECK(vegetation_density_map(&dm, centre, 4.0f, grid, 2, 2));
    CHECK_NEAR(grid[0], 1.0f, 1e-6f);
    CHECK_NEAR(grid[1], 0.0f, 1e-6f);
    CHECK_NEAR(grid[2], 0.0f, 1e-6f);
    CHECK_NEAR(grid[3], 1.0f, 1e-6f);
    CHECK(!vegetation_density_map(&dm, centre, 0.0f, grid, 2, 2));
    ecosystem_manager_shutdown(&dm);
}

static void test_ecosystem_manager(void) {
    EcosystemManager m;
    CHECK(!ecosystem_manager_init(NULL, 4));
    CHECK(!ecosystem_manager_init(&m, 0));
    CHECK(ecosystem_manager_init(&m, 4));
    CHECK(m.chunk_count == 4);
    CHECK(m.chunks != NULL);
    CHECK_NEAR(m.simulation_speed, 1.0f, 1e-6f);
    CHECK(m.simulate_reproduction && m.simulate_death && m.simulate_predation);

    uint32_t creatures = 99;
    uint32_t vegetation = 99;
    uint32_t chunks = 0;
    CHECK(ecosystem_statistics(&m, &creatures, &vegetation, &chunks));
    CHECK(creatures == 0 && vegetation == 0 && chunks == 4);
    CHECK(ecosystem_statistics(&m, NULL, NULL, NULL)); /* out-parameters are optional */
    CHECK(!ecosystem_statistics(NULL, &creatures, &vegetation, &chunks));

    /* time: one day is 100 time units, and the season advances one step per wrapped day */
    CHECK(ecosystem_manager_update(&m, 50.0f));
    CHECK_NEAR(m.day_night_cycle, 0.5f, 1e-4f);
    CHECK_NEAR(m.season, 0.0f, 1e-6f);
    CHECK(ecosystem_manager_update(&m, 60.0f)); /* 0.5 + 0.6 wraps to 0.1 */
    CHECK_NEAR(m.day_night_cycle, 0.1f, 1e-4f);
    CHECK_NEAR(m.season, 0.00274f, 1e-6f);
    CHECK(!ecosystem_manager_update(&m, 0.0f));
    CHECK(!ecosystem_manager_update(NULL, 1.0f));

    /* chunk update: creatures age, vegetation grows */
    EcosystemChunk *ch = &m.chunks[0];
    ch->creatures = calloc(1, sizeof(WildlifeCreature));
    ch->creature_count = 1;
    ch->creature_capacity = 1;
    ch->vegetation = calloc(1, sizeof(VegetationInstance));
    ch->vegetation_count = 1;
    ch->vegetation_capacity = 1;
    const float origin[3] = {0.0f, 0.0f, 0.0f};
    CHECK(vegetation_spawn(&ch->vegetation[0], 1, origin, 1.0f));
    ch->vegetation[0].health = 50.0f;
    m.season = 0.0f;
    CHECK(ecosystem_update_chunk(&m, ch, 2.0f));
    CHECK_NEAR(ch->creatures[0].age, 2.0f, 1e-6f);
    CHECK_NEAR(ch->vegetation[0].age, 2.0f, 1e-6f);
    CHECK(ch->vegetation[0].health > 0.0f);
    CHECK(!ecosystem_update_chunk(&m, ch, 0.0f));
    CHECK(!ecosystem_update_chunk(&m, NULL, 1.0f));
    ecosystem_manager_shutdown(&m);

    /* population pressure: mean of creature load and vegetation load, per chunk */
    EcosystemManager b;
    CHECK(ecosystem_manager_init(&b, 2));
    b.chunks[0].creatures = calloc(2, sizeof(WildlifeCreature));
    b.chunks[0].creature_count = 2;
    b.chunks[0].vegetation_count = 6;
    b.chunks[1].vegetation_count = 4;
    CHECK(ecosystem_balance_populations(&b));
    /* creatures 2 / (2 * 50) = 0.02, vegetation 10 / (2 * 200) = 0.025, mean 0.0225 */
    CHECK_NEAR(b.chunks[0].population_pressure, 0.0225f, 1e-5f);
    CHECK_NEAR(b.chunks[1].population_pressure, 0.0225f, 1e-5f);
    CHECK(!ecosystem_balance_populations(NULL));
    ecosystem_manager_shutdown(&b);

    /* extinction: a species with no creatures is reported (reintroduction is a TODO) */
    EcosystemManager x;
    CHECK(ecosystem_manager_init(&x, 1));
    x.species = calloc(1, sizeof(WildlifeSpecies));
    x.species_count = 1;
    x.species[0].id = 7;
    set_name(x.species[0].name, sizeof x.species[0].name, "ghost");
    CHECK(ecosystem_handle_extinction(&x));
    CHECK(!ecosystem_handle_extinction(NULL));
    CHECK(ecosystem_seasonal_migration(&x));
    CHECK(!ecosystem_seasonal_migration(NULL));
    ecosystem_manager_shutdown(&x);

    /* initial spawn: each chunk gets density * max_vegetation_per_chunk instances per type */
    CHECK(!ecosystem_spawn_initial(NULL));
    EcosystemManager s;
    CHECK(ecosystem_manager_init(&s, 2));
    s.max_vegetation_per_chunk = 10.0f;
    s.chunks[0].chunk_x = 0;
    s.chunks[0].chunk_z = 0;
    s.chunks[1].chunk_x = 1;
    s.chunks[1].chunk_z = 0;
    s.vegetation_types = calloc(1, sizeof(VegetationType));
    s.vegetation_type_count = 1;
    s.vegetation_types[0].id = 1; /* grass, registered in setup_vegetation_types */
    s.vegetation_types[0].density = 0.5f;
    s.vegetation_types[0].scale_min = 1.0f;
    s.vegetation_types[0].scale_max = 2.0f;
    CHECK(ecosystem_spawn_initial(&s));
    CHECK(s.chunks[0].vegetation_count == 5);
    CHECK(s.chunks[1].vegetation_count == 5);
    for (uint32_t c = 0; c < 2; c++) {
        const EcosystemChunk *ck = &s.chunks[c];
        for (uint32_t i = 0; i < ck->vegetation_count; i++) {
            const VegetationInstance *vi = &ck->vegetation[i];
            CHECK(vi->position[0] >= ck->chunk_x * 16.0f && vi->position[0] <= ck->chunk_x * 16.0f + 16.0f);
            CHECK(vi->position[2] >= ck->chunk_z * 16.0f && vi->position[2] <= ck->chunk_z * 16.0f + 16.0f);
        }
    }
    uint32_t spawned = 0;
    CHECK(ecosystem_statistics(&s, NULL, &spawned, NULL));
    CHECK(spawned == 10);
    ecosystem_manager_shutdown(&s);
}

static void test_serialization(void) {
    EcosystemManager a;
    CHECK(ecosystem_manager_init(&a, 2));
    EcosystemChunk *c0 = &a.chunks[0];
    c0->chunk_x = 0;
    c0->chunk_z = 0;
    c0->creatures = calloc(1, sizeof(WildlifeCreature));
    c0->creature_count = 1;
    c0->creature_capacity = 1;
    c0->creatures[0].id = 9;
    c0->creatures[0].species_id = 3;
    c0->creatures[0].age = 4.5f;
    c0->vegetation = calloc(2, sizeof(VegetationInstance));
    c0->vegetation_count = 2;
    c0->vegetation_capacity = 2;
    const float p1[3] = {1.0f, 2.0f, 3.0f};
    const float p2[3] = {4.0f, 5.0f, 6.0f};
    CHECK(vegetation_spawn(&c0->vegetation[0], 1, p1, 1.0f));
    CHECK(vegetation_spawn(&c0->vegetation[1], 2, p2, 1.0f));
    a.chunks[1].chunk_x = 1;

    uint32_t cr = 0;
    uint32_t vg = 0;
    uint32_t chn = 0;
    CHECK(ecosystem_statistics(&a, &cr, &vg, &chn));
    CHECK(cr == 1 && vg == 2 && chn == 2);

    static _Alignas(16) uint8_t buf[4096];
    uint32_t written = 0;
    CHECK(!ecosystem_serialize(&a, buf, 8, &written)); /* the header alone needs 12 bytes */
    CHECK(ecosystem_serialize(&a, buf, sizeof buf, &written));
    CHECK(written > 12);

    EcosystemManager b;
    memset(&b, 0, sizeof b);
    uint32_t read = 0;
    CHECK(ecosystem_deserialize(&b, buf, written, &read));
    CHECK(read == written);
    CHECK(b.chunk_count == 2);
    CHECK(b.chunks[0].creature_count == 1);
    CHECK(b.chunks[0].creatures != NULL);
    CHECK(b.chunks[0].creatures[0].id == 9 && b.chunks[0].creatures[0].species_id == 3);
    CHECK_NEAR(b.chunks[0].creatures[0].age, 4.5f, 1e-6f);
    CHECK(b.chunks[0].vegetation_count == 2);
    CHECK(b.chunks[0].vegetation[1].type_id == 2);
    CHECK_NEAR(b.chunks[0].vegetation[1].position[2], 6.0f, 1e-6f);
    CHECK(b.chunks[1].creature_count == 0 && b.chunks[1].vegetation_count == 0);

    /* a corrupted magic number is rejected */
    buf[0] ^= 0xFFu;
    EcosystemManager bad;
    memset(&bad, 0, sizeof bad);
    CHECK(!ecosystem_deserialize(&bad, buf, written, &read));
    buf[0] ^= 0xFFu;

    /* a truncated buffer is rejected */
    EcosystemManager trunc;
    memset(&trunc, 0, sizeof trunc);
    CHECK(!ecosystem_deserialize(&trunc, buf, written - 4, &read));

    ecosystem_manager_shutdown(&trunc);
    ecosystem_manager_shutdown(&bad);
    ecosystem_manager_shutdown(&b);
    ecosystem_manager_shutdown(&a);
}

static void test_creatures(void) {
    CHECK(species_database_init(0));
    WildlifeSpecies stag = make_species("stag", 5);
    CHECK(species_register(&stag));
    const WildlifeSpecies *found[4];
    CHECK(species_find_by_biome(5, found, 4) == 1);
    CHECK(found[0] != NULL);
    const uint32_t stag_id = found[0] != NULL ? found[0]->id : 0;

    /* spawn: requires a known species, and starts idle at full health and energy */
    const float pos[3] = {1.0f, 0.0f, 2.0f};
    WildlifeCreature c;
    CHECK(!creature_spawn(NULL, stag_id, pos));
    CHECK(!creature_spawn(&c, 9999, pos)); /* unknown species */
    CHECK(creature_spawn(&c, stag_id, pos));
    CHECK(c.id != 0);
    CHECK(c.species_id == stag_id);
    CHECK_NEAR(c.health, 100.0f, 1e-6f);
    CHECK_NEAR(c.energy, 100.0f, 1e-6f);
    CHECK(c.activity == ACTIVITY_IDLE);
    CHECK_NEAR(c.spawn_position[0], 1.0f, 1e-6f);
    CHECK_NEAR(c.spawn_position[2], 2.0f, 1e-6f);

    WildlifeCreature d;
    CHECK(creature_spawn(&d, stag_id, pos));
    CHECK(d.id != c.id);

    /* needs: hunger and thirst accrue at the species rates, and idling drains energy */
    CHECK(creature_update_needs(&c, 1.0f));
    CHECK_NEAR(c.hunger, 0.1f, 1e-5f);
    CHECK_NEAR(c.thirst, 0.2f, 1e-5f);
    CHECK_NEAR(c.energy, 99.5f, 1e-4f);
    CHECK_NEAR(c.age, 1.0f, 1e-5f);
    CHECK_NEAR(c.health, 100.0f, 1e-5f);

    /* a starving creature takes damage */
    c.hunger = 85.0f;
    CHECK(creature_update_needs(&c, 1.0f));
    CHECK_NEAR(c.health, 98.0f, 1e-4f);

    /* sleeping restores energy, clamped to 100 */
    c.hunger = 0.0f;
    c.health = 100.0f;
    c.activity = ACTIVITY_SLEEPING;
    c.energy = 50.0f;
    CHECK(creature_update_needs(&c, 1.0f));
    CHECK_NEAR(c.energy, 52.0f, 1e-4f);
    c.energy = 99.9f;
    CHECK(creature_update_needs(&c, 1.0f));
    CHECK_NEAR(c.energy, 100.0f, 1e-4f);
    CHECK(!creature_update_needs(NULL, 1.0f));

    /* fleeing: a threat inside the fear radius sends the creature straight away from it */
    const float threat_near[3] = {pos[0] + 5.0f, pos[1], pos[2]}; /* 5 away, fear radius 10 */
    CHECK(creature_flee_from(&c, threat_near));
    CHECK(c.activity == ACTIVITY_FLEEING);
    CHECK_NEAR(c.velocity[0], -8.0f, 1e-4f); /* flee_speed 8, directly away along -x */
    CHECK_NEAR(c.velocity[2], 0.0f, 1e-4f);
    CHECK(c.path_point_count == 1);
    CHECK_NEAR(c.path_points[0][0], pos[0] - 15.0f, 1e-4f); /* fear_radius * 1.5 beyond */
    CHECK_NEAR(c.path_points[0][2], pos[2], 1e-4f);

    /* a threat beyond the fear radius changes nothing */
    c.activity = ACTIVITY_IDLE;
    const float threat_far[3] = {pos[0] + 30.0f, pos[1], pos[2]};
    CHECK(!creature_flee_from(&c, threat_far));
    CHECK(c.activity == ACTIVITY_IDLE);

    /* search helpers: a found point lies inside the search radius; otherwise the creature's own
     * position is returned. The placeholder logic is not asserted beyond this contract. */
    float water[3] = {0.0f, 0.0f, 0.0f};
    if (creature_find_water(&c, water)) {
        const float dx = water[0] - c.position[0];
        const float dz = water[2] - c.position[2];
        CHECK(sqrtf(dx * dx + dz * dz) <= 200.0f + 1e-3f);
    } else {
        CHECK_NEAR(water[0], c.position[0], 1e-6f);
        CHECK_NEAR(water[2], c.position[2], 1e-6f);
    }
    CHECK(!creature_find_water(NULL, water));

    float shelter[3] = {0.0f, 0.0f, 0.0f};
    if (creature_find_shelter(&c, shelter)) {
        const float dx = shelter[0] - c.position[0];
        const float dz = shelter[2] - c.position[2];
        CHECK(sqrtf(dx * dx + dz * dz) <= 150.0f + 1e-3f);
    } else {
        CHECK_NEAR(shelter[0], c.position[0], 1e-6f);
        CHECK_NEAR(shelter[2], c.position[2], 1e-6f);
    }
    CHECK(!creature_find_shelter(NULL, shelter));

    /* despawn removes a creature once */
    CHECK(creature_despawn(c.id));
    CHECK(!creature_despawn(c.id));
    CHECK(creature_despawn(d.id));
    CHECK(!creature_despawn(123456u));
}

int main(void) {
    setup_vegetation_types();
    test_species_database();
    test_vegetation();
    test_ecosystem_manager();
    test_serialization();
    test_creatures();

    if (g_failures != 0) {
        fprintf(stderr, "test_ecosystem: %d check(s) failed\n", g_failures);
        return 1;
    }
    printf("test_ecosystem: all checks passed\n");
    return 0;
}
