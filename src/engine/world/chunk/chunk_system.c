#include "world/chunk/chunk_system.h"
#include "core/logger.h"
#include <string.h>

#define MAX_CHUNKS 1024

typedef struct {
  int32_t x, z;
  bool loaded;
  uint64_t memory_size;
} ChunkEntry;

static struct {
  float load_distance;
  float unload_distance;
  bool async_loading;
  ChunkEntry chunks[MAX_CHUNKS];
  uint32_t loaded_count;
  uint64_t total_memory;
} chunk_state;

void chunk_sys_init(void) {
  chunk_state.load_distance = 200.0f;
  chunk_state.unload_distance = 250.0f;
  chunk_state.async_loading = true;
  chunk_state.loaded_count = 0;
  chunk_state.total_memory = 0;
  memset(chunk_state.chunks, 0, sizeof(chunk_state.chunks));
  LOG_INFO("Chunk System Initialized");
}

void chunk_sys_shutdown(void) { LOG_INFO("Chunk System Shutdown"); }

void chunk_sys_update(float delta_time) {
  // Placeholder for dynamic loading/unloading based on player position
}

void chunk_sys_set_load_distance(float distance) {
  chunk_state.load_distance = distance;
}

float chunk_sys_get_load_distance(void) { return chunk_state.load_distance; }

void chunk_sys_set_unload_distance(float distance) {
  chunk_state.unload_distance = distance;
}

float chunk_sys_get_unload_distance(void) {
  return chunk_state.unload_distance;
}

static int find_chunk(int32_t x, int32_t z) {
  for (int i = 0; i < MAX_CHUNKS; i++) {
    if (chunk_state.chunks[i].loaded && chunk_state.chunks[i].x == x &&
        chunk_state.chunks[i].z == z) {
      return i;
    }
  }
  return -1;
}

void chunk_sys_force_load(int32_t chunk_x, int32_t chunk_z) {
  if (find_chunk(chunk_x, chunk_z) >= 0) {
    LOG_INFO("Chunk (%d, %d) already loaded", chunk_x, chunk_z);
    return;
  }

  for (int i = 0; i < MAX_CHUNKS; i++) {
    if (!chunk_state.chunks[i].loaded) {
      chunk_state.chunks[i].x = chunk_x;
      chunk_state.chunks[i].z = chunk_z;
      chunk_state.chunks[i].loaded = true;
      chunk_state.chunks[i].memory_size = 65536; // 64KB per chunk
      chunk_state.loaded_count++;
      chunk_state.total_memory += 65536;
      LOG_INFO("Loaded chunk (%d, %d)", chunk_x, chunk_z);
      return;
    }
  }
  LOG_WARN("Max chunks reached, cannot load (%d, %d)", chunk_x, chunk_z);
}

void chunk_sys_force_unload(int32_t chunk_x, int32_t chunk_z) {
  int idx = find_chunk(chunk_x, chunk_z);
  if (idx >= 0) {
    chunk_state.total_memory -= chunk_state.chunks[idx].memory_size;
    chunk_state.chunks[idx].loaded = false;
    chunk_state.loaded_count--;
    LOG_INFO("Unloaded chunk (%d, %d)", chunk_x, chunk_z);
  }
}

uint32_t chunk_sys_get_loaded_count(void) { return chunk_state.loaded_count; }

uint64_t chunk_sys_get_memory_usage(void) { return chunk_state.total_memory; }

void chunk_sys_set_async_loading_enabled(bool enabled) {
  chunk_state.async_loading = enabled;
}

bool chunk_sys_is_async_loading_enabled(void) {
  return chunk_state.async_loading;
}
