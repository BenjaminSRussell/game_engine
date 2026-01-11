// src/chunk/chunk_manager_optimized.c
//
// Module Overview:
// This module extends the core `ChunkManager` functionality with optimized
// strategies for dynamically managing the loaded state of game world chunks.
// Its primary focus is on memory and performance efficiency by implementing
// mechanisms to unload chunks that are outside a certain radius from a central
// point (e.g., the player) and to evict least recently used (LRU) chunks when
// memory pressure arises. It also provides an API to query for all chunks
// within a specified radial distance.
//
// Key Flows:
// 1. **Unloading Distant Chunks (`chunk_manager_unload_distant`):**
//    - Iterates through all currently loaded chunks.
//    - For each chunk, it calculates its squared distance from a given
//    `position` (typically the player's).
//    - If a chunk's distance exceeds `max_distance`, it triggers
//    `chunk_manager_unload`
//      to remove the chunk from memory.
// 2. **Querying Chunks in Radius (`chunk_manager_get_chunks_in_radius`):**
//    - Iterates through loaded chunks and checks if their center falls within a
//    specified
//      `radius` from a given `position`.
//    - Returns an array of pointers to `Chunk`s that meet this criteria, up to
//    `max_count`.
// 3. **LRU Eviction (`chunk_manager_evict_lru`):**
//    - Scans all loaded chunks to find the one with the oldest `last_accessed`
//    timestamp.
//    - Unloads the identified least recently used chunk to free up resources,
//      typically called when the chunk manager needs to make space for new
//      chunks.
//
// Invariants:
// - A valid `ChunkManager` instance must be provided and initialized.
// - `max_distance` and `radius` parameters are in world units.
// - `last_accessed` timestamps on `Chunk`s must be updated (e.g., by
// `chunk_manager_get`)
//   for accurate LRU eviction.
// - The module relies on the core `chunk_manager_unload` function from
// `src/chunk/chunk.c`
//   for the actual memory deallocation and state change.
//
// (Additional comments from the file indicating external dependencies and
// roadmaps are preserved.) Chunk manager for load/unload and generation.
// Roadmap: docs/CHUNK_MANAGER_OPTIMIZED_ROADMAP.md.
#include <chunk/chunk.h>
#include <math.h>
#include <math/vec3.h>
#include <stdlib.h>
#include <string.h>

// Unload chunks that are too far from position
void chunk_manager_unload_distant(ChunkManager *manager, Vec3 position,
                                  f32 max_distance) {
  if (!manager)
    return;

  f32 max_dist_sq = max_distance * max_distance;

  for (u32 i = 0; i < manager->capacity; i++) {
    Chunk *chunk = &manager->chunks[i];
    if (chunk->state == CHUNK_STATE_UNLOADED)
      continue;

    // Calculate chunk center position
    i32 chunk_wx = chunk->pos.x * CHUNK_SIZE + CHUNK_SIZE / 2;
    i32 chunk_wy = chunk->pos.y * CHUNK_SIZE + CHUNK_SIZE / 2;
    i32 chunk_wz = chunk->pos.z * CHUNK_SIZE + CHUNK_SIZE / 2;

    Vec3 chunk_pos = vec3((f32)chunk_wx, (f32)chunk_wy, (f32)chunk_wz);
    Vec3 diff = vec3_sub(position, chunk_pos);
    f32 dist_sq = vec3_dot(diff, diff);

    if (dist_sq > max_dist_sq) {
      // Unload chunk
      ChunkPos pos = chunk->pos;
      chunk_manager_unload(manager, pos);
    }
  }
}

// Get chunks in radius (for rendering)
u32 chunk_manager_get_chunks_in_radius(ChunkManager *manager, Vec3 position,
                                       f32 radius, Chunk **out_chunks,
                                       u32 max_count) {
  if (!manager || !out_chunks)
    return 0;

  f32 radius_sq = radius * radius;
  u32 count = 0;

  for (u32 i = 0; i < manager->capacity && count < max_count; i++) {
    Chunk *chunk = &manager->chunks[i];
    if (chunk->state != CHUNK_STATE_READY)
      continue;

    // Calculate chunk center
    i32 chunk_wx = chunk->pos.x * CHUNK_SIZE + CHUNK_SIZE / 2;
    i32 chunk_wy = chunk->pos.y * CHUNK_SIZE + CHUNK_SIZE / 2;
    i32 chunk_wz = chunk->pos.z * CHUNK_SIZE + CHUNK_SIZE / 2;

    Vec3 chunk_pos = vec3((f32)chunk_wx, (f32)chunk_wy, (f32)chunk_wz);
    Vec3 diff = vec3_sub(position, chunk_pos);
    f32 dist_sq = vec3_dot(diff, diff);

    if (dist_sq <= radius_sq) {
      out_chunks[count++] = chunk;
    }
  }

  return count;
}

// LRU eviction - unload least recently used chunk
void chunk_manager_evict_lru(ChunkManager *manager) {
  if (!manager)
    return;

  Chunk *lru_chunk = NULL;
  u64 lru_time = UINT64_MAX;

  for (u32 i = 0; i < manager->capacity; i++) {
    Chunk *chunk = &manager->chunks[i];
    if (chunk->state != CHUNK_STATE_UNLOADED &&
        chunk->last_accessed < lru_time) {
      lru_time = chunk->last_accessed;
      lru_chunk = chunk;
    }
  }

  if (lru_chunk) {
    ChunkPos pos = lru_chunk->pos;
    chunk_manager_unload(manager, pos);
  }
}
