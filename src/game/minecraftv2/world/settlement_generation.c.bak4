// Settlement layout and building generation.
// Roadmap: docs/WORLD_ROADMAP.md.

#include <block/block.h>
#include <chunk/chunk.h>
#include <math.h>
#include <npc/npc.h>
#include <stdlib.h>
#include <string.h>
#include <world/settlement_generation.h>

// Building types
typedef enum {
  BUILDING_HOUSE_SMALL,
  BUILDING_HOUSE_MEDIUM,
  BUILDING_HOUSE_LARGE,
  BUILDING_SHOP,
  BUILDING_TOWN_HALL,
  BUILDING_BLACKSMITH,
  BUILDING_COUNT
} BuildingType;

// Building definition
typedef struct {
  BuildingType type;
  i32 x, y, z;
  i32 width, height, depth;
  u32 npc_spawn_count;
} Building;

// Settlement layout
typedef struct {
  Building *buildings;
  u32 building_count;
  u32 building_capacity;
  i32 center_x, center_y, center_z;
  u32 radius;
} SettlementLayout;

// Generate house building
static void settlement_generate_house(ChunkManager *chunks, i32 x, i32 y, i32 z,
                                      i32 width, i32 height, i32 depth,
                                      BuildingType type) {
  if (!chunks)
    return;

  // Walls
  for (i32 dx = 0; dx < width; dx++) {
    for (i32 dz = 0; dz < depth; dz++) {
      for (i32 dy = 0; dy < height; dy++) {
        i32 wx = x + dx;
        i32 wy = y + dy;
        i32 wz = z + dz;

        BlockID block = BLOCK_AIR;

        // Floor
        if (dy == 0) {
          block = BLOCK_WOOD;
        }
        // Ceiling
        else if (dy == height - 1) {
          block = BLOCK_WOOD;
        }
        // Walls
        else if (dx == 0 || dx == width - 1 || dz == 0 || dz == depth - 1) {
          block = BLOCK_WOOD;
        }

        if (block != BLOCK_AIR) {
          ChunkPos cp = world_to_chunk_pos(wx, wy, wz);
          Chunk *chunk = chunk_manager_get_or_create(chunks, cp);
          if (chunk) {
            i32 lx = wx - cp.x * CHUNK_SIZE;
            i32 ly = wy - cp.y * CHUNK_SIZE;
            i32 lz = wz - cp.z * CHUNK_SIZE;

            if (lx >= 0 && lx < CHUNK_SIZE && ly >= 0 && ly < CHUNK_SIZE &&
                lz >= 0 && lz < CHUNK_SIZE) {
              chunk_set_block(chunk, lx, ly, lz, block);
            }
          }
        }
      }
    }
  }

  // Door opening
  i32 door_x = x + width / 2;
  i32 door_z = z;
  for (i32 dy = 1; dy < 3; dy++) {
    i32 wy = y + dy;
    ChunkPos cp = world_to_chunk_pos(door_x, wy, door_z);
    Chunk *chunk = chunk_manager_get(chunks, cp);
    if (chunk) {
      i32 lx = door_x - cp.x * CHUNK_SIZE;
      i32 ly = wy - cp.y * CHUNK_SIZE;
      i32 lz = door_z - cp.z * CHUNK_SIZE;

      if (lx >= 0 && lx < CHUNK_SIZE && ly >= 0 && ly < CHUNK_SIZE && lz >= 0 &&
          lz < CHUNK_SIZE) {
        chunk_set_block(chunk, lx, ly, lz, BLOCK_AIR);
      }
    }
  }
}

// Generate settlement layout
static SettlementLayout *
settlement_generate_layout(i32 x, i32 y, i32 z, u32 seed, u32 building_count) {
  SettlementLayout *layout =
      (SettlementLayout *)calloc(1, sizeof(SettlementLayout));
  if (!layout)
    return NULL;

  layout->building_capacity = building_count * 2;
  layout->buildings =
      (Building *)calloc(layout->building_capacity, sizeof(Building));
  layout->center_x = x;
  layout->center_y = y;
  layout->center_z = z;
  layout->radius = 30; // Settlement radius

  // Generate buildings in circular pattern
  for (u32 i = 0;
       i < building_count && layout->building_count < layout->building_capacity;
       i++) {
    f32 angle = (f32)i / (f32)building_count * 2.0f * 3.14159f;
    f32 radius = 10.0f + ((seed + i) % 15); // 10-24 blocks from center

    i32 bx = x + (i32)(cosf(angle) * radius);
    i32 bz = z + (i32)(sinf(angle) * radius);
    i32 by = y;

    Building *building = &layout->buildings[layout->building_count++];
    building->type = (BuildingType)((seed + i) % BUILDING_COUNT);
    building->x = bx;
    building->y = by;
    building->z = bz;

    // Size based on type
    switch (building->type) {
    case BUILDING_HOUSE_SMALL:
      building->width = 5;
      building->height = 4;
      building->depth = 5;
      building->npc_spawn_count = 1;
      break;
    case BUILDING_HOUSE_MEDIUM:
      building->width = 7;
      building->height = 5;
      building->depth = 7;
      building->npc_spawn_count = 2;
      break;
    case BUILDING_HOUSE_LARGE:
      building->width = 9;
      building->height = 6;
      building->depth = 9;
      building->npc_spawn_count = 3;
      break;
    case BUILDING_SHOP:
      building->width = 7;
      building->height = 5;
      building->depth = 7;
      building->npc_spawn_count = 1;
      break;
    case BUILDING_TOWN_HALL:
      building->width = 11;
      building->height = 7;
      building->depth = 11;
      building->npc_spawn_count = 2;
      break;
    case BUILDING_BLACKSMITH:
      building->width = 7;
      building->height = 5;
      building->depth = 7;
      building->npc_spawn_count = 1;
      break;
    default:
      building->width = 5;
      building->height = 4;
      building->depth = 5;
      building->npc_spawn_count = 1;
      break;
    }
  }

  return layout;
}

// Generate settlement at position
bool settlement_generate(ChunkManager *chunks, NPCSystem *npc_system, i32 x,
                         i32 y, i32 z, u32 seed) {
  if (!chunks)
    return false;

  // Generate layout
  u32 building_count = 5 + (seed % 8); // 5-12 buildings
  SettlementLayout *layout =
      settlement_generate_layout(x, y, z, seed, building_count);
  if (!layout)
    return false;

  // Place all buildings
  for (u32 i = 0; i < layout->building_count; i++) {
    Building *building = &layout->buildings[i];
    settlement_generate_house(chunks, building->x, building->y, building->z,
                              building->width, building->height,
                              building->depth, building->type);

    // Spawn NPCs
    if (npc_system) {
      for (u32 j = 0; j < building->npc_spawn_count; j++) {
        i32 npc_x = building->x + building->width / 2;
        i32 npc_z = building->z + building->depth / 2;
        i32 npc_y = building->y + 1;

        Vec3 npc_pos = vec3((f32)npc_x + 0.5f, (f32)npc_y, (f32)npc_z + 0.5f);
        npc_create(npc_system, npc_pos, NPC_TYPE_VILLAGER);
      }
    }
  }

  // Generate roads between buildings
  for (u32 i = 0; i < layout->building_count; i++) {
    Building *building = &layout->buildings[i];

    // Road to center
    i32 start_x = building->x + building->width / 2;
    i32 start_z = building->z + building->depth / 2;
    i32 end_x = layout->center_x;
    i32 end_z = layout->center_z;

    // Place road blocks
    i32 steps = abs(end_x - start_x) + abs(end_z - start_z);
    for (i32 step = 0; step < steps; step++) {
      f32 t = (f32)step / (f32)steps;
      i32 rx = (i32)(start_x + (end_x - start_x) * t);
      i32 rz = (i32)(start_z + (end_z - start_z) * t);
      i32 ry = y;

      // 3x3 road
      for (i32 dx = -1; dx <= 1; dx++) {
        for (i32 dz = -1; dz <= 1; dz++) {
          i32 wx = rx + dx;
          i32 wz = rz + dz;

          ChunkPos cp = world_to_chunk_pos(wx, ry, wz);
          Chunk *chunk = chunk_manager_get_or_create(chunks, cp);
          if (chunk) {
            i32 lx = wx - cp.x * CHUNK_SIZE;
            i32 ly = ry - cp.y * CHUNK_SIZE;
            i32 lz = wz - cp.z * CHUNK_SIZE;

            if (lx >= 0 && lx < CHUNK_SIZE && ly >= 0 && ly < CHUNK_SIZE &&
                lz >= 0 && lz < CHUNK_SIZE) {
              chunk_set_block(chunk, lx, ly, lz, BLOCK_COBBLESTONE);
            }
          }
        }
      }
    }
  }

  // Free layout
  if (layout->buildings)
    free(layout->buildings);
  free(layout);

  return true;
}

// Generate settlements in chunk
void settlement_generate_in_chunk(ChunkManager *chunks, NPCSystem *npc_system,
                                  Chunk *chunk, u32 seed) {
  if (!chunks || !chunk)
    return;

  // Rare chance to spawn settlement
  u32 hash =
      ((u32)chunk->pos.x * 73856093) ^ ((u32)chunk->pos.z * 19349663) ^ seed;
  f32 chance = (hash % 10000) / 10000.0f;

  if (chance < 0.005f) { // 0.5% chance per chunk
    // Find surface position
    i32 surface_y = -1;
    for (i32 y = CHUNK_SIZE - 1; y >= 0; y--) {
      BlockID block = chunk_get_block(chunk, CHUNK_SIZE / 2, y, CHUNK_SIZE / 2);
      if (block != BLOCK_AIR && block != BLOCK_WATER) {
        surface_y = y;
        break;
      }
    }

    if (surface_y >= 0) {
      i32 world_x = chunk->pos.x * CHUNK_SIZE + CHUNK_SIZE / 2;
      i32 world_y = chunk->pos.y * CHUNK_SIZE + surface_y;
      i32 world_z = chunk->pos.z * CHUNK_SIZE + CHUNK_SIZE / 2;

      settlement_generate(chunks, npc_system, world_x, world_y, world_z, seed);
    }
  }
}
