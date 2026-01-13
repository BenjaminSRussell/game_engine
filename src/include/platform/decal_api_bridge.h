// Decal System API Bridge
// Exposes decal rendering to VoxelForgeStudio

#ifndef DECAL_API_BRIDGE_H
#define DECAL_API_BRIDGE_H

#include "include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Decal System API
// ============================================================================

/// Create a decal
/// @param texture_path Path to decal texture
/// @param pos_x X position
/// @param pos_y Y position
/// @param pos_z Z position
/// @param size Decal size
/// @return Decal ID
uint64_t decal_create(const char *texture_path, float pos_x, float pos_y,
                      float pos_z, float size);

/// Destroy a decal
void decal_destroy(uint64_t decal_id);

/// Set decal opacity
void decal_set_opacity(uint64_t decal_id, float opacity);

/// Set decal rotation
void decal_set_rotation(uint64_t decal_id, float angle);

/// Enable/disable decal system
void decal_set_enabled(bool enabled);

/// Check if decal system is enabled
bool decal_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif // DECAL_API_BRIDGE_H
