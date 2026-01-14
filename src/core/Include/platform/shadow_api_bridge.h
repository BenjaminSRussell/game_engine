// Shadow Cascades API Bridge
// Exposes shadow configuration to VoxelForgeStudio

#ifndef SHADOW_API_BRIDGE_H
#define SHADOW_API_BRIDGE_H

#include <common.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Shadow API
// ============================================================================

/// Enable/disable shadows
void shadow_set_enabled(bool enabled);

/// Check if shadows are enabled
bool shadow_is_enabled(void);

/// Set number of cascades (1-4)
void shadow_set_cascade_count(uint32_t count);

/// Get number of cascades
uint32_t shadow_get_cascade_count(void);

/// Set shadow resolution (e.g. 1024, 2048, 4096)
void shadow_set_resolution(uint32_t resolution);

/// Get shadow resolution
uint32_t shadow_get_resolution(void);

/// Set split lambda (0.0 - 1.0, controls split distribution)
void shadow_set_split_lambda(float lambda);

/// Get split lambda
float shadow_get_split_lambda(void);

/// Set soft shadows enabled
void shadow_set_soft_shadows(bool enabled);

#ifdef __cplusplus
}
#endif

#endif // SHADOW_API_BRIDGE_H
