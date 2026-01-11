// materials/decal_system.h
// Deferred decal projection system
#ifndef DECAL_SYSTEM_H
#define DECAL_SYSTEM_H

#include "include/common.h"
#include "include/math/mat4.h"
#include "include/math/vec3.h"

#ifdef __OBJC__
#import <Metal/Metal.h>
typedef id<MTLTexture> MTLTextureHandle;
typedef id<MTLDevice> MTLDeviceHandle;
typedef id<MTLRenderPipelineState> MTLRenderPipelineStateHandle;
typedef id<MTLDepthStencilState> MTLDepthStencilStateHandle;
typedef id<MTLBuffer> MTLBufferHandle;
typedef id<MTLRenderCommandEncoder> MTLRenderCommandEncoderHandle;
#else
typedef void *MTLTextureHandle;
typedef void *MTLDeviceHandle;
typedef void *MTLRenderPipelineStateHandle;
typedef void *MTLDepthStencilStateHandle;
typedef void *MTLBufferHandle;
typedef void *MTLRenderCommandEncoderHandle;
#endif

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

  MTLTextureHandle albedo_texture;
  MTLTextureHandle normal_texture;
  MTLTextureHandle material_texture; // R=metallic, G=roughness, B=AO

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

  MTLDeviceHandle device;
  MTLRenderPipelineStateHandle pipeline_state;
  MTLDepthStencilStateHandle depth_stencil;
  MTLBufferHandle decal_buffer;

} DecalSystem;

#ifdef __cplusplus
extern "C" {
#endif

// System
DecalSystem *decal_system_create(MTLDeviceHandle device);
void decal_system_destroy(DecalSystem *system);

// Decal management
u32 decal_add(DecalSystem *system, const Vec3 *position, const Vec3 *size,
              f32 rotation);
void decal_remove(DecalSystem *system, u32 decal_id);
void decal_set_textures(DecalSystem *system, u32 decal_id,
                        MTLTextureHandle albedo, MTLTextureHandle normal,
                        MTLTextureHandle material);
void decal_set_blend_mode(DecalSystem *system, u32 decal_id,
                          DecalBlendMode mode);

// Rendering (deferred)
void decal_render(DecalSystem *system, MTLRenderCommandEncoderHandle encoder,
                  MTLTextureHandle gbuffer_depth,
                  MTLTextureHandle gbuffer_normal, const Mat4 *view_proj);

#ifdef __cplusplus
}
#endif

#endif // DECAL_SYSTEM_H
