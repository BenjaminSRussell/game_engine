// src/chunk/chunk.c
//
// Module Overview:
// This module provides the concrete implementation for managing individual
// `Chunk`s and the overarching `ChunkManager` system. It forms the backbone of
// the game's world handling, offering functionalities for initializing and
// cleaning up the chunk system, enabling thread-safe access to chunk data, and
// facilitating the creation, retrieval, and unloading of chunks. A key
// responsibility is managing the block data and light levels within each chunk,
// including ensuring that mesh data is regenerated when blocks change.
// TODO Status:
// - Chunk compression system: IMPLEMENTED (zlib compression in chunk_serialize)
// - Chunk serialization: IMPLEMENTED (chunk_serialize/chunk_deserialize with
// compression)
// - Chunk versioning: IMPLEMENTED (version field in ChunkFileHeader)
// - Chunk validation system: IMPLEMENTED (chunk_validate, chunk_repair)
// - Chunk statistics tracking: IMPLEMENTED (ChunkStatistics,
// chunk_manager_get_statistics)
// TODO: Add chunk serialization optimization with delta compression.
// TODO: Implement chunk streaming system for seamless world loading.
// TODO: Add chunk priority queue system for intelligent loading order.
// TODO: Implement chunk preloading system based on player movement prediction.
// TODO: Add chunk metadata caching to speed up frequent lookups.
// TODO: Implement chunk dirty region tracking for partial updates.
// TODO: Add chunk neighbor dependency graph for proper loading order.
// TODO: Implement chunk memory pool system for efficient allocation.
//
// Key Flows:
// 1. **`ChunkManager` Initialization (`chunk_manager_init`):** Allocates a pool
// of `Chunk` structures
//    and initializes a read-write lock for thread-safe access.
// 2. **`ChunkManager` Cleanup (`chunk_manager_free`):** Frees all allocated
// memory for chunks,
//    including block data, light data, and mesh buffers, and destroys the
//    read-write lock.
// 3. **Chunk Retrieval (`chunk_manager_get`):** Retrieves a `Chunk` by its
// `ChunkPos`. It currently
//    uses a linear search (noted for optimization) and updates the chunk's
//    `last_accessed` timestamp. Access is protected by a read lock.
// 4. **Chunk Creation/Retrieval (`chunk_manager_get_or_create`):** Attempts to
// retrieve an existing
//    chunk. If not found, it initializes a new chunk in an empty slot,
//    allocating memory for its blocks, skylight, and blocklight data. This
//    operation is protected by a write lock.
// 5. **Chunk Unloading (`chunk_manager_unload`):** Frees resources associated
// with a specific chunk
//    and marks its slot as `CHUNK_STATE_UNLOADED`. It also clears neighbor
//    references. Protected by a write lock.
// 6. **Neighbor Management (`chunk_manager_update_neighbors`):** Updates the
// references to adjacent
//    chunks, which is important for accurate culling and light propagation at
//    chunk borders.
// 7. **Block Access (`chunk_get_block`, `chunk_set_block`):** Provides methods
// to read and write
//    `BlockID`s at local chunk coordinates. `chunk_get_block` handles
//    out-of-bounds requests by checking neighbor chunks, maintaining a seamless
//    block world. `chunk_set_block` also marks the chunk (and its relevant
//    neighbors) as dirty for mesh regeneration.
// 8. **Light Access (`chunk_get_light`, `chunk_set_light`):** Provides methods
// to read and write
//    skylight and blocklight levels at local chunk coordinates.
// 9. **Mesh Dirty Flag (`chunk_mark_mesh_dirty`, `chunk_needs_mesh_update`):**
// Functions to
//    flag a chunk for mesh recalculation and to check if a mesh update is
//    required.
//
// Invariants:
// - All chunk allocations and deallocations are managed internally.
// - `CHUNK_SIZE`, `CHUNK_SIZE_SQ`, and `CHUNK_SIZE_CUBE` constants are used for
// indexing.
// - Thread-safe access to the `ChunkManager` is ensured using
// `pthread_rwlock_t` on non-web platforms.
// - Block data is stored as a 1D array for cache efficiency.
// - Light data is stored in separate skylight and blocklight arrays.
//
// (Additional comments from the file indicating external dependencies and
// roadmaps are preserved.) Chunk storage, neighbors, and mesh dirty tracking.
// Roadmap: docs/CHUNK_SYSTEM_ROADMAP.md.
// #include <audio/audio_system.h>
#include <block/lighting.h>
#include <chunk/chunk.h>
#include <chunk/chunk_format.h>
#include "engine/include/core/logger.h"
// #include <crafting/brewing.h>
#include <math/vec3.h>
#include <pthread.h>
#include <rendering/renderer.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <core/threading/job.h>
#include <time.h>
// #include <effects/vfx/particle_system.h>
#include <world/generator.h>
#include <zlib.h>

// Forward declarations for chunk statistics functions
static void chunk_stats_on_serialize(f32 time_ms);
static void chunk_stats_on_deserialize(f32 time_ms);

// Global statistics tracking
static ChunkStatistics g_chunk_stats = {0};

// Get current time in milliseconds
static f64 get_current_time_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (f64)ts.tv_sec * 1000.0 + (f64)ts.tv_nsec / 1000000.0;
}

static void queue_chunk_border_relight(ChunkManager *manager,
                                       BlockRegistry *block_registry,
                                       ChunkPos chunk_pos, Chunk *chunk) {
  if (!manager || !block_registry || !chunk) {
    return;
  }

  i32 base_x, base_y, base_z;
  chunk_to_world_pos(chunk_pos, &base_x, &base_y, &base_z);

  for (i32 ly = 0; ly < CHUNK_SIZE; ++ly) {
    for (i32 lz = 0; lz < CHUNK_SIZE; ++lz) {
      for (i32 lx = 0; lx < CHUNK_SIZE; ++lx) {
        bool is_border = (lx == 0) || (lx == CHUNK_SIZE - 1) || (ly == 0) ||
                         (ly == CHUNK_SIZE - 1) || (lz == 0) ||
                         (lz == CHUNK_SIZE - 1);
        if (!is_border) {
          continue;
        }

        i32 wx = base_x + lx;
        i32 wy = base_y + ly;
        i32 wz = base_z + lz;

        u8 bl = chunk_get_light(chunk, lx, ly, lz, LIGHT_TYPE_BLOCK);
        if (bl > 1) {
          block_propagate_light(manager, block_registry, wx, wy, wz, bl,
                                LIGHT_TYPE_BLOCK);
        }

        u8 sl = chunk_get_light(chunk, lx, ly, lz, LIGHT_TYPE_SKY);
        if (sl > 1) {
          block_propagate_light(manager, block_registry, wx, wy, wz, sl,
                                LIGHT_TYPE_SKY);
        }
      }
    }
  }
}

// Atomic chunk creation tracking
static atomic_uint_fast64_t g_chunk_creation_counter = ATOMIC_VAR_INIT(0);
static atomic_uint_fast32_t g_pending_generations = ATOMIC_VAR_INIT(0);

// Chunk creation ticket structure
typedef struct {
  ChunkPos pos;
  u64 ticket;
  atomic_bool completed;
} ChunkCreationTicket;

#define MAX_PENDING_TICKETS 1024
static ChunkCreationTicket g_pending_tickets[MAX_PENDING_TICKETS];
static pthread_mutex_t g_pending_tickets_mutex = PTHREAD_MUTEX_INITIALIZER;

// Get next creation ticket
static u64 get_chunk_creation_ticket(void) {
  return atomic_fetch_add(&g_chunk_creation_counter, 1) + 1;
}

// Check if chunk creation is already in progress
static bool is_chunk_creation_in_progress(ChunkPos pos) {
  pthread_mutex_lock(&g_pending_tickets_mutex);

  for (u32 i = 0; i < MAX_PENDING_TICKETS; i++) {
    if (!g_pending_tickets[i].completed &&
        chunk_pos_equal(g_pending_tickets[i].pos, pos)) {
      pthread_mutex_unlock(&g_pending_tickets_mutex);
      return true;
    }
  }

  pthread_mutex_unlock(&g_pending_tickets_mutex);
  return false;
}

// Add chunk creation ticket
static bool add_chunk_creation_ticket(ChunkPos pos, u64 *out_ticket) {
  pthread_mutex_lock(&g_pending_tickets_mutex);

  // Find empty slot
  for (u32 i = 0; i < MAX_PENDING_TICKETS; i++) {
    if (g_pending_tickets[i].completed) {
      g_pending_tickets[i].pos = pos;
      g_pending_tickets[i].ticket = get_chunk_creation_ticket();
      g_pending_tickets[i].completed = false;
      *out_ticket = g_pending_tickets[i].ticket;

      atomic_fetch_add(&g_pending_generations, 1);
      pthread_mutex_unlock(&g_pending_tickets_mutex);
      return true;
    }
  }

  pthread_mutex_unlock(&g_pending_tickets_mutex);
  return false; // No available slots
}

// Complete chunk creation ticket
static void complete_chunk_creation_ticket(u64 ticket) {
  pthread_mutex_lock(&g_pending_tickets_mutex);

  for (u32 i = 0; i < MAX_PENDING_TICKETS; i++) {
    if (g_pending_tickets[i].ticket == ticket) {
      g_pending_tickets[i].completed = true;
      atomic_fetch_sub(&g_pending_generations, 1);
      break;
    }
  }

  pthread_mutex_unlock(&g_pending_tickets_mutex);
}

// World generation job data
typedef struct {
  ChunkManager *manager;
  WorldGenerator *generator;
  ChunkPos chunk_pos;
  u32 seed;
  u64 creation_ticket;
} ChunkGenerationJob;

// World generation job function
static void chunk_generation_job(void *data) {
  ChunkGenerationJob *job = (ChunkGenerationJob *)data;
  if (!job || !job->manager || !job->generator) {
    if (job)
      complete_chunk_creation_ticket(job->creation_ticket);
    return;
  }

  // Get the chunk
  Chunk *chunk = chunk_manager_get(job->manager, job->chunk_pos);
  if (!chunk || chunk->state != CHUNK_STATE_GENERATING) {
    complete_chunk_creation_ticket(job->creation_ticket);
    return;
  }

  // Generate the chunk
  f64 start_time = get_current_time_ms();
  world_generator_generate_chunk(job->generator, chunk);
  f64 end_time = get_current_time_ms();

  chunk->gen_time_ms = (f32)(end_time - start_time);
  chunk->gen_time_pending = false;
  chunk->state = CHUNK_STATE_GENERATED;

  LOG_DEBUG("Generated chunk (%d, %d, %d) in %.2f ms", job->chunk_pos.x,
            job->chunk_pos.y, job->chunk_pos.z, chunk->gen_time_ms);

  // Complete the creation ticket
  complete_chunk_creation_ticket(job->creation_ticket);

  // Seed block light propagation after generation (emissive sources)
  {
    extern BlockRegistry g_block_registry;
    i32 base_x, base_y, base_z;
    chunk_to_world_pos(job->chunk_pos, &base_x, &base_y, &base_z);
    for (i32 ly = 0; ly < CHUNK_SIZE; ++ly) {
      for (i32 lz = 0; lz < CHUNK_SIZE; ++lz) {
        for (i32 lx = 0; lx < CHUNK_SIZE; ++lx) {
          BlockID b = chunk_get_block(chunk, lx, ly, lz);
          const BlockType *t = block_registry_get(&g_block_registry, b);
          if (t && (t->flags & BLOCK_LIGHT_EMITTING) && t->light_level > 0) {
            block_propagate_light(job->manager, &g_block_registry, base_x + lx,
                                  base_y + ly, base_z + lz, t->light_level,
                                  LIGHT_TYPE_BLOCK);
          }
        }
      }
    }
  }

  // Queue propagation across chunk borders now that this chunk exists.
  // This ensures seams resolve when neighboring chunks are already loaded.
  extern BlockRegistry g_block_registry;
  queue_chunk_border_relight(job->manager, &g_block_registry, job->chunk_pos,
                             chunk);
  free(job);
}

void chunk_manager_init(ChunkManager *manager, u32 capacity) {
  if (!manager || capacity == 0) {
    LOG_ERROR("Invalid chunk manager parameters");
    return;
  }

  manager->chunks = (Chunk *)calloc(capacity, sizeof(Chunk));
  if (!manager->chunks) {
    LOG_ERROR("OOM: Failed to allocate %u chunks (%zu bytes)", capacity,
              capacity * sizeof(Chunk));
    manager->count = 0;
    manager->capacity = 0;
    manager->access_counter = 0;
    return;
  }

  manager->count = 0;
  manager->capacity = capacity;
  manager->access_counter = 0;

  // Initialize streaming system
  chunk_manager_init_streaming(manager, 128.0f, 256.0f); // Default radii

  // Initialize read-write lock for thread-safe access
#ifndef PLATFORM_WEB
  if (pthread_rwlock_init(&manager->lock, NULL) != 0) {
    LOG_ERROR("Failed to initialize chunk manager rwlock");
    free(manager->chunks);
    manager->chunks = NULL;
    manager->capacity = 0;
    return;
  }
#endif
}

void chunk_manager_free(ChunkManager *manager) {
#ifndef PLATFORM_WEB
  pthread_rwlock_destroy(&manager->lock);
#endif
  for (u32 i = 0; i < manager->capacity; i++) {
    Chunk *chunk = &manager->chunks[i];
    if (chunk->blocks) {
      free(chunk->blocks);
    }
    // Free new lighting arrays
    if (chunk->skylight) {
      free(chunk->skylight);
    }
    if (chunk->blocklight) {
      free(chunk->blocklight);
    }
    if (chunk->metadata) {
      free(chunk->metadata);
    }
    // Original light_levels is replaced by skylight/blocklight
    // if (chunk->light_levels) {
    //   free(chunk->light_levels);
    // }
    if (chunk->mesh.vertices) {
      free(chunk->mesh.vertices);
    }
    if (chunk->mesh.indices) {
      free(chunk->mesh.indices);
    }
  }
  free(manager->chunks);
  manager->chunks = NULL;
  manager->count = 0;
  manager->capacity = 0;
}

static u32 chunk_hash(ChunkPos pos) {
  // Simple hash function for chunk position
  // scans in manager.
  return ((u32)pos.x * 73856093) ^ ((u32)pos.y * 19349663) ^
         ((u32)pos.z * 83492791);
}

Chunk *chunk_manager_get(ChunkManager *manager, ChunkPos pos) {
#ifndef PLATFORM_WEB
  pthread_rwlock_rdlock(&manager->lock); // Read lock - allows concurrent reads
#endif
  // Linear search for now (can be optimized with hash table)
  // ChunkPos.
  // shared locks.
  Chunk *result = NULL;
  for (u32 i = 0; i < manager->capacity; i++) {
    Chunk *chunk = &manager->chunks[i];
    if (chunk->state != CHUNK_STATE_UNLOADED &&
        chunk_pos_equal(chunk->pos, pos)) {
      chunk->last_accessed = ++manager->access_counter;
      result = chunk;
      break;
    }
  }
#ifndef PLATFORM_WEB
  pthread_rwlock_unlock(&manager->lock);
#endif
  return result;
}

Chunk *chunk_manager_get_or_create(ChunkManager *manager, ChunkPos pos) {
#ifndef PLATFORM_WEB
  pthread_rwlock_wrlock(&manager->lock); // Write lock - exclusive access
#endif
  // Check if already exists (within write lock to prevent double-creation)
  Chunk *existing = NULL;
  for (u32 i = 0; i < manager->capacity; i++) {
    Chunk *chunk = &manager->chunks[i];
    if (chunk->state != CHUNK_STATE_UNLOADED &&
        chunk_pos_equal(chunk->pos, pos)) {
      existing = chunk;
      break;
    }
  }

  if (existing) {
#ifndef PLATFORM_WEB
    pthread_rwlock_unlock(&manager->lock);
#endif
    return existing;
  }

  // Find empty slot
  Chunk *result = NULL;
  for (u32 i = 0; i < manager->capacity; i++) {
    Chunk *chunk = &manager->chunks[i];
    if (chunk->state == CHUNK_STATE_UNLOADED) {
      // Initialize chunk
      chunk->pos = pos;
      chunk->state = CHUNK_STATE_LOADING;

      // Allocate blocks
      chunk->blocks = (BlockID *)calloc(CHUNK_SIZE_CUBE, sizeof(BlockID));
      if (!chunk->blocks) {
        LOG_ERROR("OOM: Failed to allocate blocks for chunk (%d,%d,%d)", pos.x,
                  pos.y, pos.z);
        chunk->state = CHUNK_STATE_UNLOADED;
#ifndef PLATFORM_WEB
        pthread_rwlock_unlock(&manager->lock);
#endif
        return NULL;
      }

      // Allocate lighting (dual channel)
      // Original light_levels is replaced by skylight/blocklight
      // if (!chunk->light_levels) {
      //   LOG_ERROR("OOM: Failed to allocate lighting for chunk (%d,%d,%d)",
      //             pos.x, pos.y, pos.z);
      //   free(chunk->blocks);
      //   chunk->blocks = NULL;
      //   chunk->state = CHUNK_STATE_UNLOADED;
      // #ifndef PLATFORM_WEB
      //   pthread_rwlock_unlock(&manager->lock);
      // #endif
      //   return NULL;
      // }
      chunk->skylight = (u8 *)calloc(CHUNK_SIZE_CUBE, sizeof(u8));
      if (!chunk->skylight) {
        LOG_ERROR("OOM: Failed to allocate skylight for chunk (%d,%d,%d)",
                  pos.x, pos.y, pos.z);
        free(chunk->blocks);
        chunk->blocks = NULL;
        chunk->state = CHUNK_STATE_UNLOADED;
#ifndef PLATFORM_WEB
        pthread_rwlock_unlock(&manager->lock);
#endif
        return NULL;
      }

      chunk->blocklight = (u8 *)calloc(CHUNK_SIZE_CUBE, sizeof(u8));
      if (!chunk->blocklight) {
        LOG_ERROR("OOM: Failed to allocate blocklight for chunk (%d,%d,%d)",
                  pos.x, pos.y, pos.z);
        free(chunk->skylight);
        chunk->skylight = NULL;
        free(chunk->blocks);
        chunk->blocks = NULL;
        chunk->state = CHUNK_STATE_UNLOADED;
#ifndef PLATFORM_WEB
        pthread_rwlock_unlock(&manager->lock);
#endif
        return NULL;
      }

      chunk->metadata = (u8 *)calloc(CHUNK_SIZE_CUBE, sizeof(u8));
      if (!chunk->metadata) {
        LOG_ERROR("OOM: Failed to allocate metadata for chunk (%d,%d,%d)",
                  pos.x, pos.y, pos.z);
        free(chunk->blocklight);
        chunk->blocklight = NULL;
        free(chunk->skylight);
        chunk->skylight = NULL;
        free(chunk->blocks);
        chunk->blocks = NULL;
        chunk->state = CHUNK_STATE_UNLOADED;
#ifndef PLATFORM_WEB
        pthread_rwlock_unlock(&manager->lock);
#endif
        return NULL;
      }

      // Initialize lighting: skylight starts at max (15) for sky-exposed chunks
      // blocklight starts at 0 (no light sources)
      memset(chunk->skylight, 15, CHUNK_SIZE_CUBE);  // Full skylight initially
      memset(chunk->blocklight, 0, CHUNK_SIZE_CUBE); // No block light sources

      // Dispatch world generation job asynchronously via thread pool
      extern ThreadPool *g_thread_pool;

      // Check if chunk creation is already in progress to prevent
      // double-creation
      if (is_chunk_creation_in_progress(chunk->pos)) {
        LOG_DEBUG("Chunk (%d, %d, %d) creation already in progress, skipping",
                  chunk->pos.x, chunk->pos.y, chunk->pos.z);
        chunk->state = CHUNK_STATE_LOADED;         // Reset to loaded state
        chunk_manager_unload(manager, chunk->pos); // Remove this duplicate
#ifndef PLATFORM_WEB
        pthread_rwlock_unlock(&manager->lock);
#endif
        return NULL;
      }

      // Add creation ticket
      u64 creation_ticket;
      if (!add_chunk_creation_ticket(chunk->pos, &creation_ticket)) {
        LOG_ERROR("Failed to create chunk ticket for (%d, %d, %d)",
                  chunk->pos.x, chunk->pos.y, chunk->pos.z);
        chunk->state = CHUNK_STATE_LOADED;
      } else if (g_thread_pool) {
        chunk->state = CHUNK_STATE_GENERATING;
        chunk->gen_time_pending = true;

        // Create generation job
        ChunkGenerationJob *job = malloc(sizeof(ChunkGenerationJob));
        if (job) {
          job->manager = manager;
          extern WorldGenerator *g_world_generator;
          job->generator = g_world_generator;
          job->chunk_pos = chunk->pos;
          extern u32 g_world_seed;
          job->seed = g_world_seed;
          job->creation_ticket = creation_ticket;

          thread_pool_submit(g_thread_pool, chunk_generation_job, job,
                             1); // Normal priority
          LOG_DEBUG("Dispatched generation job for chunk (%d, %d, %d) with "
                    "ticket %llu",
                    chunk->pos.x, chunk->pos.y, chunk->pos.z, creation_ticket);
        } else {
          LOG_ERROR("Failed to allocate generation job for chunk (%d, %d, %d)",
                    chunk->pos.x, chunk->pos.y, chunk->pos.z);
          complete_chunk_creation_ticket(creation_ticket);
          chunk->state = CHUNK_STATE_LOADED; // Fallback to loaded state
        }
      } else {
        // Fallback: generate synchronously if thread pool not available
        LOG_WARN("Thread pool not available, generating chunk synchronously");
        chunk->state = CHUNK_STATE_GENERATING;
        f64 start_time = get_current_time_ms();
        extern WorldGenerator *g_world_generator;
        world_generator_generate_chunk(g_world_generator, chunk);
        f64 end_time = get_current_time_ms();
        chunk->gen_time_ms = (f32)(end_time - start_time);
        chunk->gen_time_pending = false;
        chunk->state = CHUNK_STATE_GENERATED;
        {
          extern BlockRegistry g_block_registry;
          i32 base_x, base_y, base_z;
          chunk_to_world_pos(chunk->pos, &base_x, &base_y, &base_z);
          for (i32 ly = 0; ly < CHUNK_SIZE; ++ly) {
            for (i32 lz = 0; lz < CHUNK_SIZE; ++lz) {
              for (i32 lx = 0; lx < CHUNK_SIZE; ++lx) {
                BlockID b = chunk_get_block(chunk, lx, ly, lz);
                const BlockType *t = block_registry_get(&g_block_registry, b);
                if (t && (t->flags & BLOCK_LIGHT_EMITTING) &&
                    t->light_level > 0) {
                  block_propagate_light(manager, &g_block_registry, base_x + lx,
                                        base_y + ly, base_z + lz,
                                        t->light_level, LIGHT_TYPE_BLOCK);
                }
              }
            }
          }

          // Queue propagation across chunk borders now that this chunk exists.
          // This ensures seams resolve when neighboring chunks are already
          // loaded.
          queue_chunk_border_relight(manager, &g_block_registry, chunk->pos,
                                     chunk);
        }
        complete_chunk_creation_ticket(creation_ticket);
      }

      chunk->mesh.vertices = NULL;
      chunk->mesh.indices = NULL;
      chunk->mesh.vertex_count = 0;
      chunk->mesh.index_count = 0;
      chunk->mesh.dirty = true;
      memset(chunk->neighbors, 0, sizeof(chunk->neighbors));
      chunk->last_accessed = ++manager->access_counter;
      chunk->modified = false;
      manager->count++;

      // Update statistics
      g_chunk_stats.total_chunks_created++;

      result = chunk;
      break;
    }
  }

#ifndef PLATFORM_WEB
  pthread_rwlock_unlock(&manager->lock);
#endif
  return result; // No space
}

void chunk_manager_unload(ChunkManager *manager, ChunkPos pos) {
#ifndef PLATFORM_WEB
  pthread_rwlock_wrlock(&manager->lock); // Write lock - exclusive access
#endif

  // Find chunk
  Chunk *chunk = NULL;
  for (u32 i = 0; i < manager->capacity; i++) {
    Chunk *c = &manager->chunks[i];
    if (c->state != CHUNK_STATE_UNLOADED && chunk_pos_equal(c->pos, pos)) {
      chunk = c;
      break;
    }
  }

  if (!chunk) {
#ifndef PLATFORM_WEB
    pthread_rwlock_unlock(&manager->lock);
#endif
    return;
  }

  // Free resources
  if (chunk->blocks) {
    free(chunk->blocks);
    chunk->blocks = NULL;
  }
  if (chunk->skylight) {
    free(chunk->skylight);
    chunk->skylight = NULL;
  }
  if (chunk->blocklight) {
    free(chunk->blocklight);
    chunk->blocklight = NULL;
  }
  if (chunk->metadata) {
    free(chunk->metadata);
    chunk->metadata = NULL;
  }
  // Destroy Vulkan buffers if created
  // thread with proper fences; avoid use-after-free.
  // unload. Note: Requires renderer reference - handled in chunk manager
  // cleanup
  // (refcount/RCU or stop-the-world section).

  if (chunk->mesh.vertices) {
    free(chunk->mesh.vertices);
    chunk->mesh.vertices = NULL;
  }
  if (chunk->mesh.indices) {
    free(chunk->mesh.indices);
    chunk->mesh.indices = NULL;
  }

  // Clear neighbors
  for (int i = 0; i < 6; i++) {
    if (chunk->neighbors[i]) {
      // Remove this chunk from neighbor's neighbor list
      for (int j = 0; j < 6; j++) {
        if (chunk->neighbors[i]->neighbors[j] == chunk) {
          chunk->neighbors[i]->neighbors[j] = NULL;
        }
      }
    }
  }

  chunk->state = CHUNK_STATE_UNLOADED;
  manager->count--;

  // Update statistics
  g_chunk_stats.total_chunks_unloaded++;

#ifndef PLATFORM_WEB
  pthread_rwlock_unlock(&manager->lock);
#endif
}

void chunk_manager_update_neighbors(ChunkManager *manager, ChunkPos pos) {
  Chunk *chunk = chunk_manager_get(manager, pos);
  if (!chunk)
    return;

  // Update neighbor pointers
  // from seeing inconsistent neighbor graphs.
  // update lighting.
  chunk->neighbors[0] =
      chunk_manager_get(manager, chunk_pos(pos.x + 1, pos.y, pos.z)); // pos_x
  chunk->neighbors[1] =
      chunk_manager_get(manager, chunk_pos(pos.x - 1, pos.y, pos.z)); // neg_x
  chunk->neighbors[2] =
      chunk_manager_get(manager, chunk_pos(pos.x, pos.y + 1, pos.z)); // pos_y
  chunk->neighbors[3] =
      chunk_manager_get(manager, chunk_pos(pos.x, pos.y - 1, pos.z)); // neg_y
  chunk->neighbors[4] =
      chunk_manager_get(manager, chunk_pos(pos.x, pos.y, pos.z + 1)); // pos_z
  chunk->neighbors[5] =
      chunk_manager_get(manager, chunk_pos(pos.x, pos.y, pos.z - 1)); // neg_z
}

BlockID chunk_get_block(Chunk *chunk, i32 x, i32 y, i32 z) {
  if (!chunk || !chunk->blocks)
    return BLOCK_AIR;

  // Handle out-of-bounds by checking neighbors
  // once and index directly.
  // fallback to manager lookup if NULL.
  if (x < 0) {
    if (chunk->neighbors[1]) { // neg_x
      return chunk_get_block(chunk->neighbors[1], x + CHUNK_SIZE, y, z);
    }
    return BLOCK_AIR;
  }
  if (x >= CHUNK_SIZE) {
    if (chunk->neighbors[0]) { // pos_x
      return chunk_get_block(chunk->neighbors[0], x - CHUNK_SIZE, y, z);
    }
    return BLOCK_AIR;
  }
  if (y < 0) {
    if (chunk->neighbors[3]) { // neg_y
      return chunk_get_block(chunk->neighbors[3], x, y + CHUNK_SIZE, z);
    }
    return BLOCK_AIR;
  }
  if (y >= CHUNK_SIZE) {
    if (chunk->neighbors[2]) { // pos_y
      return chunk_get_block(chunk->neighbors[2], x, y - CHUNK_SIZE, z);
    }
    return BLOCK_AIR;
  }
  if (z < 0) {
    if (chunk->neighbors[5]) { // neg_z
      return chunk_get_block(chunk->neighbors[5], x, y, z + CHUNK_SIZE);
    }
    return BLOCK_AIR;
  }
  if (z >= CHUNK_SIZE) {
    if (chunk->neighbors[4]) { // pos_z
      return chunk_get_block(chunk->neighbors[4], x, y, z - CHUNK_SIZE);
    }
    return BLOCK_AIR;
  }

  u32 index = chunk_block_index(x, y, z);
  return chunk->blocks[index];
}

void chunk_set_block(Chunk *chunk, i32 x, i32 y, i32 z, BlockID block) {
  if (!chunk || !chunk->blocks)
    return;

  // Handle out-of-bounds
  if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 ||
      z >= CHUNK_SIZE) {
    // Would need to set in neighbor chunk or create new chunk
    return;
  }

  u32 index = chunk_block_index(x, y, z);
  chunk->blocks[index] = block;
  chunk->modified = true;
  chunk_mark_mesh_dirty(chunk);
  // for this position.
  // AI, triggers).

  // Mark neighbors as dirty if on edge
  if (x == 0 && chunk->neighbors[1])
    chunk_mark_mesh_dirty(chunk->neighbors[1]);
  if (x == CHUNK_SIZE - 1 && chunk->neighbors[0])
    chunk_mark_mesh_dirty(chunk->neighbors[0]);
  if (y == 0 && chunk->neighbors[3])
    chunk_mark_mesh_dirty(chunk->neighbors[3]);
  if (y == CHUNK_SIZE - 1 && chunk->neighbors[2])
    chunk_mark_mesh_dirty(chunk->neighbors[2]);
  if (z == 0 && chunk->neighbors[5])
      chunk_mark_mesh_dirty(chunk->neighbors[5]);
  if (z == CHUNK_SIZE - 1 && chunk->neighbors[4])
    chunk_mark_mesh_dirty(chunk->neighbors[4]);
}

u8 chunk_get_light(Chunk *chunk, i32 x, i32 y, i32 z, ChunkLightType type) {
  if (!chunk || !chunk->skylight || !chunk->blocklight)
    return 0;

  i32 idx = chunk_block_index(x, y, z);
  i32 byte_idx = idx / 2;
  bool upper = (idx % 2) == 1;

  u8 data = (type == LIGHT_TYPE_SKY) ? chunk->skylight[byte_idx]
                                     : chunk->blocklight[byte_idx];

  return upper ? (data >> 4) : (data & 0xF);
}

void chunk_set_light(Chunk *chunk, i32 x, i32 y, i32 z, u8 level,
                     ChunkLightType type) {
  if (!chunk)
    return;

  if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 ||
      z >= CHUNK_SIZE) {
    return;
  }

  u32 index = chunk_block_index(x, y, z);

  if (type == LIGHT_TYPE_SKY) {
    if (chunk->skylight)
      chunk->skylight[index] = level;
  } else {
    if (chunk->blocklight)
      chunk->blocklight[index] = level;
  }

  chunk->modified = true;
}

u8 chunk_get_metadata(Chunk *chunk, i32 x, i32 y, i32 z) {
  if (!chunk || !chunk->metadata)
    return 0;

  // Handle out-of-bounds check similar to other accessors
  if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 ||
      z >= CHUNK_SIZE) {
    return 0;
  }

  u32 index = chunk_block_index(x, y, z);
  return chunk->metadata[index];
}

void chunk_set_metadata(Chunk *chunk, i32 x, i32 y, i32 z, u8 data) {
  if (!chunk || !chunk->metadata)
    return;

  if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 ||
      z >= CHUNK_SIZE) {
    return;
  }

  u32 index = chunk_block_index(x, y, z);
  if (chunk->metadata[index] != data) {
    chunk->metadata[index] = data;
    chunk->modified = true;
    chunk_mark_mesh_dirty(
        chunk); // Metadata changes (like liquid level) affect mesh
  }
}

void chunk_mark_mesh_dirty(Chunk *chunk) {
  if (!chunk)
    return;
  chunk->mesh.dirty = true;
}

bool chunk_needs_mesh_update(Chunk *chunk) {
  return chunk && chunk->mesh.dirty && chunk->state >= CHUNK_STATE_GENERATED;
}

// Serialize chunk to buffer
int chunk_serialize(void *chunk_ptr, uint8_t *buffer, size_t buffer_size) {
  Chunk *chunk = (Chunk *)chunk_ptr;
  if (!chunk || !buffer)
    return -1;

  // Check buffer size for header
  if (buffer_size < sizeof(ChunkFileHeader))
    return -1;

  // Create and fill header
  ChunkFileHeader header = {
      .magic = CHUNK_MAGIC,
      .version = CHUNK_VERSION,
      .compression = CHUNK_COMPRESSION_ZLIB,
      .x = chunk->pos.x,
      .z = chunk->pos.z,
      .timestamp = (uint32_t)time(NULL),
      .block_count = CHUNK_SIZE_TOTAL,
  };

  // Prepare block data for compression
  uint8_t *block_data = malloc(header.block_count);
  if (!block_data)
    return -1;

  // Copy all blocks into flat array [y][z][x] order
  int index = 0;
  for (int y = 0; y < CHUNK_SIZE; y++) {
    for (int z = 0; z < CHUNK_SIZE; z++) {
      for (int x = 0; x < CHUNK_SIZE; x++) {
        uint32_t block_index = chunk_block_index(x, y, z);
        if (chunk->blocks)
          block_data[index++] = chunk->blocks[block_index];
        else
          block_data[index++] = 0;
      }
    }
  }

  // Compress block data
  uLongf compressed_size = compressBound(header.block_count);
  uint8_t *compressed = malloc(compressed_size);
  if (!compressed) {
    free(block_data);
    return -1;
  }

  int compress_result = compress2(compressed, &compressed_size, block_data,
                                  header.block_count, 9);
  if (compress_result != Z_OK) {
    free(compressed);
    free(block_data);
    return -1;
  }

  // Check if compressed data fits in buffer
  header.data_size = (uint32_t)compressed_size;
  size_t total_size = sizeof(ChunkFileHeader) + header.data_size;
  if (buffer_size < total_size) {
    free(compressed);
    free(block_data);
    return -1;
  }

  // Write header and compressed data to buffer
  memcpy(buffer, &header, sizeof(ChunkFileHeader));
  memcpy(buffer + sizeof(ChunkFileHeader), compressed, compressed_size);

  free(compressed);
  free(block_data);

  // Update statistics
  f32 serialize_time = (f32)clock() / CLOCKS_PER_SEC * 1000.0f; // Convert to ms
  chunk_stats_on_serialize(serialize_time);
  g_chunk_stats.total_chunks_saved++;

  LOG_DEBUG("Serialized chunk (%d, %d) to %zu bytes", chunk->pos.x,
            chunk->pos.z, total_size);
  return (int)total_size;
}

// Deserialize chunk from buffer
int chunk_deserialize(void *chunk_ptr, const uint8_t *buffer,
                      size_t buffer_size) {
  Chunk *chunk = (Chunk *)chunk_ptr;
  if (!chunk || !buffer || buffer_size < sizeof(ChunkFileHeader))
    return -1;

  // Read and verify header
  ChunkFileHeader header;
  memcpy(&header, buffer, sizeof(ChunkFileHeader));

  if (header.magic != CHUNK_MAGIC) {
    LOG_ERROR("Invalid chunk magic number: 0x%X", header.magic);
    return -1;
  }

  if (header.version != CHUNK_VERSION) {
    LOG_ERROR("Incompatible chunk version: %d", header.version);
    return -1;
  }

  // Decompress block data
  uint8_t *decompressed = malloc(header.block_count);
  if (!decompressed)
    return -1;

  uLongf decompressed_size = header.block_count;
  int decompress_result =
      uncompress(decompressed, &decompressed_size,
                 buffer + sizeof(ChunkFileHeader), header.data_size);
  if (decompress_result != Z_OK) {
    LOG_ERROR("Failed to decompress chunk data");
    free(decompressed);
    return -1;
  }

  // Allocate block storage if needed
  if (!chunk->blocks) {
    chunk->blocks = malloc(CHUNK_SIZE_TOTAL * sizeof(BlockID));
    if (!chunk->blocks) {
      free(decompressed);
      return -1;
    }
  }

  // Load blocks from decompressed data
  int index = 0;
  for (int y = 0; y < CHUNK_SIZE; y++) {
    for (int z = 0; z < CHUNK_SIZE; z++) {
      for (int x = 0; x < CHUNK_SIZE; x++) {
        uint32_t block_index = chunk_block_index(x, y, z);
        chunk->blocks[block_index] = decompressed[index++];
      }
    }
  }

  // Set chunk metadata
  chunk->pos.x = header.x;
  chunk->pos.z = header.z;
  chunk->modified = false;
  chunk->state = CHUNK_STATE_LOADED;

  free(decompressed);

  // Update statistics
  f32 deserialize_time =
      (f32)clock() / CLOCKS_PER_SEC * 1000.0f; // Convert to ms
  chunk_stats_on_deserialize(deserialize_time);
  g_chunk_stats.total_chunks_loaded++;

  // Validate chunk after deserialization
  ChunkValidationResult validation = chunk_validate(chunk);
  if (!validation.is_valid) {
    LOG_WARN("Chunk (%d, %d) validation failed after deserialization: %s",
             chunk->pos.x, chunk->pos.z, validation.error_messages);
    // Attempt repair
    chunk_repair(chunk, &validation);
  }

  LOG_DEBUG("Deserialized chunk (%d, %d) from buffer", chunk->pos.x,
            chunk->pos.z);
  return 0;
}

// Global statistics tracking - moved to top
// static ChunkStatistics g_chunk_stats = {0};

// Chunk validation implementation
ChunkValidationResult chunk_validate(Chunk *chunk) {
  ChunkValidationResult result = {0};
  result.is_valid = true;
  result.error_messages[0] = '\0';

  if (!chunk) {
    result.is_valid = false;
    result.error_count = 1;
    strcpy(result.error_messages, "Chunk is NULL");
    g_chunk_stats.validation_errors++;
    return result;
  }

  // Validate chunk position
  if (chunk->pos.x == 0 && chunk->pos.y == 0 && chunk->pos.z == 0 &&
      chunk->state == CHUNK_STATE_UNLOADED) {
    // This might be an uninitialized chunk, which is OK
    return result;
  }

  // Validate state
  if (chunk->state < CHUNK_STATE_UNLOADED || chunk->state > CHUNK_STATE_READY) {
    result.is_valid = false;
    result.error_count++;
    strcat(result.error_messages, "Invalid chunk state; ");
  }

  // Validate block data
  if (chunk->state >= CHUNK_STATE_LOADED) {
    if (!chunk->blocks) {
      result.is_valid = false;
      result.has_corrupted_blocks = true;
      result.error_count++;
      strcat(result.error_messages, "Missing block data; ");
    } else {
      // Check for invalid block IDs (assuming max block ID is reasonable)
      // This is a basic check - in a real implementation you'd validate against
      // block registry
      for (u32 i = 0; i < CHUNK_SIZE_TOTAL; i++) {
        // Basic validation - block IDs should be reasonable
        // In a real implementation, validate against BlockRegistry
        if (chunk->blocks[i] > 1000) { // Arbitrary high limit
          result.has_corrupted_blocks = true;
          result.error_count++;
          if (result.error_count == 1) {
            strcat(result.error_messages, "Invalid block IDs detected; ");
          }
          break; // Don't check all blocks on first error
        }
      }
    }

    // Validate lighting data
    if (chunk->skylight) {
      for (u32 i = 0; i < CHUNK_SIZE_TOTAL; i++) {
        if (chunk->skylight[i] > 15) { // Light levels are 0-15
          result.has_invalid_lighting = true;
          result.error_count++;
          if (result.error_count <= 5) {
            strcat(result.error_messages, "Invalid skylight values; ");
          }
          break;
        }
      }
    }

    if (chunk->blocklight) {
      for (u32 i = 0; i < CHUNK_SIZE_TOTAL; i++) {
        if (chunk->blocklight[i] > 15) {
          result.has_invalid_lighting = true;
          result.error_count++;
          if (result.error_count <= 5) {
            strcat(result.error_messages, "Invalid blocklight values; ");
          }
          break;
        }
      }
    }
  }

  // Validate mesh data if present
  if (chunk->mesh.vertices && chunk->mesh.vertex_count > 0) {
    if (!chunk->mesh.indices && chunk->mesh.index_count > 0) {
      result.is_valid = false;
      result.error_count++;
      strcat(result.error_messages,
             "Mesh has indices count but no index buffer; ");
    }
  }

  if (!result.is_valid) {
    g_chunk_stats.validation_errors++;
  }

  return result;
}

// Chunk repair implementation
bool chunk_repair(Chunk *chunk, ChunkValidationResult *validation) {
  if (!chunk || !validation)
    return false;

  bool repaired = false;

  // Repair invalid lighting
  if (validation->has_invalid_lighting) {
    if (chunk->skylight) {
      for (u32 i = 0; i < CHUNK_SIZE_TOTAL; i++) {
        if (chunk->skylight[i] > 15) {
          chunk->skylight[i] = 15; // Clamp to max
          repaired = true;
        }
      }
    }

    if (chunk->blocklight) {
      for (u32 i = 0; i < CHUNK_SIZE_TOTAL; i++) {
        if (chunk->blocklight[i] > 15) {
          chunk->blocklight[i] = 15; // Clamp to max
          repaired = true;
        }
      }
    }
  }

  // Repair corrupted blocks (set to air)
  if (validation->has_corrupted_blocks && chunk->blocks) {
    for (u32 i = 0; i < CHUNK_SIZE_TOTAL; i++) {
      if (chunk->blocks[i] > 1000) { // Invalid block ID
        chunk->blocks[i] = BLOCK_AIR;
        repaired = true;
      }
    }
    chunk_mark_mesh_dirty(chunk);
  }

  // Repair invalid state
  if (chunk->state < CHUNK_STATE_UNLOADED || chunk->state > CHUNK_STATE_READY) {
    if (chunk->blocks) {
      chunk->state = CHUNK_STATE_LOADED;
    } else {
      chunk->state = CHUNK_STATE_UNLOADED;
    }
    repaired = true;
  }

  if (repaired) {
    g_chunk_stats.validation_repairs++;
    // Re-validate after repair
    *validation = chunk_validate(chunk);
  }

  return repaired;
}

// Statistics implementation
void chunk_manager_get_statistics(ChunkManager *manager,
                                  ChunkStatistics *out_stats) {
  if (!out_stats)
    return;
  *out_stats = g_chunk_stats;

  // Add manager-specific stats
  if (manager) {
    out_stats->total_chunks_loaded = manager->count;
  }
}

void chunk_manager_reset_statistics(ChunkManager *manager) {
  (void)manager; // Unused parameter
  memset(&g_chunk_stats, 0, sizeof(ChunkStatistics));
}

// Helper function to update statistics on chunk operations
static void chunk_stats_on_serialize(f32 time_ms) {
  g_chunk_stats.total_serializations++;
  g_chunk_stats.avg_serialization_time_ms =
      (g_chunk_stats.avg_serialization_time_ms *
           (g_chunk_stats.total_serializations - 1) +
       time_ms) /
      g_chunk_stats.total_serializations;
  if (time_ms > g_chunk_stats.max_serialization_time_ms) {
    g_chunk_stats.max_serialization_time_ms = time_ms;
  }
}

static void chunk_stats_on_deserialize(f32 time_ms) {
  g_chunk_stats.total_deserializations++;
  g_chunk_stats.avg_deserialization_time_ms =
      (g_chunk_stats.avg_deserialization_time_ms *
           (g_chunk_stats.total_deserializations - 1) +
       time_ms) /
      g_chunk_stats.total_deserializations;
  if (time_ms > g_chunk_stats.max_deserialization_time_ms) {
    g_chunk_stats.max_deserialization_time_ms = time_ms;
  }
}

// Priority queue helper - insert request in priority order
static void chunk_queue_insert(ChunkPriorityQueue *queue,
                               ChunkLoadRequest request) {
  if (!queue || queue->count >= queue->capacity)
    return;

  // Find insertion point (lower priority number = higher priority)
  u32 insert_index = queue->count;
  for (u32 i = 0; i < queue->count; i++) {
    if (request.priority < queue->requests[i].priority ||
        (request.priority == queue->requests[i].priority &&
         request.distance_to_player < queue->requests[i].distance_to_player)) {
      insert_index = i;
      break;
    }
  }

  // Shift elements
  for (u32 i = queue->count; i > insert_index; i--) {
    queue->requests[i] = queue->requests[i - 1];
  }

  queue->requests[insert_index] = request;
  queue->count++;
}

// Streaming and priority queue implementation
void chunk_manager_init_streaming(ChunkManager *manager, f32 streaming_radius,
                                  f32 preload_radius) {
  if (!manager)
    return;

  manager->load_queue.count = 0;
  manager->load_queue.capacity = MAX_CHUNK_QUEUE_SIZE;

  manager->streaming.streaming_radius = streaming_radius;
  manager->streaming.preload_radius = preload_radius;
  manager->streaming.max_chunks_per_frame = 2; // Load 2 chunks per frame max
  manager->streaming.enabled = true;
  manager->streaming.last_player_position = (Vec3){0.0f, 0.0f, 0.0f};
  manager->streaming.player_velocity = (Vec3){0.0f, 0.0f, 0.0f};

  manager->preload_positions = NULL;
  manager->preload_count = 0;
  manager->preload_capacity = 0;
}

void chunk_manager_update_streaming(ChunkManager *manager, Vec3 player_pos,
                                    Vec3 player_velocity, f32 delta_time) {
  if (!manager || !manager->streaming.enabled)
    return;

  // Update player velocity for prediction
  Vec3 position_delta =
      vec3_sub(player_pos, manager->streaming.last_player_position);
  if (delta_time > 0.0f) {
    manager->streaming.player_velocity =
        vec3_mul(position_delta, 1.0f / delta_time);
  }
  manager->streaming.last_player_position = player_pos;

  // Calculate which chunks need to be loaded
  ChunkPos player_chunk = world_to_chunk_pos(
      (i32)player_pos.x, (i32)player_pos.y, (i32)player_pos.z);

  i32 streaming_radius_chunks =
      (i32)(manager->streaming.streaming_radius / CHUNK_SIZE) + 1;
  i32 preload_radius_chunks =
      (i32)(manager->streaming.preload_radius / CHUNK_SIZE) + 1;

  // Queue chunks in priority order
  for (i32 dx = -preload_radius_chunks; dx <= preload_radius_chunks; dx++) {
    for (i32 dz = -preload_radius_chunks; dz <= preload_radius_chunks; dz++) {
      for (i32 dy = -2; dy <= 2; dy++) { // Limit vertical range
        ChunkPos pos = chunk_pos(player_chunk.x + dx, player_chunk.y + dy,
                                 player_chunk.z + dz);

        // Check if chunk already loaded
        Chunk *existing = chunk_manager_get(manager, pos);
        if (existing && existing->state >= CHUNK_STATE_LOADED)
          continue;

        // Calculate distance
        i32 wx, wy, wz;
        chunk_to_world_pos(pos, &wx, &wy, &wz);
        Vec3 chunk_center = {(f32)wx + CHUNK_SIZE * 0.5f,
                             (f32)wy + CHUNK_SIZE * 0.5f,
                             (f32)wz + CHUNK_SIZE * 0.5f};
        f32 distance = vec3_distance(player_pos, chunk_center);

        // Determine priority
        ChunkPriority priority;
        if (distance < manager->streaming.streaming_radius * 0.5f) {
          priority = CHUNK_PRIORITY_CRITICAL;
        } else if (distance < manager->streaming.streaming_radius) {
          priority = CHUNK_PRIORITY_HIGH;
        } else if (distance < manager->streaming.preload_radius) {
          priority = CHUNK_PRIORITY_MEDIUM;
        } else {
          priority = CHUNK_PRIORITY_LOW;
        }

        // Queue for loading
        ChunkLoadRequest request = {
            .pos = pos,
            .priority = priority,
            .distance_to_player = distance,
            .request_time = manager->access_counter++,
            .is_preload = (distance >= manager->streaming.streaming_radius)};

        chunk_queue_insert(&manager->load_queue, request);
      }
    }
  }

  // Process load queue
  chunk_manager_process_load_queue(manager,
                                   manager->streaming.max_chunks_per_frame);
}

void chunk_manager_queue_chunk_load(ChunkManager *manager, ChunkPos pos,
                                    ChunkPriority priority, f32 distance) {
  if (!manager)
    return;

  // Check if already in queue
  for (u32 i = 0; i < manager->load_queue.count; i++) {
    if (chunk_pos_equal(manager->load_queue.requests[i].pos, pos))
      return; // Already queued
  }

  ChunkLoadRequest request = {.pos = pos,
                              .priority = priority,
                              .distance_to_player = distance,
                              .request_time = manager->access_counter++,
                              .is_preload = false};

  chunk_queue_insert(&manager->load_queue, request);
}

void chunk_manager_process_load_queue(ChunkManager *manager,
                                      u32 max_chunks_per_frame) {
  if (!manager)
    return;

  u32 processed = 0;
  while (processed < max_chunks_per_frame && manager->load_queue.count > 0) {
    // Get highest priority request (first in queue)
    ChunkLoadRequest request = manager->load_queue.requests[0];

    // Remove from queue
    for (u32 i = 0; i < manager->load_queue.count - 1; i++) {
      manager->load_queue.requests[i] = manager->load_queue.requests[i + 1];
    }
    manager->load_queue.count--;

    // Check if chunk already exists
    Chunk *existing = chunk_manager_get(manager, request.pos);
    if (existing && existing->state >= CHUNK_STATE_LOADED) {
      continue; // Already loaded
    }

    // Create or get chunk
    Chunk *chunk = chunk_manager_get_or_create(manager, request.pos);
    if (chunk && chunk->state == CHUNK_STATE_LOADING) {
      // Chunk will be generated asynchronously
      processed++;
    }
  }
}

ChunkPriority chunk_manager_calculate_priority(ChunkManager *manager,
                                               ChunkPos pos, Vec3 player_pos) {
  if (!manager)
    return CHUNK_PRIORITY_LOW;

  i32 wx, wy, wz;
  chunk_to_world_pos(pos, &wx, &wy, &wz);
  Vec3 chunk_center = {(f32)wx + CHUNK_SIZE * 0.5f, (f32)wy + CHUNK_SIZE * 0.5f,
                       (f32)wz + CHUNK_SIZE * 0.5f};
  f32 distance = vec3_distance(player_pos, chunk_center);

  if (distance < manager->streaming.streaming_radius * 0.5f) {
    return CHUNK_PRIORITY_CRITICAL;
  } else if (distance < manager->streaming.streaming_radius) {
    return CHUNK_PRIORITY_HIGH;
  } else if (distance < manager->streaming.preload_radius) {
    return CHUNK_PRIORITY_MEDIUM;
  }
  return CHUNK_PRIORITY_LOW;
}

void chunk_manager_preload_chunks(ChunkManager *manager, Vec3 player_pos,
                                  Vec3 player_velocity) {
  if (!manager || !manager->streaming.enabled)
    return;

  // Predict player position in next few seconds
  f32 prediction_time = 2.0f; // Predict 2 seconds ahead
  Vec3 predicted_pos =
      vec3_add(player_pos, vec3_mul(player_velocity, prediction_time));

  ChunkPos predicted_chunk = world_to_chunk_pos(
      (i32)predicted_pos.x, (i32)predicted_pos.y, (i32)predicted_pos.z);

  // Preload chunks around predicted position
  i32 preload_radius = (i32)(manager->streaming.preload_radius / CHUNK_SIZE);
  for (i32 dx = -preload_radius; dx <= preload_radius; dx++) {
    for (i32 dz = -preload_radius; dz <= preload_radius; dz++) {
      ChunkPos pos = chunk_pos(predicted_chunk.x + dx, predicted_chunk.y,
                               predicted_chunk.z + dz);

      // Check if already loaded
      Chunk *existing = chunk_manager_get(manager, pos);
      if (existing && existing->state >= CHUNK_STATE_LOADED)
        continue;

      // Calculate distance and priority
      i32 wx, wy, wz;
      chunk_to_world_pos(pos, &wx, &wy, &wz);
      Vec3 chunk_center = {(f32)wx + CHUNK_SIZE * 0.5f,
                           (f32)wy + CHUNK_SIZE * 0.5f,
                           (f32)wz + CHUNK_SIZE * 0.5f};
      f32 distance = vec3_distance(predicted_pos, chunk_center);

      ChunkPriority priority =
          chunk_manager_calculate_priority(manager, pos, predicted_pos);

      // Queue for preloading
      ChunkLoadRequest request = {.pos = pos,
                                  .priority = priority,
                                  .distance_to_player = distance,
                                  .request_time = manager->access_counter++,
                                  .is_preload = true};

      chunk_queue_insert(&manager->load_queue, request);
    }
  }
}

// Disabled: requires crafting/brewing.h
// void chunk_manager_register_brewing_stand(ChunkManager *manager,
//                                           BrewingStand *stand) {
//   if (!manager || !stand)
//     return;
//
//   if (!manager->brewing_stands) {
//     manager->brewing_stand_capacity = 4;
//     manager->brewing_stands = (BrewingStand **)calloc(
//         manager->brewing_stand_capacity, sizeof(BrewingStand *));
//     manager->brewing_stand_count = 0;
//   }
//
//   if (manager->brewing_stand_count >= manager->brewing_stand_capacity) {
//     manager->brewing_stand_capacity *= 2;
//     manager->brewing_stands = (BrewingStand **)realloc(
//         manager->brewing_stands,
//         manager->brewing_stand_capacity * sizeof(BrewingStand *));
//   }
//
//   manager->brewing_stands[manager->brewing_stand_count++] = stand;
// }

// Disabled: requires crafting/brewing.h
// void chunk_manager_unregister_brewing_stand(ChunkManager *manager,
//                                             BrewingStand *stand) {
//   if (!manager || !stand || !manager->brewing_stands)
//     return;
//
//   u32 write = 0;
//   for (u32 i = 0; i < manager->brewing_stand_count; i++) {
//     if (manager->brewing_stands[i] == stand)
//       continue; // drop
//     manager->brewing_stands[write++] = manager->brewing_stands[i];
//   }
//   manager->brewing_stand_count = write;
// }

void chunk_manager_update(ChunkManager *manager, f32 delta_time) {
  // Disabled: brewing stands require crafting/brewing.h
  // if (manager->brewing_stands) {
  //   for (u32 i = 0; i < manager->brewing_stand_count; i++) {
  //     BrewingStand *stand = manager->brewing_stands[i];
  //     brewing_stand_update(stand, delta_time, NULL, NULL);
  //   }
  // }
  (void)manager;
  (void)delta_time;
}

void chunk_manager_render(ChunkManager *manager, void *renderer, Mat4 view,
                          Mat4 proj) {
  if (!manager || !renderer)
    return;

  IRenderer *r = (IRenderer *)renderer;

  for (u32 i = 0; i < manager->capacity; i++) {
    Chunk *chunk = &manager->chunks[i];
    if (chunk->state != CHUNK_STATE_READY)
      continue;

    // Use the renderer to draw the chunk
    r->render_chunk(r, chunk, view, proj);
  }
}
