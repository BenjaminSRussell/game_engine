// Weather VFX API Bridge
// Exposes weather effects to VoxelForgeStudio

#ifndef WEATHER_VFX_API_BRIDGE_H
#define WEATHER_VFX_API_BRIDGE_H

#include "include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Weather VFX API
// ============================================================================

typedef enum {
  PLATFORM_WEATHER_CLEAR = 0,
  PLATFORM_WEATHER_RAIN = 1,
  PLATFORM_WEATHER_SNOW = 2,
  PLATFORM_WEATHER_STORM = 3,
  PLATFORM_WEATHER_FOG = 4
} PlatformWeatherType;

/// Set current weather type
/// Set current weather type
void weather_set_type(PlatformWeatherType type);

/// Get current weather type
PlatformWeatherType weather_get_type(void);

/// Set rain intensity (0.0 - 1.0)
void weather_set_rain_intensity(float intensity);

/// Get rain intensity
float weather_get_rain_intensity(void);

/// Set snow intensity (0.0 - 1.0)
void weather_set_snow_intensity(float intensity);

/// Get snow intensity
float weather_get_snow_intensity(void);

/// Trigger lightning strike at position
void weather_trigger_lightning(float x, float y, float z);

/// Set wind strength (0.0 - 1.0)
void weather_set_wind_strength(float strength);

/// Get wind strength
float weather_get_wind_strength(void);

/// Set fog density (0.0 - 1.0)
void weather_set_fog_density(float density);

/// Get fog density
float weather_get_fog_density(void);

#ifdef __cplusplus
}
#endif

#endif // WEATHER_VFX_API_BRIDGE_H
