// Bounds Visualization API Bridge
// Exposes bounding box/sphere visualization to VoxelForgeStudio

#ifndef BOUNDS_VISUALIZATION_API_BRIDGE_H
#define BOUNDS_VISUALIZATION_API_BRIDGE_H

#include "include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Bounds Visualization API
// ============================================================================

/// Enable/disable AABB (Axis Aligned Bounding Box) visualization
/// @param entity_id Entity to modify
/// @param enabled Enabled state
void bounds_vis_set_aabb_enabled(uint64_t entity_id, bool enabled);

/// Enable/disable OBB (Oriented Bounding Box) visualization
void bounds_vis_set_obb_enabled(uint64_t entity_id, bool enabled);

/// Enable/disable Bounding Sphere visualization
void bounds_vis_set_sphere_enabled(uint64_t entity_id, bool enabled);

/// Check AABB enabled status
bool bounds_vis_get_aabb_enabled(uint64_t entity_id);

/// Check OBB enabled status
bool bounds_vis_get_obb_enabled(uint64_t entity_id);

/// Check Sphere enabled status
bool bounds_vis_get_sphere_enabled(uint64_t entity_id);

/// Set global visualization color for bounds
void bounds_vis_set_color(float r, float g, float b, float a);

/// Set global bounds lines width
void bounds_vis_set_width(float width);

#ifdef __cplusplus
}
#endif

#endif // BOUNDS_VISUALIZATION_API_BRIDGE_H
