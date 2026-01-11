// geometry/nanite/nanite_render.c
// #import "geometry/nanite/nanite_render.h"

// TODO: Implement BVH-based cluster culling using GPU compute (pre-pass)
// TODO: Add support for HW-accelerated raytracing on Nanite meshes (fallback
// paths)
// TODO: Implement Level-of-Detail (LOD) selection based on pixel error and
// distance
// TODO: Add support for Cluster-based shadow mapping and virtual shadow maps
// (VSM)
// TODO: Implement a robust cluster streaming system with prioritized
// decompression
// TODO: Add support for Programmable Rasterization vs Fixed-Function paths
// TODO: Implement Spatio-Temporal Cluster visibility tracking for better
// culling
// TODO: Add support for Nanite-integrated displacement and tessellation
// TODO: Implement a virtual geometry cache for high-frequency foliage and
// crowds
// TODO: Research integration with Lumen for hardware-accelerated GI reflections
// TODO: Add support for mesh painting and runtime cluster modifications
// TODO: Implement a comprehensive Nanite debugger (Cluster visualization, LOD
// levels)
// TODO: Research and implement cluster-based occlusion culling using HZB
// (Hierarchical Z-Buffer)
// TODO: Add support for vertex-colored clusters and multi-material assignment
// TODO: Implement a GPU-driven mesh pipeline using Mesh Shaders (NV/Metal 3
// support)
// TODO: Add logic for LOD-aware material blending and texture streaming
// integration
// TODO: Research and implement cluster-based light-linking and shadow-masking
// TODO: Implement a robust cluster hierarchy builder (offline tool extension)
// TODO: Add support for Nanite-integrated decals and volumetric effects
// TODO: Research and implement cluster-based motion-vector generation for
// DLSS/FSR Complete Nanite rendering pipeline
#include "include/core/logger.h"
#include "include/geometry/nanite/nanite_cluster.h"
#import <Metal/Metal.h>
#include <simd/simd.h>
#include <stdlib.h>
#include <string.h>

// GPU culling results
typedef struct {
  u32 visible_cluster_count;
  u32 *visible_cluster_indices;
  id<MTLBuffer> visible_buffer;
} NaniteCullingResult;

// Rendering system
typedef struct {
  id<MTLDevice> device;
  id<MTLComputePipelineState> culling_pipeline;
  id<MTLRenderPipelineState> render_pipeline;

  // Culling buffers
  id<MTLBuffer> cluster_bounds_buffer;
  id<MTLBuffer> visible_clusters_buffer;
  id<MTLBuffer> indirect_args_buffer;

} NaniteRenderer;

NaniteRenderer *nanite_renderer_create(id<MTLDevice> device,
                                       id<MTLLibrary> shader_lib) {
  NaniteRenderer *renderer =
      (NaniteRenderer *)calloc(1, sizeof(NaniteRenderer));
  renderer->device = device;

  @autoreleasepool {
    NSError *error = nil;

    // Create culling compute pipeline
    id<MTLFunction> culling_function =
        [shader_lib newFunctionWithName:@"nanite_cull_clusters"];
    if (culling_function) {
      renderer->culling_pipeline =
          [device newComputePipelineStateWithFunction:culling_function
                                                error:&error];
      if (!renderer->culling_pipeline) {
        LOG_ERROR("Failed to create Nanite culling pipeline: %s",
                  [[error localizedDescription] UTF8String]);
      }
    }

    // Create rendering pipeline
    MTLRenderPipelineDescriptor *renderDesc =
        [[MTLRenderPipelineDescriptor alloc] init];
    renderDesc.vertexFunction =
        [shader_lib newFunctionWithName:@"nanite_vertex"];
    renderDesc.fragmentFunction =
        [shader_lib newFunctionWithName:@"nanite_fragment"];
    renderDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    renderDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

    renderer->render_pipeline =
        [device newRenderPipelineStateWithDescriptor:renderDesc error:&error];
    if (!renderer->render_pipeline) {
      LOG_ERROR("Failed to create Nanite render pipeline: %s",
                [[error localizedDescription] UTF8String]);
    }

    // Allocate culling buffers
    renderer->visible_clusters_buffer =
        [device newBufferWithLength:sizeof(u32) * 100000
                            options:MTLResourceStorageModeShared];
    renderer->indirect_args_buffer =
        [device newBufferWithLength:sizeof(u32) * 5
                            options:MTLResourceStorageModeShared];
  }

  LOG_INFO("Nanite renderer created");
  return renderer;
}

void nanite_render_mesh(NaniteRenderer *renderer,
                        id<MTLRenderCommandEncoder> encoder, NaniteMesh *mesh,
                        const Mat4 *view_proj, const Vec3 *camera_pos) {
  if (!renderer || !encoder || !mesh)
    return;

  @autoreleasepool {
    // Set render pipeline
    [encoder setRenderPipelineState:renderer->render_pipeline];

    // Bind mesh buffers
    [encoder setVertexBuffer:(__bridge id<MTLBuffer>)mesh->gpu_vertex_buffer
                      offset:0
                     atIndex:0];
    [encoder setVertexBuffer:(__bridge id<MTLBuffer>)mesh->gpu_cluster_buffer
                      offset:0
                     atIndex:1];

    // Bind view-projection matrix
    [encoder setVertexBytes:view_proj length:sizeof(Mat4) atIndex:2];

    // Draw visible clusters
    // In full implementation, this would use indirect drawing from GPU culling
    // results
    for (u32 lod = 0; lod < mesh->lod_count; lod++) {
      u32 cluster_start = mesh->lod_offsets[lod];
      u32 cluster_count = (lod < mesh->lod_count - 1)
                              ? (mesh->lod_offsets[lod + 1] - cluster_start)
                              : (mesh->cluster_count - cluster_start);

      for (u32 i = 0; i < cluster_count; i++) {
        NaniteCluster *cluster = &mesh->clusters[cluster_start + i];

        // Simple frustum cull on CPU (GPU compute would be better)
        // TODO: Use GPU culling compute shader

        [encoder setVertexBytes:&cluster->vertex_offset
                         length:sizeof(u32)
                        atIndex:3];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                    vertexStart:0
                    vertexCount:cluster->triangle_count * 3];
      }
    }
  }
}

void nanite_gpu_cull(NaniteRenderer *renderer, id<MTLCommandBuffer> cmd,
                     NaniteMesh *mesh, const Mat4 *view_proj,
                     const Vec3 *camera_pos) {
  if (!renderer || !cmd || !mesh || !renderer->culling_pipeline)
    return;

  @autoreleasepool {
    id<MTLComputeCommandEncoder> compute = [cmd computeCommandEncoder];
    [compute setComputePipelineState:renderer->culling_pipeline];

    // Bind cluster data
    [compute setBuffer:(__bridge id<MTLBuffer>)mesh->gpu_cluster_buffer
                offset:0
               atIndex:0];
    [compute setBuffer:renderer->visible_clusters_buffer offset:0 atIndex:1];
    [compute setBuffer:renderer->indirect_args_buffer offset:0 atIndex:2];

    // Bind culling parameters
    [compute setBytes:view_proj length:sizeof(Mat4) atIndex:3];
    [compute setBytes:camera_pos length:sizeof(Vec3) atIndex:4];

    u32 cluster_count = mesh->cluster_count;
    [compute setBytes:&cluster_count length:sizeof(u32) atIndex:5];

    // Dispatch culling
    MTLSize gridSize = MTLSizeMake((cluster_count + 63) / 64, 1, 1);
    MTLSize threadgroupSize = MTLSizeMake(64, 1, 1);

    [compute dispatchThreadgroups:gridSize
            threadsPerThreadgroup:threadgroupSize];
    [compute endEncoding];
  }
}

void nanite_renderer_destroy(NaniteRenderer *renderer) {
  if (!renderer)
    return;

  renderer->culling_pipeline = nil;
  renderer->render_pipeline = nil;
  renderer->cluster_bounds_buffer = nil;
  renderer->visible_clusters_buffer = nil;
  renderer->indirect_args_buffer = nil;

  free(renderer);
}
