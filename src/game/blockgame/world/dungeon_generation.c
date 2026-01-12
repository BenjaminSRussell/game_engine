// Dungeon generation logic.
// Roadmap: docs/WORLD_ROADMAP.md.
#include <block/block.h>
#include <chunk/chunk.h>
#include <core/logger.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <world/dungeon_generation.h>
#include <world/structures.h>

// Dungeon room types
typedef enum {
  DUNGEON_ROOM_ENTRANCE,
  DUNGEON_ROOM_CORRIDOR,
  DUNGEON_ROOM_CHAMBER,
  DUNGEON_ROOM_TREASURE,
  DUNGEON_ROOM_BOSS,
  DUNGEON_ROOM_COUNT
} DungeonRoomType;

// Dungeon room
typedef struct {
  DungeonRoomType type;
  i32 x, y, z;
  i32 width, height, depth;
  bool connected[4]; // North, East, South, West
} DungeonRoom;

// Dungeon layout
typedef struct {
  DungeonRoom *rooms;
  u32 room_count;
  u32 room_capacity;
  i32 start_x, start_y, start_z;
} DungeonLayout;

// Generate dungeon layout
static DungeonLayout *dungeon_generate_layout(i32 x, i32 y, i32 z, u32 seed,
                                              u32 room_count) {
  DungeonLayout *layout = (DungeonLayout *)calloc(1, sizeof(DungeonLayout));
  if (!layout)
    return NULL;

  layout->room_capacity = room_count * 2;
  layout->rooms =
      (DungeonRoom *)calloc(layout->room_capacity, sizeof(DungeonRoom));
  layout->start_x = x;
  layout->start_y = y;
  layout->start_z = z;

// VFX: Dungeon generation particles and ambient sounds
// Note: Actual particle/audio systems would be called here when fully
// integrated
#if 0 // Enable when particle/audio systems are ready
    {
        // Spawn eerie ambient sound at dungeon entrance
        // audio_play_3d_sound("dungeon/ambient_wind", vec3(x, y, z), 0.5f, 0.8f);
        // audio_play_3d_sound("dungeon/distant_echo", vec3(x, y, z), 0.3f, 0.6f);
        
        // Spawn dark particle effects (dust motes, shadows)
        // particle_spawn("dungeon_dust", vec3(x, y, z), 20);
        
        // Subtle earth tremor for large dungeons near player
        if (room_count > 7) {  // Large dungeon
            // Vec3 player_pos = player_get_position();
            // f32 distance = vec3_distance(player_pos, vec3(x, y, z));
            // if (distance < 30.0f) {
            //     // Slight rumble effect
            //     camera_shake(0.05f, 0.5f);  // subtle, longer duration
            //     audio_play_3d_sound("dungeon/rumble", vec3(x, y, z), 0.4f, 0.7f);
            // }
        }
    }
#endif

  // Generate entrance room
  DungeonRoom *entrance = &layout->rooms[layout->room_count++];
  entrance->type = DUNGEON_ROOM_ENTRANCE;
  entrance->x = x;
  entrance->y = y;
  entrance->z = z;
  entrance->width = 7;
  entrance->height = 5;
  entrance->depth = 7;
  memset(entrance->connected, 0, sizeof(entrance->connected));

  // Generate additional rooms
  i32 current_x = x;
  i32 current_z = z;

  for (u32 i = 1; i < room_count && layout->room_count < layout->room_capacity;
       i++) {
    // Random direction
    u32 dir = (seed + i * 73856093) % 4;
    i32 offset_x = 0, offset_z = 0;

    switch (dir) {
    case 0:
      offset_z = -10;
      break; // North
    case 1:
      offset_x = 10;
      break; // East
    case 2:
      offset_z = 10;
      break; // South
    case 3:
      offset_x = -10;
      break; // West
    }

    current_x += offset_x;
    current_z += offset_z;

    DungeonRoom *room = &layout->rooms[layout->room_count++];
    room->type =
        (i == room_count - 1) ? DUNGEON_ROOM_TREASURE : DUNGEON_ROOM_CHAMBER;
    room->x = current_x;
    room->y = y;
    room->z = current_z;
    room->width = 5 + ((seed + i) % 5);      // 5-9 blocks
    room->height = 4 + ((seed + i * 2) % 3); // 4-6 blocks
    room->depth = 5 + ((seed + i * 3) % 5);  // 5-9 blocks
    memset(room->connected, 0, sizeof(room->connected));
  }

  return layout;
}

// Place dungeon room in world
static void dungeon_place_room(ChunkManager *chunks, DungeonRoom *room) {
  if (!chunks || !room)
    return;

  // Place walls and floor
  for (i32 dx = 0; dx < room->width; dx++) {
    for (i32 dz = 0; dz < room->depth; dz++) {
      for (i32 dy = 0; dy < room->height; dy++) {
        i32 wx = room->x + dx;
        i32 wy = room->y + dy;
        i32 wz = room->z + dz;

        BlockID block = BLOCK_AIR;

        // Floor
        if (dy == 0) {
          block = BLOCK_COBBLESTONE;
        }
        // Ceiling
        else if (dy == room->height - 1) {
          block = BLOCK_COBBLESTONE;
        }
        // Walls
        else if (dx == 0 || dx == room->width - 1 || dz == 0 ||
                 dz == room->depth - 1) {
          block = BLOCK_COBBLESTONE;
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

  // Place chest in treasure room
  if (room->type == DUNGEON_ROOM_TREASURE) {
    i32 chest_x = room->x + room->width / 2;
    i32 chest_z = room->z + room->depth / 2;
    i32 chest_y = room->y + 1;

    // Place chest block at center    // Set chest block
    // chunk_manager_set_block(chunks, chest_x, chest_y, chest_z, BLOCK_CHEST);

    // Manual set block since manager convenience might be missing
    ChunkPos cpos = world_to_chunk_pos(chest_x, chest_y, chest_z);
    Chunk *chunk = chunk_manager_get(chunks, cpos);
    if (chunk) {
      i32 lx = chest_x - cpos.x * CHUNK_SIZE;
      i32 ly = chest_y - cpos.y * CHUNK_SIZE;
      i32 lz = chest_z - cpos.z * CHUNK_SIZE;
      // Use BLOCK_WOOD if CHEST is missing for now
      chunk_set_block(chunk, lx, ly, lz, BLOCK_WOOD);
    }
    LOG_INFO("Placed treasure chest at dungeon room (%d, %d, %d)", chest_x,
             chest_y, chest_z);
  }
}

// Generate dungeon at position
bool dungeon_generate(ChunkManager *chunks, i32 x, i32 y, i32 z, u32 seed) {
  if (!chunks)
    return false;

  // Generate layout
  DungeonLayout *layout =
      dungeon_generate_layout(x, y, z, seed, 5 + (seed % 5)); // 5-9 rooms
  if (!layout)
    return false;

  // Place all rooms
  for (u32 i = 0; i < layout->room_count; i++) {
    dungeon_place_room(chunks, &layout->rooms[i]);
  }

  // Place corridors between rooms
  for (u32 i = 1; i < layout->room_count; i++) {
    DungeonRoom *prev = &layout->rooms[i - 1];
    DungeonRoom *curr = &layout->rooms[i];

    // Create corridor
    i32 start_x = prev->x + prev->width / 2;
    i32 start_z = prev->z + prev->depth / 2;
    i32 end_x = curr->x + curr->width / 2;
    i32 end_z = curr->z + curr->depth / 2;

    // Place corridor blocks
    i32 steps = abs(end_x - start_x) + abs(end_z - start_z);
    for (i32 step = 0; step < steps; step++) {
      f32 t = (f32)step / (f32)steps;
      i32 cx = (i32)(start_x + (end_x - start_x) * t);
      i32 cz = (i32)(start_z + (end_z - start_z) * t);
      i32 cy = y;

      // 3x3 corridor
      for (i32 dx = -1; dx <= 1; dx++) {
        for (i32 dz = -1; dz <= 1; dz++) {
          for (i32 dy = 0; dy < 3; dy++) {
            i32 wx = cx + dx;
            i32 wy = cy + dy;
            i32 wz = cz + dz;

            BlockID block = BLOCK_AIR;
            if (dy == 0)
              block = BLOCK_COBBLESTONE; // Floor
            else if (dy == 2)
              block = BLOCK_COBBLESTONE; // Ceiling
            else if (abs(dx) == 1 || abs(dz) == 1)
              block = BLOCK_COBBLESTONE; // Walls

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
    }
  }

  // Free layout
  if (layout->rooms)
    free(layout->rooms);
  free(layout);

  return true;
}

// Generate dungeons in chunk
void dungeon_generate_in_chunk(ChunkManager *chunks, Chunk *chunk, u32 seed) {
  if (!chunks || !chunk)
    return;

  // Rare chance to spawn dungeon
  u32 hash =
      ((u32)chunk->pos.x * 73856093) ^ ((u32)chunk->pos.z * 19349663) ^ seed;
  f32 chance = (hash % 10000) / 10000.0f;

  if (chance < 0.01f) { // 1% chance per chunk
    // Find underground position
    i32 world_x = chunk->pos.x * CHUNK_SIZE + CHUNK_SIZE / 2;
    i32 world_z = chunk->pos.z * CHUNK_SIZE + CHUNK_SIZE / 2;
    i32 world_y = 20 + (hash % 20); // Between Y 20-40

    dungeon_generate(chunks, world_x, world_y, world_z, seed);
  }
}
