#include <block/block.h>
#include <combat/combat.h>
#include <core/logger.h>
#include <core/memory.h>
#include <ecs/component_ids.h>
#include <ecs/components/health.h>
#include <ecs/components/npc.h>
#include <ecs/components/rigidbody.h>
#include <ecs/components/transform.h>
#include <ecs/ecs.h>
#include <math.h>
#include <math/vec3.h>
#include <npc/npc.h>
#include <npc/npc_abilities.h>
#include <npc/npc_ai.h>
#include <npc/npc_combat.h>
#include <npc/npc_combat_behavior.h>
#include <npc/npc_perf.h>
#include <npc/npc_schedule.h>
#include <npc/npc_visuals.h>
#include <npc/pathfinding.h>
#include <physics/physics.h>
#include <player/player.h>
#include <stdlib.h>
#include <string.h>
#include <weather/weather.h>

// NPC spawn points: IMPLEMENTED (validation for safe spawn locations).
// ... (rest of the comments)
void npc_system_init(NPCSystem *system, struct World *ecs,
                     struct PhysicsWorld *physics) {
  if (!system)
    return;
  system->ecs = ecs;
  system->physics = physics;
  LOG_INFO("NPC System initialized");
}

void npc_system_free(NPCSystem *system) {
  if (!system)
    return;
  LOG_INFO("NPC System freed");
  memset(system, 0, sizeof(NPCSystem));
}

void npc_spawn_in_chunk(NPCSystem *system, Chunk *chunk,
                        struct WorldGenerator *generator) {
  if (!system || !chunk || !generator)
    return;
  // TODO: Implement procedural NPC spawning based on biomes and world
  // generation
}

void npc_despawn_distant(NPCSystem *system) {
  if (!system)
    return;
  // TODO: Implement despawning NPCs that are too far from players
}

// ... (npc_create implementation)
EntityID npc_create(NPCSystem *system, Vec3 position, NPCType type) {
  if (!system)
    return 0;

  Entity entity = ecs_create_entity((World *)system->ecs);
  EntityID entity_id = entity.id;
  if (entity_id == 0) {
    LOG_WARN("Failed to create NPC entity");
    return 0;
  }

  // Add Transform component
  TransformComponent *transform = (TransformComponent *)ecs_add_component(
      (World *)system->ecs, entity, TRANSFORM_COMPONENT_ID, NULL);
  if (!transform) {
    LOG_WARN("Failed to add Transform component to entity %u", entity_id);
    ecs_destroy_entity((World *)system->ecs, entity);
    return 0;
  }
  transform->position = position;
  transform->rotation = quat_identity();

  // Add NPC component
  NPCComponent *npc = (NPCComponent *)ecs_add_component(
      (World *)system->ecs, entity, NPC_COMPONENT_ID, NULL);
  if (!npc) {
    LOG_WARN("Failed to add NPC component to entity %u", entity_id);
    ecs_destroy_entity((World *)system->ecs, entity);
    return 0;
  }
  // ... (npc initialization)
  npc->type = type;
  npc->state = NPC_STATE_IDLE;
  npc->target = 0; // Should be INVALID_ENTITY
  npc->path_length = 0;
  npc->current_path_index = 0;
  npc->behavior_timer = 0.0f;
  npc->breed_cooldown = 0.0f;
  npc->panic_timer = 0.0f;
  npc->flee_target = 0; // Should be INVALID_ENTITY
  npc->behavior_flags = 0;
  npc->growth_timer = 0.0f;
  npc->mood = NPC_MOOD_NEUTRAL;
  npc->schedule_timer = 0.0f;
  npc->schedule_index = 0;
  npc->last_attacker = 0; // Should be INVALID_ENTITY
  npc->time_since_last_attacked = 0.0f;
  npc->reputation = 0;
  npc->relations_count = 0;

  // Jobs and daily life (Milestone 3)
  npc->job = NPC_JOB_NONE;
  npc->current_task = NPC_TASK_NONE;
  npc->schedule_phase = NPC_SCHEDULE_MORNING;
  npc->task_timer = 0.0f;
  npc->home = 0;
  npc->workplace = 0;
  npc->hunger = 80.0f;
  npc->energy = 100.0f;
  npc->social_need = 50.0f;

  // Get stats for this NPC type
  NPCStats stats = npc_get_stats(type);

  HealthComponent *health = (HealthComponent *)ecs_add_component(
      (World *)system->ecs, entity, HEALTH_COMPONENT_ID, NULL);
  health->health = stats.max_health;
  health->max_health = stats.max_health;

  RigidBodyComponent *rb_comp = (RigidBodyComponent *)ecs_add_component(
      (World *)system->ecs, entity, RIGIDBODY_COMPONENT_ID, NULL);

  // Create physics body
  RigidBody *body = rigid_body_create(BODY_TYPE_DYNAMIC, position);
  rigid_body_set_mass(body, 1.0f);
  // rigid_body_set_friction(body, 0.6f); // Might be rb_set_friction
  // rigid_body_set_restitution(body, 0.0f);

  // Use a box for the NPC collider for now (0.6x1.8x0.6)
  // Collider *collider = collider_create_box(vec3(0.3f, 0.9f, 0.3f));
  // rigid_body_attach_collider(body, collider);

  rb_comp->body = body;
  physics_world_add_body(system->physics, body);

  return entity_id;
}

// Update NPC
void npc_update(NPCSystem *system, f32 delta_time) {
  if (!system)
    return;

  npc_profile_begin("npc_update");
  npc_scheduler_tick();

  // Get global weather system for weather effects
  extern WeatherSystem *g_weather_system;

  // Get player position for LOD/culling
  Vec3 player_pos = vec3(0, 0, 0);
  extern PlayerSystem *g_player_system;
  if (g_player_system && g_player_system->player) {
    TransformComponent *player_t = (TransformComponent *)ecs_get_component(
        (World *)system->ecs, (Entity){g_player_system->player->entity_id, 0},
        TRANSFORM_COMPONENT_ID);
    if (player_t)
      player_pos = player_t->position;
  }

  npc_lod_update(system, player_pos);

  QueryDesc desc = {0};
  ComponentTypeID components[] = {NPC_COMPONENT_ID, TRANSFORM_COMPONENT_ID};
  desc.all_components = components;
  desc.all_count = 2;
  Query *query = ecs_query_create((World *)system->ecs, &desc);

  Entity entity;
  void *comps[2];
  while (ecs_query_next(query, &entity, comps)) {
    EntityID entity_id = entity.id;
    if (!npc_scheduler_should_update(entity_id))
      continue;
    if (!npc_should_update(entity_id, delta_time))
      continue;

    npc_profile_begin("npc_ai_update");
    NPCComponent *npc = (NPCComponent *)comps[0];

    // Apply weather effects to NPC behavior
    if (g_weather_system) {
      WeatherType current_weather = weather_get_current_type(g_weather_system);
      f32 weather_intensity = weather_get_intensity(g_weather_system);

      // NPCs seek shelter during storms
      if (current_weather == WEATHER_STORM ||
          current_weather == WEATHER_BLIZZARD) {
        if (weather_intensity > 0.5f) {
          npc->behavior_flags |= NPC_BEHAVIOR_SEEK_SHELTER;
        }
      } else {
        npc->behavior_flags &= ~NPC_BEHAVIOR_SEEK_SHELTER;
      }

      // NPCs move slower during heavy rain/snow
      if (current_weather == WEATHER_RAIN_HEAVY ||
          current_weather == WEATHER_SNOW_HEAVY) {
        npc->behavior_flags |= NPC_BEHAVIOR_SLOWED;
      } else {
        npc->behavior_flags &= ~NPC_BEHAVIOR_SLOWED;
      }

      // NPCs are more active during clear weather
      if (current_weather == WEATHER_CLEAR) {
        npc->behavior_flags |= NPC_BEHAVIOR_ACTIVE;
      } else {
        npc->behavior_flags &= ~NPC_BEHAVIOR_ACTIVE;
      }
    }

    npc_ai_update(system, entity_id, npc, delta_time);
    npc_profile_end("npc_ai_update");
  }

  ecs_query_destroy((World *)system->ecs, query);
  npc_profile_end("npc_update");
}

// Forward declarations for behavior handlers
static void npc_update_passive(NPCSystem *system, EntityID entity,
                               NPCComponent *npc,
                               TransformComponent *npc_transform,
                               f32 delta_time);
static void npc_update_hostile(NPCSystem *system, EntityID entity,
                               NPCComponent *npc,
                               TransformComponent *npc_transform,
                               f32 delta_time);
static void npc_update_neutral(NPCSystem *system, EntityID entity,
                               NPCComponent *npc,
                               TransformComponent *npc_transform,
                               f32 delta_time);

// NPC AI update - delegates to type-specific handlers
void npc_ai_update(NPCSystem *system, EntityID entity, NPCComponent *npc,
                   f32 delta_time) {
  if (!npc)
    return;

  TransformComponent *npc_transform = (TransformComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){entity, 0}, TRANSFORM_COMPONENT_ID);
  if (!npc_transform)
    return;

  npc->time_since_last_attacked += delta_time;

  // Update needs (Milestone 3)
  npc->hunger -= delta_time * 0.5f;      // 0.5% per second
  npc->energy -= delta_time * 0.3f;      // 0.3% per second
  npc->social_need -= delta_time * 0.2f; // 0.2% per second
  if (npc->hunger < 0.0f)
    npc->hunger = 0.0f;
  if (npc->energy < 0.0f)
    npc->energy = 0.0f;
  if (npc->social_need < 0.0f)
    npc->social_need = 0.0f;

  // Update task timer
  if (npc->task_timer > 0.0f)
    npc->task_timer -= delta_time;
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

  // Update timers
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

  // Get stats for behavior type
  NPCStats stats = npc_get_stats(npc->type);

  if (npc->panic_timer > 0.0f) {
    npc->mood = NPC_MOOD_SCARED;
  } else if (npc->behavior_flags & NPC_FLAG_IN_LOVE) {
    npc->mood = NPC_MOOD_HAPPY;
  } else {
    npc->mood = NPC_MOOD_NEUTRAL;
  }

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

  // Check for special abilities
  npc_check_special_abilities(system, entity, npc, npc_transform, delta_time);

  // Update schedule and tasks (Milestone 3)
  npc_schedule_update(system, entity, npc, delta_time);

  // Update combat behavior (Milestone 4)
  npc_combat_behavior_update(system, entity, npc, delta_time);

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
  if (npc->path_length > 0 && npc->current_path_index < npc->path_length) {
    Vec3 target_pos = npc->path[npc->current_path_index];
    Vec3 direction =
        vec3_normalize(vec3_sub(target_pos, npc_transform->position));

    RigidBodyComponent *rb_comp = (RigidBodyComponent *)ecs_get_component(
        (World *)system->ecs, (Entity){entity, 0}, RIGIDBODY_COMPONENT_ID);
    if (rb_comp && rb_comp->body) {
      rigid_body_set_velocity(rb_comp->body,
                              vec3_mul(direction, stats.move_speed));
    }

    if (vec3_length(vec3_sub(npc_transform->position, target_pos)) < 0.5f) {
      npc->current_path_index++;
    }
  }
}

// Passive animal behavior (cows, pigs, chickens)
static void npc_update_passive(NPCSystem *system, EntityID entity_id,
                               NPCComponent *npc,
                               TransformComponent *npc_transform,
                               f32 delta_time) {
  NPCStats stats = npc_get_stats(npc->type);

  // Find nearest player or threat
  QueryDesc desc = {0};
  ComponentTypeID components[] = {PLAYER_COMPONENT_ID, TRANSFORM_COMPONENT_ID};
  desc.all_components = components;
  desc.all_count = 2;
  Query *query = ecs_query_create((World *)system->ecs, &desc);

  EntityID nearest_player = 0;
  f32 nearest_distance = 999999.0f;

  Entity player_ent;
  void *player_comps[2];
  while (ecs_query_next(query, &player_ent, player_comps)) {
    TransformComponent *player_transform =
        (TransformComponent *)player_comps[1];
    f32 distance = vec3_length(
        vec3_sub(player_transform->position, npc_transform->position));
    if (distance < nearest_distance) {
      nearest_distance = distance;
      nearest_player = player_ent.id;
    }
  }
  ecs_query_destroy((World *)system->ecs, query);

  // Check health to see if damaged (triggers flee)
  HealthComponent *health = (HealthComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){entity_id, 0}, HEALTH_COMPONENT_ID);
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
      npc->behavior_timer =
          (rand() % 100) / 10.0f + 3.0f; // Wander for 3-13 seconds
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
        Vec3 random_target =
            vec3_add(npc_transform->position,
                     vec3((rand() % 20) - 10, 0, (rand() % 20) - 10));
        // Note: passing globals for now, might need better access
        extern ChunkManager *g_chunk_manager;
        extern BlockRegistry *g_block_registry;
        npc_find_path((struct ECSWorld *)system->ecs, entity_id,
                      g_chunk_manager, g_block_registry,
                      npc_transform->position, random_target, npc->path,
                      &npc->path_length, 32);
        npc->current_path_index = 0;
      }
    }
    break;

  case NPC_STATE_FLEEING:
    if (npc->panic_timer <= 0.0f ||
        nearest_distance > stats.flee_range * 2.0f) {
      npc->state = NPC_STATE_IDLE;
      npc->flee_target = 0;
    } else if (nearest_player) {
      // Run away from threat
      TransformComponent *threat_transform =
          (TransformComponent *)ecs_get_component((World *)system->ecs,
                                                  (Entity){nearest_player, 0},
                                                  TRANSFORM_COMPONENT_ID);
      if (threat_transform) {
        Vec3 flee_direction = vec3_normalize(
            vec3_sub(npc_transform->position, threat_transform->position));
        Vec3 flee_target =
            vec3_add(npc_transform->position, vec3_mul(flee_direction, 10.0f));
        extern ChunkManager *g_chunk_manager;
        extern BlockRegistry *g_block_registry;
        npc_find_path((struct ECSWorld *)system->ecs, entity_id,
                      g_chunk_manager, g_block_registry,
                      npc_transform->position, flee_target, npc->path,
                      &npc->path_length, 32);
        npc->current_path_index = 0;
      }
    }
    break;

  default:
    npc->state = NPC_STATE_IDLE;
    break;
  }
}

// Hostile mob behavior (zombies, skeletons, creepers)
static void npc_update_hostile(NPCSystem *system, EntityID entity_id,
                               NPCComponent *npc,
                               TransformComponent *npc_transform,
                               f32 delta_time) {
  NPCStats stats = npc_get_stats(npc->type);

  // Find nearest player
  QueryDesc desc = {0};
  ComponentTypeID components[] = {PLAYER_COMPONENT_ID, TRANSFORM_COMPONENT_ID};
  desc.all_components = components;
  desc.all_count = 2;
  Query *query = ecs_query_create((World *)system->ecs, &desc);

  Entity player_ent;
  void *player_comps[2];
  if (ecs_query_next(query, &player_ent, player_comps)) {
    TransformComponent *player_transform =
        (TransformComponent *)player_comps[1];
    f32 distance_to_player = vec3_length(
        vec3_sub(player_transform->position, npc_transform->position));

    switch (npc->state) {
    case NPC_STATE_IDLE:
    case NPC_STATE_WANDERING:
      if (distance_to_player < stats.detection_range) {
        npc->state = NPC_STATE_CHASING;
        npc->target = player_ent.id;
      } else if (npc->state == NPC_STATE_IDLE && rand() % 200 < 1) {
        npc->state = NPC_STATE_WANDERING;
      }

      if (npc->state == NPC_STATE_WANDERING && npc->path_length == 0) {
        if (rand() % 100 < 2) {
          Vec3 random_target =
              vec3_add(npc_transform->position,
                       vec3((rand() % 20) - 10, 0, (rand() % 20) - 10));
          extern ChunkManager *g_chunk_manager;
          extern BlockRegistry *g_block_registry;
          npc_find_path((struct ECSWorld *)system->ecs, entity_id,
                        g_chunk_manager, g_block_registry,
                        npc_transform->position, random_target, npc->path,
                        &npc->path_length, 32);
          npc->current_path_index = 0;
        }
      }
      break;

    case NPC_STATE_CHASING:
      if (distance_to_player > stats.detection_range * 1.5f) {
        npc->state = NPC_STATE_IDLE;
        npc->target = 0;
      } else if (distance_to_player <= stats.attack_range) {
        npc->state = NPC_STATE_ATTACKING;
        npc->behavior_timer = 1.0f; // Attack cooldown
      } else {
        // Update path to player every 0.5 seconds
        if (npc->behavior_timer <= 0.0f) {
          extern ChunkManager *g_chunk_manager;
          extern BlockRegistry *g_block_registry;
          npc_find_path((struct ECSWorld *)system->ecs, entity_id,
                        g_chunk_manager, g_block_registry,
                        npc_transform->position, player_transform->position,
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
        npc_melee_attack(system, entity_id, npc->target);
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
      npc->target = 0;
    } else if (npc->state == NPC_STATE_IDLE && rand() % 200 < 1) {
      npc->state = NPC_STATE_WANDERING;
    }
  }

  ecs_query_destroy((World *)system->ecs, query);
}

// Neutral NPC behavior (villagers)
static void npc_update_neutral(NPCSystem *system, EntityID entity_id,
                               NPCComponent *npc,
                               TransformComponent *npc_transform,
                               f32 delta_time) {
  NPCStats stats = npc_get_stats(npc->type);

  // Check if damaged (becomes hostile)
  HealthComponent *health = (HealthComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){entity_id, 0}, HEALTH_COMPONENT_ID);
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
        Vec3 random_target =
            vec3_add(npc_transform->position,
                     vec3((rand() % 15) - 7, 0, (rand() % 15) - 7));
        extern ChunkManager *g_chunk_manager;
        extern BlockRegistry *g_block_registry;
        npc_find_path((struct ECSWorld *)system->ecs, entity_id,
                      g_chunk_manager, g_block_registry,
                      npc_transform->position, random_target, npc->path,
                      &npc->path_length, 32);
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
      QueryDesc desc = {0};
      ComponentTypeID components[] = {PLAYER_COMPONENT_ID,
                                    TRANSFORM_COMPONENT_ID};
      desc.all_components = components;
      desc.all_count = 2;
      Query *query = ecs_query_create((World *)system->ecs, &desc);

      Entity player_ent;
      void *player_comps[2];
      if (ecs_query_next(query, &player_ent, player_comps)) {
        TransformComponent *player_transform =
            (TransformComponent *)player_comps[1];
        Vec3 flee_direction = vec3_normalize(
            vec3_sub(npc_transform->position, player_transform->position));
        Vec3 flee_target =
            vec3_add(npc_transform->position, vec3_mul(flee_direction, 10.0f));
        extern ChunkManager *g_chunk_manager;
        extern BlockRegistry *g_block_registry;
        npc_find_path((struct ECSWorld *)system->ecs, entity_id,
                      g_chunk_manager, g_block_registry,
                      npc_transform->position, flee_target, npc->path,
                      &npc->path_length, 32);
        npc->current_path_index = 0;
      }

      ecs_query_destroy((World *)system->ecs, query);
    }
    break;

  default:
    npc->state = NPC_STATE_IDLE;
    break;
  }
}

void npc_begin_trading(NPCSystem *system, EntityID npc_entity,
                       EntityID player) {
  if (!system)
    return;
  NPCComponent *npc = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){npc_entity, 0}, NPC_COMPONENT_ID);
  if (!npc)
    return;
  npc->behavior_flags |= NPC_FLAG_TRADING;
  npc->state = NPC_STATE_IDLE;
  npc->path_length = 0;
  npc->target = player;
}

void npc_end_trading(NPCSystem *system, EntityID npc_entity) {
  if (!system)
    return;
  NPCComponent *npc = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){npc_entity, 0}, NPC_COMPONENT_ID);
  if (!npc)
    return;
  npc->behavior_flags &= ~NPC_FLAG_TRADING;
  npc->target = 0;
}

void npc_begin_dialogue(NPCSystem *system, EntityID npc_entity,
                        EntityID player) {
  if (!system)
    return;
  NPCComponent *npc = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){npc_entity, 0}, NPC_COMPONENT_ID);
  if (!npc)
    return;
  npc->behavior_flags |= NPC_FLAG_DIALOGUE;
  npc->state = NPC_STATE_IDLE;
  npc->path_length = 0;
  npc->target = player;
}

void npc_end_dialogue(NPCSystem *system, EntityID npc_entity) {
  if (!system)
    return;
  NPCComponent *npc = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){npc_entity, 0}, NPC_COMPONENT_ID);
  if (!npc)
    return;
  npc->behavior_flags &= ~NPC_FLAG_DIALOGUE;
  npc->target = 0;
}

i16 npc_get_reputation(NPCSystem *system, EntityID npc_entity) {
  if (!system)
    return 0;
  NPCComponent *npc = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){npc_entity, 0}, NPC_COMPONENT_ID);
  if (!npc)
    return 0;
  return npc->reputation;
}

void npc_set_reputation(NPCSystem *system, EntityID npc_entity, i16 value) {
  if (!system)
    return;
  NPCComponent *npc = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){npc_entity, 0}, NPC_COMPONENT_ID);
  if (!npc)
    return;
  npc->reputation = value;
}

void npc_adjust_reputation(NPCSystem *system, EntityID npc_entity, i16 delta) {
  if (!system)
    return;
  NPCComponent *npc = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){npc_entity, 0}, NPC_COMPONENT_ID);
  if (!npc)
    return;
  npc->reputation += delta;
}

i16 npc_get_relation(NPCSystem *system, EntityID npc_entity, EntityID other) {
  if (!system)
    return 0;
  NPCComponent *npc = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){npc_entity, 0}, NPC_COMPONENT_ID);
  if (!npc)
    return 0;
  for (u8 i = 0; i < npc->relations_count; i++) {
    if (npc->relations_entities[i] == other)
      return npc->relations_values[i];
  }
  return 0;
}

void npc_set_relation(NPCSystem *system, EntityID npc_entity, EntityID other,
                      i16 value) {
  if (!system)
    return;
  NPCComponent *npc = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){npc_entity, 0}, NPC_COMPONENT_ID);
  if (!npc)
    return;
  for (u8 i = 0; i < npc->relations_count; i++) {
    if (npc->relations_entities[i] == other) {
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

void npc_adjust_relation(NPCSystem *system, EntityID npc_entity, EntityID other,
                         i16 delta) {
  if (!system)
    return;
  NPCComponent *npc = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){npc_entity, 0}, NPC_COMPONENT_ID);
  if (!npc)
    return;
  for (u8 i = 0; i < npc->relations_count; i++) {
    if (npc->relations_entities[i] == other) {
      npc->relations_values[i] =
          CLAMP(npc->relations_values[i] + delta, -100, 100);
      return;
    }
  }
  if (npc->relations_count < 8) {
    u8 idx = npc->relations_count++;
    npc->relations_entities[idx] = other;
    npc->relations_values[idx] = CLAMP(delta, -100, 100);
  }
}
