#include "platform/weather_vfx_api_bridge.h"
#include "core/logger.h"
#include "renderer/systems/weather_system.h"

// Delegates to the actual Weather System

void weather_set_type(PlatformWeatherType type) { weather_sys_set_type((WeatherType)type); }

PlatformWeatherType weather_get_type(void) { return (PlatformWeatherType)weather_sys_get_type(); }

void weather_set_rain_intensity(float intensity) {
  weather_sys_set_rain_intensity(intensity);
}

float weather_get_rain_intensity(void) {
  return weather_sys_get_rain_intensity();
}

void weather_set_snow_intensity(float intensity) {
  weather_sys_set_snow_intensity(intensity);
}

float weather_get_snow_intensity(void) {
  return weather_sys_get_snow_intensity();
}

void weather_trigger_lightning(float x, float y, float z) {
  weather_sys_trigger_lightning(x, y, z);
}

void weather_set_wind_strength(float strength) {
  weather_sys_set_wind_strength(strength);
}

float weather_get_wind_strength(void) {
  return weather_sys_get_wind_strength();
}

void weather_set_fog_density(float density) {
  weather_sys_set_fog_density(density);
}

float weather_get_fog_density(void) { return weather_sys_get_fog_density(); }
