// materials/decal_system.h
// Deferred decal projection system
#ifndef DECAL_SYSTEM_H
#define DECAL_SYSTEM_H

#include "include/common.h"
#include "include/math/mat4.h"
#include "include/math/vec3.h"
#import <Metal/Metal.h>

#define MAX_DECALS 2048

typedef enum {
  DECAL_BLEND_TRANSLUCENT,
  DECAL_BLEND_STAIN,
  DECAL_BLEND_NORMAL,
  DECAL_BLEND_EMISSIVE,
  DECAL_BLEND_VOLUMETRIC
} DecalBlendMode;

typedef struct {
  Mat4 transform;         // World space transform
  Mat4 inverse_transform; // For projection

  Vec3 size; // Box extents
  f32 fade_distance;

  id<MTLTexture> albedo_texture;
  id<MTLTexture> normal_texture;
  id<MTLTexture> material_texture; // R=metallic, G=roughness, B=AO

  Vec4 tint_color;
  f32 opacity;
  f32 normal_strength;

  DecalBlendMode blend_mode;
  bool receive_gbuffer_normal;
  u32 render_order; // Sort priority

  bool active;
} Decal;

typedef struct {
  Decal decals[MAX_DECALS];
  u32 decal_count;

  id<MTLDevice> device;
  id<MTLRenderPipelineState> pipeline_state;
  id<MTLDepthStencilState> depth_stencil;
  id<MTLBuffer> decal_buffer;

} DecalSystem;

#ifdef __cplusplus
extern "C" {
#endif

// System
DecalSystem *decal_system_create(id<MTLDevice> device);
void decal_system_destroy(DecalSystem *system);

// Decal management
u32 decal_add(DecalSystem *system, const Vec3 *position, const Vec3 *size,
              f32 rotation);
void decal_remove(DecalSystem *system, u32 decal_id);
void decal_set_textures(DecalSystem *system, u32 decal_id,
                        id<MTLTexture> albedo, id<MTLTexture> normal,
                        id<MTLTexture> material);
void decal_set_blend_mode(DecalSystem *system, u32 decal_id,
                          DecalBlendMode mode);

// Rendering (deferred)
void decal_render(DecalSystem *system, id<MTLRenderCommandEncoder> encoder,
                  id<MTLTexture> gbuffer_depth, id<MTLTexture> gbuffer_normal,
                  const Mat4 *view_proj);

#ifdef __cplusplus
}
#endif

#endif // DECAL_SYSTEM_H
