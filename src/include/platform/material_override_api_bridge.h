// Material Override API Bridge
// Exposes scene material override system to VoxelForgeStudio

#ifndef MATERIAL_OVERRIDE_API_BRIDGE_H
#define MATERIAL_OVERRIDE_API_BRIDGE_H

#include "include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Material Override API
// ============================================================================

typedef enum {
  MATERIAL_OVERRIDE_NONE = 0,
  MATERIAL_OVERRIDE_WIREFRAME = 1,
  MATERIAL_OVERRIDE_CLAY = 2,
  MATERIAL_OVERRIDE_NORMAL = 3,
  MATERIAL_OVERRIDE_ALBEDO = 4,
  MATERIAL_OVERRIDE_ROUGHNESS = 5,
  MATERIAL_OVERRIDE_METALLIC = 6
} MaterialOverrideMode;

/// Set the global material override mode
void material_override_set_mode(MaterialOverrideMode mode);

/// Get the current override mode
MaterialOverrideMode material_override_get_mode(void);

/// Set wireframe line width
void material_override_set_wireframe_width(float width);

/// Set wireframe color
void material_override_set_wireframe_color(float r, float g, float b, float a);

#ifdef __cplusplus
}
#endif

#endif // MATERIAL_OVERRIDE_API_BRIDGE_H
