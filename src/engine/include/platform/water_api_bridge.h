// Water System API Bridge
// Exposes water rendering to VoxelForgeStudio

#ifndef WATER_API_BRIDGE_H
#define WATER_API_BRIDGE_H

#include "include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Water System API
// ============================================================================

/// Enable/disable water rendering
void water_set_enabled(bool enabled);

/// Check if water is enabled
bool water_is_enabled(void);

/// Set water quality (0 = Low, 1 = Medium, 2 = High)
void water_set_quality(uint32_t quality);

/// Get water quality
uint32_t water_get_quality(void);

/// Set wave amplitude
void water_set_wave_amplitude(float amplitude);

/// Get wave amplitude
float water_get_wave_amplitude(void);

/// Set wave frequency
void water_set_wave_frequency(float frequency);

/// Get wave frequency
float water_get_wave_frequency(void);

/// Enable/disable reflections
void water_set_reflections_enabled(bool enabled);

/// Check if reflections are enabled
bool water_get_reflections_enabled(void);

/// Enable/disable refractions
void water_set_refractions_enabled(bool enabled);

/// Check if refractions are enabled
bool water_get_refractions_enabled(void);

#ifdef __cplusplus
}
#endif

#endif // WATER_API_BRIDGE_H
