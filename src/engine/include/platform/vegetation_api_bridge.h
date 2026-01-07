// Vegetation System API Bridge
// Exposes vegetation rendering to VoxelForgeStudio

#ifndef VEGETATION_API_BRIDGE_H
#define VEGETATION_API_BRIDGE_H

#include "include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Vegetation System API
// ============================================================================

/// Enable/disable vegetation rendering
void vegetation_set_enabled(bool enabled);

/// Check if vegetation is enabled
bool vegetation_is_enabled(void);

/// Set vegetation density (0.0 - 1.0)
void vegetation_set_density(float density);

/// Get vegetation density
float vegetation_get_density(void);

/// Set wind strength (0.0 - 1.0)
void vegetation_set_wind_strength(float strength);

/// Get wind strength
float vegetation_get_wind_strength(void);

/// Set LOD bias (-1.0 to 1.0, negative = more detail)
void vegetation_set_lod_bias(float bias);

/// Get LOD bias
float vegetation_get_lod_bias(void);

#ifdef __cplusplus
}
#endif

#endif // VEGETATION_API_BRIDGE_H
