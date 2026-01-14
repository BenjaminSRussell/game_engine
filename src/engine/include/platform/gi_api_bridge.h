// Global Illumination API Bridge
// Exposes GI configuration to VoxelForgeStudio

#ifndef GI_API_BRIDGE_H
#define GI_API_BRIDGE_H

#include <common.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Global Illumination API
// ============================================================================

/// Enable/disable GI
void gi_set_enabled(bool enabled);

/// Check if GI is enabled
bool gi_is_enabled(void);

/// Set GI quality (0 = Low, 1 = Medium, 2 = High, 3 = Ultra)
void gi_set_quality(uint32_t quality);

/// Get GI quality
uint32_t gi_get_quality(void);

/// Set GI intensity (brightness multiplier)
void gi_set_intensity(float intensity);

/// Get GI intensity
float gi_get_intensity(void);

/// Set sample count
void gi_set_samples(uint32_t samples);

/// Get sample count
uint32_t gi_get_samples(void);

#ifdef __cplusplus
}
#endif

#endif // GI_API_BRIDGE_H
