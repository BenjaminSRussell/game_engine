// Enhanced NPC spawn logic with caps, time/light-based rules, and despawning.
#include <block/block.h>
#include <chunk/chunk.h>
#include <core/logger.h>
#include <ecs/component_ids.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include <math.h>
#include <math/vec3.h>
#include <npc/npc.h>
#include <player/player.h>
#include <stdlib.h>
#include <world/generator.h>

// Spawn configuration
#define MAX_NPCS_PER_CHUNK 8
#define MAX_HOSTILE_PER_CHUNK 4
#define MAX_PASSIVE_PER_CHUNK 6
#define SPAWN_CHECK_RADIUS 4
#define DESPAWN_DISTANCE 96.0f

// Count NPCs in a chunk
static u32 count_npcs_in_chunk(NPCSystem *system, ChunkPos chunk_pos) {
  ComponentTypeID components[] = {NPC_COMPONENT_ID, TRANSFORM_COMPONENT_ID};
  EntityQuery query;
  ecs_query_init(&query, 1024);
  ecs_query_entities((World *)system->ecs, &query, components, 2);

  u32 count = 0;
  for (u32 i = 0; i < query.count; i++) {
    EntityID entity = query.entities[i];
    TransformComponent *transform = (TransformComponent *)ecs_get_component(
        (World *)system->ecs, (Entity){entity, 0}, TRANSFORM_COMPONENT_ID);
    if (transform) {
      ChunkPos entity_chunk = world_to_chunk_pos((i32)transform->position.x,
                                                 (i32)transform->position.y,
                                                 (i32)transform->position.z);
      if (entity_chunk.x == chunk_pos.x && entity_chunk.z == chunk_pos.z) {
        count++;
      }
    }
  }

  ecs_query_free(&query);
  return count;
}

// Count NPCs by behavior type
static u32 count_npcs_by_behavior(NPCSystem *system, ChunkPos chunk_pos,
                                  NPCBehavior behavior) {
  ComponentTypeID components[] = {NPC_COMPONENT_ID, TRANSFORM_COMPONENT_ID};
  EntityQuery query;
  ecs_query_init(&query, 1024);
  ecs_query_entities((World *)system->ecs, &query, components, 2);

  u32 count = 0;
  for (u32 i = 0; i < query.count; i++) {
    EntityID entity = query.entities[i];
    NPCComponent *npc = (NPCComponent *)ecs_get_component(
        (World *)system->ecs, (Entity){entity, 0}, NPC_COMPONENT_ID);
    TransformComponent *transform = (TransformComponent *)ecs_get_component(
        (World *)system->ecs, (Entity){entity, 0}, TRANSFORM_COMPONENT_ID);
    if (npc && transform) {
      NPCStats stats = npc_get_stats(npc->type);
      if (stats.behavior == behavior) {
        ChunkPos entity_chunk = world_to_chunk_pos((i32)transform->position.x,
                                                   (i32)transform->position.y,
                                                   (i32)transform->position.z);
        if (entity_chunk.x == chunk_pos.x && entity_chunk.z == chunk_pos.z) {
          count++;
        }
      }
    }
  }

  ecs_query_free(&query);
  return count;
}

// Check if position has sufficient light for passive spawning
static bool has_sufficient_light(Chunk *chunk, i32 local_x, i32 local_y,
                                 i32 local_z) {
  // Passive mobs need light level >= 9
  // For now, just check if it's not in a cave (simple heuristic)
  if (local_y < CHUNK_SIZE / 2) {
    // Underground - check if there's sky access
    for (i32 y = local_y + 1; y < CHUNK_SIZE; y++) {
      BlockID block = chunk_get_block(chunk, local_x, y, local_z);
      if (block != BLOCK_AIR) {
        return false; // Blocked from sky
      }
    }
  }
  return true;
}

// Check if position is dark enough for hostile spawning
static bool is_dark_enough(Chunk *chunk, i32 local_x, i32 local_y,
                           i32 local_z) {
  // Hostile mobs need light level <= 7
  // For now, just check if underground or enclosed
  if (local_y < CHUNK_SIZE / 2) {
    return true; // Underground
  }

  // Check if enclosed by blocks
  i32 enclosed_count = 0;
  for (i32 dy = -1; dy <= 1; dy++) {
    for (i32 dx = -1; dx <= 1; dx++) {
      for (i32 dz = -1; dz <= 1; dz++) {
        i32 check_x = local_x + dx;
        i32 check_y = local_y + dy;
        i32 check_z = local_z + dz;
        if (check_x >= 0 && check_x < CHUNK_SIZE && check_y >= 0 &&
            check_y < CHUNK_SIZE && check_z >= 0 && check_z < CHUNK_SIZE) {
          BlockID block = chunk_get_block(chunk, check_x, check_y, check_z);
          if (block != BLOCK_AIR && block != BLOCK_WATER) {
            enclosed_count++;
          }
        }
      }
    }
  }

  return enclosed_count > 15; // Fairly enclosed
}

// Spawn NPCs in chunk with enhanced rules
void npc_spawn_in_chunk(NPCSystem *system, Chunk *chunk,
                        struct WorldGenerator *generator) {
  if (!system || !chunk || !generator)
    return;

  ChunkPos chunk_pos = chunk->pos;

  // Check spawn caps
  u32 total_npcs = count_npcs_in_chunk(system, chunk_pos);
  if (total_npcs >= MAX_NPCS_PER_CHUNK)
    return;

  u32 hostile_count =
      count_npcs_by_behavior(system, chunk_pos, NPC_BEHAVIOR_HOSTILE);
  u32 passive_count =
      count_npcs_by_behavior(system, chunk_pos, NPC_BEHAVIOR_PASSIVE);

  // Try spawning at random positions
  for (i32 attempt = 0; attempt < 4; attempt++) {
    i32 x = SPAWN_CHECK_RADIUS + rand() % (CHUNK_SIZE - SPAWN_CHECK_RADIUS * 2);
    i32 z = SPAWN_CHECK_RADIUS + rand() % (CHUNK_SIZE - SPAWN_CHECK_RADIUS * 2);

    i32 world_x = chunk->pos.x * CHUNK_SIZE + x;
    i32 world_z = chunk->pos.z * CHUNK_SIZE + z;

    // Find surface
    i32 surface_y = -1;
    for (i32 y = CHUNK_SIZE - 1; y >= 0; y--) {
      BlockID block = chunk_get_block(chunk, x, y, z);
      if (block != BLOCK_AIR && block != BLOCK_WATER) {
        surface_y = y;
        break;
      }
    }

    if (surface_y < 0 || surface_y >= CHUNK_SIZE - 2)
      continue;

    i32 world_y = chunk->pos.y * CHUNK_SIZE + surface_y;

    // Get biome
    WorldGenerator *gen = (WorldGenerator *)generator;
    BiomeType biome = world_generator_get_biome(gen, world_x, world_y, world_z);

    // Generate spawn chance
    u32 hash =
        ((u32)world_x * 73856093u) ^ ((u32)world_z * 19349663u) ^ gen->seed;
    f32 chance = (hash % 1000) / 1000.0f;

    // Determine NPC type based on biome, light, and time
    NPCType npc_type = NPC_TYPE_VILLAGER;
    bool spawn = false;

    // Check lighting conditions
    bool sufficient_light = has_sufficient_light(chunk, x, surface_y + 1, z);
    bool dark_enough = is_dark_enough(chunk, x, surface_y + 1, z);

    switch (biome) {
    case BIOME_PLAINS:
    case BIOME_SAVANNA:
      // Passive animals in daylight
      if (sufficient_light && passive_count < MAX_PASSIVE_PER_CHUNK) {
        if (chance < 0.03f) {
          npc_type = NPC_TYPE_COW;
          spawn = true;
        } else if (chance < 0.06f) {
          npc_type = NPC_TYPE_PIG;
          spawn = true;
        } else if (chance < 0.08f) {
          npc_type = NPC_TYPE_CHICKEN;
          spawn = true;
        }
      }
      // Hostile mobs in dark areas
      else if (dark_enough && hostile_count < MAX_HOSTILE_PER_CHUNK) {
        if (chance < 0.02f) {
          npc_type = NPC_TYPE_ZOMBIE;
          spawn = true;
        } else if (chance < 0.04f) {
          npc_type = NPC_TYPE_SKELETON;
          spawn = true;
        }
      }
      // Villagers rarely
      else if (sufficient_light && chance < 0.01f) {
        npc_type = NPC_TYPE_VILLAGER;
        spawn = true;
      }
      break;

    case BIOME_FOREST:
      // Forest animals
      if (sufficient_light && passive_count < MAX_PASSIVE_PER_CHUNK) {
        if (chance < 0.04f) {
          npc_type = NPC_TYPE_PIG;
          spawn = true;
        } else if (chance < 0.07f) {
          npc_type = NPC_TYPE_CHICKEN;
          spawn = true;
        }
      }
      // Hostiles in dark forest
      else if (dark_enough && hostile_count < MAX_HOSTILE_PER_CHUNK) {
        if (chance < 0.03f) {
          npc_type = NPC_TYPE_ZOMBIE;
          spawn = true;
        }
      }
      break;

    case BIOME_DESERT:
      // Mostly hostile in desert
      if (dark_enough && hostile_count < MAX_HOSTILE_PER_CHUNK &&
          chance < 0.02f) {
        npc_type = NPC_TYPE_ZOMBIE;
        spawn = true;
      }
      break;

    default:
      // No spawning in other biomes for now
      continue;
    }

    if (spawn) {
      Vec3 spawn_pos =
          vec3((f32)world_x + 0.5f, (f32)world_y + 1.0f, (f32)world_z + 0.5f);
      npc_create(system, spawn_pos, npc_type);

      // Update counts
      NPCStats stats = npc_get_stats(npc_type);
      if (stats.behavior == NPC_BEHAVIOR_HOSTILE) {
        hostile_count++;
      } else if (stats.behavior == NPC_BEHAVIOR_PASSIVE) {
        passive_count++;
      }
      total_npcs++;

      if (total_npcs >= MAX_NPCS_PER_CHUNK)
        break;
    }
  }
}

// Despawn NPCs that are too far from players
void npc_despawn_distant(NPCSystem *system) {
  if (!system)
    return;

  // Find all players
  EntityQuery player_query;
  ecs_query_init(&player_query, 16);
  ComponentTypeID player_components[] = {PLAYER_COMPONENT_ID,
                                         TRANSFORM_COMPONENT_ID};
  ecs_query_entities(system->ecs, &player_query, player_components, 2);

  if (player_query.count == 0) {
    ecs_query_free(&player_query);
    return;
  }

  // Find all NPCs
  EntityQuery npc_query;
  ecs_query_init(&npc_query, 1024);
  ComponentTypeID npc_components[] = {NPC_COMPONENT_ID, TRANSFORM_COMPONENT_ID};
  ecs_query_entities(system->ecs, &npc_query, npc_components, 2);

  for (u32 i = 0; i < npc_query.count; i++) {
    EntityID entity = npc_query.entities[i];
    TransformComponent *npc_transform =
        ecs_get_component(system->ecs, entity, TRANSFORM_COMPONENT_ID);
    if (!npc_transform)
      continue;

    // Check distance to nearest player
    f32 nearest_distance = 999999.0f;
    for (u32 j = 0; j < player_query.count; j++) {
      TransformComponent *player_transform = ecs_get_component(
          system->ecs, player_query.entities[j], TRANSFORM_COMPONENT_ID);
      if (player_transform) {
        f32 distance = vec3_length(
            vec3_sub(player_transform->position, npc_transform->position));
        if (distance < nearest_distance) {
          nearest_distance = distance;
        }
      }
    }

    // Despawn if too far
    if (nearest_distance > DESPAWN_DISTANCE) {
      ecs_destroy_entity(system->ecs, entity);
    }
  }

  ecs_query_free(&npc_query);
  ecs_query_free(&player_query);
}
