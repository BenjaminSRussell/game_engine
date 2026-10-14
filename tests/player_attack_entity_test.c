/*
 * player_attack_entity validates components, health, and melee range, then
 * calls combat_create_melee_attack. This target compiles that source directly
 * and stubs ECS so it does not link Engine.
 */

#include "core/logging/unified_logger.h"
#include "ecs/component_ids.h"
#include "ecs/components/health.h"
#include "ecs/components/transform.h"
#include "gameplay/combat/combat_system.h"
#include "gameplay/combat/player_combat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ID 8

static bool g_is_player[MAX_ID];
static HealthComponent *g_health[MAX_ID];
static TransformComponent *g_transform[MAX_ID];

static int g_attack_calls;
static World *g_attack_world;
static Entity g_attack_attacker;
static Vec3 g_attack_position;
static Vec3 g_attack_direction;
static f32 g_attack_damage;
static f32 g_attack_range;
static Entity g_attack_result;

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
    (void)level;
    (void)category;
    (void)file;
    (void)line;
    (void)function;
    (void)format;
}

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void expect(int cond, const char *msg) {
    if (!cond) {
        fail(msg);
    }
}

static int near(f32 a, f32 b) {
    f32 d = a - b;
    if (d < 0.0f) {
        d = -d;
    }
    return d < 0.001f;
}

static Entity make_entity(u32 id) {
    Entity entity;
    entity.id = id;
    entity.generation = 1;
    return entity;
}

static void reset_stubs(void) {
    memset(g_is_player, 0, sizeof(g_is_player));
    memset(g_health, 0, sizeof(g_health));
    memset(g_transform, 0, sizeof(g_transform));
    g_attack_calls = 0;
    g_attack_world = NULL;
    g_attack_attacker = make_entity(0);
    g_attack_position = vec3_zero();
    g_attack_direction = vec3_zero();
    g_attack_damage = 0.0f;
    g_attack_range = 0.0f;
    g_attack_result = make_entity(4);
}

bool ecs_has_component(World *world, Entity entity, ECSComponentID component_id) {
    (void)world;
    if (entity.id >= MAX_ID) {
        return false;
    }
    if (component_id == PLAYER_COMPONENT_ID) {
        return g_is_player[entity.id];
    }
    if (component_id == HEALTH_COMPONENT_ID) {
        return g_health[entity.id] != NULL;
    }
    if (component_id == TRANSFORM_COMPONENT_ID) {
        return g_transform[entity.id] != NULL;
    }
    return false;
}

void *ecs_get_component(World *world, Entity entity, ECSComponentID component_id) {
    (void)world;
    if (entity.id >= MAX_ID) {
        return NULL;
    }
    if (component_id == HEALTH_COMPONENT_ID) {
        return g_health[entity.id];
    }
    if (component_id == TRANSFORM_COMPONENT_ID) {
        return g_transform[entity.id];
    }
    return NULL;
}

Entity combat_create_melee_attack(World *world, Entity attacker, Vec3 position,
                                  Vec3 direction, f32 damage, f32 range) {
    g_attack_calls++;
    g_attack_world = world;
    g_attack_attacker = attacker;
    g_attack_position = position;
    g_attack_direction = direction;
    g_attack_damage = damage;
    g_attack_range = range;
    return g_attack_result;
}

static TransformComponent transform_at(f32 x, f32 y, f32 z) {
    TransformComponent transform;
    memset(&transform, 0, sizeof(transform));
    transform.position = vec3_create(x, y, z);
    return transform;
}

static void expect_no_attack(const char *msg) {
    expect(g_attack_calls == 0, msg);
}

int main(void) {
    static World world;
    Entity player = make_entity(1);
    Entity target = make_entity(2);
    HealthComponent player_health;
    HealthComponent target_health;
    TransformComponent player_transform;
    TransformComponent target_transform;

    memset(&player_health, 0, sizeof(player_health));
    memset(&target_health, 0, sizeof(target_health));
    player_health.is_alive = true;
    player_health.max_health = 20.0f;
    player_health.health = 20.0f;
    target_health.is_alive = true;
    target_health.max_health = 20.0f;
    target_health.health = 20.0f;

    reset_stubs();
    expect(!player_attack_entity(NULL, player, target), "null world");
    expect_no_attack("null world spawned an attack");

    reset_stubs();
    g_is_player[1] = true;
    expect(!player_attack_entity(&world, make_entity(0), target), "player id 0");
    expect(!player_attack_entity(&world, player, make_entity(0)), "target id 0");
    expect_no_attack("invalid id spawned an attack");

    reset_stubs();
    player_transform = transform_at(0.0f, 0.0f, 0.0f);
    target_transform = transform_at(1.0f, 0.0f, 0.0f);
    g_health[1] = &player_health;
    g_health[2] = &target_health;
    g_transform[1] = &player_transform;
    g_transform[2] = &target_transform;
    expect(!player_attack_entity(&world, player, target), "missing player component");
    expect_no_attack("non-player spawned an attack");

    reset_stubs();
    g_is_player[1] = true;
    player_health.is_alive = false;
    g_health[1] = &player_health;
    g_health[2] = &target_health;
    g_transform[1] = &player_transform;
    g_transform[2] = &target_transform;
    expect(!player_attack_entity(&world, player, target), "dead player");
    expect_no_attack("dead player spawned an attack");

    reset_stubs();
    player_health.is_alive = true;
    target_health.is_alive = false;
    g_is_player[1] = true;
    g_health[1] = &player_health;
    g_health[2] = &target_health;
    g_transform[1] = &player_transform;
    g_transform[2] = &target_transform;
    expect(!player_attack_entity(&world, player, target), "dead target");
    expect_no_attack("dead target spawned an attack");

    reset_stubs();
    target_health.is_alive = true;
    g_is_player[1] = true;
    g_health[1] = &player_health;
    g_health[2] = &target_health;
    g_transform[2] = &target_transform;
    expect(!player_attack_entity(&world, player, target), "missing player transform");
    expect_no_attack("missing player transform spawned an attack");

    reset_stubs();
    g_is_player[1] = true;
    g_transform[1] = &player_transform;
    expect(!player_attack_entity(&world, player, target), "missing target transform");
    expect_no_attack("missing target transform spawned an attack");

    reset_stubs();
    g_is_player[1] = true;
    player_transform = transform_at(0.0f, 0.0f, 0.0f);
    target_transform = transform_at(0.0f, 3.0f, 0.0f);
    g_transform[1] = &player_transform;
    g_transform[2] = &target_transform;
    expect(!player_attack_entity(&world, player, target), "out of range");
    expect_no_attack("out of range spawned an attack");

    reset_stubs();
    g_is_player[1] = true;
    player_transform = transform_at(1.0f, 2.0f, 3.0f);
    target_transform = transform_at(1.0f, 4.0f, 3.0f);
    g_transform[1] = &player_transform;
    g_transform[2] = &target_transform;
    expect(player_attack_entity(&world, player, target), "in range attack");
    expect(g_attack_calls == 1, "in range call count");
    expect(g_attack_world == &world, "attack world");
    expect(g_attack_attacker.id == player.id, "attacker id");
    expect(near(g_attack_position.x, 1.0f) && near(g_attack_position.y, 2.0f) &&
               near(g_attack_position.z, 3.0f),
           "attack position");
    expect(near(g_attack_direction.x, 0.0f) && near(g_attack_direction.y, 1.0f) &&
               near(g_attack_direction.z, 0.0f),
           "attack direction");
    expect(near(g_attack_damage, 10.0f), "melee damage");
    expect(near(g_attack_range, 2.5f), "melee range");

    reset_stubs();
    g_is_player[1] = true;
    player_transform = transform_at(0.0f, 0.0f, 0.0f);
    target_transform = transform_at(2.5f, 0.0f, 0.0f);
    g_transform[1] = &player_transform;
    g_transform[2] = &target_transform;
    expect(player_attack_entity(&world, player, target), "range boundary");
    expect(g_attack_calls == 1, "boundary call count");
    expect(near(g_attack_direction.x, 1.0f) && near(g_attack_direction.y, 0.0f) &&
               near(g_attack_direction.z, 0.0f),
           "boundary direction");

    reset_stubs();
    g_is_player[1] = true;
    player_transform = transform_at(4.0f, 5.0f, 6.0f);
    target_transform = player_transform;
    g_transform[1] = &player_transform;
    g_transform[2] = &target_transform;
    expect(player_attack_entity(&world, player, target), "same position");
    expect(g_attack_calls == 1, "same position call count");
    expect(near(g_attack_direction.x, 0.0f) && near(g_attack_direction.y, 0.0f) &&
               near(g_attack_direction.z, 0.0f),
           "zero direction");

    reset_stubs();
    g_attack_result = make_entity(0);
    g_is_player[1] = true;
    g_transform[1] = &player_transform;
    g_transform[2] = &target_transform;
    expect(!player_attack_entity(&world, player, target), "melee create failed");
    expect(g_attack_calls == 1, "failed create still called combat");

    printf("player_attack_entity test passed\n");
    return 0;
}
