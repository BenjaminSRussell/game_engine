#ifndef LUMEN_GI_H
#define LUMEN_GI_H

#include "include/common.h"
#include "include/math/mat4.h"
#include "include/math/vec3.h"

// Voxel Clipmap Settings
#define LUMEN_CLIPMAP_LEVELS 4
#define LUMEN_VOXEL_RESOLUTION 128
#define LUMEN_MAX_LIGHTS 64

typedef struct {
  float r, g, b, a; // Radiance + Opacity
} LumenVoxel;

typedef struct {
  Vec3 center;
  float voxel_size;
  uint32_t clip_level;
  // GPU Handle for 3D Texture (RGBA16F)
  void *voxel_grid_texture;
} LumenClipmapLevel;

typedef struct {
  LumenClipmapLevel clips[LUMEN_CLIPMAP_LEVELS];

  // Radiance Cache
  void *radiance_cache_texture;

  // Settings
  float indirect_intensity;
  float sky_occlusion_strength;
  bool enable_multi_bounce;

  // Denoiser state
  void *history_buffer;

} LumenGISystem;

#ifdef __cplusplus
extern "C" {
#endif

// Lifecycle
LumenGISystem *lumen_create(void);
void lumen_destroy(LumenGISystem *sys);

// Core Update
void lumen_update(LumenGISystem *sys, Vec3 view_origin, Mat4 view_proj);

// Voxelization (Scene Injection)
void lumen_inject_mesh(LumenGISystem *sys, const Mat4 *transform,
                       const void *vertices, uint32_t vertex_count);

// Lighting
void lumen_inject_light(LumenGISystem *sys, Vec3 pos, Vec3 color,
                        float intensity, float radius);

// Rendering / Sampling
// Returns indirect lighting at world position using cone tracing
Vec3 lumen_sample_irradiance(LumenGISystem *sys, Vec3 pos, Vec3 normal);

#ifdef __cplusplus
}
#endif

#endif
