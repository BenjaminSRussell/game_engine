// mtl_integration.m
// Complete Metal integration layer tying all systems together
#include "../../include/core/logger.h"
#include "../../include/rendering/metal_integration.h"
#import <Metal/Metal.h>
#import <MetalFX/MetalFX.h>
#include <stdlib.h>

MetalIntegratedRenderer *metal_integrated_create(id<MTLDevice> device,
                                                 u32 output_width,
                                                 u32 output_height,
                                                 MetalFXQuality quality) {
  MetalIntegratedRenderer *renderer =
      (MetalIntegratedRenderer *)calloc(1, sizeof(MetalIntegratedRenderer));
  renderer->output_width = output_width;
  renderer->output_height = output_height;
  renderer->quality_preset = quality;

  // Calculate render resolution based on quality
  metalfx_get_render_resolution(output_width, output_height, quality,
                                &renderer->render_width,
                                &renderer->render_height);

  // Create MetalFX upscaler
  if (metalfx_is_supported(device)) {
    MetalFXConfig config = {.input_width = renderer->render_width,
                            .input_height = renderer->render_height,
                            .output_width = output_width,
                            .output_height = output_height,
                            .mode = METALFX_MODE_TEMPORAL,
                            .quality = quality,
                            .enable_auto_exposure = true,
                            .sharpness = 0.7f};

    renderer->upscaler = metalfx_create_temporal_upscaler(device, &config);
    renderer->enable_metalfx = (renderer->upscaler != NULL);
  }

  // Create Lumen GI system
  renderer->lumen =
      lumen_create(device, output_width / 4, output_height / 4, 100);

  // Create VRS rate map
  if (metal_vrs_is_supported(device)) {
    renderer->vrs_map = metal_vrs_create_rate_map(
        device, renderer->render_width, renderer->render_height);
    renderer->enable_vrs = (renderer->vrs_map != NULL);
  }

  // Create render textures
  MTLTextureDescriptor *desc = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                   width:renderer->render_width
                                  height:renderer->render_height
                               mipmapped:NO];
  desc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
  renderer->color_buffer = [device newTextureWithDescriptor:desc];

  desc.pixelFormat = MTLPixelFormatDepth32Float;
  renderer->depth_buffer = [device newTextureWithDescriptor:desc];

  desc.pixelFormat = MTLPixelFormatRG16Float;
  renderer->motion_vectors = [device newTextureWithDescriptor:desc];

  desc.pixelFormat = MTLPixelFormatRGBA16Float;
  renderer->normals = [device newTextureWithDescriptor:desc];
  renderer->albedo = [device newTextureWithDescriptor:desc];
  renderer->gi_output = [device newTextureWithDescriptor:desc];

  LOG_INFO("Metal integrated renderer created: %ux%u -> %ux%u (quality: %d)",
           renderer->render_width, renderer->render_height, output_width,
           output_height, quality);

  return renderer;
}

void metal_integrated_render_frame(MetalIntegratedRenderer *renderer,
                                   id<MTLCommandBuffer> cmd,
                                   const Mat4 *view_proj,
                                   const Vec3 *camera_pos, NaniteMesh **meshes,
                                   u32 mesh_count,
                                   id<MTLTexture> output_texture) {
  if (!renderer || !cmd)
    return;

  // 1. Geometry pass with Nanite (render to lower resolution)
  MTLRenderPassDescriptor *geoPass =
      [MTLRenderPassDescriptor renderPassDescriptor];
  geoPass.colorAttachments[0].texture = renderer->color_buffer;
  geoPass.colorAttachments[0].loadAction = MTLLoadActionClear;
  geoPass.colorAttachments[0].clearColor =
      MTLClearColorMake(0.1, 0.1, 0.12, 1.0);
  geoPass.depthAttachment.texture = renderer->depth_buffer;
  geoPass.depthAttachment.loadAction = MTLLoadActionClear;
  geoPass.depthAttachment.clearDepth = 1.0;

  id<MTLRenderCommandEncoder> encoder =
      [cmd renderCommandEncoderWithDescriptor:geoPass];

  // Apply VRS if enabled
  if (renderer->enable_vrs && renderer->vrs_map) {
    metal_vrs_apply_to_pass(encoder, renderer->vrs_map);
  }

  // Render Nanite meshes (simplified - would use GPU culling compute first)
  for (u32 i = 0; i < mesh_count; i++) {
    if (meshes[i]) {
      // TODO: Actual Nanite rendering
      // nanite_render_mesh(encoder, meshes[i], view_proj);
    }
  }

  [encoder endEncoding];

  // 2. Lumen GI pass
  if (renderer->lumen) {
    lumen_trace_gi(renderer->lumen, cmd, renderer->depth_buffer,
                   renderer->normals, renderer->albedo, renderer->gi_output,
                   NULL);
  }

  // 3. Lighting + compositing pass
  // TODO: Combine geometry + GI

  // 4. MetalFX upscaling
  if (renderer->enable_metalfx && renderer->upscaler) {
    // Get jitter for temporal
    f32 jitter_x, jitter_y;
    metalfx_get_jitter_offset(0, &jitter_x,
                              &jitter_y); // TODO: actual frame index

    metalfx_upscale(renderer->upscaler, cmd, renderer->color_buffer,
                    renderer->depth_buffer, renderer->motion_vectors,
                    output_texture);
  } else {
    // No upscaling - just copy
    id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];
    [blit copyFromTexture:renderer->color_buffer toTexture:output_texture];
    [blit endEncoding];
  }
}

void metal_integrated_set_quality(MetalIntegratedRenderer *renderer,
                                  MetalFXQuality quality) {
  if (!renderer)
    return;

  renderer->quality_preset = quality;
  metalfx_get_render_resolution(renderer->output_width, renderer->output_height,
                                quality, &renderer->render_width,
                                &renderer->render_height);

  LOG_INFO("Quality changed to %d, new render resolution: %ux%u", quality,
           renderer->render_width, renderer->render_height);
}

void metal_integrated_destroy(MetalIntegratedRenderer *renderer) {
  if (!renderer)
    return;

  metalfx_destroy_upscaler(renderer->upscaler);
  metal_vrs_destroy_rate_map(renderer->vrs_map);
  lumen_destroy(renderer->lumen);

  renderer->color_buffer = nil;
  renderer->depth_buffer = nil;
  renderer->motion_vectors = nil;
  renderer->normals = nil;
  renderer->albedo = nil;
  renderer->gi_output = nil;

  free(renderer);
}
