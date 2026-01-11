// Special NPC abilities implementation (Creeper explosion, Skeleton arrows,
// etc).
#include <ecs/component_ids.h>
#include <ecs/components/health.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include <ecs/ecs.h>
#include <game/mode.h>
#include <math.h>
#include <math/vec3.h>
#include <npc/npc.h>
#include <npc/npc_abilities.h>
#include <npc/npc_combat.h>
#include <physics/physics.h>
#include <stdlib.h>

// Creeper explosion logic
void npc_creeper_explode(struct NPCSystem *system, EntityID entity,
                         Vec3 position) {
  if (!system)
    return;

  f32 explosion_radius = 3.0f;
  f32 explosion_damage = 25.0f;

  // Find all entities in explosion radius
  QueryDesc desc = {0};
  ComponentType components[] = {TRANSFORM_COMPONENT_ID};
  desc.all_components = components;
  desc.all_count = 1;
  Query *query = ecs_query_create((World *)system->ecs, &desc);

  Entity target_entity;
  void *comps[1];
  while (ecs_query_next(query, &target_entity, comps)) {
    EntityID target = target_entity.id;
    if (target == entity)
      continue; // Don't damage self

    TransformComponent *target_transform = (TransformComponent *)comps[0];
    if (!target_transform)
      continue;

    // Calculate distance to explosion
    f32 distance = vec3_length(vec3_sub(target_transform->position, position));
    if (distance > explosion_radius)
      continue;

    // Calculate damage falloff
    f32 damage_factor = 1.0f - (distance / explosion_radius);
    f32 damage_amount = explosion_damage * damage_factor;

    // Calculate knockback
    Vec3 knockback_dir =
        vec3_normalize(vec3_sub(target_transform->position, position));
    f32 knockback_force = 15.0f * damage_factor;

    // Create damage info
    DamageInfo damage = {.amount = damage_amount,
                         .type = DAMAGE_TYPE_EXPLOSION,
                         .source = entity,
                         .knockback_direction = knockback_dir,
                         .knockback_force = knockback_force};

    // Apply damage
    void *player_comp = ecs_get_component((World *)system->ecs, target_entity,
                                          PLAYER_COMPONENT_ID);
    if (player_comp) {
      survival_take_damage((World *)system->ecs, target_entity, damage.amount);
      LOG_INFO("Creeper explosion damaged player %u for %.2f damage", target,
               damage.amount);
      continue;
    }

    HealthComponent *health = (HealthComponent *)ecs_get_component(
        (World *)system->ecs, target_entity, HEALTH_COMPONENT_ID);
    if (health) {
      npc_take_damage(system, target, &damage);
    }
  }

  ecs_query_destroy((World *)system->ecs, query);

  // Spawn explosion particles and sound
  LOG_INFO(
      "Creeper explosion VFX at (%.2f, %.2f, %.2f): particles, smoke, sound",
      position.x, position.y, position.z);

  // Destroy the creeper
  npc_on_death(system, entity, position, NPC_TYPE_CREEPER);
}

// Update creeper behavior (charge towards player and explode)
void npc_creeper_update(struct NPCSystem *system, EntityID entity,
                        NPCComponent *creeper, TransformComponent *transform,
                        f32 delta_time) {
  if (!creeper || !transform)
    return;

  // Check if near player
  QueryDesc player_desc = {0};
  ComponentType player_components[] = {PLAYER_COMPONENT_ID,
                                       TRANSFORM_COMPONENT_ID};
  player_desc.all_components = player_components;
  player_desc.all_count = 2;
  Query *player_query = ecs_query_create((World *)system->ecs, &player_desc);

  Entity p_entity;
  void *p_comps[2];
  if (ecs_query_next(player_query, &p_entity, p_comps)) {
    TransformComponent *player_transform = (TransformComponent *)p_comps[1];
    if (player_transform) {
      f32 distance = vec3_length(
          vec3_sub(player_transform->position, transform->position));

      // Start hissing (charge up) when within 2 blocks
      if (distance < 2.0f) {
        if (creeper->behavior_timer <= 0.0f) {
          creeper->behavior_timer = 1.5f; // 1.5 second fuse
        }

        creeper->behavior_timer -= delta_time;

        // Explode when timer reaches zero
        if (creeper->behavior_timer <= 0.0f) {
          npc_creeper_explode(system, entity, transform->position);
        }
      } else {
        // Stop charging if player moves away
        if (creeper->behavior_timer > 0.0f && creeper->behavior_timer < 1.5f) {
          creeper->behavior_timer = 0.0f;
        }
      }
    }
  }

  ecs_query_destroy((World *)system->ecs, player_query);
}

// Skeleton ranged attack
void npc_skeleton_shoot(struct NPCSystem *system, EntityID entity,
                        Vec3 position, Vec3 target_position) {
  if (!system)
    return;

  // Calculate direction to target
  Vec3 direction = vec3_normalize(vec3_sub(target_position, position));

  // Add slight upward arc for arrow trajectory
  direction.y += 0.2f;
  direction = vec3_normalize(direction);

  // Create arrow projectile entity (placeholder until projectile system is
  // implemented)
  Vec3 arrow_velocity = vec3_mul(direction, 20.0f);
  f32 arrow_damage = 4.0f;

  LOG_INFO("Skeleton %u shot arrow from (%.2f, %.2f, %.2f) towards (%.2f, "
           "%.2f, %.2f) with velocity (%.2f, %.2f, %.2f) and damage %.2f",
           entity, position.x, position.y, position.z, target_position.x,
           target_position.y, target_position.z, arrow_velocity.x,
           arrow_velocity.y, arrow_velocity.z, arrow_damage);

  // arrow_entity = ecs_create_entity(system->ecs);
  // ProjectileComponent *projectile = ecs_add_component(system->ecs,
  // arrow_entity, PROJECTILE_COMPONENT_ID); projectile->velocity =
  // arrow_velocity; projectile->damage = arrow_damage; projectile->owner =
  // entity;

  (void)direction;
}

// Update skeleton behavior (maintain distance and shoot)
void npc_skeleton_update(struct NPCSystem *system, EntityID entity,
                         NPCComponent *skeleton, TransformComponent *transform,
                         f32 delta_time) {
  if (!skeleton || !transform)
    return;

  // Check if player in range
  QueryDesc p_desc = {0};
  ComponentType p_components[] = {PLAYER_COMPONENT_ID, TRANSFORM_COMPONENT_ID};
  p_desc.all_components = p_components;
  p_desc.all_count = 2;
  Query *p_query = ecs_query_create((World *)system->ecs, &p_desc);

  Entity p_ent;
  void *p_c[2];
  if (ecs_query_next(p_query, &p_ent, p_c)) {
    TransformComponent *player_transform = (TransformComponent *)p_c[1];
    if (player_transform) {
      f32 distance = vec3_length(
          vec3_sub(player_transform->position, transform->position));

      NPCStats stats = npc_get_stats(skeleton->type);

      // If too close, back away
      if (distance < stats.attack_range * 0.5f) {
        skeleton->state = NPC_STATE_FLEEING;
        skeleton->flee_target = p_ent.id;
        skeleton->panic_timer = 2.0f;
      }
      // If in range, shoot
      else if (distance <= stats.attack_range &&
               skeleton->behavior_timer <= 0.0f) {
        npc_skeleton_shoot(system, entity, transform->position,
                           player_transform->position);
        skeleton->behavior_timer = 2.0f; // 2 second cooldown
      }
    }
  }

  ecs_query_destroy((World *)system->ecs, p_query);
}

// Check and execute special abilities for NPCs
void npc_check_special_abilities(struct NPCSystem *system, EntityID entity,
                                 NPCComponent *npc,
                                 TransformComponent *transform,
                                 f32 delta_time) {
  if (!npc || !transform)
    return;

  switch (npc->type) {
  case NPC_TYPE_CREEPER:
    npc_creeper_update(system, entity, npc, transform, delta_time);
    break;

  case NPC_TYPE_SKELETON:
    npc_skeleton_update(system, entity, npc, transform, delta_time);
    break;

  default:
    // No special abilities
    break;
  }
}
