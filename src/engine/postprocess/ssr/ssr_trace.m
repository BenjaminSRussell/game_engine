/*
 * ssr_trace.m
 * SSR System Implementation - V2 with Advanced Features
 */

#import "ssr_trace.h"
#import "../../../geometry/culling/occlusion/hzb_builder.h"
#import <Metal/Metal.h>
#import <simd/simd.h>
#include <stdio.h>
#include <stdlib.h>

static id<MTLComputePipelineState>
create_pipeline(id<MTLDevice> device, NSString *name, NSError **error) {
  id<MTLLibrary> library = [device newDefaultLibrary];
  if (!library)
    return nil;
  id<MTLFunction> function = [library newFunctionWithName:name];
  if (!function)
    return nil;
  return [device newComputePipelineStateWithFunction:function error:error];
}

int ssr_init(ssr_system_t *ssr, id<MTLDevice> device, uint32_t width,
             uint32_t height) {
  if (!ssr || !device)
    return -1;
  memset(ssr, 0, sizeof(ssr_system_t));

  ssr->quality.max_steps = 64;
  ssr->quality.stochastic_samples = 4;
  ssr->quality.bilateral_radius = 4.0f;
  ssr->quality.enable_temporal = true;
  ssr->quality.enable_stochastic = true;
  ssr->quality.enable_inpainting = true;
  ssr->quality.enable_box_projection = false;

  ssr->max_steps = 64;
  ssr->max_distance = 100.0f;
  ssr->thickness = 0.5f;
  ssr->frame_index = 0;

  MTLTextureDescriptor *desc = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                   width:width
                                  height:height
                               mipmapped:NO];
  desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
  desc.storageMode = MTLStorageModePrivate;

  ssr->ray_hit_texture = [device newTextureWithDescriptor:desc];
  ssr->ray_hit_accumulation = [device newTextureWithDescriptor:desc];
  ssr->ray_hit_inpainted = [device newTextureWithDescriptor:desc];
  ssr->history_hit = [device newTextureWithDescriptor:desc];

  ssr->reflection_texture = [device newTextureWithDescriptor:desc];
  ssr->filtered_reflection = [device newTextureWithDescriptor:desc];
  ssr->history_reflection = [device newTextureWithDescriptor:desc];

  if (!ssr->ray_hit_texture || !ssr->history_hit)
    return -2;

  NSError *error = nil;
  ssr->trace_pipeline = create_pipeline(device, @"ssr_trace", &error);
  ssr->trace_stochastic_pipeline =
      create_pipeline(device, @"ssr_trace_stochastic", &error);
  ssr->reproject_pipeline = create_pipeline(device, @"ssr_reproject", &error);
  ssr->inpaint_pipeline = create_pipeline(device, @"ssr_inpaint", &error);
  ssr->bilateral_filter_pipeline =
      create_pipeline(device, @"ssr_bilateral_filter", &error);
  ssr->resolve_pipeline = create_pipeline(device, @"ssr_resolve", &error);
  ssr->composite_pipeline = create_pipeline(device, @"ssr_composite", &error);

  if (!ssr->trace_pipeline || !ssr->resolve_pipeline)
    return -4;

  ssr->hzb_builder = malloc(sizeof(hzb_builder_t));
  hzb_builder_init((hzb_builder_t *)ssr->hzb_builder, device, width, height);

  ssr->initialized = true;
  return 0;
}

void ssr_shutdown(ssr_system_t *ssr) {
  if (!ssr)
    return;
  if (ssr->hzb_builder) {
    hzb_builder_shutdown((hzb_builder_t *)ssr->hzb_builder);
    free(ssr->hzb_builder);
  }
  // ... release other resources (ARC handles ObjC objects)
  ssr->initialized = false;
}

void ssr_resize(ssr_system_t *ssr, id<MTLDevice> device, uint32_t width,
                uint32_t height) {
  ssr_shutdown(ssr);
  ssr_init(ssr, device, width, height);
}

void ssr_set_quality(ssr_system_t *ssr, ssr_quality_settings_t quality) {
  if (ssr)
    ssr->quality = quality;
}

void ssr_set_environment(ssr_system_t *ssr, id<MTLTexture> cubemap) {
  if (ssr)
    ssr->environment_cubemap = cubemap;
}

void ssr_render(ssr_system_t *ssr, id<MTLCommandBuffer> cmd, gbuffer_t *gbuffer,
                camera_t *camera, id<MTLTexture> scene_color,
                id<MTLTexture> final_output) {
  if (!ssr || !ssr->initialized || !cmd)
    return;

  if (ssr->hzb_builder)
    hzb_builder_build((hzb_builder_t *)ssr->hzb_builder, cmd,
                      (id<MTLTexture>)gbuffer->depth);

  id<MTLTexture> hzb_tex =
      ssr->hzb_builder
          ? hzb_builder_get_texture((hzb_builder_t *)ssr->hzb_builder)
          : (id<MTLTexture>)gbuffer->depth;

  ssr_uniforms_t uniforms;
  uniforms.view_proj = camera->view_proj;
  uniforms.inv_view_proj = simd_inverse(camera->view_proj);
  uniforms.prev_view_proj = camera->prev_view_proj;
  uniforms.camera_pos = camera->position;
  uniforms.screen_size = simd_make_uint2(gbuffer->width, gbuffer->height);
  uniforms.max_steps = ssr->quality.max_steps;
  uniforms.thickness = ssr->thickness;
  uniforms.initial_step = 0.1f;
  uniforms.max_distance = ssr->max_distance;
  uniforms.roughness_threshold = 0.8f;
  uniforms.edge_fade_distance = 0.1f;
  uniforms.frame_index = ssr->frame_index;
  uniforms.temporal_blend_weight = 0.9f;
  uniforms.anisotropy = 0.5f; // Hardcoded visual test value for now

  // Stub Box Projection data
  uniforms.probe_position = camera->position;
  uniforms.probe_box_min = camera->position - 50.0f;
  uniforms.probe_box_max = camera->position + 50.0f;

  MTLSize threadgroups =
      MTLSizeMake((gbuffer->width + 7) / 8, (gbuffer->height + 7) / 8, 1);
  MTLSize threadsPerGroup = MTLSizeMake(8, 8, 1);
  id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];

  // 1. Trace
  id<MTLTexture> trace_out = ssr->ray_hit_texture;
  if (ssr->quality.enable_stochastic && ssr->trace_stochastic_pipeline) {
    [encoder setComputePipelineState:ssr->trace_stochastic_pipeline];
    [encoder setTexture:(id<MTLTexture>)gbuffer->depth atIndex:0];
    [encoder setTexture:(id<MTLTexture>)gbuffer->normal atIndex:1];
    [encoder setTexture:(id<MTLTexture>)gbuffer->material atIndex:2];
    [encoder setTexture:hzb_tex atIndex:3];
    [encoder setTexture:ssr->ray_hit_accumulation atIndex:4];
    [encoder setBytes:&uniforms length:sizeof(uniforms) atIndex:0];
    [encoder dispatchThreadgroups:threadgroups
            threadsPerThreadgroup:threadsPerGroup];
    trace_out = ssr->ray_hit_accumulation;
  } else {
    [encoder setComputePipelineState:ssr->trace_pipeline];
    // ... same bindings ...
    [encoder setTexture:(id<MTLTexture>)gbuffer->depth atIndex:0];
    [encoder setTexture:(id<MTLTexture>)gbuffer->normal atIndex:1];
    [encoder setTexture:(id<MTLTexture>)gbuffer->material atIndex:2];
    [encoder setTexture:hzb_tex atIndex:3];
    [encoder setTexture:ssr->ray_hit_texture atIndex:4];
    [encoder setBytes:&uniforms length:sizeof(uniforms) atIndex:0];
    [encoder dispatchThreadgroups:threadgroups
            threadsPerThreadgroup:threadsPerGroup];
  }

  // 2. Reproject
  id<MTLTexture> reproject_out = trace_out;
  if (ssr->quality.enable_temporal && ssr->reproject_pipeline) {
    [encoder memoryBarrierWithScope:MTLBarrierScopeTextures];
    [encoder setComputePipelineState:ssr->reproject_pipeline];
    [encoder setTexture:trace_out atIndex:0];
    [encoder setTexture:ssr->history_hit atIndex:1];
    [encoder setTexture:(id<MTLTexture>)gbuffer->velocity atIndex:2];
    [encoder setTexture:(id<MTLTexture>)gbuffer->depth atIndex:3];
    [encoder setTexture:ssr->ray_hit_accumulation atIndex:4]; // Temp
    [encoder setTexture:ssr->history_hit atIndex:5];          // Ping-pong out
    [encoder setBytes:&uniforms length:sizeof(uniforms) atIndex:0];
    [encoder dispatchThreadgroups:threadgroups
            threadsPerThreadgroup:threadsPerGroup];
    reproject_out = ssr->ray_hit_accumulation;
  }

  // 3. Inpaint (New)
  id<MTLTexture> inpaint_out = reproject_out;
  if (ssr->quality.enable_inpainting && ssr->inpaint_pipeline) {
    [encoder memoryBarrierWithScope:MTLBarrierScopeTextures];
    [encoder setComputePipelineState:ssr->inpaint_pipeline];
    [encoder setTexture:reproject_out atIndex:0];
    [encoder setTexture:ssr->ray_hit_inpainted atIndex:1];
    [encoder dispatchThreadgroups:threadgroups
            threadsPerThreadgroup:threadsPerGroup];
    inpaint_out = ssr->ray_hit_inpainted;
  }

  // 4. Resolve
  [encoder memoryBarrierWithScope:MTLBarrierScopeTextures];
  [encoder setComputePipelineState:ssr->resolve_pipeline];
  [encoder setTexture:inpaint_out atIndex:0];
  [encoder setTexture:scene_color atIndex:1];
  [encoder setTexture:ssr->history_reflection atIndex:2]; // For multi-bounce
  [encoder setTexture:ssr->reflection_texture atIndex:3];
  [encoder dispatchThreadgroups:threadgroups
          threadsPerThreadgroup:threadsPerGroup];

  // 5. Filter
  id<MTLTexture> filter_out = ssr->reflection_texture;
  if (ssr->bilateral_filter_pipeline) {
    [encoder memoryBarrierWithScope:MTLBarrierScopeTextures];
    [encoder setComputePipelineState:ssr->bilateral_filter_pipeline];
    [encoder setTexture:ssr->reflection_texture atIndex:0];
    [encoder setTexture:(id<MTLTexture>)gbuffer->depth atIndex:1];
    [encoder setTexture:(id<MTLTexture>)gbuffer->normal atIndex:2];
    [encoder setTexture:ssr->filtered_reflection atIndex:3];
    [encoder setBytes:&uniforms length:sizeof(uniforms) atIndex:0];
    [encoder dispatchThreadgroups:threadgroups
            threadsPerThreadgroup:threadsPerGroup];
    filter_out = ssr->filtered_reflection;
  }

  // 6. Composite
  [encoder memoryBarrierWithScope:MTLBarrierScopeTextures];
  [encoder setComputePipelineState:ssr->composite_pipeline];
  [encoder setTexture:filter_out atIndex:0];
  [encoder setTexture:(id<MTLTexture>)gbuffer->material atIndex:1];
  [encoder setTexture:ssr->environment_cubemap ? ssr->environment_cubemap
                                               : scene_color
              atIndex:2];
  [encoder setTexture:(id<MTLTexture>)gbuffer->normal atIndex:3];
  [encoder setTexture:scene_color atIndex:4];
  [encoder setTexture:final_output atIndex:5];
  [encoder setBytes:&uniforms length:sizeof(uniforms) atIndex:0];
  [encoder dispatchThreadgroups:threadgroups
          threadsPerThreadgroup:threadsPerGroup];

  [encoder endEncoding];
  ssr->frame_index++;
}
