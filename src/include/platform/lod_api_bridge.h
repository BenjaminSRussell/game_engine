// LOD Control API Bridge
// Exposes LOD system controls to VoxelForgeStudio

#ifndef LOD_API_BRIDGE_H
#define LOD_API_BRIDGE_H

#include "include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// LOD Control API
// ============================================================================

/// Set global LOD bias (-1.0 to 1.0, negative = more detail)
void lod_set_bias(float bias);

/// Get global LOD bias
float lod_get_bias(void);

/// Set LOD distance multiplier
void lod_set_distance_multiplier(float multiplier);

/// Get LOD distance multiplier
float lod_get_distance_multiplier(void);

/// Enable/disable LOD system
void lod_set_enabled(bool enabled);

/// Check if LOD is enabled
bool lod_is_enabled(void);

/// Force specific LOD level for debugging (-1 = auto)
void lod_set_force_level(int32_t level);

/// Get forced LOD level
int32_t lod_get_force_level(void);

#ifdef __cplusplus
}
#endif

#endif // LOD_API_BRIDGE_H
