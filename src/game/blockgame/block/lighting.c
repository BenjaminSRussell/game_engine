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
#include "engine/include/core/logger.h"
#include <stdlib.h>

// Light level caching system
#define LIGHT_CACHE_SIZE 4096
#define LIGHT_CACHE_MASK (LIGHT_CACHE_SIZE - 1)

typedef struct {
    i32 x, y, z;
    u8 skylight;
    u8 blocklight;
    u32 frame_counter; // Cache validity timestamp
    bool valid;
} LightCacheEntry;

static LightCacheEntry g_light_cache[LIGHT_CACHE_SIZE];
static u32 g_light_cache_frame = 0;

// Hash function for light cache
static inline u32 light_cache_hash(i32 x, i32 y, i32 z) {
    // Simple hash combining coordinates
    u32 hash = ((u32)x * 73856093) ^ ((u32)y * 19349663) ^ ((u32)z * 83492791);
    return hash & LIGHT_CACHE_MASK;
}

// Get cached light level
static bool light_cache_get(i32 x, i32 y, i32 z, u8 *skylight, u8 *blocklight) {
    u32 index = light_cache_hash(x, y, z);
    LightCacheEntry *entry = &g_light_cache[index];
    
    // Check if cache entry is valid and not stale
    if (entry->valid && 
        entry->x == x && entry->y == y && entry->z == z &&
        (g_light_cache_frame - entry->frame_counter) < 60) { // Valid for 60 frames
        
        if (skylight) *skylight = entry->skylight;
        if (blocklight) *blocklight = entry->blocklight;
        return true;
    }
    
    return false;
}

// Store light level in cache
static void light_cache_set(i32 x, i32 y, i32 z, u8 skylight, u8 blocklight) {
    u32 index = light_cache_hash(x, y, z);
    LightCacheEntry *entry = &g_light_cache[index];
    
    entry->x = x;
    entry->y = y;
    entry->z = z;
    entry->skylight = skylight;
    entry->blocklight = blocklight;
    entry->frame_counter = g_light_cache_frame;
    entry->valid = true;
}

// Invalidate cache entry (when block changes)
static void light_cache_invalidate(i32 x, i32 y, i32 z) {
    u32 index = light_cache_hash(x, y, z);
    g_light_cache[index].valid = false;
}

// Update cache frame counter (call once per frame)
void lighting_cache_update_frame(void) {
    g_light_cache_frame++;
    if (g_light_cache_frame == 0) {
        // Handle frame counter wraparound by clearing cache
        memset(g_light_cache, 0, sizeof(g_light_cache));
    }
}

// Public cache invalidation function (call when blocks change)
void lighting_cache_invalidate(i32 x, i32 y, i32 z) {
    light_cache_invalidate(x, y, z);
    
    // Also invalidate neighboring positions since light can propagate through them
    const i32 neighbors[6][3] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                               {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
    
    for (u32 i = 0; i < 6; i++) {
        i32 nx = x + neighbors[i][0];
        i32 ny = y + neighbors[i][1];
        i32 nz = z + neighbors[i][2];
        light_cache_invalidate(nx, ny, nz);
    }
}

// Optimized flood fill light propagation
#define LIGHT_PROPAGATION_BATCH_SIZE 256
#define MAX_LIGHT_LEVEL 15

typedef struct {
  i32 x, y, z;
  u8 level;
  u8 direction; // Direction from which light came (0-5)
} LightNode;

static LightNode g_flood_fill_queue[MAX_LIGHT_QUEUE];
static u32 g_flood_fill_head = 0;
static u32 g_flood_fill_tail = 0;

// Direction vectors for 6 faces (optimized for flood fill)
static const i32 g_direction_vectors[6][3] = {
  {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, 
  {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
};

// Optimized light propagation using flood fill algorithm
static void lighting_flood_fill_propagate(ChunkManager *manager, BlockRegistry *registry,
                                        i32 start_x, i32 start_y, i32 start_z, 
                                        u8 start_level, ChunkLightType type) {
    if (!manager || !registry || start_level == 0)
        return;
    
    // Reset flood fill queue
    g_flood_fill_head = 0;
    g_flood_fill_tail = 0;
    
    // Add starting position to queue
    g_flood_fill_queue[g_flood_fill_tail] = (LightNode){start_x, start_y, start_z, start_level, 255};
    g_flood_fill_tail = (g_flood_fill_tail + 1) % MAX_LIGHT_QUEUE;
    
    u32 processed = 0;
    const u32 max_per_batch = LIGHT_PROPAGATION_BATCH_SIZE;
    
    while (g_flood_fill_head != g_flood_fill_tail && processed < max_per_batch) {
        LightNode current = g_flood_fill_queue[g_flood_fill_head];
        g_flood_fill_head = (g_flood_fill_head + 1) % MAX_LIGHT_QUEUE;
        
        // Skip if light level is too low
        if (current.level <= 1) {
            processed++;
            continue;
        }
        
        // Propagate to all 6 neighbors
        for (u8 dir = 0; dir < 6; dir++) {
            // Skip propagating back to the direction we came from
            if (current.direction != 255 && dir == (5 - current.direction)) {
                continue;
            }
            
            i32 nx = current.x + g_direction_vectors[dir][0];
            i32 ny = current.y + g_direction_vectors[dir][1];
            i32 nz = current.z + g_direction_vectors[dir][2];
            
            // Check bounds and get block
            BlockID block_id = block_get(manager, nx, ny, nz);
            if (block_id == BLOCK_INVALID) {
                continue;
            }
            
            // Check if block blocks light
            if (block_id != BLOCK_AIR) {
                const BlockType *bt = block_registry_get(registry, block_id);
                if (bt && bt->opacity > 0) {
                    continue; // Opaque block blocks light
                }
            }
            
            // Calculate light level for neighbor
            u8 neighbor_level = current.level - 1;
            
            // Get current light level at neighbor position
            ChunkPos ncp = world_to_chunk_pos(nx, ny, nz);
            Chunk *neighbor_chunk = chunk_manager_get(manager, ncp);
            if (!neighbor_chunk) {
                continue;
            }
            
            i32 nlx = nx - ncp.x * CHUNK_SIZE;
            i32 nly = ny - ncp.y * CHUNK_SIZE;
            i32 nlz = nz - ncp.z * CHUNK_SIZE;
            
            u8 current_light = chunk_get_light(neighbor_chunk, nlx, nly, nlz, type);
            
            // Only propagate if we can increase the light level
            if (neighbor_level > current_light) {
                // Set the light level immediately
                if (type == LIGHT_TYPE_SKY) {
                    if (!neighbor_chunk->skylight) {
                        neighbor_chunk->skylight = calloc(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE, sizeof(u8));
                    }
                    i32 index = nlx + nlz * CHUNK_SIZE + nly * CHUNK_SIZE * CHUNK_SIZE;
                    neighbor_chunk->skylight[index] = neighbor_level;
                } else {
                    if (!neighbor_chunk->blocklight) {
                        neighbor_chunk->blocklight = calloc(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE, sizeof(u8));
                    }
                    i32 index = nlx + nlz * CHUNK_SIZE + nly * CHUNK_SIZE * CHUNK_SIZE;
                    neighbor_chunk->blocklight[index] = neighbor_level;
                }
                
                // Add to queue for further propagation
                u32 next_tail = (g_flood_fill_tail + 1) % MAX_LIGHT_QUEUE;
                if (next_tail != g_flood_fill_head) {
                    g_flood_fill_queue[g_flood_fill_tail] = (LightNode){nx, ny, nz, neighbor_level, dir};
                    g_flood_fill_tail = next_tail;
                }
            }
        }
        
        processed++;
    }
    
    if (processed > 0) {
        LOG_TRACE("Flood fill propagated light to %u blocks", processed);
    }
}

u8 block_calculate_light_level(ChunkManager *chunks, BlockRegistry *registry,
                               i32 x, i32 y, i32 z) {
  if (!chunks || !registry)
    return 0;

  // Try to get from cache first
  u8 cached_sky, cached_block;
  if (light_cache_get(x, y, z, &cached_sky, &cached_block)) {
    // Return the maximum of skylight and blocklight
    return cached_sky > cached_block ? cached_sky : cached_block;
  }

  // Cache miss - calculate light levels
  ChunkPos cp = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(chunks, cp);
  if (!chunk) {
    // Cache the result even for missing chunks
    light_cache_set(x, y, z, 0, 0);
    return 0;
  }

  i32 lx = x - cp.x * CHUNK_SIZE;
  i32 ly = y - cp.y * CHUNK_SIZE;
  i32 lz = z - cp.z * CHUNK_SIZE;

  // Check if block emits light
  BlockID block = chunk_get_block(chunk, lx, ly, lz);
  const BlockType *bt = block_registry_get(registry, block);
  if (bt && block_emits_light(bt)) {
    u8 emitted_light = bt->light_level;
    // Cache the result
    light_cache_set(x, y, z, emitted_light, emitted_light);
    return emitted_light;
  }

  // Calculate light from neighbors
  u8 max_sky_light = 0;
  u8 max_block_light = 0;
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
    
    u8 neighbor_sky = chunk_get_light(nchunk, nlx, nly, nlz, LIGHT_TYPE_SKY);
    u8 neighbor_block = chunk_get_light(nchunk, nlx, nly, nlz, LIGHT_TYPE_BLOCK);
    
    if (neighbor_sky > 0) neighbor_sky--;
    if (neighbor_block > 0) neighbor_block--;
    
    if (neighbor_sky > max_sky_light) max_sky_light = neighbor_sky;
    if (neighbor_block > max_block_light) max_block_light = neighbor_block;
  }

  // Cache the calculated result
  light_cache_set(x, y, z, max_sky_light, max_block_light);

  // Return the maximum light level
  return max_sky_light > max_block_light ? max_sky_light : max_block_light;
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
  if (!manager || !registry || light_level == 0)
    return;

  ChunkLightType type = (light_type == 0) ? LIGHT_TYPE_SKY : LIGHT_TYPE_BLOCK;
  
  // Use optimized flood fill propagation
  lighting_flood_fill_propagate(manager, registry, x, y, z, light_level, type);
}
