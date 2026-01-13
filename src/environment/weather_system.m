// weather_system.m - Implementation
#include "include/environment/weather_system.h"
#include "include/core/logger.h"
#import <Metal/Metal.h>
#include <stdlib.h>

WeatherSystem *weather_system_create(id<MTLDevice> device) {
  WeatherSystem *sys = calloc(1, sizeof(WeatherSystem));
  sys->device = device;
  sys->max_particles = 10000;
  sys->particle_buffer =
      [device newBufferWithLength:sys->max_particles * sizeof(Vec3) * 2
                          options:MTLResourceStorageModeShared];
  sys->current_state.time_of_day = 12.0f;
  LOG_INFO("Weather system created");
  return sys;
}

void weather_set_type(WeatherSystem *system, WeatherType type,
                      f32 transition_time) {
  system->transition_speed = 1.0f / transition_time;
  switch (type) {
  case WEATHER_RAINY:
    system->target_state.precipitation_intensity = 0.8f;
    system->target_state.cloud_coverage = 0.9f;
    break;
  case WEATHER_CLEAR:
    system->target_state.precipitation_intensity = 0.0f;
    system->target_state.cloud_coverage = 0.2f;
    break;
  default:
    break;
  }
}

void weather_update(WeatherSystem *system, id<MTLCommandBuffer> cmd,
                    f32 delta_time) {
  system->current_state.time_of_day += delta_time / 3600.0f;
  if (system->current_state.time_of_day >= 24.0f)
    system->current_state.time_of_day -= 24.0f;

  // Lerp to target
  system->transition_factor += delta_time * system->transition_speed;
  if (system->transition_factor > 1.0f)
    system->transition_factor = 1.0f;
}

Vec3 weather_get_sun_direction(WeatherSystem *system) {
  f32 hour_angle = (system->current_state.time_of_day - 12.0f) * M_PI / 12.0f;
  return vec3(sinf(hour_angle), -cosf(hour_angle), 0.2f);
}
