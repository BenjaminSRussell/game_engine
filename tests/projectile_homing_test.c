/*
 * Homing projectiles must steer at the target entity's transform.
 * A hardcoded origin fails this: the shot dives toward (0,0,0) instead of +X.
 * Compiles projectile.c directly and stubs ECS so it does not link Engine.
 */

#include "ecs/component_ids.h"
#include "ecs/components/transform.h"
#include "gameplay/combat/projectile.h"

#include "../src/engine/gameplay/combat/status_effects.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ID 16

static ProjectileComponent g_projectiles[MAX_ID];
static int g_has_projectile[MAX_ID];
static TransformComponent *g_transforms[MAX_ID];
static u32 g_next_id;
static World g_world;

Vec3 projectile_get_velocity(Entity entity);

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

static Entity make_entity(u32 id) {
    Entity entity;
    entity.id = id;
    entity.generation = 1;
    return entity;
}

Entity ecs_create_entity(World *world) {
    (void)world;
    if (g_next_id == 0 || g_next_id >= MAX_ID) {
        return INVALID_ENTITY;
    }
    return make_entity(g_next_id++);
}

void ecs_destroy_entity(World *world, Entity entity) {
    (void)world;
    (void)entity;
}

bool ecs_add_component(World *world, Entity entity, ECSComponentID component_id,
                       const void *data) {
    (void)world;
    if (entity.id == 0 || entity.id >= MAX_ID || !data) {
        return false;
    }
    if (component_id == PROJECTILE_COMPONENT_ID) {
        g_projectiles[entity.id] = *(const ProjectileComponent *)data;
        g_has_projectile[entity.id] = 1;
    }
    return true;
}

void *ecs_get_component(World *world, Entity entity, ECSComponentID component_id) {
    (void)world;
    if (entity.id == 0 || entity.id >= MAX_ID) {
        return NULL;
    }
    if (component_id == PROJECTILE_COMPONENT_ID && g_has_projectile[entity.id]) {
        return &g_projectiles[entity.id];
    }
    if (component_id == TRANSFORM_COMPONENT_ID) {
        return g_transforms[entity.id];
    }
    return NULL;
}

bool hitbox_system_raycast(const Vec3 *start, const Vec3 *end, Entity source_entity,
                           Vec3 *out_hit_point, Vec3 *out_hit_normal,
                           Entity *out_hit_entity) {
    (void)start;
    (void)end;
    (void)source_entity;
    (void)out_hit_point;
    (void)out_hit_normal;
    if (out_hit_entity) {
        *out_hit_entity = INVALID_ENTITY;
    }
    return false;
}

u32 hitbox_system_query_sphere(const Vec3 *center, f32 radius, Entity source_entity,
                               Entity *out_entities, u32 max_entities) {
    (void)center;
    (void)radius;
    (void)source_entity;
    (void)out_entities;
    (void)max_entities;
    return 0;
}

DamageEvent *damage_event_create(Entity source, Entity target, f32 amount,
                                 DamageType type) {
    (void)source;
    (void)target;
    (void)amount;
    (void)type;
    return NULL;
}

void status_sys_apply_effect_with_source(uint64_t entity_id, StatusEffectType type,
                                        float duration, float magnitude,
                                        uint64_t source_entity_id) {
    (void)entity_id;
    (void)type;
    (void)duration;
    (void)magnitude;
    (void)source_entity_id;
}

Vec3 vec3_reflect(Vec3 v, Vec3 normal) {
    (void)v;
    (void)normal;
    return vec3_zero();
}

static void restart(void) {
    projectile_system_shutdown();
    memset(g_projectiles, 0, sizeof(g_projectiles));
    memset(g_has_projectile, 0, sizeof(g_has_projectile));
    memset(g_transforms, 0, sizeof(g_transforms));
    g_next_id = 3;
    expect(projectile_system_init(&g_world), "projectile_system_init");
}

static TransformComponent transform_at(f32 x, f32 y, f32 z) {
    TransformComponent transform;
    memset(&transform, 0, sizeof(transform));
    transform.position = vec3_create(x, y, z);
    return transform;
}

static Entity spawn_forward(void) {
    Entity source = make_entity(1);
    Entity projectile = projectile_spawn(&g_world, vec3_create(0.0f, 10.0f, 0.0f),
                                         vec3_create(0.0f, 0.0f, 1.0f), 20.0f, source,
                                         5.0f);
    expect(projectile.id != 0, "spawn");
    return projectile;
}

static void expect_axis(Vec3 vel, int axis, const char *msg) {
    f32 major = axis == 0 ? vel.x : vel.z;
    f32 other_a = axis == 0 ? vel.y : vel.x;
    f32 other_b = axis == 0 ? vel.z : vel.y;
    if (major <= 15.0f || other_a < -1.0f || other_a > 1.0f || other_b < -1.0f ||
        other_b > 1.0f) {
        fprintf(stderr, "FAIL: %s (vel %.3f %.3f %.3f)\n", msg, vel.x, vel.y, vel.z);
        exit(1);
    }
}

int main(void) {
    Entity target = make_entity(2);
    TransformComponent target_transform;
    Entity projectile;
    ProjectileComponent *authored;

    restart();
    projectile = spawn_forward();
    projectile_system_update(&g_world, 0.05f);
    expect_axis(projectile_get_velocity(projectile), 2, "straight shot stays +Z");

    restart();
    target_transform = transform_at(40.0f, 10.0f, 0.0f);
    g_transforms[target.id] = &target_transform;
    projectile = spawn_forward();
    authored = ecs_get_component(&g_world, projectile, PROJECTILE_COMPONENT_ID);
    expect(authored != NULL, "spawned projectile component");
    projectile_set_homing(authored, target, 1.0f);
    projectile_system_update(&g_world, 0.05f);
    expect_axis(projectile_get_velocity(projectile), 0, "homes toward +X target");

    restart();
    g_transforms[target.id] = &target_transform;
    projectile = spawn_forward();
    authored = ecs_get_component(&g_world, projectile, PROJECTILE_COMPONENT_ID);
    projectile_set_homing(authored, target, 1.0f);
    authored->homing_delay = 10.0f;
    projectile_system_update(&g_world, 0.05f);
    expect_axis(projectile_get_velocity(projectile), 2, "homing delay holds +Z");

    restart();
    projectile = spawn_forward();
    authored = ecs_get_component(&g_world, projectile, PROJECTILE_COMPONENT_ID);
    projectile_set_homing(authored, target, 1.0f);
    projectile_system_update(&g_world, 0.05f);
    expect_axis(projectile_get_velocity(projectile), 2,
                "missing transform does not steer at the origin");

    projectile_system_shutdown();
    printf("projectile homing test passed\n");
    return 0;
}
