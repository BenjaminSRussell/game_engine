// Environment/Weather System
#ifndef WEATHER_SYSTEM_H
#define WEATHER_SYSTEM_H

#include <common.h>
#include "math/vec3.h"
#import <Metal/Metal.h>

typedef enum {
  WEATHER_CLEAR,
  WEATHER_CLOUDY,
  WEATHER_RAINY,
  WEATHER_STORMY,
  WEATHER_SNOWY,
  WEATHER_FOGGY
} WeatherType;

typedef struct {
  // Rain/Snow
  f32 precipitation_intensity; // 0.0 to 1.0
  f32 precipitation_wetness;   // Surface wetness accumulator
  f32 puddle_coverage;

  // Wind
  Vec3 wind_direction;
  f32 wind_speed;
  f32 wind_turbulence;

  // Lightning
  f32 lightning_frequency;
  Vec3 lightning_color;

  // Atmosphere
  f32 cloud_coverage;
  f32 fog_density;
  f32 ambient_temperature; // Celsius

  // Day/Night Cycle
  f32 time_of_day; // 0.0 to 24.0
  f32 sun_intensity;
  Vec3 sun_color;
  Vec3 moon_color;

} WeatherState;

typedef struct {
  WeatherState current_state;
  WeatherState target_state;
  f32 transition_factor;
  f32 transition_speed;

  // Particles
  id<MTLBuffer> precipitation_particles;
  u32 max_particles;

  // Rendering resources
  id<MTLTexture> wetness_mask;
  id<MTLTexture> sky_lut;
  id<MTLRenderPipelineState> sky_pipeline;
  id<MTLRenderPipelineState> precipitation_pipeline;
  id<MTLComputePipelineState> physics_update_pipeline;

  id<MTLDevice> device;

} WeatherSystem;

#ifdef __cplusplus
extern "C" {
#endif

WeatherSystem *weather_system_create(id<MTLDevice> device);
void weather_system_destroy(WeatherSystem *system);

void weather_set_type(WeatherSystem *system, WeatherType type,
                      f32 transition_time);
void weather_set_time(WeatherSystem *system, f32 time_of_day);

void weather_update(WeatherSystem *system, id<MTLCommandBuffer> cmd,
                    f32 delta_time);
void weather_render_sky(WeatherSystem *system,
                        id<MTLRenderCommandEncoder> encoder,
                        const void *view_proj);
void weather_render_effects(WeatherSystem *system,
                            id<MTLRenderCommandEncoder> encoder,
                            const void *view_proj);

// Getters for other systems
Vec3 weather_get_sun_direction(WeatherSystem *system);
Vec3 weather_get_wind_at(WeatherSystem *system, Vec3 position);
f32 weather_get_wetness(WeatherSystem *system);

#ifdef __cplusplus
}
#endif

#endif // WEATHER_SYSTEM_H
