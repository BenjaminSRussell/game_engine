// Terrain Clipmap API Bridge
// Exposes terrain LOD and streaming configuration to VoxelForgeStudio

#ifndef TERRAIN_API_BRIDGE_H
#define TERRAIN_API_BRIDGE_H

#include "include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Terrain Clipmap API
// ============================================================================

/// Set terrain LOD count
void terrain_set_lod_count(uint32_t count);

/// Get terrain LOD count
uint32_t terrain_get_lod_count(void);

/// Set clipmap center position
void terrain_set_center(float x, float y, float z);

/// Set terrain tile size
void terrain_set_tile_size(float size);

/// Get terrain tile size
float terrain_get_tile_size(void);

/// Set height scale
void terrain_set_height_scale(float scale);

/// Get height scale
float terrain_get_height_scale(void);

/// Enable/disable terrain rendering
void terrain_set_enabled(bool enabled);

/// Check if terrain is enabled
bool terrain_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif // TERRAIN_API_BRIDGE_H
