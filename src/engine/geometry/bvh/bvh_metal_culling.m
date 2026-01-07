/*
 * bvh_metal_culling.m
 * Metal compute culling integration
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#import "bvh_metal_culling.h"
#import <Metal/Metal.h>
#import <simd/simd.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================
 */

struct bvh_metal_context {
  id<MTLDevice> device;
  id<MTLCommandQueue> command_queue;
  id<MTLComputePipelineState> cull_pipeline;
  id<MTLBuffer> object_buffer;
  id<MTLBuffer> uniforms_buffer;
  uint32_t object_capacity;
  uint32_t object_count;
  bvh_metal_stats_t stats;
};

// Matches struct in shader
struct CullingUniforms {
  vector_float4 frustum_planes[6];
  uint32_t node_count;
  uint32_t max_prims;
  uint32_t root_node;
  uint32_t padding;
};

/* ============================================================================
 * API
 * ============================================================================
 */

bvh_metal_context_t *bvh_metal_init(void *metal_device) {
  if (!metal_device)
    return NULL;

  bvh_metal_context_t *ctx =
      (bvh_metal_context_t *)calloc(1, sizeof(bvh_metal_context_t));
  ctx->device = (__bridge id<MTLDevice>)metal_device;
  ctx->command_queue = [ctx->device newCommandQueue];
  ctx->object_capacity = 4096;
  ctx->object_count = 0;

  // Load compute shader
  NSError *error = nil;
  id<MTLLibrary> library = [ctx->device newDefaultLibrary];
  if (!library) {
    // Just for fallback if default library isn't found
    // In a real engine, this would load from a specific path
    // For now, return NULL or handle failure
    free(ctx);
    return NULL;
  }

  id<MTLFunction> cull_func = [library newFunctionWithName:@"cull_objects"];
  if (!cull_func) {
    free(ctx);
    return NULL;
  }

  ctx->cull_pipeline =
      [ctx->device newComputePipelineStateWithFunction:cull_func error:&error];
  if (!ctx->cull_pipeline) {
    // Log error description
    free(ctx);
    return NULL;
  }

  // Create initial object buffer
  ctx->object_buffer =
      [ctx->device newBufferWithLength:ctx->object_capacity *
                                       64 // Approx size of ObjectInstance
                               options:MTLResourceStorageModeShared];

  // Create uniforms buffer
  ctx->uniforms_buffer =
      [ctx->device newBufferWithLength:sizeof(struct CullingUniforms)
                               options:MTLResourceStorageModeShared];

  return ctx;
}

void bvh_metal_shutdown(bvh_metal_context_t *ctx) {
  if (!ctx)
    return;

  // ARC handles ObjC object release
  free(ctx);
}

void bvh_metal_upload_objects(bvh_metal_context_t *ctx, const void *data,
                              uint32_t count) {
  if (!ctx || !data || count == 0)
    return;

  // Resize if needed
  if (count > ctx->object_capacity) {
    ctx->object_capacity = count + 1024;
    ctx->object_buffer =
        [ctx->device newBufferWithLength:ctx->object_capacity * 64
                                 options:MTLResourceStorageModeShared];
  }

  void *ptr = [ctx->object_buffer contents];
  // Assuming data is already in correct Metal layout (ObjectInstance)
  memcpy(ptr, data, count * 64);

  ctx->object_count = count;
#if defined(MAC_OS_VERSION_11_0) &&                                            \
    MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_VERSION_11_0
  [ctx->object_buffer didModifyRange:NSMakeRange(0, count * 64)];
#endif
}

void bvh_metal_dispatch_cull(bvh_metal_context_t *ctx,
                             const vec4_t frustum_planes[6],
                             void *out_visible_count_buffer_handle,
                             void *out_indirect_command_buffer_handle) {
  if (!ctx || !ctx->cull_pipeline)
    return;

  id<MTLBuffer> visible_count =
      (__bridge id<MTLBuffer>)out_visible_count_buffer_handle;
  id<MTLBuffer> indirect_cmds =
      (__bridge id<MTLBuffer>)out_indirect_command_buffer_handle;

  if (!visible_count || !indirect_cmds)
    return;

  // Update uniforms
  struct CullingUniforms *uniforms =
      (struct CullingUniforms *)[ctx->uniforms_buffer contents];
  for (int i = 0; i < 6; i++) {
    uniforms->frustum_planes[i] =
        (vector_float4){frustum_planes[i].x, frustum_planes[i].y,
                        frustum_planes[i].z, frustum_planes[i].w};
  }
  uniforms->node_count = ctx->object_count;
  uniforms->max_prims = ctx->object_count;

#if defined(MAC_OS_VERSION_11_0) &&                                            \
    MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_VERSION_11_0
  [ctx->uniforms_buffer
      didModifyRange:NSMakeRange(0, sizeof(struct CullingUniforms))];
#endif

  id<MTLCommandBuffer> cmd_buffer = [ctx->command_queue commandBuffer];
  id<MTLComputeCommandEncoder> encoder = [cmd_buffer computeCommandEncoder];

  [encoder setComputePipelineState:ctx->cull_pipeline];
  [encoder setBuffer:ctx->object_buffer offset:0 atIndex:0];
  [encoder setBuffer:indirect_cmds offset:0 atIndex:1];
  [encoder setBuffer:visible_count offset:0 atIndex:2];
  [encoder setBuffer:ctx->uniforms_buffer offset:0 atIndex:3];

  NSUInteger threadGroupSize =
      MIN(ctx->cull_pipeline.maxTotalThreadsPerThreadgroup, 256);
  MTLSize threadsPerThreadgroup = MTLSizeMake(threadGroupSize, 1, 1);
  MTLSize threadsPerGrid = MTLSizeMake(ctx->object_count, 1, 1);

  [encoder dispatchThreads:threadsPerGrid
      threadsPerThreadgroup:threadsPerThreadgroup];
  [encoder endEncoding];
  [cmd_buffer commit];

  // Update stats (approximate CPU side)
  ctx->stats.objects_processed += ctx->object_count;
}

void bvh_metal_get_stats(bvh_metal_context_t *ctx,
                         bvh_metal_stats_t *out_stats) {
  if (!ctx || !out_stats)
    return;
  *out_stats = ctx->stats;
}
