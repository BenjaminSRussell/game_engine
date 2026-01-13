// lighting/lumen/lumen_gi.m
// Lumen Global Illumination Implementation
//
// TODO: Implement Screen-Trace integration for short-range indirect
// reflections.
// TODO: Add support for World-Space Probe Grid (Lumen Probes) with spherical
// harmonics.
// TODO: Implement Surface Cache (Lumen Cards) for high-fidelity multi-bounce
// GI.
// TODO: Add support for hardware ray-tracing (Metal RT) for long-range GI and
// shadows.
// TODO: Implement temporal accumulation and denoising for GI signals (Guided
// Denoising).
// TODO: Add support for probe-interpolation using tetrahedral mesh or octree
// acceleration.
// TODO: Implement irradiance caching with spatial/temporal reuse.
// TODO: Add support for emissive surface contribution to the GI scene.
// TODO: Implement Voxel-Trace fallback for platforms without hardware RT
// support.
// TODO: Add support for multi-bounce GI through surface-cache feedback loops.
// TODO: Implement high-fidelity sky-occlusion based on atmospheric scattering.
// TODO: Research and implement Importance Sampling for dynamic light sources.
// TODO: Add support for volumetric fog integration with Lumen GI.
// TODO: Implement a GPU-driven scene-representation update (Card-to-Voxel /
// Card-to-Mesh).
// TODO: Add support for reflective shadow maps (RSM) for primary light bounces.
// TODO: Implement a robust radiance-cache management system with budget-aware
// updates.
// TODO: Add support for Nanite-aware Lumen scene-representation
// (Distance-Fields for Nanite).
// TODO: Research and implement AI-driven GI denoising models (DLSS-RR
// equivalent).
#include "../../include/lighting/lumen/lumen_gi.h"
#include "../../include/common.h"
#include "../../include/core/logger.h"
#import <Metal/Metal.h>
#include <math.h>
#import <simd/simd.h>
#include <stdlib.h>
#include <string.h>

// Surface Cache (Lumen Cards) for high-fidelity multi-bounce GI
typedef struct {
  Vec3 position;
  Vec3 normal;
  Vec3 albedo;
  Vec3 emissive;
  f32 roughness;
  f32 metallic;
  f32 ao;
  
  // GI data
  Vec3 indirect_lighting;
  Vec3 direct_lighting;
  f32 visibility;
  
  // Cache metadata
  u32 last_update_frame;
  f32 confidence;
  u32 card_id;
  bool is_valid;
} LumenCard;

typedef struct {
  LumenCard *cards;
  u32 card_count;
  u32 max_cards;
  
  // GPU resources
  id<MTLBuffer> card_buffer;
  id<MTLTexture> card_atlas;
  id<MTLTexture> gi_atlas;
  
  // Card management
  u32 card_resolution;      // Resolution per card (e.g., 64x64)
  u32 cards_per_dimension; // Cards per atlas dimension
  u32 update_budget;       // Max cards to update per frame
  u32 current_update_index;
  
  // Multi-bounce feedback
  id<MTLComputePipelineState> card_update_pipeline;
  id<MTLComputePipelineState> card_trace_pipeline;
  f32 bounce_intensity;
  u32 max_bounce_count;
  bool enable_feedback_loops;
} LumenSurfaceCache;

LumenSurfaceCache *lumen_surface_cache_create(id<MTLDevice> device, id<MTLLibrary> shader_lib, 
                                             u32 max_cards, u32 card_resolution) {
  LumenSurfaceCache *cache = (LumenSurfaceCache *)calloc(1, sizeof(LumenSurfaceCache));
  cache->max_cards = max_cards;
  cache->card_resolution = card_resolution;
  cache->cards_per_dimension = 256 / card_resolution; // Assume 256x atlas
  cache->update_budget = 64; // Update 64 cards per frame
  cache->bounce_intensity = 0.7f;
  cache->max_bounce_count = 3;
  cache->enable_feedback_loops = true;
  
  cache->cards = (LumenCard *)calloc(max_cards, sizeof(LumenCard));
  
  @autoreleasepool {
    // Create card update pipeline
    id<MTLFunction> card_update_function = [shader_lib newFunctionWithName:@"lumen_card_update_compute"];
    if (card_update_function) {
      NSError *error = nil;
      cache->card_update_pipeline = [device newComputePipelineStateWithFunction:card_update_function error:&error];
      if (!cache->card_update_pipeline) {
        LOG_ERROR("Failed to create card update pipeline: %s", [[error localizedDescription] UTF8String]);
      }
    }
    
    // Create card trace pipeline
    id<MTLFunction> card_trace_function = [shader_lib newFunctionWithName:@"lumen_card_trace_compute"];
    if (card_trace_function) {
      NSError *error = nil;
      cache->card_trace_pipeline = [device newComputePipelineStateWithFunction:card_trace_function error:&error];
      if (!cache->card_trace_pipeline) {
        LOG_ERROR("Failed to create card trace pipeline: %s", [[error localizedDescription] UTF8String]);
      }
    }
    
    // Create GPU buffers
    cache->card_buffer = [device newBufferWithLength:max_cards * sizeof(LumenCard) 
                                                 options:MTLResourceStorageModeManaged];
    
    // Create card atlas for surface data
    MTLTextureDescriptor *cardAtlasDesc = [MTLTextureDescriptor 
        texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
        width:256
        height:256
        mipmapped:NO];
    cardAtlasDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    cardAtlasDesc.storageMode = MTLStorageModePrivate;
    cache->card_atlas = [device newTextureWithDescriptor:cardAtlasDesc];
    
    // Create GI atlas for lighting data
    MTLTextureDescriptor *giAtlasDesc = [MTLTextureDescriptor 
        texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
        width:256
        height:256
        mipmapped:NO];
    giAtlasDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    giAtlasDesc.storageMode = MTLStorageModePrivate;
    cache->gi_atlas = [device newTextureWithDescriptor:giAtlasDesc];
  }
  
  LOG_INFO("Lumen surface cache created: %u cards, %ux%u resolution", 
           max_cards, card_resolution, card_resolution);
  return cache;
}

void lumen_surface_cache_update(LumenSurfaceCache *cache, id<MTLCommandBuffer> cmd,
                             const Vec3 *camera_pos, f32 delta_time) {
  if (!cache || !cmd) return;
  
  @autoreleasepool {
    // Update subset of cards each frame for performance
    u32 cards_to_update = cache->update_budget;
    u32 start_index = cache->current_update_index;
    
    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];
    [encoder setLabel:@"Lumen Surface Cache Update"];
    [encoder setComputePipelineState:cache->card_update_pipeline];
    
    [encoder setBuffer:cache->card_buffer offset:0 atIndex:0];
    [encoder setTexture:cache->card_atlas atIndex:0];
    [encoder setTexture:cache->gi_atlas atIndex:1];
    
    struct {
      Vec3 camera_position;
      f32 delta_time;
      u32 start_index;
      u32 cards_to_update;
      u32 card_resolution;
      f32 bounce_intensity;
      u32 max_bounce_count;
      u32 enable_feedback_loops;
    } update_params = {
      .camera_position = *camera_pos,
      .delta_time = delta_time,
      .start_index = start_index,
      .cards_to_update = cards_to_update,
      .card_resolution = cache->card_resolution,
      .bounce_intensity = cache->bounce_intensity,
      .max_bounce_count = cache->max_bounce_count,
      .enable_feedback_loops = cache->enable_feedback_loops ? 1 : 0
    };
    [encoder setBytes:&update_params length:sizeof(update_params) atIndex:1];
    
    MTLSize gridSize = MTLSizeMake((cards_to_update + 7) / 8, 1, 1);
    MTLSize threadgroupSize = MTLSizeMake(8, 1, 1);
    
    [encoder dispatchThreadgroups:gridSize threadsPerThreadgroup:threadgroupSize];
    [encoder endEncoding];
    
    // Update circular buffer for next frame
    cache->current_update_index = (start_index + cards_to_update) % cache->max_cards;
  }
}

void lumen_surface_cache_trace(LumenSurfaceCache *cache, id<MTLCommandBuffer> cmd,
                             id<MTLTexture> depth, id<MTLTexture> normals,
                             const Mat4 *view_proj, const Vec3 *camera_pos) {
  if (!cache || !cmd || !depth || !normals) return;
  
  @autoreleasepool {
    id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];
    [encoder setLabel:@"Lumen Surface Cache Trace"];
    [encoder setComputePipelineState:cache->card_trace_pipeline];
    
    [encoder setTexture:depth atIndex:0];
    [encoder setTexture:normals atIndex:1];
    [encoder setTexture:cache->card_atlas atIndex:2];
    [encoder setTexture:cache->gi_atlas atIndex:3];
    [encoder setBuffer:cache->card_buffer offset:0 atIndex:0];
    
    [encoder setBytes:view_proj length:sizeof(Mat4) atIndex:1];
    [encoder setBytes:camera_pos length:sizeof(Vec3) atIndex:2];
    
    struct {
      u32 card_count;
      u32 card_resolution;
      f32 trace_distance;
      f32 thickness;
    } trace_params = {
      .card_count = cache->card_count,
      .card_resolution = cache->card_resolution,
      .trace_distance = 100.0f,
      .thickness = 0.1f
    };
    [encoder setBytes:&trace_params length:sizeof(trace_params) atIndex:3];
    
    MTLSize gridSize = MTLSizeMake((depth.width + 15) / 16, (depth.height + 15) / 16, 1);
    MTLSize threadgroupSize = MTLSizeMake(16, 16, 1);
    
    [encoder dispatchThreadgroups:gridSize threadsPerThreadgroup:threadgroupSize];
    [encoder endEncoding];
  }
}

void lumen_surface_cache_destroy(LumenSurfaceCache *cache) {
  if (!cache) return;
  
  free(cache->cards);
  cache->card_buffer = nil;
  cache->card_atlas = nil;
  cache->gi_atlas = nil;
  cache->card_update_pipeline = nil;
  cache->card_trace_pipeline = nil;
  
  free(cache);
}

// Screen-Trace integration for short-range indirect reflections
typedef struct {
  id<MTLComputePipelineState> screen_trace_pipeline;
  id<MTLTexture> depth_pyramid;
  id<MTLTexture> normal_pyramid;
  id<MTLTexture> roughness_pyramid;
  id<MTLBuffer> ray_buffer;
  u32 max_rays;
  f32 max_trace_distance;
  u32 max_steps_per_ray;
  f32 ray_thickness;
} LumenScreenTrace;

LumenScreenTrace *lumen_screen_trace_create(id<MTLDevice> device, id<MTLLibrary> shader_lib, 
                                         u32 width, u32 height) {
  LumenScreenTrace *screen_trace = (LumenScreenTrace *)calloc(1, sizeof(LumenScreenTrace));
  screen_trace->max_rays = width * height;
  screen_trace->max_trace_distance = 50.0f;
  screen_trace->max_steps_per_ray = 64;
  screen_trace->ray_thickness = 0.1f;
  
  @autoreleasepool {
    // Create screen trace compute pipeline
    id<MTLFunction> screen_trace_function = [shader_lib newFunctionWithName:@"lumen_screen_trace_compute"];
    if (screen_trace_function) {
      NSError *error = nil;
      screen_trace->screen_trace_pipeline = [device newComputePipelineStateWithFunction:screen_trace_function error:&error];
      if (!screen_trace->screen_trace_pipeline) {
        LOG_ERROR("Failed to create screen trace pipeline: %s", [[error localizedDescription] UTF8String]);
      }
    }
    
    // Create depth pyramid for hierarchical Z tracing
    MTLTextureDescriptor *depthDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR32Float
                                                                                             width:width
                                                                                            height:height
                                                                                         mipmapped:YES];
    depthDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    depthDesc.storageMode = MTLStorageModePrivate;
    screen_trace->depth_pyramid = [device newTextureWithDescriptor:depthDesc];
    
    // Create normal pyramid
    MTLTextureDescriptor *normalDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                                                                                              width:width
                                                                                             height:height
                                                                                         mipmapped:YES];
    normalDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    normalDesc.storageMode = MTLStorageModePrivate;
    screen_trace->normal_pyramid = [device newTextureWithDescriptor:normalDesc];
    
    // Create roughness pyramid
    MTLTextureDescriptor *roughnessDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm
                                                                                               width:width
                                                                                              height:height
                                                                                          mipmapped:YES];
    roughnessDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    roughnessDesc.storageMode = MTLStorageModePrivate;
    screen_trace->roughness_pyramid = [device newTextureWithDescriptor:roughnessDesc];
    
    // Create ray buffer for screen-space rays
    screen_trace->ray_buffer = [device newBufferWithLength:screen_trace->max_rays * sizeof(Vec4) 
                                                     options:MTLResourceStorageModePrivate];
  }
  
  LOG_INFO("Lumen screen trace created for %ux%u", width, height);
  return screen_trace;
}

void lumen_screen_trace_update(LumenScreenTrace *screen_trace, id<MTLCommandBuffer> cmd,
                             id<MTLTexture> depth, id<MTLTexture> normals, 
                             id<MTLTexture> roughness, const Mat4 *inv_view_proj,
                             const Mat4 *prev_view_proj) {
  if (!screen_trace || !cmd || !depth || !normals) return;
  
  @autoreleasepool {
    // First pass: Build mipmapped pyramids for hierarchical tracing
    id<MTLComputeCommandEncoder> pyramid_encoder = [cmd computeCommandEncoder];
    [pyramid_encoder setLabel:@"Lumen Build Pyramids"];
    
    // Build depth pyramid (simplified - would use proper pyramid generation)
    for (u32 level = 1; level < 8; level++) {
      u32 width = depth.width >> level;
      u32 height = depth.height >> level;
      if (width < 1 || height < 1) break;
      
      [pyramid_encoder setTexture:screen_trace->depth_pyramid atIndex:0];
      [pyramid_encoder setTexture:(level == 1) ? depth : screen_trace->depth_pyramid atIndex:1];
      
      u32 level_param = level;
      [pyramid_encoder setBytes:&level_param length:sizeof(u32) atIndex:0];
      
      MTLSize gridSize = MTLSizeMake((width + 15) / 16, (height + 15) / 16, 1);
      MTLSize threadgroupSize = MTLSizeMake(16, 16, 1);
      
      [pyramid_encoder dispatchThreadgroups:gridSize threadsPerThreadgroup:threadgroupSize];
    }
    
    [pyramid_encoder endEncoding];
    
    // Second pass: Screen-space ray tracing
    id<MTLComputeCommandEncoder> trace_encoder = [cmd computeCommandEncoder];
    [trace_encoder setLabel:@"Lumen Screen Trace"];
    [trace_encoder setComputePipelineState:screen_trace->screen_trace_pipeline];
    
    [trace_encoder setTexture:depth atIndex:0];
    [trace_encoder setTexture:normals atIndex:1];
    [trace_encoder setTexture:roughness atIndex:2];
    [trace_encoder setTexture:screen_trace->depth_pyramid atIndex:3];
    [trace_encoder setTexture:screen_trace->normal_pyramid atIndex:4];
    
    [trace_encoder setBuffer:screen_trace->ray_buffer offset:0 atIndex:0];
    
    [trace_encoder setBytes:inv_view_proj length:sizeof(Mat4) atIndex:1];
    [trace_encoder setBytes:prev_view_proj length:sizeof(Mat4) atIndex:2];
    
    struct {
      f32 max_trace_distance;
      u32 max_steps_per_ray;
      f32 ray_thickness;
      f32 roughness_bias;
      u32 width, height;
    } trace_params = {
      .max_trace_distance = screen_trace->max_trace_distance,
      .max_steps_per_ray = screen_trace->max_steps_per_ray,
      .ray_thickness = screen_trace->ray_thickness,
      .roughness_bias = 0.01f,
      .width = depth.width,
      .height = depth.height
    };
    [trace_encoder setBytes:&trace_params length:sizeof(trace_params) atIndex:3];
    
    MTLSize gridSize = MTLSizeMake((depth.width + 7) / 8, (depth.height + 7) / 8, 1);
    MTLSize threadgroupSize = MTLSizeMake(8, 8, 1);
    
    [trace_encoder dispatchThreadgroups:gridSize threadsPerThreadgroup:threadgroupSize];
    [trace_encoder endEncoding];
  }
}

void lumen_screen_trace_destroy(LumenScreenTrace *screen_trace) {
  if (!screen_trace) return;
  
  screen_trace->screen_trace_pipeline = nil;
  screen_trace->depth_pyramid = nil;
  screen_trace->normal_pyramid = nil;
  screen_trace->roughness_pyramid = nil;
  screen_trace->ray_buffer = nil;
  
  free(screen_trace);
}

struct LumenGISystem {
  id<MTLDevice> device;
  id<MTLCommandQueue> command_queue;

  // Enhanced probe grid with spherical harmonics
  Vec3 *probe_sh_coefficients; // 9 SH coefficients per probe (RGB)
  f32 *probe_validity;       // Validity weights for each coefficient
  id<MTLBuffer> sh_buffer;    // GPU buffer for SH coefficients
  id<MTLComputePipelineState> probe_update_pipeline;
  id<MTLComputePipelineState> probe_interpolation_pipeline;
  f32 sh_bias;                // SH bias for energy conservation
  f32 sh_scale;               // SH scale for intensity control
  bool use_high_quality_sh;     // Use 3rd order SH (27 coeffs) vs 2nd order (9)

  // Surface cache
  LumenSurfaceCache *surface_cache;
  u32 surface_count;
  u32 max_surfaces;

  // GPU resources
  id<MTLBuffer> probe_buffer;
  id<MTLBuffer> surface_buffer;
  id<MTLComputePipelineState> trace_pipeline;
  id<MTLComputePipelineState> update_pipeline;

  // Textures
  id<MTLTexture> radiance_cache;
  id<MTLTexture> irradiance_cache;

  // Hardware Ray Tracing
  MetalRTAccelStructure *rt_scene;
  MetalRTPipeline *rt_pipeline;
  bool supports_rt;
};

// Spherical harmonics utility functions
static void lumen_sh_project_onto_coefficients(const Vec3 *direction, const Vec3 *radiance, 
                                            f32 *out_coefficients, u32 coefficient_count) {
  // Second-order spherical harmonics (9 coefficients)
  const f32 x = direction->x, y = direction->y, z = direction->z;
  const f32 x2 = x * x, y2 = y * y, z2 = z * z;
  const f32 xy = x * y, xz = x * z, yz = y * z;
  
  // SH basis functions (l=0,1,2)
  f32 basis[9];
  basis[0] = 0.282095f;                    // Y0,0
  basis[1] = 0.488603f * y;                // Y1,-1
  basis[2] = 0.488603f * z;                // Y1,0
  basis[3] = 0.488603f * x;                // Y1,1
  basis[4] = 1.092548f * xy;               // Y2,-2
  basis[5] = 1.092548f * yz;               // Y2,-1
  basis[6] = 0.315392f * (3.0f * z2 - 1.0f); // Y2,0
  basis[7] = 1.092548f * xz;               // Y2,1
  basis[8] = 0.546274f * (x2 - y2);        // Y2,2
  
  // Project radiance onto SH basis
  for (u32 i = 0; i < coefficient_count && i < 9; i++) {
    out_coefficients[i * 3 + 0] += radiance->x * basis[i]; // R
    out_coefficients[i * 3 + 1] += radiance->y * basis[i]; // G
    out_coefficients[i * 3 + 2] += radiance->z * basis[i]; // B
  }
}

static Vec3 lumen_sh_evaluate_from_coefficients(const Vec3 *direction, const f32 *coefficients, 
                                            u32 coefficient_count) {
  const f32 x = direction->x, y = direction->y, z = direction->z;
  const f32 x2 = x * x, y2 = y * y, z2 = z * z;
  const f32 xy = x * y, xz = x * z, yz = y * z;
  
  // SH basis functions
  f32 basis[9];
  basis[0] = 0.282095f;
  basis[1] = 0.488603f * y;
  basis[2] = 0.488603f * z;
  basis[3] = 0.488603f * x;
  basis[4] = 1.092548f * xy;
  basis[5] = 1.092548f * yz;
  basis[6] = 0.315392f * (3.0f * z2 - 1.0f);
  basis[7] = 1.092548f * xz;
  basis[8] = 0.546274f * (x2 - y2);
  
  // Reconstruct radiance from SH coefficients
  Vec3 result = vec3_zero();
  for (u32 i = 0; i < coefficient_count && i < 9; i++) {
    f32 weight = basis[i];
    result.x += coefficients[i * 3 + 0] * weight;
    result.y += coefficients[i * 3 + 1] * weight;
    result.z += coefficients[i * 3 + 2] * weight;
  }
  
  return result;
}

static void lumen_sh_normalize_coefficients(f32 *coefficients, u32 coefficient_count) {
  // Apply energy conservation and clamping
  for (u32 i = 0; i < coefficient_count * 3; i++) {
    coefficients[i] = fmaxf(0.0f, coefficients[i]); // Clamp to non-negative
  }
  
  // Normalize based on DC coefficient (index 0)
  f32 dc_r = coefficients[0];
  f32 dc_g = coefficients[1];
  f32 dc_b = coefficients[2];
  
  if (dc_r > 0.001f || dc_g > 0.001f || dc_b > 0.001f) {
    f32 scale = 1.0f / fmaxf(fmaxf(dc_r, dc_g), dc_b);
    for (u32 i = 0; i < coefficient_count * 3; i++) {
      coefficients[i] *= scale;
    }
  }
}

LumenGISystem *lumen_create(id<MTLDevice> device, u32 scene_width,
                            u32 scene_height, u32 scene_depth) {
  LumenGISystem *lumen = (LumenGISystem *)calloc(1, sizeof(LumenGISystem));
  lumen->device = device;
  lumen->command_queue = [device newCommandQueue];

  // Calculate probe grid dimensions
  lumen->cell_size = 2.0f; // 2 meters per cell
  lumen->grid_x = (scene_width + 1) / 2;
  lumen->grid_y = (scene_height + 1) / 2;
  lumen->grid_z = (scene_depth + 1) / 2;
  lumen->probe_count = lumen->grid_x * lumen->grid_y * lumen->grid_z;

  // Allocate probes
  lumen->probes = (LumenProbe *)calloc(lumen->probe_count, sizeof(LumenProbe));

  // Initialize probe positions
  u32 idx = 0;
  for (u32 z = 0; z < lumen->grid_z; z++) {
    for (u32 y = 0; y < lumen->grid_y; y++) {
      for (u32 x = 0; x < lumen->grid_x; x++) {
        LumenProbe *probe = &lumen->probes[idx++];
        probe->position = vec3(x * lumen->cell_size, y * lumen->cell_size,
                               z * lumen->cell_size);
        probe->radius = lumen->cell_size * 1.5f;
        probe->last_update_time = 0;
      }
    }
  }

  // Create GPU buffers
  lumen->probe_buffer =
      [device newBufferWithLength:lumen->probe_count * sizeof(LumenProbe)
                          options:MTLResourceStorageModeShared];

  // Surface cache
  lumen->max_surfaces = LUMEN_RADIANCE_CACHE_SIZE * LUMEN_RADIANCE_CACHE_SIZE;
  lumen->surface_cache = (LumenSurfaceCache *)calloc(lumen->max_surfaces,
                                                     sizeof(LumenSurfaceCache));
  lumen->surface_buffer = [device
      newBufferWithLength:lumen->max_surfaces * sizeof(LumenSurfaceCache)
                  options:MTLResourceStorageModeShared];

  LOG_INFO("Lumen GI created with %u probes (%ux%ux%u grid)",
           lumen->probe_count, lumen->grid_x, lumen->grid_y, lumen->grid_z);

  return lumen;
}

void lumen_set_raytracing_scene(LumenGISystem *lumen,
                                MetalRTAccelStructure *scene) {
  if (!lumen)
    return;
  lumen->rt_scene = scene;

  // Initialize RT if supported and not yet up
  if (!lumen->supports_rt && metal_rt_is_supported(lumen->device)) {
    lumen->supports_rt = true;
    // Note: We would load shaders here.
    // id<MTLFunction> ray_gen = [library newFunctionWithName:@"lumen_ray_gen"];
    // lumen->rt_pipeline = metal_rt_create_pipeline(lumen->device, ray_gen,
    // ...);
    LOG_INFO("Lumen Hardware Ray Tracing enabled.");
  }
}

void lumen_destroy(LumenGISystem *lumen) {
  if (!lumen)
    return;

  free(lumen->probes);
  free(lumen->surface_cache);
  lumen->probe_buffer = nil;
  lumen->surface_buffer = nil;
  lumen->command_queue = nil;
  free(lumen);
}

void lumen_update(LumenGISystem *lumen, id<MTLCommandBuffer> cmd,
                  const Vec3 *camera_pos, f32 delta_time) {
  if (!lumen || !cmd)
    return;

  // Update probe lighting (staggered - only update subset per frame)
  u32 probes_per_frame = lumen->probe_count / 8; // Update 1/8 each frame
  static u32 update_offset = 0;

  for (u32 i = 0; i < probes_per_frame; i++) {
    u32 probe_idx = (update_offset + i) % lumen->probe_count;
    LumenProbe *probe = &lumen->probes[probe_idx];
    probe->last_update_time += delta_time;
  }

  update_offset = (update_offset + probes_per_frame) % lumen->probe_count;

  // Upload to GPU
  memcpy([lumen->probe_buffer contents], lumen -> probes,
         lumen -> probe_count * sizeof(LumenProbe));
}

void lumen_trace_gi(LumenGISystem *lumen, id<MTLCommandBuffer> cmd,
                    id<MTLTexture> depth, id<MTLTexture> normals,
                    id<MTLTexture> albedo, id<MTLTexture> output_diffuse,
                    id<MTLTexture> output_specular) {
  if (!lumen || !cmd)
    return;

  // 1. Hierarchical Z-Buffer Generation (Dummypass for V1)
  // This helps skip empty space during screen-space rays.

  // 2. Hardware Ray Tracing (if available)
  if (lumen->supports_rt && lumen->rt_scene && lumen->rt_pipeline) {
    metal_rt_trace_rays(lumen->rt_pipeline, cmd, lumen->rt_scene,
                        output_diffuse, output_diffuse.width,
                        output_diffuse.height);
    // Also trace specular or combine?
    // For V1, we just demo the call.
    return;
  }

  // 3. Fallback: Screen-Space Tracing
  id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];
  [encoder setLabel:@"Lumen Trace GI"];
  [encoder setComputePipelineState:lumen->trace_pipeline];
  [encoder setTexture:depth atIndex:0];
  [encoder setTexture:normals atIndex:1];
  [encoder setTexture:albedo atIndex:2];
  [encoder setTexture:output_diffuse atIndex:3];
  [encoder setTexture:output_specular atIndex:4];
  [encoder setBuffer:lumen->probe_buffer offset:0 atIndex:0];

  // Set ray parameters
  struct {
    f32 max_trace_dist;
    f32 step_size;
    u32 max_steps;
    f32 thickness;
  } params = {10.0f, 0.1f, 100, 0.2f};
  [encoder setBytes:&params length:sizeof(params) atIndex:1];

  MTLSize threadsPerGroup = MTLSizeMake(16, 16, 1);
  MTLSize threadgroups = MTLSizeMake((output_diffuse.width + 15) / 16,
                                     (output_diffuse.height + 15) / 16, 1);

  [encoder dispatchThreadgroups:threadgroups
          threadsPerThreadgroup:threadsPerGroup];
  [encoder endEncoding];
}

void lumen_update_surface_cache(LumenGISystem *lumen, const Vec3 *positions,
                                const Vec3 *normals, const Vec3 *albedos,
                                u32 surface_count) {
  if (!lumen || !positions)
    return;

  // Repack dynamic object surfaces into the global surface cache
  for (u32 i = 0; i < surface_count && i < lumen->max_surfaces; i++) {
    LumenSurfaceCache *surface = &lumen->surface_cache[i];
    surface->position = positions[i];
    surface->normal = normals ? normals[i] : vec3(0, 1, 0);
    surface->albedo = albedos ? albedos[i] : vec3(0.5f, 0.5f, 0.5f);
    surface->emissive = vec3_zero();
    surface->roughness = 0.5f;
    surface->metallic = 0.0f;
  }

  lumen->surface_count =
      surface_count < lumen->max_surfaces ? surface_count : lumen->max_surfaces;

  // Update GPU buffer
  memcpy([lumen->surface_buffer contents], lumen -> surface_cache,
         lumen -> surface_count * sizeof(LumenSurfaceCache));
}

void lumen_place_probes_automatic(LumenGISystem *lumen,
                                  const void *scene_geometry) {
  // TODO: Analyze geometry and place probes optimally
  LOG_INFO("Auto-placing Lumen probes based on scene geometry");
}
