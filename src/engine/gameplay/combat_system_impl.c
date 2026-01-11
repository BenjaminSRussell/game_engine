// combat_system_impl.c - Implementation
#include "include/core/logger.h"
#include "include/ecs/component_ids.h"
#include "include/ecs/ecs.h"
#include "include/gameplay/combat/combat_system.h"
#include "include/gameplay/combat/damage.h"
#include "include/gameplay/combat/hitbox.h"
#include "include/gameplay/combat/projectile.h"
#include <stdlib.h>

static World *combat_world = NULL;

bool combat_system_init(World *world) {
  if (!world) {
    LOG_ERROR("Combat system: NULL world provided");
    return false;
  }

  combat_world = world;

  // Initialize subsystems
  if (!hitbox_system_init(world)) {
    LOG_ERROR("Failed to initialize hitbox system");
    return false;
  }

  damage_system_init(1024);

  if (!projectile_system_init(world)) {
    LOG_ERROR("Failed to initialize projectile system");
    damage_system_shutdown();
    hitbox_system_shutdown();
    return false;
  }

  LOG_INFO("Combat system initialized successfully");
  return true;
}

void combat_system_shutdown(void) {
  projectile_system_shutdown();
  damage_system_shutdown();
  hitbox_system_shutdown();
  combat_world = NULL;
  LOG_INFO("Combat system shutdown");
}

void combat_system_update(World *world, f32 delta_time) {
  if (!world || !combat_world)
    return;

  // Update all combat subsystems
  combat_system_update_hitboxes(world, delta_time);
  projectile_system_update(world, delta_time);
  damage_system_process_events(world, delta_time);
}

void combat_system_update_hitboxes(World *world, f32 delta_time) {
  hitbox_system_update(world, delta_time);
}

Entity combat_create_melee_attack(World *world, Entity attacker, Vec3 position,
                                  Vec3 direction, f32 damage, f32 range) {
  if (!world)
    return INVALID_ENTITY;

  Entity hitbox_entity =
      hitbox_create_temporary(world, position, direction, range, 0.5f);
  if (hitbox_entity.id == 0) {
    LOG_ERROR("Failed to create melee hitbox");
    return INVALID_ENTITY;
  }

  // Add damage component
  DamageComponent dmg = {.base_damage = damage,
                         .damage_type = DAMAGE_TYPE_MELEE,
                         .source_entity = attacker,
                         .damage_flags = DAMAGE_FLAG_KNOCKBACK};

  ecs_add_component(world, hitbox_entity, DAMAGE_COMPONENT_ID, &dmg);

  return hitbox_entity;
}

Entity combat_fire_projectile(World *world, Entity source, Vec3 position,
                              Vec3 direction, f32 speed, f32 damage) {
  if (!world)
    return INVALID_ENTITY;

  Entity projectile =
      projectile_spawn(world, position, direction, speed, source, damage);
  if (projectile.id == 0) {
    LOG_ERROR("Failed to create projectile");
    return INVALID_ENTITY;
  }

  // Add damage component
  DamageComponent dmg = {.base_damage = damage,
                         .damage_type = DAMAGE_TYPE_PROJECTILE,
                         .source_entity = source,
                         .damage_flags = DAMAGE_FLAG_KNOCKBACK};

  ecs_add_component(world, projectile, DAMAGE_COMPONENT_ID, &dmg);

  return projectile;
}
