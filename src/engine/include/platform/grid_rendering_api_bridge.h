// Grid Rendering API Bridge
// Exposes editor grid visualization to VoxelForgeStudio

#ifndef GRID_RENDERING_API_BRIDGE_H
#define GRID_RENDERING_API_BRIDGE_H

#include <common.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Grid Rendering API
// ============================================================================

/// Enable/disable grid rendering
void grid_set_enabled(bool enabled);

/// Check if grid is enabled
bool grid_is_enabled(void);

/// Set grid size (extent)
void grid_set_size(float size);

/// Get grid size
float grid_get_size(void);

/// Set grid spacing (distance between lines)
void grid_set_spacing(float spacing);

/// Get grid spacing
float grid_get_spacing(void);

/// Set primary line color (RGBA)
void grid_set_primary_color(float r, float g, float b, float a);

/// Set secondary line color (RGBA)
void grid_set_secondary_color(float r, float g, float b, float a);

/// Start a frame with grid rendering (internal use mostly, but good for bridge)
void grid_render(void *view_matrix, void *projection_matrix);

#ifdef __cplusplus
}
#endif

#endif // GRID_RENDERING_API_BRIDGE_H
