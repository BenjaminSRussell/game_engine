// VFX Optimization API Bridge
// Exposes VFX optimization controls to VoxelForgeStudio

#ifndef VFX_OPTIMIZATION_API_BRIDGE_H
#define VFX_OPTIMIZATION_API_BRIDGE_H

#include <common.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// VFX Optimization API
// ============================================================================

/// Enable/disable particle culling
void vfx_opt_set_culling_enabled(bool enabled);

/// Check if culling is enabled
bool vfx_opt_is_culling_enabled(void);

/// Set culling distance
void vfx_opt_set_culling_distance(float distance);

/// Get culling distance
float vfx_opt_get_culling_distance(void);

/// Set particle LOD bias (-1.0 to 1.0)
void vfx_opt_set_lod_bias(float bias);

/// Get particle LOD bias
float vfx_opt_get_lod_bias(void);

/// Enable/disable GPU instancing for particles
void vfx_opt_set_instancing_enabled(bool enabled);

/// Check if instancing is enabled
bool vfx_opt_is_instancing_enabled(void);

/// Set maximum active particles globally
void vfx_opt_set_max_particles(uint32_t count);

/// Get maximum particle count
uint32_t vfx_opt_get_max_particles(void);

/// Get current active particle count
uint32_t vfx_opt_get_active_particles(void);

#ifdef __cplusplus
}
#endif

#endif // VFX_OPTIMIZATION_API_BRIDGE_H
