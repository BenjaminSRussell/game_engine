/*
 * mtl_pipeline_extensions.m
 * Additional pipeline functionality
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_pipeline.h"
#import \u003cFoundation / Foundation.h\u003e
#import \u003cMetal / Metal.h\u003e
#include \u003cstdio.h\u003e
#include \u003cstring.h\u003e

/* ============================================================================
 * RENDER PIPELINE STATISTICS
 * ============================================================================
 */

metal_render_pipeline_stats_t
metal_render_pipeline_get_stats(const metal_render_pipeline_t *pipeline) {
  metal_render_pipeline_stats_t stats = {0};

  if (!pipeline || !pipeline -\u003estate) {
    return stats;
  }

  id\u003cMTLRenderPipelineState\u003e state =
      (__bridge id\u003cMTLRenderPipelineState\u003e)pipeline -\u003estate;

  // Get stats from Metal pipeline state
  stats.has_vertex_shader = ([state vertexFunction] != nil);
  stats.has_fragment_shader = ([state fragmentFunction] != nil);
  stats.uses_depth_testing = (pipeline -\u003edepth_stencil != NULL);
  stats.uses_blending = true; // Would need to track from descriptor

  // Estimate memory (rough approximation based on typical PSO sizes)
  stats.estimated_memory_bytes = 8192; // Base PSO size

  // Add memory for shader functions
  if (stats.has_vertex_shader) {
    stats.estimated_memory_bytes += 4096;
  }
  if (stats.has_fragment_shader) {
    stats.estimated_memory_bytes += 4096;
  }

  stats.creation_time_ns = 0; // Would need timing during creation

  return stats;
}

/* ============================================================================
 * COMPUTE PIPELINE VALIDATION
 * ============================================================================
 */

bool metal_compute_pipeline_validate(MTLDeviceRef device_ref,
                                     const metal_compute_pipeline_desc_t *desc,
                                     char *error_message,
                                     size_t error_message_size) {
  if (!device_ref || !desc) {
    if (error_message \u0026\u0026 error_message_size \u003e 0) {
      snprintf(error_message, error_message_size, "Invalid arguments");
    }
    return false;
  }

  if (!desc -\u003ecompute_function) {
    if (error_message \u0026\u0026 error_message_size \u003e 0) {
      snprintf(error_message, error_message_size,
               "Compute function is required");
    }
    return false;
  }

  id\u003cMTLDevice\u003e device = (__bridge id\u003cMTLDevice\u003e)device_ref;
  id\u003cMTLFunction\u003e computeFunc =
      (__bridge id\u003cMTLFunction\u003e)desc -\u003ecompute_function;

  // Validate function type
  if (computeFunc.functionType != MTLFunctionTypeKernel) {
    if (error_message \u0026\u0026 error_message_size \u003e 0) {
      snprintf(error_message, error_message_size,
               "Function is not a compute kernel");
    }
    return false;
  }

  // Get max threadgroup size from device
  NSUInteger maxThreadsPerThreadgroup = device.maxThreadsPerThreadgroup.width *
                                        device.maxThreadsPerThreadgroup.height *
                                        device.maxThreadsPerThreadgroup.depth;

  // Calculate total threads in requested threadgroup
  uint32_t total_threads =
      desc -\u003ethreadgroup_size_x * desc -\u003ethreadgroup_size_y *
                                                 desc -\u003ethreadgroup_size_z;

  if (total_threads \u003e maxThreadsPerThreadgroup) {
    if (error_message \u0026\u0026 error_message_size \u003e 0) {
      snprintf(error_message, error_message_size,
               "Threadgroup size %u exceeds device maximum %lu", total_threads,
               (unsigned long)maxThreadsPerThreadgroup);
    }
    return false;
  }

  // Validate individual dimensions
  if (desc -\u003ethreadgroup_size_x \u003e device.maxThreadsPerThreadgroup
                .width) {
    if (error_message \u0026\u0026 error_message_size \u003e 0) {
      snprintf(error_message, error_message_size,
               "Threadgroup width %u exceeds maximum %lu",
               desc -\u003ethreadgroup_size_x,
               (unsigned long)device.maxThreadsPerThreadgroup.width);
    }
    return false;
  }

  if (desc -\u003ethreadgroup_size_y \u003e device.maxThreadsPerThreadgroup
                .height) {
    if (error_message \u0026\u0026 error_message_size \u003e 0) {
      snprintf(error_message, error_message_size,
               "Threadgroup height %u exceeds maximum %lu",
               desc -\u003ethreadgroup_size_y,
               (unsigned long)device.maxThreadsPerThreadgroup.height);
    }
    return false;
  }

  if (desc -\u003ethreadgroup_size_z \u003e device.maxThreadsPerThreadgroup
                .depth) {
    if (error_message \u0026\u0026 error_message_size \u003e 0) {
      snprintf(error_message, error_message_size,
               "Threadgroup depth %u exceeds maximum %lu",
               desc -\u003ethreadgroup_size_z,
               (unsigned long)device.maxThreadsPerThreadgroup.depth);
    }
    return false;
  }

  return true;
}

uint32_t metal_get_max_threads_per_threadgroup(MTLDeviceRef device_ref) {
  if (!device_ref) {
    return 0;
  }

  id\u003cMTLDevice\u003e device = (__bridge id\u003cMTLDevice\u003e)device_ref;
  NSUInteger maxThreads = device.maxThreadsPerThreadgroup.width *
                          device.maxThreadsPerThreadgroup.height *
                          device.maxThreadsPerThreadgroup.depth;

  return (uint32_t)maxThreads;
}

metal_compute_pipeline_stats_t
metal_compute_pipeline_get_stats(const metal_compute_pipeline_t *pipeline) {
  metal_compute_pipeline_stats_t stats = {0};

  if (!pipeline || !pipeline -\u003estate) {
    return stats;
  }

  id\u003cMTLComputePipelineState\u003e state =
      (__bridge id\u003cMTLComputePipelineState\u003e)pipeline -\u003estate;

  // Get max total threads per threadgroup from pipeline state
  stats.max_total_threads_per_threadgroup =
      (uint32_t)[state maxTotalThreadsPerThreadgroup];

  // Estimate memory
  stats.estimated_memory_bytes = 6144; // Base estimate for compute PSO

  stats.creation_time_ns = 0; // Would need timing during creation

  return stats;
}

/* ============================================================================
 * PIPELINE CACHE INVALIDATION
 * ============================================================================
 */

bool metal_pipeline_cache_invalidate_by_hash(metal_pipeline_cache_t *cache,
                                             uint64_t hash) {
  if (!cache) {
    return false;
  }

  for (uint32_t i = 0; i \u003c cache -\u003ecount; i++) {
    metal_pipeline_cache_entry_t *entry = \u0026cache -\u003eentries[i];

    if (entry -\u003ein_use \u0026\u0026 entry -\u003ehash == hash) {
      // Destroy the pipeline
      if (entry -\u003eis_compute) {
        metal_destroy_compute_pipeline(
            (metal_compute_pipeline_t *)entry -\u003epipeline);
      } else {
        metal_destroy_render_pipeline(
            (metal_render_pipeline_t *)entry -\u003epipeline);
      }

      // Update stats
      cache -\u003estats.total_memory_bytes -= entry -\u003ememory_size;
      cache -\u003estats.total_pipelines--;

      // Mark as unused
      entry -\u003epipeline = NULL;
      entry -\u003ein_use = false;

      return true;
    }
  }

  return false; // Not found
}

void metal_pipeline_cache_invalidate_all(metal_pipeline_cache_t *cache) {
  if (!cache) {
    return;
  }

  // Destroy all pipelines
  for (uint32_t i = 0; i \u003c cache -\u003ecount; i++) {
    metal_pipeline_cache_entry_t *entry = \u0026cache -\u003eentries[i];

    if (entry -\u003ein_use \u0026\u0026 entry -\u003epipeline) {
      if (entry -\u003eis_compute) {
        metal_destroy_compute_pipeline(
            (metal_compute_pipeline_t *)entry -\u003epipeline);
      } else {
        metal_destroy_render_pipeline(
            (metal_render_pipeline_t *)entry -\u003epipeline);
      }

      entry -\u003epipeline = NULL;
      entry -\u003ein_use = false;
    }
  }

  // Reset cache state
  cache -\u003ecount = 0;
  cache -\u003estats.total_pipelines = 0;
  cache -\u003estats.total_memory_bytes = 0;

  NSLog(@"Pipeline cache invalidated - all entries cleared");
}

/* End of mtl_pipeline_extensions.m */
