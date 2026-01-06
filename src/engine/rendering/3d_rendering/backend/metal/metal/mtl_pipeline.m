/*
 * mtl_pipeline.m
 * Metal Pipeline State Objects Implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_pipeline.h"
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================
 */

static metal_pipeline_cache_t g_pipeline_cache = {0};
static bool g_initialized = false;

/* ============================================================================
 * INITIALIZATION
 * ============================================================================
 */

int metal_pipeline_init(void) {
  if (g_initialized) {
    return 0;
  }

  metal_pipeline_cache_init(&g_pipeline_cache);
  g_initialized = true;

  return 0;
}

void metal_pipeline_shutdown(void) {
  if (!g_initialized) {
    return;
  }

  metal_pipeline_cache_shutdown(&g_pipeline_cache);
  g_initialized = false;
}

/* ============================================================================
 * SHADER LIBRARY
 * ============================================================================
 */

metal_shader_library_t *metal_load_shader_library(MTLDeviceRef device_ref,
                                                  const char *path) {
  if (!device_ref || !path) {
    return NULL;
  }

  id<MTLDevice> device = (__bridge id<MTLDevice>)device_ref;

  NSString *nsPath = [NSString stringWithUTF8String:path];
  NSError *error = nil;

  id<MTLLibrary> library = [device newLibraryWithFile:nsPath error:&error];

  if (error || !library) {
    NSLog(@"Failed to load shader library from %@: %@", nsPath, error);
    return NULL;
  }

  metal_shader_library_t *result =
      (metal_shader_library_t *)calloc(1, sizeof(metal_shader_library_t));
  if (!result) {
    return NULL;
  }

  result->library = (__bridge_retained MTLLibraryRef)library;
  strncpy(result->name, path, sizeof(result->name) - 1);

  return result;
}

metal_shader_library_t *metal_load_shader_library_data(MTLDeviceRef device_ref,
                                                       const void *data,
                                                       size_t size) {
  if (!device_ref || !data || size == 0) {
    return NULL;
  }

  id<MTLDevice> device = (__bridge id<MTLDevice>)device_ref;

  dispatch_data_t dispatchData =
      dispatch_data_create(data, size, NULL, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
  NSError *error = nil;

  id<MTLLibrary> library = [device newLibraryWithData:dispatchData
                                                error:&error];

  if (error || !library) {
    NSLog(@"Failed to load shader library from data: %@", error);
    return NULL;
  }

  metal_shader_library_t *result =
      (metal_shader_library_t *)calloc(1, sizeof(metal_shader_library_t));
  if (!result) {
    return NULL;
  }

  result->library = (__bridge_retained MTLLibraryRef)library;
  strncpy(result->name, "<embedded>", sizeof(result->name) - 1);

  return result;
}

MTLFunctionRef metal_get_function(metal_shader_library_t *library,
                                  const char *name) {
  if (!library || !library->library || !name) {
    return NULL;
  }

  id<MTLLibrary> mtlLibrary = (__bridge id<MTLLibrary>)library->library;
  NSString *functionName = [NSString stringWithUTF8String:name];

  id<MTLFunction> function = [mtlLibrary newFunctionWithName:functionName];

  if (!function) {
    NSLog(@"Failed to find function '%@' in library", functionName);
    return NULL;
  }

  return (__bridge_retained MTLFunctionRef)function;
}

void metal_destroy_shader_library(metal_shader_library_t *library) {
  if (!library) {
    return;
  }

  if (library->library) {
    CFRelease(library->library);
    library->library = NULL;
  }

  free(library);
}

/* ============================================================================
 * VERTEX DESCRIPTOR
 * ============================================================================
 */

void metal_vertex_descriptor_init(metal_vertex_descriptor_t *desc) {
  if (!desc) {
    return;
  }

  memset(desc, 0, sizeof(metal_vertex_descriptor_t));
}

void metal_vertex_descriptor_add_attribute(metal_vertex_descriptor_t *desc,
                                           uint32_t location,
                                           metal_vertex_format_t format,
                                           uint32_t offset,
                                           uint32_t buffer_index) {
  if (!desc || location >= METAL_MAX_VERTEX_ATTRIBUTES) {
    return;
  }

  if (desc->attribute_count >= METAL_MAX_VERTEX_ATTRIBUTES) {
    return;
  }

  metal_vertex_attribute_t *attr = &desc->attributes[location];
  attr->format = format;
  attr->offset = offset;
  attr->buffer_index = buffer_index;

  if (location >= desc->attribute_count) {
    desc->attribute_count = location + 1;
  }
}

void metal_vertex_descriptor_set_layout(metal_vertex_descriptor_t *desc,
                                        uint32_t buffer_index, uint32_t stride,
                                        uint32_t step_rate) {
  if (!desc || buffer_index >= METAL_MAX_VERTEX_BUFFERS) {
    return;
  }

  metal_vertex_buffer_layout_t *layout = &desc->layouts[buffer_index];
  layout->stride = stride;
  layout->step_rate = step_rate;

  if (buffer_index >= desc->layout_count) {
    desc->layout_count = buffer_index + 1;
  }
}

/* ============================================================================
 * DEPTH/STENCIL STATE
 * ============================================================================
 */

metal_depth_stencil_state_t *
metal_create_depth_stencil_state(MTLDeviceRef device_ref,
                                 const metal_depth_stencil_desc_t *desc) {
  if (!device_ref || !desc) {
    return NULL;
  }

  id<MTLDevice> device = (__bridge id<MTLDevice>)device_ref;

  MTLDepthStencilDescriptor *mtlDesc = [[MTLDepthStencilDescriptor alloc] init];

  // Map compare function
  MTLCompareFunction compareFunc;
  switch (desc->depth_compare) {
  case METAL_COMPARE_NEVER:
    compareFunc = MTLCompareFunctionNever;
    break;
  case METAL_COMPARE_LESS:
    compareFunc = MTLCompareFunctionLess;
    break;
  case METAL_COMPARE_EQUAL:
    compareFunc = MTLCompareFunctionEqual;
    break;
  case METAL_COMPARE_LESS_EQUAL:
    compareFunc = MTLCompareFunctionLessEqual;
    break;
  case METAL_COMPARE_GREATER:
    compareFunc = MTLCompareFunctionGreater;
    break;
  case METAL_COMPARE_NOT_EQUAL:
    compareFunc = MTLCompareFunctionNotEqual;
    break;
  case METAL_COMPARE_GREATER_EQUAL:
    compareFunc = MTLCompareFunctionGreaterEqual;
    break;
  case METAL_COMPARE_ALWAYS:
    compareFunc = MTLCompareFunctionAlways;
    break;
  default:
    compareFunc = MTLCompareFunctionLess;
    break;
  }

  mtlDesc.depthCompareFunction = compareFunc;
  mtlDesc.depthWriteEnabled = desc->depth_write_enabled;

  id<MTLDepthStencilState> state =
      [device newDepthStencilStateWithDescriptor:mtlDesc];

  if (!state) {
    NSLog(@"Failed to create depth/stencil state");
    return NULL;
  }

  metal_depth_stencil_state_t *result = (metal_depth_stencil_state_t *)calloc(
      1, sizeof(metal_depth_stencil_state_t));
  if (!result) {
    return NULL;
  }

  result->state = (__bridge_retained MTLDepthStencilStateRef)state;
  result->desc = *desc;

  return result;
}

void metal_destroy_depth_stencil_state(metal_depth_stencil_state_t *state) {
  if (!state) {
    return;
  }

  if (state->state) {
    CFRelease(state->state);
    state->state = NULL;
  }

  free(state);
}

/* ============================================================================
 * RENDER PIPELINE
 * ============================================================================
 */

static MTLVertexDescriptor *
create_mtl_vertex_descriptor(const metal_vertex_descriptor_t *desc) {
  if (!desc || desc->attribute_count == 0) {
    return nil;
  }

  MTLVertexDescriptor *mtlDesc = [[MTLVertexDescriptor alloc] init];

  // Set up attributes
  for (uint32_t i = 0; i < desc->attribute_count; i++) {
    const metal_vertex_attribute_t *attr = &desc->attributes[i];

    MTLVertexFormat format;
    switch (attr->format) {
    case METAL_VERTEX_FORMAT_FLOAT:
      format = MTLVertexFormatFloat;
      break;
    case METAL_VERTEX_FORMAT_FLOAT2:
      format = MTLVertexFormatFloat2;
      break;
    case METAL_VERTEX_FORMAT_FLOAT3:
      format = MTLVertexFormatFloat3;
      break;
    case METAL_VERTEX_FORMAT_FLOAT4:
      format = MTLVertexFormatFloat4;
      break;
    case METAL_VERTEX_FORMAT_UCHAR4_NORMALIZED:
      format = MTLVertexFormatUChar4Normalized;
      break;
    default:
      format = MTLVertexFormatInvalid;
      break;
    }

    mtlDesc.attributes[i].format = format;
    mtlDesc.attributes[i].offset = attr->offset;
    mtlDesc.attributes[i].bufferIndex = attr->buffer_index;
  }

  // Set up layouts
  for (uint32_t i = 0; i < desc->layout_count; i++) {
    const metal_vertex_buffer_layout_t *layout = &desc->layouts[i];

    mtlDesc.layouts[i].stride = layout->stride;
    mtlDesc.layouts[i].stepFunction = (layout->step_rate == 0)
                                          ? MTLVertexStepFunctionPerVertex
                                          : MTLVertexStepFunctionPerInstance;
    mtlDesc.layouts[i].stepRate =
        (layout->step_rate == 0) ? 1 : layout->step_rate;
  }

  return mtlDesc;
}

metal_render_pipeline_t *
metal_create_render_pipeline(MTLDeviceRef device_ref,
                             const metal_render_pipeline_desc_t *desc) {
  if (!device_ref || !desc) {
    return NULL;
  }

  if (!desc->vertex_function || !desc->fragment_function) {
    NSLog(@"Render pipeline requires both vertex and fragment functions");
    return NULL;
  }

  id<MTLDevice> device = (__bridge id<MTLDevice>)device_ref;
  id<MTLFunction> vertexFunc = (__bridge id<MTLFunction>)desc->vertex_function;
  id<MTLFunction> fragmentFunc =
      (__bridge id<MTLFunction>)desc->fragment_function;

  MTLRenderPipelineDescriptor *pipelineDesc =
      [[MTLRenderPipelineDescriptor alloc] init];
  pipelineDesc.vertexFunction = vertexFunc;
  pipelineDesc.fragmentFunction = fragmentFunc;

  // Color attachment
  pipelineDesc.colorAttachments[0].pixelFormat =
      (MTLPixelFormat)desc->color_format;

  // Blend state
  if (desc->blend_state.blend_enabled) {
    pipelineDesc.colorAttachments[0].blendingEnabled = YES;
    pipelineDesc.colorAttachments[0].sourceRGBBlendFactor =
        (MTLBlendFactor)desc->blend_state.src_rgb_blend;
    pipelineDesc.colorAttachments[0].destinationRGBBlendFactor =
        (MTLBlendFactor)desc->blend_state.dst_rgb_blend;
    pipelineDesc.colorAttachments[0].rgbBlendOperation =
        (MTLBlendOperation)desc->blend_state.rgb_blend_op;
    pipelineDesc.colorAttachments[0].sourceAlphaBlendFactor =
        (MTLBlendFactor)desc->blend_state.src_alpha_blend;
    pipelineDesc.colorAttachments[0].destinationAlphaBlendFactor =
        (MTLBlendFactor)desc->blend_state.dst_alpha_blend;
    pipelineDesc.colorAttachments[0].alphaBlendOperation =
        (MTLBlendOperation)desc->blend_state.alpha_blend_op;
  }

  // Depth attachment
  if (desc->depth_format != METAL_PIXEL_FORMAT_INVALID) {
    pipelineDesc.depthAttachmentPixelFormat =
        (MTLPixelFormat)desc->depth_format;
  }

  // Vertex descriptor
  pipelineDesc.vertexDescriptor =
      create_mtl_vertex_descriptor(&desc->vertex_descriptor);

  NSError *error = nil;
  id<MTLRenderPipelineState> pipelineState =
      [device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];

  if (error || !pipelineState) {
    NSLog(@"Failed to create render pipeline state: %@", error);
    return NULL;
  }

  // Create depth/stencil state if depth format is specified
  id<MTLDepthStencilState> depthStencilState = nil;
  if (desc->depth_format != METAL_PIXEL_FORMAT_INVALID) {
    metal_depth_stencil_desc_t dsDesc = {.depth_compare = METAL_COMPARE_LESS,
                                         .depth_write_enabled = true,
                                         .stencil_enabled = false};

    metal_depth_stencil_state_t *dsState =
        metal_create_depth_stencil_state(device_ref, &dsDesc);
    if (dsState) {
      depthStencilState = (__bridge id<MTLDepthStencilState>)dsState->state;
      free(dsState); // We only need the state object
    }
  }

  metal_render_pipeline_t *result =
      (metal_render_pipeline_t *)calloc(1, sizeof(metal_render_pipeline_t));
  if (!result) {
    return NULL;
  }

  result->state = (__bridge_retained MTLRenderPipelineStateRef)pipelineState;
  result->depth_stencil =
      depthStencilState
          ? (__bridge_retained MTLDepthStencilStateRef)depthStencilState
          : NULL;
  result->cull_mode = desc->cull_mode;
  result->fill_mode = desc->fill_mode;
  result->hash = metal_hash_render_pipeline_desc(desc);

  return result;
}

void metal_destroy_render_pipeline(metal_render_pipeline_t *pipeline) {
  if (!pipeline) {
    return;
  }

  if (pipeline->state) {
    CFRelease(pipeline->state);
    pipeline->state = NULL;
  }

  if (pipeline->depth_stencil) {
    CFRelease(pipeline->depth_stencil);
    pipeline->depth_stencil = NULL;
  }

  free(pipeline);
}

/* ============================================================================
 * COMPUTE PIPELINE
 * ============================================================================
 */

metal_compute_pipeline_t *
metal_create_compute_pipeline(MTLDeviceRef device_ref,
                              const metal_compute_pipeline_desc_t *desc) {
  if (!device_ref || !desc || !desc->compute_function) {
    return NULL;
  }

  id<MTLDevice> device = (__bridge id<MTLDevice>)device_ref;
  id<MTLFunction> computeFunc =
      (__bridge id<MTLFunction>)desc->compute_function;

  NSError *error = nil;
  id<MTLComputePipelineState> pipelineState =
      [device newComputePipelineStateWithFunction:computeFunc error:&error];

  if (error || !pipelineState) {
    NSLog(@"Failed to create compute pipeline state: %@", error);
    return NULL;
  }

  metal_compute_pipeline_t *result =
      (metal_compute_pipeline_t *)calloc(1, sizeof(metal_compute_pipeline_t));
  if (!result) {
    return NULL;
  }

  result->state = (__bridge_retained MTLComputePipelineStateRef)pipelineState;
  result->threadgroup_size_x = desc->threadgroup_size_x;
  result->threadgroup_size_y = desc->threadgroup_size_y;
  result->threadgroup_size_z = desc->threadgroup_size_z;
  result->hash = metal_hash_compute_pipeline_desc(desc);

  return result;
}

void metal_destroy_compute_pipeline(metal_compute_pipeline_t *pipeline) {
  if (!pipeline) {
    return;
  }

  if (pipeline->state) {
    CFRelease(pipeline->state);
    pipeline->state = NULL;
  }

  free(pipeline);
}

/* ============================================================================
 * PIPELINE CACHE
 * ============================================================================
 */

void metal_pipeline_cache_init(metal_pipeline_cache_t *cache) {
  if (!cache) {
    return;
  }

  memset(cache, 0, sizeof(metal_pipeline_cache_t));
}

void metal_pipeline_cache_shutdown(metal_pipeline_cache_t *cache) {
  if (!cache) {
    return;
  }

  for (uint32_t i = 0; i < cache->count; i++) {
    metal_pipeline_cache_entry_t *entry = &cache->entries[i];

    if (entry->in_use && entry->pipeline) {
      if (entry->is_compute) {
        metal_destroy_compute_pipeline(
            (metal_compute_pipeline_t *)entry->pipeline);
      } else {
        metal_destroy_render_pipeline(
            (metal_render_pipeline_t *)entry->pipeline);
      }
      entry->pipeline = NULL;
      entry->in_use = false;
    }
  }

  cache->count = 0;
}

metal_render_pipeline_t *
metal_pipeline_cache_get_render(metal_pipeline_cache_t *cache,
                                MTLDeviceRef device,
                                const metal_render_pipeline_desc_t *desc) {
  if (!cache || !device || !desc) {
    return NULL;
  }

  uint64_t hash = metal_hash_render_pipeline_desc(desc);

  // Search for existing pipeline
  for (uint32_t i = 0; i < cache->count; i++) {
    metal_pipeline_cache_entry_t *entry = &cache->entries[i];

    if (entry->in_use && !entry->is_compute && entry->hash == hash) {
      return (metal_render_pipeline_t *)entry->pipeline;
    }
  }

  // Create new pipeline
  metal_render_pipeline_t *pipeline =
      metal_create_render_pipeline(device, desc);
  if (!pipeline) {
    return NULL;
  }

  // Add to cache if space available
  if (cache->count < METAL_PIPELINE_CACHE_SIZE) {
    metal_pipeline_cache_entry_t *entry = &cache->entries[cache->count++];
    entry->hash = hash;
    entry->pipeline = pipeline;
    entry->is_compute = false;
    entry->in_use = true;
  }

  return pipeline;
}

metal_compute_pipeline_t *
metal_pipeline_cache_get_compute(metal_pipeline_cache_t *cache,
                                 MTLDeviceRef device,
                                 const metal_compute_pipeline_desc_t *desc) {
  if (!cache || !device || !desc) {
    return NULL;
  }

  uint64_t hash = metal_hash_compute_pipeline_desc(desc);

  // Search for existing pipeline
  for (uint32_t i = 0; i < cache->count; i++) {
    metal_pipeline_cache_entry_t *entry = &cache->entries[i];

    if (entry->in_use && entry->is_compute && entry->hash == hash) {
      return (metal_compute_pipeline_t *)entry->pipeline;
    }
  }

  // Create new pipeline
  metal_compute_pipeline_t *pipeline =
      metal_create_compute_pipeline(device, desc);
  if (!pipeline) {
    return NULL;
  }

  // Add to cache if space available
  if (cache->count < METAL_PIPELINE_CACHE_SIZE) {
    metal_pipeline_cache_entry_t *entry = &cache->entries[cache->count++];
    entry->hash = hash;
    entry->pipeline = pipeline;
    entry->is_compute = true;
    entry->in_use = true;
  }

  return pipeline;
}

/* ============================================================================
 * UTILITIES - HASHING
 * ============================================================================
 */

// Simple FNV-1a hash
static uint64_t fnv1a_hash(const void *data, size_t size) {
  const uint64_t FNV_OFFSET = 14695981039346656037ULL;
  const uint64_t FNV_PRIME = 1099511628211ULL;

  uint64_t hash = FNV_OFFSET;
  const uint8_t *bytes = (const uint8_t *)data;

  for (size_t i = 0; i < size; i++) {
    hash ^= bytes[i];
    hash *= FNV_PRIME;
  }

  return hash;
}

uint64_t
metal_hash_render_pipeline_desc(const metal_render_pipeline_desc_t *desc) {
  if (!desc) {
    return 0;
  }

  // Hash key components
  struct {
    void *vertex_func;
    void *fragment_func;
    uint32_t color_format;
    uint32_t depth_format;
    uint32_t cull_mode;
    uint32_t fill_mode;
    uint32_t blend_enabled;
  } key = {.vertex_func = desc->vertex_function,
           .fragment_func = desc->fragment_function,
           .color_format = desc->color_format,
           .depth_format = desc->depth_format,
           .cull_mode = desc->cull_mode,
           .fill_mode = desc->fill_mode,
           .blend_enabled = desc->blend_state.blend_enabled};

  return fnv1a_hash(&key, sizeof(key));
}

uint64_t
metal_hash_compute_pipeline_desc(const metal_compute_pipeline_desc_t *desc) {
  if (!desc) {
    return 0;
  }

  // Hash key components
  struct {
    void *compute_func;
    uint32_t threadgroup_x;
    uint32_t threadgroup_y;
    uint32_t threadgroup_z;
  } key = {.compute_func = desc->compute_function,
           .threadgroup_x = desc->threadgroup_size_x,
           .threadgroup_y = desc->threadgroup_size_y,
           .threadgroup_z = desc->threadgroup_size_z};

  return fnv1a_hash(&key, sizeof(key));
}

/* End of mtl_pipeline.m */
