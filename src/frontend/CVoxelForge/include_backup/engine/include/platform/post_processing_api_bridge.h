// Post Processing API Bridge
// Exposes advanced post-processing effects to VoxelForgeStudio

#ifndef POST_PROCESSING_API_BRIDGE_H
#define POST_PROCESSING_API_BRIDGE_H

#include "../common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Post Processing API
// ============================================================================

/// Enable/disable motion blur
void postfx_set_motion_blur_enabled(bool enabled);

/// Check if motion blur is enabled
bool postfx_is_motion_blur_enabled(void);

/// Set motion blur strength (0.0 - 1.0)
void postfx_set_motion_blur_strength(float strength);

/// Get motion blur strength
float postfx_get_motion_blur_strength(void);

/// Enable/disable chromatic aberration
void postfx_set_chromatic_aberration_enabled(bool enabled);

/// Check if chromatic aberration is enabled
bool postfx_is_chromatic_aberration_enabled(void);

/// Set chromatic aberration intensity (0.0 - 1.0)
void postfx_set_chromatic_aberration_intensity(float intensity);

/// Get chromatic aberration intensity
float postfx_get_chromatic_aberration_intensity(void);

/// Enable/disable film grain
void postfx_set_film_grain_enabled(bool enabled);

/// Check if film grain is enabled
bool postfx_is_film_grain_enabled(void);

/// Set film grain intensity (0.0 - 1.0)
void postfx_set_film_grain_intensity(float intensity);

/// Get film grain intensity
float postfx_get_film_grain_intensity(void);

/// Enable/disable vignette
void postfx_set_vignette_enabled(bool enabled);

/// Check if vignette is enabled
bool postfx_is_vignette_enabled(void);

/// Set vignette intensity (0.0 - 1.0)
void postfx_set_vignette_intensity(float intensity);

/// Get vignette intensity
float postfx_get_vignette_intensity(void);

#ifdef __cplusplus
}
#endif

#endif // POST_PROCESSING_API_BRIDGE_H
