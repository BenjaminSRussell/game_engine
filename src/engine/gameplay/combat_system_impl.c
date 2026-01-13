// combat_system_impl.c - Implementation
#include "include/core/logger.h"
#include "include/ecs/component_ids.h"
#include "include/ecs/ecs.h"
#include "include/ecs/components/transform.h"
#include "include/gameplay/combat/combat_system.h"
#include "include/gameplay/combat/damage.h"
#include "include/gameplay/combat/hitbox.h"
#include "include/gameplay/combat/projectile.h"
#include <stdlib.h>

static World *combat_world = NULL;

bool combat_system_init(World *world) {
  if (!world) {
    LOG_ERROR(LOG_CAT_GAME, "Combat system: NULL world provided");
    return false;
  }

  combat_world = world;

  // Initialize subsystems
  if (!hitbox_system_init(world)) {
    LOG_ERROR(LOG_CAT_GAME, "Failed to initialize hitbox system");
    return false;
  }

  damage_system_init(1024);

  if (!projectile_system_init(world)) {
    LOG_ERROR(LOG_CAT_GAME, "Failed to initialize projectile system");
    damage_system_shutdown();
    hitbox_system_shutdown();
    return false;
  }

  LOG_INFO(LOG_CAT_GAME, "Combat system initialized successfully");
  return true;
}

void combat_system_shutdown(void) {
  projectile_system_shutdown();
  damage_system_shutdown();
  hitbox_system_shutdown();
  combat_world = NULL;
  LOG_INFO(LOG_CAT_GAME, "Combat system shutdown");
}

void combat_system_update(World *world, f32 delta_time) {
  if (!world || !combat_world)
    return;

  // Update all combat subsystems
  combat_system_update_hitboxes(world, delta_time);
  projectile_system_update(world, delta_time);
  damage_system_process_events(world, delta_time);
  
  // Update enhanced combat systems
  // combat_process_combos(world, delta_time);
  // combat_update_status_effects(world, delta_time);
  // combat_process_area_effects(world, delta_time);
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
    LOG_ERROR(LOG_CAT_GAME, "Failed to create melee hitbox");
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
    LOG_ERROR(LOG_CAT_GAME, "Failed to create projectile");
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

Entity combat_create_ability_attack(World *world, Entity caster, Vec3 position,
                                   Vec3 direction, uint32_t ability_id, f32 damage) {
  // Disabled due to missing component definitions
  return INVALID_ENTITY;
}

bool combat_apply_status_effect(World *world, Entity target, Entity source,
                               uint32_t status_id, f32 duration) {
  // Disabled due to missing component definitions
  return false;
}

void combat_create_area_effect(World *world, Vec3 center, f32 radius,
                              Entity source, f32 damage, uint32_t damage_type) {
  // Disabled due to missing component definitions
}

bool combat_start_combo(World *world, Entity attacker, uint32_t combo_id) {
  // Disabled due to missing component definitions
  return false;
}

bool combat_advance_combo(World *world, Entity attacker, uint32_t attack_id) {
  // Disabled due to missing component definitions
  return false;
}

void combat_process_combos(World *world, f32 delta_time) {
  // Disabled due to missing component definitions
}

void combat_update_status_effects(World *world, f32 delta_time) {
  // Disabled due to missing component definitions
}

void combat_process_area_effects(World *world, f32 delta_time) {
  // Disabled due to missing component definitions
}
