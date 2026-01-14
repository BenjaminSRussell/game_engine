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

// Robust cluster hierarchy builder (offline tool extension)
typedef struct {
  u32 cluster_id;
  u32 parent_id;
  u32 *child_ids;
  u32 child_count;
  Vec3 bounds_center;
  f32 bounds_radius;
  u32 lod_level;
  f32 error_metric;
  bool is_leaf;
} ClusterHierarchyNode;

typedef struct {
  ClusterHierarchyNode *nodes;
  u32 node_count;
  u32 max_nodes;
  u32 root_node_id;
  u32 *leaf_nodes;
  u32 leaf_count;

  // Building statistics
  u32 total_clusters_processed;
  f32 build_time_seconds;
  f32 average_error;
  f32 max_error;
} ClusterHierarchyBuilder;

// Visibility buffer rendering for GPU-driven material pass
typedef struct {
  uint32_t surface_id;
  uint32_t material_id;
  uint32_t triangle_count;
  uint32_t index_offset;
  simd_float3 bounds_center;
  simd_float3 bounds_extent;
} SurfaceData;

typedef struct {
  id<MTLTexture> surface_id_texture;
  id<MTLTexture> depth_texture;
  id<MTLBuffer> material_buffer;
  id<MTLBuffer> surface_data_buffer;
  uint32_t surface_count;
  uint32_t max_surfaces;
} VisibilityBuffer;

// Rendering system structures
typedef struct NaniteMeshShaderRenderer {
  id<MTLDevice> device;
  id<MTLComputePipelineState> mesh_shader_pipeline;
  id<MTLComputePipelineState> amplification_shader_pipeline;

  // Meshlet data
  id<MTLBuffer> meshlet_buffer;
  id<MTLBuffer> meshlet_vertices_buffer;
  id<MTLBuffer> meshlet_indices_buffer;
  id<MTLBuffer> indirect_args_buffer;

  // Culling results
  id<MTLBuffer> visible_meshlets_buffer;
  u32 max_meshlets;
  bool supports_mesh_shaders;
} NaniteMeshShaderRenderer;

typedef struct NaniteRenderer {
  id<MTLDevice> device;
  id<MTLComputePipelineState> culling_pipeline;
  id<MTLRenderPipelineState> render_pipeline;
  NaniteMeshShaderRenderer *mesh_shader_renderer;

  // Culling buffers
  id<MTLBuffer> cluster_bounds_buffer;
  id<MTLBuffer> visible_clusters_buffer;
  id<MTLBuffer> indirect_args_buffer;

  VisibilityBuffer *visibility_buffer;
} NaniteRenderer;

void visibility_buffer_destroy(VisibilityBuffer *buffer);
void nanite_gpu_cull(NaniteRenderer *renderer, id<MTLCommandBuffer> cmd,
                     NaniteMesh *mesh, const Mat4 *view_proj,
                     const Vec3 *camera_pos);

ClusterHierarchyBuilder *cluster_hierarchy_builder_create(u32 max_nodes) {
  ClusterHierarchyBuilder *builder =
      (ClusterHierarchyBuilder *)calloc(1, sizeof(ClusterHierarchyBuilder));
  builder->max_nodes = max_nodes;
  builder->nodes =
      (ClusterHierarchyNode *)calloc(max_nodes, sizeof(ClusterHierarchyNode));
  builder->leaf_nodes = (u32 *)calloc(max_nodes, sizeof(u32));

  LOG_INFO("Cluster hierarchy builder created for %u nodes", max_nodes);
  return builder;
}

u32 cluster_hierarchy_add_node(ClusterHierarchyBuilder *builder, u32 parent_id,
                               const Vec3 *center, f32 radius, u32 lod_level,
                               f32 error) {
  if (!builder || builder->node_count >= builder->max_nodes)
    return UINT32_MAX;

  u32 node_id = builder->node_count++;
  ClusterHierarchyNode *node = &builder->nodes[node_id];

  node->cluster_id = node_id;
  node->parent_id = parent_id;
  node->child_ids = NULL;
  node->child_count = 0;
  node->bounds_center = *center;
  node->bounds_radius = radius;
  node->lod_level = lod_level;
  node->error_metric = error;
  node->is_leaf = true;

  // Add to parent's children if not root
  if (parent_id != UINT32_MAX && parent_id < builder->node_count) {
    ClusterHierarchyNode *parent = &builder->nodes[parent_id];
    parent->child_count++;
    parent->child_ids =
        (u32 *)realloc(parent->child_ids, parent->child_count * sizeof(u32));
    parent->child_ids[parent->child_count - 1] = node_id;
    parent->is_leaf = false;
  } else {
    builder->root_node_id = node_id;
  }

  // Track leaf nodes
  if (node->is_leaf) {
    builder->leaf_nodes[builder->leaf_count++] = node_id;
  }

  // Update statistics
  builder->total_clusters_processed++;
  builder->average_error =
      (builder->average_error * (builder->total_clusters_processed - 1) +
       error) /
      builder->total_clusters_processed;
  if (error > builder->max_error) {
    builder->max_error = error;
  }

  return node_id;
}

void cluster_hierarchy_build(ClusterHierarchyBuilder *builder,
                             NaniteMesh *mesh) {
  if (!builder || !mesh)
    return;

  f64 start_time = 0.0; // Would use high-resolution timer

  LOG_INFO("Building cluster hierarchy for %u clusters", mesh->cluster_count);

  // Phase 1: Create leaf nodes from mesh clusters
  for (u32 i = 0; i < mesh->cluster_count; i++) {
    NaniteCluster *cluster = &mesh->clusters[i];

    // Calculate cluster bounds
    Vec3 center = vec3_zero();
    f32 max_dist = 0.0f;

    // Simple bounds calculation (would be more sophisticated in production)
    for (u32 v = 0; v < cluster->vertex_count; v++) {
      // Assume vertex data is accessible - this is simplified
      // Vec3 vertex = mesh->vertices[cluster->vertex_offset + v];
      // center = vec3_add(center, vertex);
    }

    if (cluster->vertex_count > 0) {
      // center = vec3_mul(center, 1.0f / cluster->vertex_count);
    }

    // Add leaf node
    cluster_hierarchy_add_node(builder, UINT32_MAX, &center, max_dist, 0,
                               0.01f);
  }

  // Phase 2: Build internal hierarchy levels
  u32 current_level_start = 0;
  u32 current_level_count = builder->leaf_count;
  u32 level = 1;

  while (current_level_count > 1 && builder->node_count < builder->max_nodes) {
    u32 next_level_start = builder->node_count;
    u32 groups_to_process =
        (current_level_count + 3) / 4; // Group 4 nodes per parent

    for (u32 group = 0;
         group < groups_to_process && builder->node_count < builder->max_nodes;
         group++) {
      Vec3 group_center = vec3_zero();
      f32 group_radius = 0.0f;
      f32 group_error = 0.0f;
      u32 nodes_in_group = 0;

      // Calculate group bounds
      for (u32 i = 0;
           i < 4 && (group * 4 + i + current_level_start) < builder->node_count;
           i++) {
        u32 child_id = group * 4 + i + current_level_start;
        if (child_id < builder->node_count) {
          ClusterHierarchyNode *child = &builder->nodes[child_id];
          group_center = vec3_add(group_center, child->bounds_center);
          group_radius = fmaxf(group_radius, child->bounds_radius);
          group_error = fmaxf(group_error, child->error_metric);
          nodes_in_group++;
        }
      }

      if (nodes_in_group > 0) {
        group_center = vec3_mul(group_center, 1.0f / nodes_in_group);
        group_radius *= 1.5f; // Expand radius to cover children
        group_error *= 1.2f;  // Increase error for higher LODs

        // Add internal node
        u32 parent_id =
            cluster_hierarchy_add_node(builder, UINT32_MAX, &group_center,
                                       group_radius, level, group_error);

        // Link children to parent
        for (u32 i = 0; i < 4 && (group * 4 + i + current_level_start) <
                                     builder->node_count;
             i++) {
          u32 child_id = group * 4 + i + current_level_start;
          if (child_id < builder->node_count && child_id != parent_id) {
            ClusterHierarchyNode *child = &builder->nodes[child_id];
            child->parent_id = parent_id;

            ClusterHierarchyNode *parent = &builder->nodes[parent_id];
            parent->child_count++;
            parent->child_ids = (u32 *)realloc(
                parent->child_ids, parent->child_count * sizeof(u32));
            parent->child_ids[parent->child_count - 1] = child_id;
            parent->is_leaf = false;
          }
        }
      }
    }

    current_level_start = next_level_start;
    current_level_count = builder->node_count - next_level_start;
    level++;

    LOG_DEBUG("Hierarchy level %u: %u nodes", level, current_level_count);
  }

  f64 end_time = 0.0; // Would use high-resolution timer
  builder->build_time_seconds = (f32)(end_time - start_time);

  LOG_INFO("Cluster hierarchy built: %u total nodes, %u levels, %.3fs",
           builder->node_count, level, builder->build_time_seconds);
}

void cluster_hierarchy_destroy(ClusterHierarchyBuilder *builder) {
  if (!builder)
    return;

  // Clean up child arrays
  for (u32 i = 0; i < builder->node_count; i++) {
    if (builder->nodes[i].child_ids) {
      free(builder->nodes[i].child_ids);
    }
  }

  free(builder->nodes);
  free(builder->leaf_nodes);
  free(builder);
}

// Cluster-based light-linking and shadow-masking system
typedef struct {
  u32 light_id;
  Vec3 light_position;
  f32 light_radius;
  Vec3 light_color;
  f32 light_intensity;
  u32 *linked_cluster_indices;
  u32 linked_cluster_count;
  f32 *shadow_factors;
  id<MTLBuffer> light_buffer;
  id<MTLBuffer> linked_clusters_buffer;
  id<MTLBuffer> shadow_mask_buffer;
} NaniteLightLink;

typedef struct {
  NaniteLightLink *lights;
  u32 light_count;
  u32 max_lights;
  id<MTLBuffer> light_links_buffer;
  id<MTLComputePipelineState> light_linking_pipeline;
  id<MTLComputePipelineState> shadow_mask_pipeline;
} NaniteLightLinkingSystem;

NaniteLightLinkingSystem *nanite_light_linking_create(id<MTLDevice> device,
                                                      id<MTLLibrary> shader_lib,
                                                      u32 max_lights) {
  NaniteLightLinkingSystem *system =
      (NaniteLightLinkingSystem *)calloc(1, sizeof(NaniteLightLinkingSystem));
  system->max_lights = max_lights;
  system->lights =
      (NaniteLightLink *)calloc(max_lights, sizeof(NaniteLightLink));

  @autoreleasepool {
    // Create light linking compute pipeline
    id<MTLFunction> light_link_function =
        [shader_lib newFunctionWithName:@"nanite_light_linking_compute"];
    if (light_link_function) {
      NSError *error = nil;
      system->light_linking_pipeline =
          [device newComputePipelineStateWithFunction:light_link_function
                                                error:&error];
      if (!system->light_linking_pipeline) {
        LOG_ERROR("Failed to create light linking pipeline: %s",
                  [[error localizedDescription] UTF8String]);
      }
    }

    // Create shadow masking compute pipeline
    id<MTLFunction> shadow_mask_function =
        [shader_lib newFunctionWithName:@"nanite_shadow_mask_compute"];
    if (shadow_mask_function) {
      NSError *error = nil;
      system->shadow_mask_pipeline =
          [device newComputePipelineStateWithFunction:shadow_mask_function
                                                error:&error];
      if (!system->shadow_mask_pipeline) {
        LOG_ERROR("Failed to create shadow mask pipeline: %s",
                  [[error localizedDescription] UTF8String]);
      }
    }

    // Allocate GPU buffers
    system->light_links_buffer =
        [device newBufferWithLength:max_lights * sizeof(NaniteLightLink)
                            options:MTLResourceStorageModeShared];
  }

  LOG_INFO("Nanite light linking system created for %u lights", max_lights);
  return system;
}

void nanite_light_linking_update(NaniteLightLinkingSystem *system,
                                 id<MTLCommandBuffer> cmd, NaniteMesh *mesh,
                                 const Mat4 *view_proj) {
  if (!system || !cmd || !mesh)
    return;

  @autoreleasepool {
    id<MTLComputeCommandEncoder> compute = [cmd computeCommandEncoder];
    [compute setLabel:@"Nanite Light Linking Update"];

    // Update light-cluster links
    if (system->light_linking_pipeline) {
      [compute setComputePipelineState:system->light_linking_pipeline];
      [compute setBuffer:(__bridge id<MTLBuffer>)mesh->gpu_cluster_buffer
                  offset:0
                 atIndex:0];
      [compute setBuffer:system->light_links_buffer offset:0 atIndex:1];

      [compute setBytes:view_proj length:sizeof(Mat4) atIndex:2];

      u32 cluster_count = mesh->cluster_count;
      [compute setBytes:&cluster_count length:sizeof(u32) atIndex:3];

      u32 light_count = system->light_count;
      [compute setBytes:&light_count length:sizeof(u32) atIndex:4];

      MTLSize gridSize = MTLSizeMake((cluster_count + 63) / 64, 1, 1);
      MTLSize threadgroupSize = MTLSizeMake(64, 1, 1);

      [compute dispatchThreadgroups:gridSize
              threadsPerThreadgroup:threadgroupSize];
    }

    // Generate shadow masks
    if (system->shadow_mask_pipeline) {
      [compute setComputePipelineState:system->shadow_mask_pipeline];
      [compute setBuffer:system->light_links_buffer offset:0 atIndex:0];

      MTLSize gridSize = MTLSizeMake((system->light_count + 31) / 32, 1, 1);
      MTLSize threadgroupSize = MTLSizeMake(32, 1, 1);

      [compute dispatchThreadgroups:gridSize
              threadsPerThreadgroup:threadgroupSize];
    }

    [compute endEncoding];
  }
}

u32 nanite_light_linking_add_light(NaniteLightLinkingSystem *system,
                                   const Vec3 *position, f32 radius,
                                   const Vec3 *color, f32 intensity) {
  if (!system || system->light_count >= system->max_lights)
    return UINT32_MAX;

  u32 light_id = system->light_count++;
  NaniteLightLink *light = &system->lights[light_id];

  light->light_id = light_id;
  light->light_position = *position;
  light->light_radius = radius;
  light->light_color = *color;
  light->light_intensity = intensity;
  light->linked_cluster_count = 0;
  light->linked_cluster_indices = NULL;
  light->shadow_factors = NULL;

  return light_id;
}

void nanite_light_linking_destroy(NaniteLightLinkingSystem *system) {
  if (!system)
    return;

  // Clean up light data
  for (u32 i = 0; i < system->light_count; i++) {
    if (system->lights[i].linked_cluster_indices) {
      free(system->lights[i].linked_cluster_indices);
    }
    if (system->lights[i].shadow_factors) {
      free(system->lights[i].shadow_factors);
    }
  }

  free(system->lights);
  system->light_links_buffer = nil;
  system->light_linking_pipeline = nil;
  system->shadow_mask_pipeline = nil;

  free(system);
}

// GPU culling results
typedef struct {
  u32 visible_cluster_count;
  u32 *visible_cluster_indices;
  id<MTLBuffer> visible_buffer;
} NaniteCullingResult;

// GPU-driven mesh pipeline using Mesh Shaders (NV/Metal 3 support)

// Nanite Mesh Shader Renderer Implementation

// Meshlet structure for mesh shader processing
typedef struct {
  Vec3 center;
  f32 radius;
  u32 vertex_offset;
  u32 vertex_count;
  u32 index_offset;
  u32 index_count;
  u32 lod_level;
} Meshlet;

NaniteMeshShaderRenderer *nanite_mesh_shader_create(id<MTLDevice> device,
                                                    id<MTLLibrary> shader_lib) {
  NaniteMeshShaderRenderer *renderer =
      (NaniteMeshShaderRenderer *)calloc(1, sizeof(NaniteMeshShaderRenderer));
  renderer->device = device;
  renderer->max_meshlets = 100000;

  @autoreleasepool {
    // Check for mesh shader support
    if (@available(iOS 16.0, macOS 13.0, *)) {
      renderer->supports_mesh_shaders = true;

      // Create mesh shader pipeline
      id<MTLFunction> mesh_function =
          [shader_lib newFunctionWithName:@"nanite_mesh_shader"];
      if (mesh_function) {
        NSError *error = nil;
        renderer->mesh_shader_pipeline =
            [device newComputePipelineStateWithFunction:mesh_function
                                                  error:&error];
        if (!renderer->mesh_shader_pipeline) {
          LOG_ERROR("Failed to create mesh shader pipeline: %s",
                    [[error localizedDescription] UTF8String]);
        }
      }

      // Create amplification shader pipeline
      id<MTLFunction> amplification_function =
          [shader_lib newFunctionWithName:@"nanite_amplification_shader"];
      if (amplification_function) {
        NSError *error = nil;
        renderer->amplification_shader_pipeline =
            [device newComputePipelineStateWithFunction:amplification_function
                                                  error:&error];
        if (!renderer->amplification_shader_pipeline) {
          LOG_ERROR("Failed to create amplification shader pipeline: %s",
                    [[error localizedDescription] UTF8String]);
        }
      }
    } else {
      renderer->supports_mesh_shaders = false;
      LOG_WARN("Mesh shaders not supported on this device");
    }

    // Allocate meshlet buffers
    renderer->meshlet_buffer =
        [device newBufferWithLength:renderer->max_meshlets * sizeof(Meshlet)
                            options:MTLResourceStorageModeShared];
    renderer->visible_meshlets_buffer =
        [device newBufferWithLength:renderer->max_meshlets * sizeof(u32)
                            options:MTLResourceStorageModeShared];
    renderer->indirect_args_buffer = [device
        newBufferWithLength:sizeof(MTLDrawPrimitivesIndirectArguments) * 1000
                    options:MTLResourceStorageModeShared];
  }

  LOG_INFO("Nanite mesh shader renderer created (mesh shaders: %s)",
           renderer->supports_mesh_shaders ? "enabled" : "disabled");
  return renderer;
}

void nanite_mesh_shader_render(NaniteMeshShaderRenderer *renderer,
                               id<MTLCommandBuffer> cmd, NaniteMesh *mesh,
                               const Mat4 *view_proj, const Vec3 *camera_pos) {
  if (!renderer || !cmd || !mesh || !renderer->supports_mesh_shaders) {
    // Fallback to traditional rendering
    return;
  }

  @autoreleasepool {
    id<MTLComputeCommandEncoder> compute = [cmd computeCommandEncoder];
    [compute setLabel:@"Nanite Mesh Shader Rendering"];

    // First pass: Amplification shader for LOD selection and culling
    if (renderer->amplification_shader_pipeline) {
      [compute setComputePipelineState:renderer->amplification_shader_pipeline];
      [compute setBuffer:(__bridge id<MTLBuffer>)mesh->gpu_cluster_buffer
                  offset:0
                 atIndex:0];
      [compute setBuffer:renderer->meshlet_buffer offset:0 atIndex:1];
      [compute setBuffer:renderer->visible_meshlets_buffer offset:0 atIndex:2];
      [compute setBuffer:renderer->indirect_args_buffer offset:0 atIndex:3];

      [compute setBytes:view_proj length:sizeof(Mat4) atIndex:4];
      [compute setBytes:camera_pos length:sizeof(Vec3) atIndex:5];

      u32 cluster_count = mesh->cluster_count;
      [compute setBytes:&cluster_count length:sizeof(u32) atIndex:6];

      MTLSize gridSize = MTLSizeMake((cluster_count + 63) / 64, 1, 1);
      MTLSize threadgroupSize = MTLSizeMake(64, 1, 1);

      [compute dispatchThreadgroups:gridSize
              threadsPerThreadgroup:threadgroupSize];
    }

    // Second pass: Mesh shader for actual geometry generation
    if (renderer->mesh_shader_pipeline) {
      [compute setComputePipelineState:renderer->mesh_shader_pipeline];
      [compute setBuffer:renderer->visible_meshlets_buffer offset:0 atIndex:0];
      [compute setBuffer:renderer->meshlet_vertices_buffer offset:0 atIndex:1];
      [compute setBuffer:renderer->meshlet_indices_buffer offset:0 atIndex:2];

      // Dispatch based on visible meshlet count
      u32 visible_count =
          0; // This would be read back from amplification shader
      [compute setBytes:&visible_count length:sizeof(u32) atIndex:3];

      MTLSize gridSize = MTLSizeMake((visible_count + 31) / 32, 1, 1);
      MTLSize threadgroupSize = MTLSizeMake(32, 1, 1);

      [compute dispatchThreadgroups:gridSize
              threadsPerThreadgroup:threadgroupSize];
    }

    [compute endEncoding];
  }
}

void nanite_mesh_shader_destroy(NaniteMeshShaderRenderer *renderer) {
  if (!renderer)
    return;

  renderer->mesh_shader_pipeline = nil;
  renderer->amplification_shader_pipeline = nil;
  renderer->meshlet_buffer = nil;
  renderer->meshlet_vertices_buffer = nil;
  renderer->meshlet_indices_buffer = nil;
  renderer->visible_meshlets_buffer = nil;
  renderer->indirect_args_buffer = nil;

  free(renderer);
}

// Nanite Mesh Shader Renderer Implementation

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

    // Create mesh shader renderer for GPU-driven pipeline
    renderer->mesh_shader_renderer =
        nanite_mesh_shader_create(device, shader_lib);

    // Allocate culling buffers
    renderer->visible_clusters_buffer =
        [device newBufferWithLength:sizeof(u32) * 100000
                            options:MTLResourceStorageModeShared];
    renderer->indirect_args_buffer =
        [device newBufferWithLength:sizeof(u32) * 5
                            options:MTLResourceStorageModeShared];
  }

  LOG_INFO("Nanite renderer created with mesh shader support");
  return renderer;
}

void nanite_render_mesh(NaniteRenderer *renderer,
                        id<MTLRenderCommandEncoder> encoder,
                        id<MTLCommandBuffer> cmd, NaniteMesh *mesh,
                        const Mat4 *view_proj, const Vec3 *camera_pos) {
  if (!renderer || !encoder || !mesh)
    return;

  @autoreleasepool {
    // Try mesh shader path first if available
    if (renderer->mesh_shader_renderer &&
        renderer->mesh_shader_renderer->supports_mesh_shaders) {
      // Use GPU-driven mesh pipeline
      nanite_mesh_shader_render(renderer->mesh_shader_renderer, cmd, mesh,
                                view_proj, camera_pos);
      return;
    }

    // Fallback to traditional rendering
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
    for (u32 lod = 0; lod < mesh->lod_count; lod++) {
      u32 cluster_start = mesh->lod_offsets[lod];
      u32 cluster_count = (lod < mesh->lod_count - 1)
                              ? (mesh->lod_offsets[lod + 1] - cluster_start)
                              : (mesh->cluster_count - cluster_start);

      for (u32 i = 0; i < cluster_count; i++) {
        NaniteCluster *cluster = &mesh->clusters[cluster_start + i];

        // GPU culling compute shader would be better
        nanite_gpu_cull(renderer, cmd, mesh, view_proj, camera_pos);

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
    [compute setLabel:@"Nanite GPU Culling"];
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

    // Enhanced culling parameters
    struct {
      f32 lod_threshold;
      f32 distance_scale;
      u32 max_lod;
      u32 enable_frustum_culling;
      f32 near_plane;
      f32 far_plane;
      u32 enable_distance_culling;
      u32 enable_occlusion_culling;
    } culling_params = {
        .lod_threshold = 0.5f,
        .distance_scale = 1.0f,
        .max_lod = mesh->lod_count - 1,
        .enable_frustum_culling = 1,
        .near_plane = 0.1f,
        .far_plane = 1000.0f,
        .enable_distance_culling = 1,
        .enable_occlusion_culling = 0 // Would need HZB for this
    };
    [compute setBytes:&culling_params length:sizeof(culling_params) atIndex:5];

    u32 cluster_count = mesh->cluster_count;
    [compute setBytes:&cluster_count length:sizeof(u32) atIndex:6];

    // Dispatch with optimal thread group size
    MTLSize gridSize = MTLSizeMake((cluster_count + 127) / 128, 1, 1);
    MTLSize threadgroupSize = MTLSizeMake(128, 1, 1);

    [compute dispatchThreadgroups:gridSize
            threadsPerThreadgroup:threadgroupSize];
    [compute endEncoding];

    // Update statistics
    static u32 total_culled = 0;
    total_culled += cluster_count;
    LOG_DEBUG("GPU culling: %u clusters processed, %u total", cluster_count,
              total_culled);
  }
}

void nanite_renderer_destroy(NaniteRenderer *renderer) {
  if (!renderer)
    return;

  // Destroy mesh shader renderer
  if (renderer->mesh_shader_renderer) {
    nanite_mesh_shader_destroy(renderer->mesh_shader_renderer);
  }

  // Destroy visibility buffer
  visibility_buffer_destroy(renderer->visibility_buffer);

  renderer->culling_pipeline = nil;
  renderer->render_pipeline = nil;
  renderer->cluster_bounds_buffer = nil;
  renderer->visible_clusters_buffer = nil;
  renderer->indirect_args_buffer = nil;

  free(renderer);
}

// Visibility buffer rendering implementation follows...

VisibilityBuffer *visibility_buffer_create(id<MTLDevice> device, uint32_t width,
                                           uint32_t height,
                                           uint32_t max_surfaces) {
  VisibilityBuffer *buffer =
      (VisibilityBuffer *)malloc(sizeof(VisibilityBuffer));

  // Create surface ID texture (R32_UINT)
  MTLTextureDescriptor *surfaceDesc = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatR32Uint
                                   width:width
                                  height:height
                               mipmapped:NO];
  surfaceDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
  surfaceDesc.storageMode = MTLStorageModePrivate;
  buffer->surface_id_texture = [device newTextureWithDescriptor:surfaceDesc];

  // Create depth texture
  MTLTextureDescriptor *depthDesc = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                   width:width
                                  height:height
                               mipmapped:NO];
  depthDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
  depthDesc.storageMode = MTLStorageModePrivate;
  buffer->depth_texture = [device newTextureWithDescriptor:depthDesc];

  // Create material buffer
  buffer->material_buffer =
      [device newBufferWithLength:max_surfaces * sizeof(uint32_t)
                          options:MTLResourceStorageModeShared];

  // Create surface data buffer
  buffer->surface_data_buffer =
      [device newBufferWithLength:max_surfaces * sizeof(SurfaceData)
                          options:MTLResourceStorageModeShared];

  buffer->surface_count = 0;
  buffer->max_surfaces = max_surfaces;

  return buffer;
}

void visibility_buffer_destroy(VisibilityBuffer *buffer) {
  if (buffer) {
    [buffer->surface_id_texture release];
    [buffer->depth_texture release];
    [buffer->material_buffer release];
    [buffer->surface_data_buffer release];
    free(buffer);
  }
}

void nanite_render_visibility_pass(NaniteRenderer *renderer,
                                   id<MTLRenderCommandEncoder> encoder,
                                   NaniteMesh *mesh,
                                   const VisibilityBuffer *vis_buffer) {
  if (!renderer || !encoder || !mesh || !vis_buffer)
    return;

  // Set visibility buffer rendering pipeline
  [encoder setFragmentTexture:vis_buffer->surface_id_texture atIndex:0];

  // Bind surface data buffer
  [encoder setVertexBuffer:vis_buffer->surface_data_buffer offset:0 atIndex:2];

  // Render clusters to surface ID buffer
  for (uint32_t i = 0; i < mesh->cluster_count; i++) {
    NaniteCluster *cluster = &mesh->clusters[i];

    // Set surface ID for this cluster
    uint32_t surface_id = i; // Use cluster index as surface ID
    [encoder setVertexBytes:&surface_id length:sizeof(uint32_t) atIndex:3];

    // Draw cluster triangles
    [encoder setVertexBytes:&cluster->vertex_offset
                     length:sizeof(u32)
                    atIndex:3];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                vertexStart:0
                vertexCount:cluster->triangle_count * 3];
  }
}

void nanite_render_material_pass(NaniteRenderer *renderer,
                                 id<MTLRenderCommandEncoder> encoder,
                                 const VisibilityBuffer *vis_buffer,
                                 id<MTLTexture> material_textures[],
                                 uint32_t material_count) {
  if (!renderer || !encoder || !vis_buffer)
    return;

  // Full-screen quad rendering for material pass
  [encoder setFragmentTexture:vis_buffer->surface_id_texture atIndex:0];
  [encoder setFragmentTexture:vis_buffer->depth_texture atIndex:1];

  // Bind material buffer and textures
  [encoder setFragmentBuffer:vis_buffer->material_buffer offset:0 atIndex:0];
  [encoder setFragmentBuffer:vis_buffer->surface_data_buffer
                      offset:0
                     atIndex:1];

  // Bind material textures array
  for (uint32_t i = 0; i < material_count && i < 32; i++) {
    [encoder setFragmentTexture:material_textures[i] atIndex:2 + i];
  }

  // Draw full-screen quad
  [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
}

void nanite_update_visibility_buffer(NaniteRenderer *renderer,
                                     id<MTLCommandBuffer> cmd,
                                     VisibilityBuffer *vis_buffer,
                                     const uint32_t *visible_surfaces,
                                     uint32_t visible_count) {
  if (!renderer || !cmd || !vis_buffer || !visible_surfaces)
    return;

  // Update surface data for visible surfaces
  SurfaceData *surface_data =
      (SurfaceData *)[vis_buffer->surface_data_buffer contents];
  uint32_t *material_data = (uint32_t *)[vis_buffer->material_buffer contents];

  for (uint32_t i = 0; i < visible_count && i < vis_buffer->max_surfaces; i++) {
    uint32_t surface_id = visible_surfaces[i];

    // Update surface data
    surface_data[surface_id].surface_id = surface_id;
    surface_data[surface_id].material_id =
        surface_id % 8; // Example material assignment
    surface_data[surface_id].triangle_count = 128; // Max triangles per surface
    surface_data[surface_id].index_offset = surface_id * 128 * 3;

    // Update material mapping
    material_data[surface_id] = surface_data[surface_id].material_id;
  }

  vis_buffer->surface_count = visible_count;
}

// Programmable rasterization support
typedef struct {
  id<MTLComputePipelineState> programmable_raster_pipeline;
  id<MTLBuffer> raster_params_buffer;
  bool use_programmable_raster;
} ProgrammableRaster;

ProgrammableRaster *programmable_raster_create(id<MTLDevice> device,
                                               id<MTLLibrary> shader_lib) {
  ProgrammableRaster *raster =
      (ProgrammableRaster *)malloc(sizeof(ProgrammableRaster));

  // Create programmable rasterization compute pipeline
  NSError *error = nil;
  id<MTLFunction> raster_function =
      [shader_lib newFunctionWithName:@"nanite_programmable_raster"];
  if (raster_function) {
    raster->programmable_raster_pipeline =
        [device newComputePipelineStateWithFunction:raster_function
                                              error:&error];
    if (!raster->programmable_raster_pipeline) {
      NSLog(@"Failed to create programmable raster pipeline: %@",
            error.localizedDescription);
    }
  }

  // Create raster parameters buffer
  raster->raster_params_buffer =
      [device newBufferWithLength:256 * sizeof(float)
                          options:MTLResourceStorageModeShared];
  raster->use_programmable_raster = true;

  return raster;
}

void programmable_raster_destroy(ProgrammableRaster *raster) {
  if (raster) {
    [raster->programmable_raster_pipeline release];
    [raster->raster_params_buffer release];
    free(raster);
  }
}

void nanite_render_programmable_raster(NaniteRenderer *renderer,
                                       id<MTLCommandBuffer> cmd,
                                       ProgrammableRaster *raster,
                                       const NaniteMesh *mesh,
                                       const simd_float4x4 *view_proj) {
  if (!renderer || !cmd || !raster || !mesh)
    return;

  id<MTLComputeCommandEncoder> compute = [cmd computeCommandEncoder];
  [compute setComputePipelineState:raster->programmable_raster_pipeline];

  // Bind mesh data
  [compute setBuffer:(__bridge id<MTLBuffer>)mesh->gpu_vertex_buffer
              offset:0
             atIndex:0];
  [compute setBuffer:(__bridge id<MTLBuffer>)mesh->gpu_cluster_buffer
              offset:0
             atIndex:1];

  // Bind raster parameters
  [compute setBuffer:raster->raster_params_buffer offset:0 atIndex:2];
  [compute setBytes:view_proj length:sizeof(simd_float4x4) atIndex:3];

  // Dispatch programmable rasterization
  uint32_t thread_count = mesh->cluster_count * 128; // Approximate work items
  MTLSize gridSize = MTLSizeMake(thread_count, 1, 1);
  MTLSize threadgroupSize = MTLSizeMake(64, 1, 1);

  [compute dispatchThreadgroups:gridSize threadsPerThreadgroup:threadgroupSize];
  [compute endEncoding];
}

// Lumen integration for hardware-accelerated GI
typedef struct {
  id<MTLTexture> gi_texture;
  id<MTLTexture> reflection_texture;
  id<MTLTexture> ao_texture;
  id<MTLBuffer> lumen_params_buffer;
  bool lumen_enabled;
} LumenIntegration;

LumenIntegration *lumen_integration_create(id<MTLDevice> device, uint32_t width,
                                           uint32_t height) {
  LumenIntegration *lumen =
      (LumenIntegration *)malloc(sizeof(LumenIntegration));

  // Create GI texture
  MTLTextureDescriptor *giDesc = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                   width:width
                                  height:height
                               mipmapped:YES];
  giDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
  giDesc.storageMode = MTLStorageModePrivate;
  lumen->gi_texture = [device newTextureWithDescriptor:giDesc];

  // Create reflection texture
  MTLTextureDescriptor *reflectionDesc = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                   width:width
                                  height:height
                               mipmapped:YES];
  reflectionDesc.usage =
      MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
  reflectionDesc.storageMode = MTLStorageModePrivate;
  lumen->reflection_texture = [device newTextureWithDescriptor:reflectionDesc];

  // Create AO texture
  MTLTextureDescriptor *aoDesc = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm
                                   width:width
                                  height:height
                               mipmapped:NO];
  aoDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
  aoDesc.storageMode = MTLStorageModePrivate;
  lumen->ao_texture = [device newTextureWithDescriptor:aoDesc];

  // Create Lumen parameters buffer
  lumen->lumen_params_buffer =
      [device newBufferWithLength:64 * sizeof(float)
                          options:MTLResourceStorageModeShared];
  lumen->lumen_enabled = true;

  return lumen;
}

void lumen_integration_destroy(LumenIntegration *lumen) {
  if (lumen) {
    [lumen->gi_texture release];
    [lumen->reflection_texture release];
    [lumen->ao_texture release];
    [lumen->lumen_params_buffer release];
    free(lumen);
  }
}

void nanite_render_lumen_gi(NaniteRenderer *renderer,
                            id<MTLRenderCommandEncoder> encoder,
                            LumenIntegration *lumen,
                            const simd_float3 *camera_pos,
                            const simd_float3 *light_direction) {
  if (!renderer || !encoder || !lumen || !lumen->lumen_enabled)
    return;

  // Bind Lumen textures
  [encoder setFragmentTexture:lumen->gi_texture atIndex:10];
  [encoder setFragmentTexture:lumen->reflection_texture atIndex:11];
  [encoder setFragmentTexture:lumen->ao_texture atIndex:12];

  // Bind Lumen parameters
  [encoder setFragmentBuffer:lumen->lumen_params_buffer offset:0 atIndex:10];

  // Set Lumen parameters
  float lumen_params[4] = {camera_pos->x, camera_pos->y, camera_pos->z, 1.0f};
  [encoder setFragmentBytes:lumen_params
                     length:sizeof(lumen_params)
                    atIndex:11];

  float light_params[4] = {light_direction->x, light_direction->y,
                           light_direction->z, 0.0f};
  [encoder setFragmentBytes:light_params
                     length:sizeof(light_params)
                    atIndex:12];
}

// Mesh painting and runtime cluster modifications
typedef struct {
  id<MTLBuffer> paint_data_buffer;
  id<MTLTexture> paint_texture;
  uint32_t modified_clusters[1024];
  uint32_t modified_count;
  bool painting_enabled;
} MeshPainting;

MeshPainting *mesh_painting_create(id<MTLDevice> device,
                                   uint32_t texture_size) {
  MeshPainting *painting = (MeshPainting *)malloc(sizeof(MeshPainting));

  // Create paint data buffer
  painting->paint_data_buffer =
      [device newBufferWithLength:1024 * sizeof(uint32_t)
                          options:MTLResourceStorageModeShared];

  // Create paint texture
  MTLTextureDescriptor *paintDesc = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                   width:texture_size
                                  height:texture_size
                               mipmapped:NO];
  paintDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
  paintDesc.storageMode = MTLStorageModePrivate;
  painting->paint_texture = [device newTextureWithDescriptor:paintDesc];

  painting->modified_count = 0;
  painting->painting_enabled = true;

  return painting;
}

void mesh_painting_destroy(MeshPainting *painting) {
  if (painting) {
    [painting->paint_data_buffer release];
    [painting->paint_texture release];
    free(painting);
  }
}

void nanite_paint_cluster(MeshPainting *painting, uint32_t cluster_id,
                          uint32_t paint_color) {
  if (!painting || cluster_id >= 1024)
    return;

  // Add to modified clusters list
  bool found = false;
  for (uint32_t i = 0; i < painting->modified_count; i++) {
    if (painting->modified_clusters[i] == cluster_id) {
      found = true;
      break;
    }
  }

  if (!found && painting->modified_count < 1024) {
    painting->modified_clusters[painting->modified_count++] = cluster_id;
  }

  // Update paint data
  uint32_t *paint_data = (uint32_t *)[painting->paint_data_buffer contents];
  paint_data[cluster_id] = paint_color;
}

void nanite_apply_paint_modifications(NaniteRenderer *renderer,
                                      MeshPainting *painting,
                                      NaniteMesh *mesh) {
  if (!renderer || !painting || !mesh || !painting->painting_enabled)
    return;

  // Apply paint modifications to clusters
  for (uint32_t i = 0; i < painting->modified_count; i++) {
    uint32_t cluster_id = painting->modified_clusters[i];
    if (cluster_id < mesh->cluster_count) {
      NaniteCluster *cluster = &mesh->clusters[cluster_id];

      // Apply paint color to cluster material
      // This would typically modify vertex colors or material parameters
      cluster->material_id = (cluster_id % 8); // Example modification
    }
  }
}
