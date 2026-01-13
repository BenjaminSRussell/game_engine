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
  
  // Update enhanced combat systems
  combat_process_combos(world, delta_time);
  combat_update_status_effects(world, delta_time);
  combat_process_area_effects(world, delta_time);
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

Entity combat_create_ability_attack(World *world, Entity caster, Vec3 position,
                                   Vec3 direction, uint32_t ability_id, f32 damage) {
  if (!world)
    return INVALID_ENTITY;

  // Create ability effect entity
  Entity ability_entity = ecs_create_entity(world);
  if (ability_entity.id == 0) {
    LOG_ERROR("Failed to create ability entity");
    return INVALID_ENTITY;
  }

  // Add transform component
  TransformComponent transform = {
      .position = position,
      .rotation = quat_from_direction(direction),
      .scale = (Vec3){1.0f, 1.0f, 1.0f}
  };
  ecs_add_component(world, ability_entity, TRANSFORM_COMPONENT_ID, &transform);

  // Add ability component
  AbilityComponent ability = {
      .ability_id = ability_id,
      .caster_entity = caster,
      .current_cooldown = 0.0f,
      .is_channeling = false,
      .channel_time = 0.0f
  };
  ecs_add_component(world, ability_entity, ABILITY_COMPONENT_ID, &ability);

  // Add damage component
  DamageComponent dmg = {
      .base_damage = damage,
      .damage_type = DAMAGE_TYPE_ABILITY,
      .source_entity = caster,
      .damage_flags = DAMAGE_FLAG_MAGICAL | DAMAGE_FLAG_AREA_OF_EFFECT
  };
  ecs_add_component(world, ability_entity, DAMAGE_COMPONENT_ID, &dmg);

  return ability_entity;
}

bool combat_apply_status_effect(World *world, Entity target, Entity source,
                               uint32_t status_id, f32 duration) {
  if (!world)
    return false;

  // Check if target has status component
  StatusComponent* status = ecs_get_component(world, target, STATUS_COMPONENT_ID);
  if (!status) {
    // Create status component
    StatusComponent new_status = {0};
    status = &new_status;
  }

  // Add new status effect
  if (status->effect_count < MAX_STATUS_EFFECTS) {
    StatusEffect* effect = &status->effects[status->effect_count];
    effect->status_id = status_id;
    effect->source_entity = source;
    effect->duration = duration;
    effect->remaining_time = duration;
    effect->stack_count = 1;
    effect->is_active = true;
    
    status->effect_count++;
    
    // Apply status component if it was newly created
    if (status == &new_status) {
      ecs_add_component(world, target, STATUS_COMPONENT_ID, &new_status);
    }
    
    return true;
  }

  return false;
}

void combat_create_area_effect(World *world, Vec3 center, f32 radius,
                              Entity source, f32 damage, uint32_t damage_type) {
  if (!world)
    return;

  // Create area effect entity
  Entity area_entity = ecs_create_entity(world);
  if (area_entity.id == 0) {
    LOG_ERROR("Failed to create area effect entity");
    return;
  }

  // Add transform component
  TransformComponent transform = {
      .position = center,
      .rotation = quat_identity(),
      .scale = (Vec3){radius, radius, radius}
  };
  ecs_add_component(world, area_entity, TRANSFORM_COMPONENT_ID, &transform);

  // Add area effect component
  AreaEffectComponent area = {
      .radius = radius,
      .duration = 1.0f,  // Instant effect
      .damage_per_second = damage,
      .damage_type = damage_type,
      .source_entity = source,
      .affects_allies = false,
      .affects_enemies = true
  };
  ecs_add_component(world, area_entity, AREA_EFFECT_COMPONENT_ID, &area);

  // Add damage component
  DamageComponent dmg = {
      .base_damage = damage,
      .damage_type = damage_type,
      .source_entity = source,
      .damage_flags = DAMAGE_FLAG_AREA_OF_EFFECT
  };
  ecs_add_component(world, area_entity, DAMAGE_COMPONENT_ID, &dmg);
}

bool combat_start_combo(World *world, Entity attacker, uint32_t combo_id) {
  if (!world)
    return false;

  // Get or create combo component
  ComboComponent* combo = ecs_get_component(world, attacker, COMBO_COMPONENT_ID);
  if (!combo) {
    ComboComponent new_combo = {0};
    combo = &new_combo;
  }

  combo->current_combo = combo_id;
  combo->combo_timer = 0.0f;
  combo->combo_count = 0;
  combo->is_active = true;

  if (combo == &new_combo) {
    ecs_add_component(world, attacker, &new_combo);
  }

  return true;
}

bool combat_advance_combo(World *world, Entity attacker, uint32_t attack_id) {
  if (!world)
    return false;

  ComboComponent* combo = ecs_get_component(world, attacker, COMBO_COMPONENT_ID);
  if (!combo || !combo->is_active) {
    return false;
  }

  // Check if attack is valid for current combo
  // This would typically look up combo data from a database
  combo->combo_count++;
  combo->combo_timer = 2.0f;  // Reset combo window

  return true;
}

void combat_process_combos(World *world, f32 delta_time) {
  if (!world)
    return;

  // Iterate through all entities with combo components
  Entity entity = {0};
  while (ecs_iterate_entities(world, &entity)) {
    ComboComponent* combo = ecs_get_component(world, entity, COMBO_COMPONENT_ID);
    if (!combo || !combo->is_active)
      continue;

    // Update combo timer
    combo->combo_timer -= delta_time;
    if (combo->combo_timer <= 0.0f) {
      // Combo expired
      combo->is_active = false;
      combo->combo_count = 0;
      combo->current_combo = 0;
    }
  }
}

void combat_update_status_effects(World *world, f32 delta_time) {
  if (!world)
    return;

  // Iterate through all entities with status components
  Entity entity = {0};
  while (ecs_iterate_entities(world, &entity)) {
    StatusComponent* status = ecs_get_component(world, entity, STATUS_COMPONENT_ID);
    if (!status)
      continue;

    // Update all status effects
    for (uint32_t i = 0; i < status->effect_count; i++) {
      StatusEffect* effect = &status->effects[i];
      if (!effect->is_active)
        continue;

      effect->remaining_time -= delta_time;
      
      // Apply status effect logic
      if (effect->remaining_time <= 0.0f) {
        effect->is_active = false;
        // Remove expired effect
        memmove(&status->effects[i], &status->effects[i + 1],
                (status->effect_count - i - 1) * sizeof(StatusEffect));
        status->effect_count--;
        i--;  // Adjust index after removal
      }
    }
  }
}

void combat_process_area_effects(World *world, f32 delta_time) {
  if (!world)
    return;

  // Iterate through all area effect entities
  Entity entity = {0};
  while (ecs_iterate_entities(world, &entity)) {
    AreaEffectComponent* area = ecs_get_component(world, entity, AREA_EFFECT_COMPONENT_ID);
    if (!area)
      continue;

    TransformComponent* transform = ecs_get_component(world, entity, TRANSFORM_COMPONENT_ID);
    if (!transform)
      continue;

    // Update area effect duration
    area->duration -= delta_time;
    
    if (area->duration > 0.0f) {
      // Apply damage to entities in area
      Entity target = {0};
      while (ecs_iterate_entities(world, &target)) {
        if (target.id == entity.id || target.id == area->source_entity.id)
          continue;

        TransformComponent* target_transform = ecs_get_component(world, target, TRANSFORM_COMPONENT_ID);
        if (!target_transform)
          continue;

        // Check if target is within area
        Vec3 distance = vec3_sub(target_transform->position, transform->position);
        if (vec3_length(distance) <= area->radius) {
          // Apply damage
          DamageEvent damage_event = {
              .target = target,
              .source = area->source_entity,
              .damage = area->damage_per_second * delta_time,
              .damage_type = area->damage_type,
              .flags = DAMAGE_FLAG_AREA_OF_EFFECT
          };
          damage_system_queue_event(world, &damage_event);
        }
      }
    } else {
      // Remove expired area effect
      ecs_destroy_entity(world, entity);
    }
  }
}
