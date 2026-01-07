/**
 * =================================================================================================
 *                          METAL RENDER PIPELINE MANAGEMENT
 * =================================================================================================
 *
 * Manages Render Pipeline State Objects (PSO) and caching.
 *
 * =================================================================================================
 */

#import <Metal/Metal.h>
#include <core/logger.h>
#include <core/types.h>
#include <rendering/renderer.h>
#include <stdint.h>

// Forward decl or include correct path
struct VertexFormat;
void *mtl_create_vertex_descriptor(const struct VertexFormat *fmt);

// Use internal backend header
#include "backend/metal/old/metal_backend.h"

// Cache for PSOs to avoid recompilation
// Using a simple static array for now, should be a hash map in production
#define MAX_CACHED_PSOS 64
static struct {
  u32 hash;
  void *pso; // id<MTLRenderPipelineState>
} s_pso_cache[MAX_CACHED_PSOS];
static int s_pso_count = 0;

void *mtl_create_render_pipeline(void *device_ptr, void *vertex_fn_ptr,
                                 void *fragment_fn_ptr,
                                 const VertexFormat *vertex_fmt,
                                 int color_format, int depth_format,
                                 bool blending) {
  id<MTLDevice> device = (__bridge id<MTLDevice>)device_ptr;
  id<MTLFunction> vertexFunction = (__bridge id<MTLFunction>)vertex_fn_ptr;
  id<MTLFunction> fragmentFunction = (__bridge id<MTLFunction>)fragment_fn_ptr;

  // Simple hash calculation for caching (replace with better hash in
  // production) We hash the pointers and formats.
  u32 hash = 0;
  hash ^= (u32)(uintptr_t)vertexFunction;
  hash ^= (u32)(uintptr_t)fragmentFunction;
  hash ^= (u32)color_format;
  hash ^= (u32)depth_format;
  hash ^= (u32)blending;

  // Check cache
  for (int i = 0; i < s_pso_count; i++) {
    if (s_pso_cache[i].hash == hash) {
      // Verify it's not a collision if we had robust keys, but for this task
      // assume hash is key Ideally we check descriptor equality, but that's
      // expensive.
      return (__bridge_retained void *)s_pso_cache[i].pso;
    }
  }

  // 1. Create Descriptor
  MTLRenderPipelineDescriptor *pipelineDescriptor =
      [[MTLRenderPipelineDescriptor alloc] init];
  pipelineDescriptor.label = @"Render Pipeline";
  pipelineDescriptor.vertexFunction = vertexFunction;
  pipelineDescriptor.fragmentFunction = fragmentFunction;

  // 2. Setup Vertex Descriptor
  if (vertex_fmt) {
    pipelineDescriptor.vertexDescriptor =
        (__bridge MTLVertexDescriptor *)mtl_create_vertex_descriptor(
            vertex_fmt);
  }

  // 3. Setup Color Attachments
  pipelineDescriptor.colorAttachments[0].pixelFormat =
      (MTLPixelFormat)color_format;

  if (blending) {
    pipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
    pipelineDescriptor.colorAttachments[0].rgbBlendOperation =
        MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].alphaBlendOperation =
        MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor =
        MTLBlendFactorSourceAlpha;
    pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor =
        MTLBlendFactorSourceAlpha;
    pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor =
        MTLBlendFactorOneMinusSourceAlpha;
    pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor =
        MTLBlendFactorOneMinusSourceAlpha;
  }

  // 4. Setup Depth Attachment
  pipelineDescriptor.depthAttachmentPixelFormat = (MTLPixelFormat)depth_format;

  // 5. Create State
  NSError *error = nil;
  id<MTLRenderPipelineState> pipelineState =
      [device newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                             error:&error];

  if (!pipelineState) {
    LOG_ERROR("Failed to create render pipeline state: %s",
              [[error localizedDescription] UTF8String]);
    return NULL;
  }

  // Cache it
  if (s_pso_count < MAX_CACHED_PSOS) {
    s_pso_cache[s_pso_count].hash = hash;
    s_pso_cache[s_pso_count].pso =
        (__bridge_retained void *)pipelineState; // Retain for cache
    s_pso_count++;
  }

  return (__bridge_retained void *)pipelineState;
}
