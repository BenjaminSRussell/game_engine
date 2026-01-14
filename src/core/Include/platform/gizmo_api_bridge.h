// Gizmo API Bridge
// Exposes gizmo manipulation tools to VoxelForgeStudio

#ifndef GIZMO_API_BRIDGE_H
#define GIZMO_API_BRIDGE_H

#include <common.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Gizmo System API
// ============================================================================

/// Gizmo operation mode
typedef enum {
  GIZMO_MODE_TRANSLATE = 0,
  GIZMO_MODE_ROTATE = 1,
  GIZMO_MODE_SCALE = 2,
  GIZMO_MODE_UNIVERSAL = 3 // All three in one
} GizmoMode;

/// Gizmo coordinate space
typedef enum { GIZMO_SPACE_WORLD = 0, GIZMO_SPACE_LOCAL = 1 } GizmoSpace;

/// Set current gizmo mode (translate/rotate/scale)
void gizmo_set_mode(GizmoMode mode);

/// Get current gizmo mode
GizmoMode gizmo_get_mode(void);

/// Set coordinate space (world or local)
void gizmo_set_space(GizmoSpace space);

/// Get current coordinate space
GizmoSpace gizmo_get_space(void);

/// Enable/disable gizmo rendering
void gizmo_set_enabled(bool enabled);

/// Check if gizmo is enabled
bool gizmo_is_enabled(void);

/// Enable/disable snapping
void gizmo_set_snap_enabled(bool enabled);

/// Check if snapping is enabled
bool gizmo_is_snap_enabled(void);

/// Set snap value for translation (in world units)
void gizmo_set_translation_snap(float value);

/// Get translation snap value
float gizmo_get_translation_snap(void);

/// Set snap value for rotation (in degrees)
void gizmo_set_rotation_snap(float degrees);

/// Get rotation snap value
float gizmo_get_rotation_snap(void);

/// Set snap value for scale (multiplier)
void gizmo_set_scale_snap(float value);

/// Get scale snap value
float gizmo_get_scale_snap(void);

/// Set gizmo size multiplier
void gizmo_set_size(float size);

/// Get gizmo size
float gizmo_get_size(void);

/// Set gizmo opacity
void gizmo_set_opacity(float opacity);

/// Get gizmo opacity
float gizmo_get_opacity(void);

/// Enable/disable gizmo axes individually
void gizmo_set_axis_enabled(uint32_t axis_index, bool enabled); // 0=X, 1=Y, 2=Z

/// Set custom colors for gizmo axes (RGB 0-1)
void gizmo_set_axis_color(uint32_t axis_index, float r, float g, float b);

#ifdef __cplusplus
}
#endif

#endif // GIZMO_API_BRIDGE_H
