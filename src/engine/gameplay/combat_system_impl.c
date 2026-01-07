/**
 * COMBAT SYSTEM - PHASE 1 IMPLEMENTATION
 * Integrated hitbox collision, damage events, and projectile physics
 */

#include "gameplay/combat/combat_system.h"
#include "gameplay/combat/damage.h"
#include "gameplay/combat/hitbox.h"
#include "gameplay/combat/projectile.h"
#include <core/logger.h>
#include <core/memory.h>
#include <ecs/component_ids.h>
#include <ecs/components/health.h>
#include <ecs/ecs.h>
#include <math/math.h>
#include <stdbool.h>
#include <stdlib.h>

// Global combat system state
static struct {
  bool initialized;
  World *world;
} g_combat_system = {0};

// ============================================================================
// COMBAT SYSTEM INITIALIZATION
// ============================================================================

bool combat_system_init(World *world) {
  if (g_combat_system.initialized)
    return true;
  if (!world)
    return false;

  g_combat_system.world = world;

  // Initialize subsystems
  damage_system_init(1000);
  projectile_system_init();

  // Register combat components
  ecs_register_component_simple(world, "HealthComponent",
                                sizeof(HealthComponent));
  ecs_register_component_simple(world, "HitboxComponent",
                                sizeof(HitboxComponent));
  ecs_register_component_simple(world, "DamageComponent",
                                sizeof(DamageComponent));
  ecs_register_component_simple(world, "ResistanceComponent",
                                sizeof(ResistanceComponent));
  ecs_register_component_simple(world, "ProjectileComponent",
                                sizeof(ProjectileComponent));

  g_combat_system.initialized = true;
  return true;
}

void combat_system_shutdown(void) {
  if (!g_combat_system.initialized)
    return;

  damage_system_shutdown();
  projectile_system_shutdown();

  g_combat_system.initialized = false;
  g_combat_system.world = NULL;
}

// ============================================================================
// COMBAT SYSTEM UPDATE
// ============================================================================

void combat_system_update(World *world, f32 delta_time) {
  if (!g_combat_system.initialized || !world)
    return;

  // Update hitbox collision detection
  combat_system_update_hitboxes(world, delta_time);

  // Update projectiles
  projectile_system_update(world, (f64)delta_time);

  // Process damage events
  damage_system_process_events(world, (f64)delta_time);

  // Clear processed damage events
  damage_system_clear_events();
}

void combat_system_update_hitboxes(World *world, f32 delta_time) {
  if (!world)
    return;

  // Query all entities with hitbox components
  QueryDesc query_desc = {0};
  ComponentType components[] = {HITBOX_COMPONENT_ID};
  query_desc.all_components = components;
  query_desc.all_count = 1;

  Query *query = ecs_query_create(world, &query_desc);
  if (!query)
    return;

  // Collect all active hitboxes
  Entity entities[256];
  HitboxComponent *hitboxes[256];
  Vec3 positions[256];
  u32 count = 0;

  Entity entity;
  void *component_ptrs[1];

  while (ecs_query_next(query, &entity, component_ptrs) && count < 256) {
    HitboxComponent *hitbox = (HitboxComponent *)component_ptrs[0];
    if (!hitbox || !hitbox->active)
      continue;
    // Get entity position (from transform)
    // For now, use world_position from hitbox
    entities[count] = entity;
    hitboxes[count] = hitbox;
    positions[count] = hitbox->world_position;
    count++;
  }

  // Test all hitbox pairs for collision
  for (u32 i = 0; i < count; i++) {
    for (u32 j = i + 1; j < count; j++) {
      HitboxCollision collision;
      bool hit = hitbox_test_collision(hitboxes[i], &positions[i], hitboxes[j],
                                       &positions[j], &collision);

        if (hit) {
          // Get damage components
          DamageComponent *damage_i = (DamageComponent *)ecs_get_component(
              world, entities[i], DAMAGE_COMPONENT_ID);
          DamageComponent *damage_j = (DamageComponent *)ecs_get_component(
              world, entities[j], DAMAGE_COMPONENT_ID);

          // Create damage events
          if (damage_i && hitboxes[i]->is_trigger) {
            damage_event_emit_simple(entities[i], entities[j],
                                     damage_i->base_damage *
                                         hitboxes[i]->damage_multiplier);
          }
          if (damage_j && hitboxes[j]->is_trigger) {
            damage_event_emit_simple(entities[j], entities[i],
                                     damage_j->base_damage *
                                         hitboxes[j]->damage_multiplier);
          }
        }
    }
  }

  ecs_query_destroy(world, query);
}

// ============================================================================
// COMBAT UTILITIES
// ============================================================================

Entity combat_create_melee_attack(World *world, Entity attacker, Vec3 position,
                                  Vec3 direction, f32 damage, f32 range) {
  if (!world)
    return INVALID_ENTITY;

  Entity attack = ecs_create_entity(world);

  // Add hitbox
  HitboxComponent hitbox = hitbox_create_sphere(range * 0.5f, 0);
  hitbox.offset = vec3_mul(direction, range * 0.5f);
  hitbox.world_position = position; // Fix: Set world position from argument
  hitbox.active = true;
  hitbox.is_trigger = true; // CRITICAL: Enable damage event emission on collision
  ecs_add_component(world, attack, HITBOX_COMPONENT_ID, &hitbox);

  // Add damage component
  DamageComponent dmg = damage_component_create_melee(damage);
  ecs_add_component(world, attack, DAMAGE_COMPONENT_ID, &dmg);

  return attack;
}

Entity combat_fire_projectile(World *world, Entity source, Vec3 position,
                              Vec3 direction, f32 speed, f32 damage) {
  return projectile_spawn(world, position, direction, speed, source, damage);
}

/*
 * COMBAT SYSTEM COMPLETE
 * Components: Hitbox, Damage, Resistance, Projectile
 * Systems: Collision Detection, Damage Processing, Projectile Physics
 * LOC: ~450+ across all files
 */
