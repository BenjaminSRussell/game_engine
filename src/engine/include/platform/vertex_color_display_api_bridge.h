// Vertex Color Display API Bridge
// Exposes vertex color visualization to VoxelForgeStudio

#ifndef VERTEX_COLOR_DISPLAY_API_BRIDGE_H
#define VERTEX_COLOR_DISPLAY_API_BRIDGE_H

#include <common.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Vertex Color Display API
// ============================================================================

/// Enable/disable vertex color visualization
/// @param entity_id Entity to modify
/// @param enabled Enabled state
void vcol_display_set_enabled(uint64_t entity_id, bool enabled);

/// Check if vertex color display is enabled
bool vcol_display_is_enabled(uint64_t entity_id);

/// Set vertex color channel mode
/// 0: RGB, 1: Red, 2: Green, 3: Blue, 4: Alpha
void vcol_display_set_channel(uint64_t entity_id, uint32_t channel_mode);

/// Get current vertex color channel mode
uint32_t vcol_display_get_channel(uint64_t entity_id);

#ifdef __cplusplus
}
#endif

#endif // VERTEX_COLOR_DISPLAY_API_BRIDGE_H
