// Volumetric rendering system
#ifndef VOLUMETRIC_RENDERING_H
#define VOLUMETRIC_RENDERING_H

#include "include/common.h"
#include "math/vec3.h"
#import <Metal/Metal.h>

// Volumetric fog
typedef struct {
  f32 density;
  f32 height_falloff;
  f32 max_opacity;
  Vec3 scattering_color;
  f32 anisotropy; // -1 to 1 (back scatter to forward scatter)

  bool directional_scattering;
  bool height_fog;
  bool distance_fog;

  f32 start_distance;
  f32 fog_cutoff_distance;

} VolumetricFogSettings;

// Volumetric clouds
typedef struct {
  f32 coverage;
  f32 density;
  f32 detail_scale;
  Vec3 wind_direction;
  f32 wind_speed;

  f32 cloud_altitude;
  f32 cloud_thickness;

  f32 absorption;
  f32 scattering;

  id<MTLTexture> weather_map;
  id<MTLTexture> noise_3d;
  id<MTLTexture> detail_noise;

} VolumetricCloudSettings;

typedef struct {
  id<MTLDevice> device;

  VolumetricFogSettings fog_settings;
  VolumetricCloudSettings cloud_settings;

  // Froxel grid (frustum voxels)
  id<MTLTexture> froxel_texture;
  u32 froxel_width, froxel_height, froxel_depth;

  // Pipelines
  id<MTLComputePipelineState> fog_inject_pipeline;
  id<MTLComputePipelineState> fog_scatter_pipeline;
  id<MTLComputePipelineState> cloud_raymarch_pipeline;

  id<MTLBuffer> uniform_buffer;

} VolumetricSystem;

#ifdef __cplusplus
extern "C" {
#endif

VolumetricSystem *volumetric_create(id<MTLDevice> device, u32 width,
                                    u32 height);
void volumetric_destroy(VolumetricSystem *system);

void volumetric_set_fog(VolumetricSystem *system,
                        const VolumetricFogSettings *settings);
void volumetric_set_clouds(VolumetricSystem *system,
                           const VolumetricCloudSettings *settings);

void volumetric_render_fog(VolumetricSystem *system, id<MTLCommandBuffer> cmd,
                           id<MTLTexture> depth, id<MTLTexture> output);

void volumetric_render_clouds(VolumetricSystem *system,
                              id<MTLCommandBuffer> cmd, id<MTLTexture> output);

#ifdef __cplusplus
}
#endif

#endif
