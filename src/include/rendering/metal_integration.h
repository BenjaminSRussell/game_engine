// rendering/metal_integration.h
// Integration layer: ties MetalFX + RT + VRS + Nanite + Lumen together
#ifndef METAL_INTEGRATION_H
#define METAL_INTEGRATION_H

#include "../geometry/nanite/nanite_cluster.h"
#include "../lighting/lumen/lumen_gi.h"
#include "metal_fx.h"
#include "metal_mesh_shaders.h"
#include "metal_raytracing.h"
#include "metal_vrs.h"

typedef struct {
  // Metal 3 features
  MetalFXUpscaler *upscaler;
  MetalRTAccelStructure *rt_scene;
  MetalVRSImage *vrs_map;
  LumenGISystem *lumen;

  // Rendering config
  MetalFXQuality quality_preset;
  bool enable_metalfx;
  bool enable_ray_tracing;
  bool enable_vrs;
  bool enable_mesh_shaders;
  bool enable_nanite;

  // Render resolution
  u32 render_width;
  u32 render_height;
  u32 output_width;
  u32 output_height;

  // Frame resources
  id<MTLTexture> color_buffer;
  id<MTLTexture> depth_buffer;
  id<MTLTexture> motion_vectors;
  id<MTLTexture> normals;
  id<MTLTexture> albedo;
  id<MTLTexture> gi_output;

} MetalIntegratedRenderer;

#ifdef __cplusplus
extern "C" {
#endif

// Create integrated renderer with all Metal 3 features
MetalIntegratedRenderer *metal_integrated_create(id<MTLDevice> device,
                                                 u32 output_width,
                                                 u32 output_height,
                                                 MetalFXQuality quality);

// Per-frame render
void metal_integrated_render_frame(MetalIntegratedRenderer *renderer,
                                   id<MTLCommandBuffer> cmd,
                                   const Mat4 *view_proj,
                                   const Vec3 *camera_pos, NaniteMesh **meshes,
                                   u32 mesh_count,
                                   id<MTLTexture> output_texture);

// Update settings
void metal_integrated_set_quality(MetalIntegratedRenderer *renderer,
                                  MetalFXQuality quality);
void metal_integrated_toggle_feature(MetalIntegratedRenderer *renderer,
                                     const char *feature_name, bool enabled);

void metal_integrated_destroy(MetalIntegratedRenderer *renderer);

#ifdef __cplusplus
}
#endif

#endif // METAL_INTEGRATION_H
