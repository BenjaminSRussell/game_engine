// Chunk Management API Bridge
// Exposes chunk loading/streaming to VoxelForgeStudio

#ifndef CHUNK_API_BRIDGE_H
#define CHUNK_API_BRIDGE_H

#include <common.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Chunk Management API
// ============================================================================

/// Set chunk load distance in chunks
void chunk_set_load_distance(float distance);

/// Get chunk load distance
float chunk_get_load_distance(void);

/// Set chunk unload distance in chunks
void chunk_set_unload_distance(float distance);

/// Get chunk unload distance
float chunk_get_unload_distance(void);

/// Get number of currently loaded chunks
uint32_t chunk_get_loaded_count(void);

/// Force load a specific chunk
void chunk_force_load(int32_t x, int32_t z);

/// Force unload a specific chunk
void chunk_force_unload(int32_t x, int32_t z);

/// Get memory used by loaded chunks (bytes)
uint64_t chunk_get_memory_usage(void);

/// Set async loading enabled
void chunk_set_async_loading(bool enabled);

/// Check if async loading is enabled
bool chunk_is_async_loading(void);

#ifdef __cplusplus
}
#endif

#endif // CHUNK_API_BRIDGE_H
