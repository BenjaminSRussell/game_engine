// Ray Tracing API Bridge
// Exposes ray tracing configuration to VoxelForgeStudio

#ifndef RAY_TRACING_API_BRIDGE_H
#define RAY_TRACING_API_BRIDGE_H

#include "include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Ray Tracing API
// ============================================================================

/// Enable/disable ray tracing features
void raytracing_set_enabled(bool enabled);

/// Check if ray tracing is enabled
bool raytracing_is_enabled(void);

/// Check if hardware supports ray tracing
bool raytracing_is_supported(void);

/// Set maximum ray bounce depth
void raytracing_set_max_bounces(uint32_t bounces);

/// Get maximum ray bounce depth
uint32_t raytracing_get_max_bounces(void);

/// Set samples per pixel for ray tracing
void raytracing_set_samples_per_pixel(uint32_t samples);

/// Get samples per pixel
uint32_t raytracing_get_samples_per_pixel(void);

/// Enable/disable denoising
void raytracing_set_denoising_enabled(bool enabled);

/// Check if denoising is enabled
bool raytracing_get_denoising_enabled(void);

#ifdef __cplusplus
}
#endif

#endif // RAY_TRACING_API_BRIDGE_H
