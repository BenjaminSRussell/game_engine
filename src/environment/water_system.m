// water_system.m - Implementation
#include "include/environment/water_system.h"
#include "include/core/logger.h"
#import <Metal/Metal.h>
#include <math.h>
#include <stdlib.h>

WaterSystem *water_system_create(id<MTLDevice> device) {
  WaterSystem *sys = calloc(1, sizeof(WaterSystem));
  sys->device = device;
  sys->max_bodies = 16;
  sys->bodies = calloc(sys->max_bodies, sizeof(WaterBody));
  LOG_INFO("Water system created");
  return sys;
}

void water_system_destroy(WaterSystem *system) {
  if (!system)
    return;
  free(system->bodies);
  free(system);
}

WaterBody *water_create_ocean(WaterSystem *system) {
  WaterBody *body = &system->bodies[system->body_count++];
  body->is_infinite_ocean = true;
  body->water_level = 0.0f;
  body->settings.amplitude = 2.0f;
  body->settings.wind_speed = 10.0f;
  return body;
}

void water_update(WaterSystem *system, id<MTLCommandBuffer> cmd,
                  f32 delta_time) {
  // FFT wave simulation update
  for (u32 i = 0; i < system->body_count; i++) {
    // Update wave spectrum, run FFT
  }
}

f32 water_get_height_at(WaterSystem *system, Vec3 position) {
  // Sample displacement map
  return 0.0f; // TODO
}
