// Occlusion Culling API Bridge
// Exposes occlusion culling controls to VoxelForgeStudio

#ifndef OCCLUSION_API_BRIDGE_H
#define OCCLUSION_API_BRIDGE_H

#include <common.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Occlusion Culling API
// ============================================================================

typedef enum {
  OCCLUSION_NONE = 0,
  OCCLUSION_PORTAL = 1,
  OCCLUSION_HARDWARE = 2
} OcclusionMode;

/// Enable/disable occlusion culling
void occlusion_set_enabled(bool enabled);

/// Check if enabled
bool occlusion_is_enabled(void);

/// Set occlusion mode
void occlusion_set_mode(OcclusionMode mode);

/// Get occlusion mode
OcclusionMode occlusion_get_mode(void);

/// Get number of visible objects after culling
uint32_t occlusion_get_visible_count(void);

/// Get number of culled objects
uint32_t occlusion_get_culled_count(void);

/// Add entity as occluder
void occlusion_add_occluder(uint64_t entity_id);

/// Remove occluder
void occlusion_remove_occluder(uint64_t entity_id);

#ifdef __cplusplus
}
#endif

#endif // OCCLUSION_API_BRIDGE_H
