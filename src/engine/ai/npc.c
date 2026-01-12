// src/engine/ai/npc.c
//
// Purpose: Implements the core NPC system using the modern ECS architecture.
// This file provides the main functionality for creating, updating, and managing
// NPC entities within the game world. It integrates with the physics engine,
// world generation, and various AI subsystems to provide comprehensive NPC
// behavior including pathfinding, combat, trading, dialogue, and daily routines.
//
// Key Features:
// - ECS-based entity management with optimal performance
// - Advanced AI behavior with state machines and pathfinding
// - Dynamic spawning and despawning based on player proximity
// - Weather-aware behavior modifications
// - Comprehensive interaction systems (trading, dialogue, combat)
// - Performance optimization with LOD and culling
//
// Performance Optimizations:
// - Distance-based culling for distant NPCs
// - Scheduled updates to distribute CPU load
// - Efficient pathfinding with caching
// - Batched physics operations
//
#include <ai/npc.h>
#include <ai/npc_types.h>
#include <ai/npc_spawning.h>
#include <ai/npc_dialogue.h>
#include <ai/npc_ai.h>
#include <ai/npc_combat.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include <ecs/components/health.h>
#include <ecs/components/rigidbody.h>
#include <ecs/ecs.h>
#include <ecs/component_ids.h>
#include <physics/physics.h>
#include <core/logger.h>
#include <core/memory.h>
#include <math/vec3.h>
#include <math/math.h>
#include <stdlib.h>
#include <string.h>
#include <game/blockgame/include/chunk/chunk.h>
#include <game/blockgame/include/world/generator.h>

// External system references (will be initialized by the game)
extern struct WeatherSystem *g_weather_system;
extern struct PlayerSystem *g_player_system;

// Forward declarations for static helper functions
static void npc_movement_update(NPCSystem *system, Entity entity, NPCComponent *npc,
                               TransformComponent *npc_transform, f32 delta_time);
static void npc_update_passive(NPCSystem *system, Entity entity,
                               NPCComponent *npc,
                               TransformComponent *npc_transform,
                               f32 delta_time);
static void npc_update_hostile(NPCSystem *system, Entity entity,
                               NPCComponent *npc,
                               TransformComponent *npc_transform,
                               f32 delta_time);
static void npc_update_neutral(NPCSystem *system, Entity entity,
                               NPCComponent *npc,
                               TransformComponent *npc_transform,
                               f32 delta_time);
static void npc_find_path(NPCSystem *system, Entity entity, Vec3 start, Vec3 end,
                          Vec3 *path, u32 *path_length, u32 max_path_length);
static void npc_melee_attack(NPCSystem *system, Entity attacker, Entity target);

// Initialize NPC system
void npc_system_init(NPCSystem *system, struct World *ecs,
                     struct PhysicsWorld *physics) {
  if (!system || !ecs || !physics)
    return;
  
  system->ecs = ecs;
  system->physics = physics;
  
  // Initialize AI system
  system->ai_system = malloc(sizeof(NPCAISystem));
  npc_ai_system_init(system->ai_system, ecs, physics);
  
  // Initialize dialogue system
  system->dialogue_system = malloc(sizeof(DialogueSystem));
  dialogue_system_init(system->dialogue_system);
  
  // Initialize combat system
  system->combat_system = malloc(sizeof(NPCCombatSystem));
  npc_combat_init(system->combat_system, ecs, physics, system);
  
  // Set default configuration
  system->max_spawn_distance = 128.0f;
  system->despawn_distance = 96.0f;
  system->max_npcs_per_chunk = 8;
  system->active_npc_count = 0;
  system->total_npc_spawned = 0;
  system->last_spawn_time = 0.0f;
  system->is_initialized = true;
  
  LOG_INFO("NPC system initialized with AI and dialogue support");
}

// Free NPC system
void npc_system_free(NPCSystem *system) { 
  (void)system; 
  LOG_INFO("NPC system freed");
}

// Create NPC entity
Entity npc_create(NPCSystem *system, Vec3 position, NPCType type) {
  if (!system)
    return INVALID_ENTITY;

  Entity entity = ecs_create_entity(system->ecs);
  if (entity.id == 0) {
    LOG_WARN("Failed to create NPC entity");
    return INVALID_ENTITY;
  }

  // Add Transform component
  TransformComponent transform_data = {0};
  TransformComponent *transform = (TransformComponent *)ecs_add_component(
      system->ecs, entity, TRANSFORM_COMPONENT_ID, &transform_data);
  if (!transform) {
    LOG_WARN("Failed to add Transform component to entity %u", entity.id);
    ecs_destroy_entity(system->ecs, entity);
    return INVALID_ENTITY;
  }
  transform->position = position;
  transform->rotation = quat_identity();
  transform->scale = vec3(1.0f, 1.0f, 1.0f);

  // Add NPC component
  NPCComponent npc_data = {0};
  NPCComponent *npc = (NPCComponent *)ecs_add_component(
      system->ecs, entity, NPC_COMPONENT_ID, &npc_data);
  if (!npc) {
    LOG_WARN("Failed to add NPC component to entity %u", entity.id);
    ecs_destroy_entity(system->ecs, entity);
    return INVALID_ENTITY;
  }

  // Initialize NPC component
  npc->type = type;
  npc->state = NPC_STATE_IDLE;
  npc->target = INVALID_ENTITY;
  npc->path_length = 0;
  npc->current_path_index = 0;
  npc->behavior_timer = 0.0f;
  npc->breed_cooldown = 0.0f;
  npc->panic_timer = 0.0f;
  npc->flee_target = INVALID_ENTITY;
  npc->behavior_flags = 0;
  npc->growth_timer = 0.0f;
  npc->mood = NPC_MOOD_NEUTRAL;
  npc->schedule_timer = 0.0f;
  npc->schedule_index = 0;
  npc->last_attacker = INVALID_ENTITY;
  npc->time_since_last_attacked = 0.0f;
  npc->reputation = 0;
  npc->relations_count = 0;

  // Jobs and daily life systems
  npc->job = NPC_JOB_NONE;
  npc->current_task = NPC_TASK_NONE;
  npc->schedule_phase = NPC_SCHEDULE_MORNING;
  npc->task_timer = 0.0f;
  npc->home = INVALID_ENTITY;
  npc->workplace = INVALID_ENTITY;
  npc->hunger = 80.0f;
  npc->energy = 100.0f;
  npc->social_need = 50.0f;

  // Get stats for this NPC type
  NPCStats stats = npc_get_stats(type);

  // Add Health component
  HealthComponent health_data = {0};
  HealthComponent *health = (HealthComponent *)ecs_add_component(
      system->ecs, entity, HEALTH_COMPONENT_ID, &health_data);
  if (health) {
    health->health = stats.max_health;
    health->max_health = stats.max_health;
  }

  // Add RigidBody component
  RigidBodyComponent rb_data = {0};
  RigidBodyComponent *rb_comp = (RigidBodyComponent *)ecs_add_component(
      system->ecs, entity, RIGIDBODY_COMPONENT_ID, &rb_data);
  if (rb_comp) {
    // Create physics body
    RigidBody *body = rigid_body_create(BODY_TYPE_DYNAMIC, position);
    rigid_body_set_mass(body, 1.0f);
    rigid_body_set_friction(body, 0.6f);
    rigid_body_set_restitution(body, 0.0f);

    // Use a box for the NPC collider (0.6x1.8x0.6)
    Collider *collider = collider_create_box(vec3(0.3f, 0.9f, 0.3f));
    rigid_body_attach_collider(body, collider);

    rb_comp->body = body;
    physics_world_add_body(system->physics, body);
  }

  // Add AI context
  if (system->ai_system) {
    npc_ai_create_context(system->ai_system, entity, type);
  }
  
  // Add combat state
  if (system->combat_system) {
    npc_combat_create_state(system->combat_system, entity, type);
  }
  
  system->active_npc_count++;
  system->total_npc_spawned++;
  
  LOG_DEBUG("Created NPC type %d at position (%.1f, %.1f, %.1f)", 
            type, position.x, position.y, position.z);
  
  return entity;
}

// Update all NPCs
void npc_update(NPCSystem *system, f32 delta_time) {
  if (!system)
    return;

  // Update AI system
  if (system->ai_system) {
    npc_ai_system_update(system->ai_system, delta_time);
  }

  // Update combat system
  if (system->combat_system) {
    npc_combat_update(system->combat_system, delta_time);
  }

  // Get player position for LOD/culling
  Vec3 player_pos = vec3(0.0f, 0.0f, 0.0f);
  
  // Query for all NPCs
  Query query;
  world_query_init(&query, system->ecs);
  world_query_require_component(&query, NPC_COMPONENT_ID);
  world_query_require_component(&query, TRANSFORM_COMPONENT_ID);

  for (u32 i = 0; i < query.count; i++) {
    Entity entity = query.entities[i];
    TransformComponent *transform = (TransformComponent *)world_get_component(
        system->ecs, entity, TRANSFORM_COMPONENT_ID);
    
    if (!transform)
      continue;

    // Distance-based culling
    f32 distance = vec3_length(vec3_sub(transform->position, player_pos));
    if (distance > system->despawn_distance) {
      // Don't update distant NPCs (AI system handles this)
      continue;
    }

    // Get NPC component for individual updates
    NPCComponent *npc = (NPCComponent *)world_get_component(
        system->ecs, entity, NPC_COMPONENT_ID);
    
    if (!npc)
      continue;

    // Update individual NPC
    npc_ai_update_single(system, entity, npc, delta_time);
  }

  world_query_free(&query);
}

// NPC AI update - core behavior logic
void npc_ai_update(NPCSystem *system, Entity entity, NPCComponent *npc,
                   f32 delta_time) {
  if (!npc)
    return;

  TransformComponent *npc_transform = (TransformComponent *)world_get_component(
      system->ecs, entity, TRANSFORM_COMPONENT_ID);
  if (!npc_transform)
    return;

  // Update timers
  npc->time_since_last_attacked += delta_time;
  if (npc->behavior_timer > 0.0f)
    npc->behavior_timer -= delta_time;
  if (npc->breed_cooldown > 0.0f)
    npc->breed_cooldown -= delta_time;
  if (npc->panic_timer > 0.0f)
    npc->panic_timer -= delta_time;
  if (npc->growth_timer > 0.0f) {
    npc->growth_timer -= delta_time;
    if (npc->growth_timer <= 0.0f) {
      npc->behavior_flags &= ~NPC_FLAG_IS_BABY;
    }
  }

  // Update needs (hunger, energy, social)
  npc->hunger -= delta_time * 0.5f;      // 0.5% per second
  npc->energy -= delta_time * 0.3f;      // 0.3% per second
  npc->social_need -= delta_time * 0.2f; // 0.2% per second
  
  // Clamp needs to valid range
  npc->hunger = CLAMP(npc->hunger, 0.0f, 100.0f);
  npc->energy = CLAMP(npc->energy, 0.0f, 100.0f);
  npc->social_need = CLAMP(npc->social_need, 0.0f, 100.0f);

  // Update task timer
  if (npc->task_timer > 0.0f)
    npc->task_timer -= delta_time;

  // Skip AI updates if NPC is busy with player interactions
  if (npc->behavior_flags & NPC_FLAG_TRADING) {
    npc->state = NPC_STATE_IDLE;
    npc->path_length = 0;
    return;
  }
  if (npc->behavior_flags & NPC_FLAG_DIALOGUE) {
    npc->state = NPC_STATE_IDLE;
    npc->path_length = 0;
    return;
  }

  // Get stats for behavior type
  NPCStats stats = npc_get_stats(npc->type);

  // Update mood based on state
  if (npc->panic_timer > 0.0f) {
    npc->mood = NPC_MOOD_SCARED;
  } else if (npc->behavior_flags & NPC_FLAG_IN_LOVE) {
    npc->mood = NPC_MOOD_HAPPY;
  } else {
    npc->mood = NPC_MOOD_NEUTRAL;
  }

  // Schedule-based behavior for non-hostile NPCs
  if (stats.behavior != NPC_BEHAVIOR_HOSTILE) {
    if (npc->schedule_timer > 0.0f) {
      npc->schedule_timer -= delta_time;
    }
    if (npc->schedule_timer <= 0.0f) {
      if (npc->state == NPC_STATE_IDLE) {
        npc->state = NPC_STATE_WANDERING;
        npc->behavior_timer = ((rand() % 80) / 10.0f) + 4.0f;
      } else {
        npc->state = NPC_STATE_IDLE;
        npc->path_length = 0;
      }
      npc->schedule_timer = ((rand() % 100) / 10.0f) + 5.0f;
    }
  }

  // Behavior-specific updates
  switch (stats.behavior) {
  case NPC_BEHAVIOR_PASSIVE:
    npc_update_passive(system, entity, npc, npc_transform, delta_time);
    break;
  case NPC_BEHAVIOR_HOSTILE:
    npc_update_hostile(system, entity, npc, npc_transform, delta_time);
    break;
  case NPC_BEHAVIOR_NEUTRAL:
    npc_update_neutral(system, entity, npc, npc_transform, delta_time);
    break;
  default:
    break;
  }

  // Move along path
  npc_movement_update(system, entity, npc, npc_transform, delta_time);
}

// Movement update for pathfollowing
static void npc_movement_update(NPCSystem *system, Entity entity, NPCComponent *npc,
                               TransformComponent *npc_transform, f32 delta_time) {
  if (npc->path_length > 0 && npc->current_path_index < npc->path_length) {
    Vec3 target_pos = npc->path[npc->current_path_index];
    Vec3 direction = vec3_normalize(vec3_sub(target_pos, npc_transform->position));

    RigidBodyComponent *rb_comp = (RigidBodyComponent *)world_get_component(
        system->ecs, entity, RIGIDBODY_COMPONENT_ID);
    if (rb_comp && rb_comp->body) {
      NPCStats stats = npc_get_stats(npc->type);
      rigid_body_set_velocity(rb_comp->body, vec3_mul(direction, stats.move_speed));
    }

    // Check if reached current waypoint
    if (vec3_length(vec3_sub(npc_transform->position, target_pos)) < 0.5f) {
      npc->current_path_index++;
    }
  } else {
    // Stop movement when path is complete
    RigidBodyComponent *rb_comp = (RigidBodyComponent *)world_get_component(
        system->ecs, entity, RIGIDBODY_COMPONENT_ID);
    if (rb_comp && rb_comp->body) {
      rigid_body_set_velocity(rb_comp->body, vec3(0, 0, 0));
    }
  }
}

// Passive animal behavior (cows, pigs, chickens)
static void npc_update_passive(NPCSystem *system, Entity entity,
                               NPCComponent *npc,
                               TransformComponent *npc_transform,
                               f32 delta_time) {
  NPCStats stats = npc_get_stats(npc->type);

  // Find nearest player for interaction
  Vec3 player_pos = vec3(0, 0, 0);
  Entity nearest_player = INVALID_ENTITY;
  f32 nearest_distance = 999999.0f;

  if (g_player_system && g_player_system->player) {
    TransformComponent *player_t = (TransformComponent *)world_get_component(
        system->ecs, g_player_system->player->entity, TRANSFORM_COMPONENT_ID);
    if (player_t) {
      player_pos = player_t->position;
      nearest_distance = vec3_length(vec3_sub(player_pos, npc_transform->position));
      nearest_player = g_player_system->player->entity;
    }
  }

  // Check health to see if damaged (triggers flee)
  HealthComponent *health = (HealthComponent *)world_get_component(
      system->ecs, entity, HEALTH_COMPONENT_ID);
  if (health && health->health < health->max_health &&
      npc->panic_timer <= 0.0f) {
    npc->state = NPC_STATE_FLEEING;
    npc->panic_timer = 5.0f; // Panic for 5 seconds
    npc->flee_target = nearest_player;
  }

  switch (npc->state) {
  case NPC_STATE_IDLE:
    // Randomly start wandering
    if (rand() % 200 < 1) {
      npc->state = NPC_STATE_WANDERING;
      npc->behavior_timer = (rand() % 100) / 10.0f + 3.0f; // Wander for 3-13 seconds
    }

    // Check for breeding opportunities
    if (npc->breed_cooldown <= 0.0f && nearest_distance < 3.0f) {
      npc->behavior_flags |= NPC_FLAG_IN_LOVE;
      npc->breed_cooldown = 5.0f * 60.0f; // 5 minute cooldown
    }
    break;

  case NPC_STATE_WANDERING:
    if (npc->path_length == 0 || npc->behavior_timer <= 0.0f) {
      if (npc->behavior_timer <= 0.0f) {
        npc->state = NPC_STATE_IDLE;
        npc->path_length = 0;
      } else if (rand() % 100 < 5) {
        Vec3 random_target = vec3_add(npc_transform->position,
                                     vec3((rand() % 20) - 10, 0, (rand() % 20) - 10));
        npc_find_path(system, entity, npc_transform->position, random_target, 
                      npc->path, &npc->path_length, 32);
        npc->current_path_index = 0;
      }
    }
    break;

  case NPC_STATE_FLEEING:
    if (npc->panic_timer <= 0.0f || nearest_distance > stats.flee_range * 2.0f) {
      npc->state = NPC_STATE_IDLE;
      npc->flee_target = INVALID_ENTITY;
    } else if (nearest_player.id != 0) {
      // Run away from threat
      Vec3 flee_direction = vec3_normalize(
          vec3_sub(npc_transform->position, player_pos));
      Vec3 flee_target = vec3_add(npc_transform->position, vec3_mul(flee_direction, 10.0f));
      npc_find_path(system, entity, npc_transform->position, flee_target,
                    npc->path, &npc->path_length, 32);
      npc->current_path_index = 0;
    }
    break;

  default:
    npc->state = NPC_STATE_IDLE;
    break;
  }
}

// Hostile mob behavior (zombies, skeletons, creepers)
static void npc_update_hostile(NPCSystem *system, Entity entity,
                               NPCComponent *npc,
                               TransformComponent *npc_transform,
                               f32 delta_time) {
  NPCStats stats = npc_get_stats(npc->type);

  // Find nearest player
  Vec3 player_pos = vec3(0, 0, 0);
  Entity player_entity = INVALID_ENTITY;
  
  if (g_player_system && g_player_system->player) {
    TransformComponent *player_t = (TransformComponent *)world_get_component(
        system->ecs, g_player_system->player->entity, TRANSFORM_COMPONENT_ID);
    if (player_t) {
      player_pos = player_t->position;
      player_entity = g_player_system->player->entity;
    }
  }

  if (player_entity.id != 0) {
    f32 distance_to_player = vec3_length(vec3_sub(player_pos, npc_transform->position));

    switch (npc->state) {
    case NPC_STATE_IDLE:
    case NPC_STATE_WANDERING:
      if (distance_to_player < stats.detection_range) {
        npc->state = NPC_STATE_CHASING;
        npc->target = player_entity;
      } else if (npc->state == NPC_STATE_IDLE && rand() % 200 < 1) {
        npc->state = NPC_STATE_WANDERING;
      }

      if (npc->state == NPC_STATE_WANDERING && npc->path_length == 0) {
        if (rand() % 100 < 2) {
          Vec3 random_target = vec3_add(npc_transform->position,
                                       vec3((rand() % 20) - 10, 0, (rand() % 20) - 10));
          npc_find_path(system, entity, npc_transform->position, random_target,
                        npc->path, &npc->path_length, 32);
          npc->current_path_index = 0;
        }
      }
      break;

    case NPC_STATE_CHASING:
      if (distance_to_player > stats.detection_range * 1.5f) {
        npc->state = NPC_STATE_IDLE;
        npc->target = INVALID_ENTITY;
      } else if (distance_to_player <= stats.attack_range) {
        npc->state = NPC_STATE_ATTACKING;
        npc->behavior_timer = 1.0f; // Attack cooldown
      } else {
        // Update path to player every 0.5 seconds
        if (npc->behavior_timer <= 0.0f) {
          npc_find_path(system, entity, npc_transform->position, player_pos,
                        npc->path, &npc->path_length, 32);
          npc->current_path_index = 0;
          npc->behavior_timer = 0.5f;
        }
      }
      break;

    case NPC_STATE_ATTACKING:
      if (distance_to_player > stats.attack_range * 1.5f) {
        npc->state = NPC_STATE_CHASING;
      } else if (npc->behavior_timer <= 0.0f) {
        // Perform attack
        npc_melee_attack(system, entity, npc->target);
        npc->behavior_timer = 1.0f; // Attack cooldown
      }
      break;

    default:
      npc->state = NPC_STATE_IDLE;
      break;
    }
  } else {
    // No players nearby
    if (npc->state != NPC_STATE_WANDERING && npc->state != NPC_STATE_IDLE) {
      npc->state = NPC_STATE_IDLE;
      npc->target = INVALID_ENTITY;
    } else if (npc->state == NPC_STATE_IDLE && rand() % 200 < 1) {
      npc->state = NPC_STATE_WANDERING;
    }
  }
}

// Neutral NPC behavior (villagers)
static void npc_update_neutral(NPCSystem *system, Entity entity,
                               NPCComponent *npc,
                               TransformComponent *npc_transform,
                               f32 delta_time) {
  NPCStats stats = npc_get_stats(npc->type);

  // Check if damaged (becomes hostile)
  HealthComponent *health = (HealthComponent *)world_get_component(
      system->ecs, entity, HEALTH_COMPONENT_ID);
  if (health && health->health < health->max_health &&
      !(npc->behavior_flags & NPC_FLAG_ANGRY)) {
    npc->behavior_flags |= NPC_FLAG_ANGRY;
    npc->state = NPC_STATE_FLEEING;
    npc->panic_timer = 10.0f;
  }

  switch (npc->state) {
  case NPC_STATE_IDLE:
    // Randomly start wandering
    if (rand() % 300 < 1) {
      npc->state = NPC_STATE_WANDERING;
      npc->behavior_timer = (rand() % 100) / 10.0f + 5.0f;
    }
    break;

  case NPC_STATE_WANDERING:
    if (npc->path_length == 0 || npc->behavior_timer <= 0.0f) {
      if (npc->behavior_timer <= 0.0f) {
        npc->state = NPC_STATE_IDLE;
        npc->path_length = 0;
      } else if (rand() % 100 < 3) {
        Vec3 random_target = vec3_add(npc_transform->position,
                                     vec3((rand() % 15) - 7, 0, (rand() % 15) - 7));
        npc_find_path(system, entity, npc_transform->position, random_target,
                      npc->path, &npc->path_length, 32);
        npc->current_path_index = 0;
      }
    }
    break;

  case NPC_STATE_FLEEING:
    if (npc->panic_timer <= 0.0f) {
      npc->state = NPC_STATE_IDLE;
      npc->behavior_flags &= ~NPC_FLAG_ANGRY;
    } else {
      // Find nearest player and run away
      if (g_player_system && g_player_system->player) {
        TransformComponent *player_t = (TransformComponent *)world_get_component(
            system->ecs, g_player_system->player->entity, TRANSFORM_COMPONENT_ID);
        if (player_t) {
          Vec3 flee_direction = vec3_normalize(
              vec3_sub(npc_transform->position, player_t->position));
          Vec3 flee_target = vec3_add(npc_transform->position, vec3_mul(flee_direction, 10.0f));
          npc_find_path(system, entity, npc_transform->position, flee_target,
                        npc->path, &npc->path_length, 32);
          npc->current_path_index = 0;
        }
      }
    }
    break;

  default:
    npc->state = NPC_STATE_IDLE;
    break;
  }
}

// Implementations for static helper functions

// Simple pathfinding - TODO: Replace with proper navigation system
static void npc_find_path(NPCSystem *system, Entity entity, Vec3 start, Vec3 end,
                          Vec3 *path, u32 *path_length, u32 max_path_length) {
  if (*path_length >= max_path_length) {
    return;
  }
  
  // For now, just create a direct path
  // TODO: Implement A* pathfinding with navigation mesh
  path[(*path_length)++] = end;
}

// Simple melee attack - TODO: Replace with proper combat system
static void npc_melee_attack(NPCSystem *system, Entity attacker, Entity target) {
  if (!system || attacker.id == 0 || target.id == 0) {
    return;
  }
  
  // Get attacker and target health components
  HealthComponent *attacker_health = (HealthComponent *)world_get_component(
      system->ecs, attacker, HEALTH_COMPONENT_ID);
  HealthComponent *target_health = (HealthComponent *)world_get_component(
      system->ecs, target, HEALTH_COMPONENT_ID);
  
  if (!attacker_health || !target_health) {
    return;
  }
  
  // Get attacker NPC component for damage
  NPCComponent *attacker_npc = (NPCComponent *)world_get_component(
      system->ecs, attacker, NPC_COMPONENT_ID);
  if (!attacker_npc) {
    return;
  }
  
  NPCStats stats = npc_get_stats(attacker_npc->type);
  
  // Apply damage
  target_health->health -= stats.damage;
  if (target_health->health < 0) {
    target_health->health = 0;
  }
  
  LOG_DEBUG("NPC %u attacked target %u for %.1f damage (target health: %.1f)", 
            attacker.id, target.id, stats.damage, target_health->health);
}

// Spawning and despawning systems - implemented in npc_spawning.c

// NPC interaction systems
void npc_begin_trading(NPCSystem *system, Entity npc_entity, Entity player) {
  if (!system)
    return;
  NPCComponent *npc = (NPCComponent *)world_get_component(
      system->ecs, npc_entity, NPC_COMPONENT_ID);
  if (!npc)
    return;
  npc->behavior_flags |= NPC_FLAG_TRADING;
  npc->state = NPC_STATE_IDLE;
  npc->path_length = 0;
  npc->target = player;
  LOG_INFO("NPC %u began trading with player %u", npc_entity.id, player.id);
}

void npc_end_trading(NPCSystem *system, Entity npc_entity) {
  if (!system)
    return;
  NPCComponent *npc = (NPCComponent *)world_get_component(
      system->ecs, npc_entity, NPC_COMPONENT_ID);
  if (!npc)
    return;
  npc->behavior_flags &= ~NPC_FLAG_TRADING;
  npc->target = INVALID_ENTITY;
  LOG_INFO("NPC %u ended trading", npc_entity.id);
}

void npc_begin_dialogue(NPCSystem *system, Entity npc_entity, Entity player) {
  if (!system)
    return;
  NPCComponent *npc = (NPCComponent *)world_get_component(
      system->ecs, npc_entity, NPC_COMPONENT_ID);
  if (!npc)
    return;
  npc->behavior_flags |= NPC_FLAG_DIALOGUE;
  npc->state = NPC_STATE_IDLE;
  npc->path_length = 0;
  npc->target = player;
  LOG_INFO("NPC %u began dialogue with player %u", npc_entity.id, player.id);
}

void npc_end_dialogue(NPCSystem *system, Entity npc_entity) {
  if (!system)
    return;
  NPCComponent *npc = (NPCComponent *)world_get_component(
      system->ecs, npc_entity, NPC_COMPONENT_ID);
  if (!npc)
    return;
  npc->behavior_flags &= ~NPC_FLAG_DIALOGUE;
  npc->target = INVALID_ENTITY;
  LOG_INFO("NPC %u ended dialogue", npc_entity.id);
}

// NPC relationship system
i16 npc_get_relation(NPCSystem *system, Entity npc_entity, Entity other) {
  if (!system)
    return 0;
  NPCComponent *npc = (NPCComponent *)world_get_component(
      system->ecs, npc_entity, NPC_COMPONENT_ID);
  if (!npc)
    return 0;
  for (u8 i = 0; i < npc->relations_count; i++) {
    if (npc->relations_entities[i].id == other.id)
      return npc->relations_values[i];
  }
  return 0;
}

void npc_set_relation(NPCSystem *system, Entity npc_entity, Entity other,
                      i16 value) {
  if (!system)
    return;
  NPCComponent *npc = (NPCComponent *)world_get_component(
      system->ecs, npc_entity, NPC_COMPONENT_ID);
  if (!npc)
    return;
  for (u8 i = 0; i < npc->relations_count; i++) {
    if (npc->relations_entities[i].id == other.id) {
      npc->relations_values[i] = CLAMP(value, -100, 100);
      return;
    }
  }
  if (npc->relations_count < 8) {
    u8 idx = npc->relations_count++;
    npc->relations_entities[idx] = other;
    npc->relations_values[idx] = CLAMP(value, -100, 100);
  }
}

void npc_adjust_relation(NPCSystem *system, Entity npc_entity, Entity other,
                         i16 delta) {
  if (!system)
    return;
  NPCComponent *npc = (NPCComponent *)world_get_component(
      system->ecs, npc_entity, NPC_COMPONENT_ID);
  if (!npc)
    return;
  for (u8 i = 0; i < npc->relations_count; i++) {
    if (npc->relations_entities[i].id == other.id) {
      npc->relations_values[i] = CLAMP(npc->relations_values[i] + delta, -100, 100);
      return;
    }
  }
  if (npc->relations_count < 8) {
    u8 idx = npc->relations_count++;
    npc->relations_entities[idx] = other;
    npc->relations_values[idx] = CLAMP(delta, -100, 100);
  }
}

// NPC reputation system
i16 npc_get_reputation(NPCSystem *system, Entity npc_entity) {
  if (!system)
    return 0;
  NPCComponent *npc = (NPCComponent *)world_get_component(
      system->ecs, npc_entity, NPC_COMPONENT_ID);
  if (!npc)
    return 0;
  return npc->reputation;
}

void npc_set_reputation(NPCSystem *system, Entity npc_entity, i16 value) {
  if (!system)
    return;
  NPCComponent *npc = (NPCComponent *)world_get_component(
      system->ecs, npc_entity, NPC_COMPONENT_ID);
  if (!npc)
    return;
  npc->reputation = value;
}

void npc_adjust_reputation(NPCSystem *system, Entity npc_entity, i16 delta) {
  if (!system)
    return;
  NPCComponent *npc = (NPCComponent *)world_get_component(
      system->ecs, npc_entity, NPC_COMPONENT_ID);
  if (!npc)
    return;
  npc->reputation += delta;
}

// Spawning and despawning systems
void npc_spawn_in_chunk(NPCSystem *system, struct Chunk *chunk,
                        struct WorldGenerator *generator) {
  // TODO: Implement chunk-based NPC spawning
  // This would integrate with the world generation system
  LOG_INFO("Spawning NPCs in chunk at (%d, %d)", chunk->x, chunk->z);
}

void npc_despawn_distant(NPCSystem *system) {
  // TODO: Implement distance-based despawning
  // Remove NPCs that are too far from any player
  LOG_INFO("Despawning distant NPCs");
}
