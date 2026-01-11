// Environment/Water System
#ifndef WATER_SYSTEM_H
#define WATER_SYSTEM_H

#include "include/common.h"
#include "include/math/vec2.h"
#include "include/math/vec3.h"
#import <Metal/Metal.h>

// FFT Ocean Wave Simulation
#define FFT_SIZE 256
#define CASCADE_COUNT 3

typedef struct {
  f32 amplitude;
  f32 wind_speed;
  Vec2 wind_direction;
  f32 choppiness;
  f32 wave_scale;
} WaveCascadeSettings;

typedef struct {
  WaveCascadeSettings cascades[CASCADE_COUNT];
  f32 foam_strength;
  f32 foam_threshold;
  Vec3 deep_color;
  Vec3 shallow_color;
  f32 scattering;
} WaterSettings;

typedef struct {
  // Water body definition
  Vec3 position;
  f32 width;
  f32 length;
  f32 water_level;
  bool is_infinite_ocean;

  // Physics
  f32 density;
  f32 drag;

  WaterSettings settings;

  // GPU resources
  id<MTLTexture> displacement_map;
  id<MTLTexture> normal_map;
  id<MTLTexture> foam_map;

  // FFT compute logic
  id<MTLComputePipelineState> spectrum_pipeline;
  id<MTLComputePipelineState> fft_pipeline;
  id<MTLComputePipelineState> assemble_pipeline;

  // Rendering
  id<MTLRenderPipelineState> render_pipeline;
  id<MTLBuffer> vertex_grid;
  u32 vertex_count;

} WaterBody;

typedef struct {
  WaterBody *bodies;
  u32 body_count;
  u32 max_bodies;

  id<MTLDevice> device;

} WaterSystem;

#ifdef __cplusplus
extern "C" {
#endif

WaterSystem *water_system_create(id<MTLDevice> device);
void water_system_destroy(WaterSystem *system);

WaterBody *water_create_ocean(WaterSystem *system);
WaterBody *water_create_lake(WaterSystem *system, Vec3 position, f32 width,
                             f32 length);
WaterBody *water_create_river(WaterSystem *system, Vec3 *points, u32 count,
                              f32 width);

void water_update(WaterSystem *system, id<MTLCommandBuffer> cmd,
                  f32 delta_time);
void water_render(WaterSystem *system, id<MTLRenderCommandEncoder> encoder,
                  const void *view_proj);

// Buoyancy / Physics Info
f32 water_get_height_at(WaterSystem *system, Vec3 position);
Vec3 water_get_velocity_at(WaterSystem *system, Vec3 position);
bool water_query_submersion(WaterSystem *system, Vec3 position, f32 *depth);

#ifdef __cplusplus
}
#endif

#endif // WATER_SYSTEM_H
