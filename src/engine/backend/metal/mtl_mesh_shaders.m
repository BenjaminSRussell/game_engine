// mtl_mesh_shaders.m
// Metal mesh shader support implementation
#include "../../include/core/logger.h"
#include "../../include/rendering/metal_mesh_shaders.h"
#import <Metal/Metal.h>

struct MetalMeshPipeline {
  id<MTLRenderPipelineState> pipeline_state;
  id<MTLFunction> object_function;
  id<MTLFunction> mesh_function;
  id<MTLFunction> fragment_function;
  MetalMeshShaderConfig config;
};

bool metal_mesh_shaders_supported(id<MTLDevice> device) {
  if (@available(macOS 13.0, iOS 16.0, *)) {
    return [device supportsFamily:MTLGPUFamilyApple7];
  }
  return false;
}

MetalMeshPipeline *
metal_create_mesh_pipeline(id<MTLDevice> device, id<MTLFunction> object_shader,
                           id<MTLFunction> mesh_shader,
                           id<MTLFunction> fragment_shader,
                           const MetalMeshShaderConfig *config) {
  if (@available(macOS 13.0, *)) {
    if (!metal_mesh_shaders_supported(device)) {
      LOG_ERROR("Mesh shaders not supported on this device");
      return NULL;
    }

    MetalMeshPipeline *pipeline =
        (MetalMeshPipeline *)calloc(1, sizeof(MetalMeshPipeline));
    pipeline->object_function = object_shader;
    pipeline->mesh_function = mesh_shader;
    pipeline->fragment_function = fragment_shader;
    pipeline->config = *config;

    // Create mesh render pipeline descriptor
    MTLMeshRenderPipelineDescriptor *desc =
        [[MTLMeshRenderPipelineDescriptor alloc] init];

    if (config->enable_amplification_shader && object_shader) {
      desc.objectFunction = object_shader;
    }
    desc.meshFunction = mesh_shader;
    desc.fragmentFunction = fragment_shader;

    // Color attachment
    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

    // Mesh shader specific settings
    desc.maxTotalThreadsPerObjectThreadgroup =
        config->max_meshlets_per_threadgroup;
    desc.maxTotalThreadsPerMeshThreadgroup = config->max_vertices_per_meshlet;

    NSError *error = nil;
    pipeline->pipeline_state =
        [device newRenderPipelineStateWithMeshDescriptor:desc
                                                 options:MTLPipelineOptionNone
                                              reflection:nil
                                                   error:&error];

    if (!pipeline->pipeline_state) {
      LOG_ERROR("Failed to create mesh shader pipeline: %s",
                [[error localizedDescription] UTF8String]);
      free(pipeline);
      return NULL;
    }

    LOG_INFO("Created mesh shader pipeline (meshlets per TG: %u, verts per "
             "meshlet: %u)",
             config->max_meshlets_per_threadgroup,
             config->max_vertices_per_meshlet);

    return pipeline;
  }

  LOG_ERROR("Mesh shaders require macOS 13.0+");
  return NULL;
}

void metal_draw_mesh_shaders(id<MTLRenderCommandEncoder> encoder,
                             MetalMeshPipeline *pipeline, u32 meshlet_count,
                             u32 threadgroups_per_grid) {
  if (!encoder || !pipeline)
    return;

  if (@available(macOS 13.0, *)) {
    [encoder setRenderPipelineState:pipeline->pipeline_state];

    // Dispatch mesh shader threadgroups
    MTLSize gridSize = MTLSizeMake(threadgroups_per_grid, 1, 1);
    MTLSize threadsPerGroup =
        MTLSizeMake(pipeline->config.max_meshlets_per_threadgroup, 1, 1);

    [encoder drawMeshThreadgroups:gridSize
        threadsPerObjectThreadgroup:threadsPerGroup
          threadsPerMeshThreadgroup:threadsPerGroup];
  }
}

void metal_destroy_mesh_pipeline(MetalMeshPipeline *pipeline) {
  if (!pipeline)
    return;

  pipeline->pipeline_state = nil;
  pipeline->object_function = nil;
  pipeline->mesh_function = nil;
  pipeline->fragment_function = nil;
  free(pipeline);
}
