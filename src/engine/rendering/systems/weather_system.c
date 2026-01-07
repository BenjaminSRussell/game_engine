#include "rendering/systems/weather_system.h"
#include "core/logger.h"

static struct {
  WeatherType current_type;
  float rain_intensity;
  float snow_intensity;
  float wind_strength;
  float fog_density;
} weather_state;

void weather_sys_init(void) {
  weather_state.current_type = WEATHER_CLEAR;
  weather_state.rain_intensity = 0.0f;
  weather_state.snow_intensity = 0.0f;
  weather_state.wind_strength = 0.1f;
  weather_state.fog_density = 0.0f;
  LOG_INFO("Weather System Initialized");
}

void weather_sys_shutdown(void) { LOG_INFO("Weather System Shutdown"); }

void weather_sys_update(float delta_time) {
  // Update particle emitters based on state
  // Update skybox shader uniforms
}

void weather_sys_set_type(WeatherType type) {
  weather_state.current_type = type;
  LOG_INFO("Weather System: Type set to %d", type);
}

WeatherType weather_sys_get_type(void) { return weather_state.current_type; }

void weather_sys_set_rain_intensity(float intensity) {
  weather_state.rain_intensity = intensity;
}

float weather_sys_get_rain_intensity(void) {
  return weather_state.rain_intensity;
}

void weather_sys_set_snow_intensity(float intensity) {
  weather_state.snow_intensity = intensity;
}

float weather_sys_get_snow_intensity(void) {
  return weather_state.snow_intensity;
}

void weather_sys_set_wind_strength(float strength) {
  weather_state.wind_strength = strength;
}

float weather_sys_get_wind_strength(void) {
  return weather_state.wind_strength;
}

void weather_sys_set_fog_density(float density) {
  weather_state.fog_density = density;
}

float weather_sys_get_fog_density(void) { return weather_state.fog_density; }

void weather_sys_trigger_lightning(float x, float y, float z) {
  LOG_INFO("Weather System: Lightning triggered at (%.2f, %.2f, %.2f)", x, y,
           z);
  // Spawn lightning bolt entity/particle
  // Play thunder sound
}
