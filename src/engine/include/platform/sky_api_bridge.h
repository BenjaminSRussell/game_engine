// Sky & Atmosphere API Bridge
// Exposes atmospheric rendering to VoxelForgeStudio

#ifndef SKY_API_BRIDGE_H
#define SKY_API_BRIDGE_H

#include "../common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Sky & Atmosphere API
// ============================================================================

/// Set sun direction (normalized)
void sky_set_sun_direction(float x, float y, float z);

/// Set time of day (0-24 hours)
void sky_set_time_of_day(float hours);

/// Get time of day
float sky_get_time_of_day(void);

/// Set atmospheric scattering intensity
void sky_set_scattering_intensity(float intensity);

/// Get scattering intensity
float sky_get_scattering_intensity(void);

/// Set cloud density
void sky_set_cloud_density(float density);

/// Get cloud density
float sky_get_cloud_density(void);

/// Enable/disable procedural clouds
void sky_set_clouds_enabled(bool enabled);

/// Check if clouds are enabled
bool sky_get_clouds_enabled(void);

#ifdef __cplusplus
}
#endif

#endif // SKY_API_BRIDGE_H
