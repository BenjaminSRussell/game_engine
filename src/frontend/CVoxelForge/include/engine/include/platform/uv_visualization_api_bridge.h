// UV Visualization API Bridge
// Exposes UV visualization features to VoxelForgeStudio

#ifndef UV_VISUALIZATION_API_BRIDGE_H
#define UV_VISUALIZATION_API_BRIDGE_H

#include "../common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// UV Visualization API
// ============================================================================

/// Enable/disable UV visualization on a specific entity
/// @param entity_id The entity to modify
/// @param enabled Whether UV visualization is enabled
/// @param channel The UV channel to visualize (0 or 1 usually)
void uv_vis_set_enabled(uint64_t entity_id, bool enabled, uint32_t channel);

/// Check if UV visualization is enabled for an entity
bool uv_vis_is_enabled(uint64_t entity_id);

/// Get the visualized UV channel for an entity
uint32_t uv_vis_get_channel(uint64_t entity_id);

/// Set global UV checkerboard texture scale
/// @param scale The tiling scale (default usually 1.0 or 10.0)
void uv_vis_set_checkerboard_scale(float scale);

/// Get global UV checkerboard texture scale
float uv_vis_get_checkerboard_scale(void);

#ifdef __cplusplus
}
#endif

#endif // UV_VISUALIZATION_API_BRIDGE_H
