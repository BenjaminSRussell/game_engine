// Block lighting system for skylight and blocklight propagation.
// TODO: Implement optimized light propagation algorithm with flood fill
// optimization.
// TODO: Add light level caching to reduce redundant calculations.
// TODO: Implement light level interpolation for smooth transitions.
// TODO: Add light level compression for memory efficiency.
// TODO: Implement light level validation to detect corruption.
// TODO: Add light level serialization for save/load.
// TODO: Implement light level batching for performance.
// TODO: Add light level preview system for creative mode.
// TODO: Implement light level debugging visualization.
// TODO: Add light level statistics and profiling.
#include <block/interaction.h>
#include <block/lighting.h>
#include <chunk/chunk.h>
#include <core/logger.h>
#include <stdlib.h>

// Simple queue for deferred lighting updates
#define MAX_LIGHT_QUEUE 4096

typedef struct {
  i32 x, y, z;
  u8 level;
  ChunkLightType type;
} LightUpdate;

static LightUpdate g_light_queue[MAX_LIGHT_QUEUE];
static u32 g_queue_head = 0;
static u32 g_queue_tail = 0;

u8 block_calculate_light_level(ChunkManager *chunks, BlockRegistry *registry,
                               i32 x, i32 y, i32 z) {
  if (!chunks || !registry)
    return 0;

  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(chunks, cp);
  if (!chunk)
    return 0;

  i32 lx = x - cp.x * CHUNK_SIZE;
  i32 ly = y - cp.y * CHUNK_SIZE;
  i32 lz = z - cp.z * CHUNK_SIZE;

  BlockID block = chunk_get_block(chunk, lx, ly, lz);
  const BlockType *bt = block_registry_get(registry, block);
  if (bt && block_emits_light(bt)) {
    return bt->light_level;
  }

  u8 max_light = 0;
  const i32 neighbors[6][3] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                               {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
  for (u32 i = 0; i < 6; i++) {
    i32 nx = x + neighbors[i][0];
    i32 ny = y + neighbors[i][1];
    i32 nz = z + neighbors[i][2];

    ChunkPos ncp = world_to_chunk_pos(nx, ny, nz);
    Chunk *nchunk = chunk_manager_get(chunks, ncp);
    if (!nchunk)
      continue;

    i32 nlx = nx - ncp.x * CHUNK_SIZE;
    i32 nly = ny - ncp.y * CHUNK_SIZE;
    i32 nlz = nz - ncp.z * CHUNK_SIZE;
    u8 neighbor_light = chunk_get_light(nchunk, nlx, nly, nlz, LIGHT_TYPE_BLOCK);
    if (neighbor_light > 0)
      neighbor_light--;
    if (neighbor_light > max_light)
      max_light = neighbor_light;
  }

  return max_light;
}

void lighting_process_queue(ChunkManager *manager, BlockRegistry *registry) {
  if (!manager || !registry)
    return;

  u32 processed = 0;
  const u32 max_per_frame = 100; // Limit updates per frame

  while (g_queue_head != g_queue_tail && processed < max_per_frame) {
    LightUpdate update = g_light_queue[g_queue_head];
    g_queue_head = (g_queue_head + 1) % MAX_LIGHT_QUEUE;

    // Actually propagate the light using BFS/flood-fill algorithm
    // Get the chunk containing the update position
    ChunkPos cp = world_to_chunk_pos(update.x, update.y, update.z);
    Chunk *chunk = chunk_manager_get(manager, cp);
    if (!chunk) {
      processed++;
      continue;
    }

    // Calculate local coordinates within chunk
    i32 local_x = update.x - cp.x * CHUNK_SIZE;
    i32 local_y = update.y - cp.y * CHUNK_SIZE;
    i32 local_z = update.z - cp.z * CHUNK_SIZE;

    // Set initial light level
    if (update.type == LIGHT_TYPE_SKY) {
      if (!chunk->skylight) {
        chunk->skylight =
            calloc(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE, sizeof(u8));
      }
      i32 index =
          local_x + local_z * CHUNK_SIZE + local_y * CHUNK_SIZE * CHUNK_SIZE;
      chunk->skylight[index] = update.level;
    } else {
      if (!chunk->blocklight) {
        chunk->blocklight =
            calloc(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE, sizeof(u8));
      }
      i32 index =
          local_x + local_z * CHUNK_SIZE + local_y * CHUNK_SIZE * CHUNK_SIZE;
      chunk->blocklight[index] = update.level;
    }

    // Propagate light to neighboring blocks (simplified BFS)
    // For now, just propagate to immediate neighbors with reduced intensity
    const i32 neighbors[6][3] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                                 {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};

    for (i32 i = 0; i < 6; i++) {
      i32 nx = update.x + neighbors[i][0];
      i32 ny = update.y + neighbors[i][1];
      i32 nz = update.z + neighbors[i][2];

      // Get block at neighbor position
      BlockID block_id = block_get(manager, nx, ny, nz);
      if (block_id != BLOCK_AIR) {
        const BlockType *bt = block_registry_get(registry, block_id);
        if (bt && bt->opacity > 0)
          continue;
      }

      // Calculate light level for neighbor (reduce by 1 for distance)
      u8 neighbor_level = update.level > 1 ? update.level - 1 : 0;
      if (neighbor_level == 0)
        continue;

      // Queue neighbor for propagation if it has higher light level
      ChunkPos ncp = world_to_chunk_pos(nx, ny, nz);
      Chunk *neighbor_chunk = chunk_manager_get(manager, ncp);
      if (!neighbor_chunk)
        continue;

      i32 neighbor_local_x = nx - ncp.x * CHUNK_SIZE;
      i32 neighbor_local_y = ny - ncp.y * CHUNK_SIZE;
      i32 neighbor_local_z = nz - ncp.z * CHUNK_SIZE;

      u8 current_level =
          chunk_get_light(neighbor_chunk, neighbor_local_x, neighbor_local_y,
                          neighbor_local_z, update.type);
      if (neighbor_level > current_level) {
        block_propagate_light(manager, registry, nx, ny, nz, neighbor_level,
                              update.type);
      }
    }

    processed++;
  }

  if (processed > 0) {
    LOG_TRACE("Processed %u light updates", processed);
  }
}

void block_propagate_light(ChunkManager *manager, BlockRegistry *registry,
                           i32 x, i32 y, i32 z, u8 light_level,
                           u32 light_type) {
  if (!manager || !registry)
    return;

  ChunkLightType type = (light_type == 0) ? LIGHT_TYPE_SKY : LIGHT_TYPE_BLOCK;

  // Queue the light update for deferred processing
  u32 next_tail = (g_queue_tail + 1) % MAX_LIGHT_QUEUE;
  if (next_tail == g_queue_head) {
    LOG_WARN("Light queue full, dropping update");
    return;
  }

  g_light_queue[g_queue_tail] = (LightUpdate){x, y, z, light_level, type};
  g_queue_tail = next_tail;
}
