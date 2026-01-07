/*
 * mtl_pipeline_extensions.m
 * Additional pipeline functionality
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "backend/metal/mtl_pipeline.h"
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * RENDER PIPELINE STATISTICS
 * ============================================================================
 */

metal_render_pipeline_stats_t
metal_render_pipeline_get_stats(const metal_render_pipeline_t *pipeline) {
  metal_render_pipeline_stats_t stats = {0};

  if (!pipeline || !pipeline->state) {
    return stats;
  }

  id<MTLRenderPipelineState> state =
      (__bridge id<MTLRenderPipelineState>)pipeline->state;

  /* Get stats from Metal pipeline state -- simplistic check */
  stats.has_vertex_shader = (state != nil);
  /* Note: In Metal, functions are baked into PSO. We can't query them back
     easily from id<MTLRenderPipelineState> in all versions. Simplified for now.
   */
  stats.has_fragment_shader = (state != nil);
  stats.uses_depth_testing = (pipeline->depth_stencil != NULL);
  stats.uses_blending = true; /* Would need to track from descriptor */

  /* Estimate memory (rough approximation based on typical PSO sizes) */
  stats.estimated_memory_bytes = 8192; /* Base PSO size */

  stats.creation_time_ns = 0;

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
    if (error_message && error_message_size > 0) {
      snprintf(error_message, error_message_size, "Invalid arguments");
    }
    return false;
  }

  if (!desc->compute_function) {
    if (error_message && error_message_size > 0) {
      snprintf(error_message, error_message_size,
               "Compute function is required");
    }
    return false;
  }

  id<MTLDevice> device = (__bridge id<MTLDevice>)device_ref;
  id<MTLFunction> computeFunc =
      (__bridge id<MTLFunction>)desc->compute_function;

  /* Validate function type */
  if (computeFunc.functionType != MTLFunctionTypeKernel) {
    if (error_message && error_message_size > 0) {
      snprintf(error_message, error_message_size,
               "Function is not a compute kernel");
    }
    return false;
  }

  /* Get max threadgroup size from device */
  MTLSize maxThreadsPerThreadgroup = device.maxThreadsPerThreadgroup;
  NSUInteger maxThreads = maxThreadsPerThreadgroup.width *
                          maxThreadsPerThreadgroup.height *
                          maxThreadsPerThreadgroup.depth;

  /* Calculate total threads in requested threadgroup */
  uint32_t total_threads = desc->threadgroup_size_x * desc->threadgroup_size_y *
                           desc->threadgroup_size_z;

  if (total_threads > maxThreads) {
    if (error_message && error_message_size > 0) {
      snprintf(error_message, error_message_size,
               "Threadgroup size %u exceeds device maximum %lu", total_threads,
               (unsigned long)maxThreads);
    }
    return false;
  }

  /* Validate individual dimensions */
  if (desc->threadgroup_size_x > maxThreadsPerThreadgroup.width) {
    if (error_message && error_message_size > 0) {
      snprintf(error_message, error_message_size,
               "Threadgroup width %u exceeds maximum %lu",
               desc->threadgroup_size_x,
               (unsigned long)maxThreadsPerThreadgroup.width);
    }
    return false;
  }

  if (desc->threadgroup_size_y > maxThreadsPerThreadgroup.height) {
    if (error_message && error_message_size > 0) {
      snprintf(error_message, error_message_size,
               "Threadgroup height %u exceeds maximum %lu",
               desc->threadgroup_size_y,
               (unsigned long)maxThreadsPerThreadgroup.height);
    }
    return false;
  }

  if (desc->threadgroup_size_z > maxThreadsPerThreadgroup.depth) {
    if (error_message && error_message_size > 0) {
      snprintf(error_message, error_message_size,
               "Threadgroup depth %u exceeds maximum %lu",
               desc->threadgroup_size_z,
               (unsigned long)maxThreadsPerThreadgroup.depth);
    }
    return false;
  }

  return true;
}

uint32_t metal_get_max_threads_per_threadgroup(MTLDeviceRef device_ref) {
  if (!device_ref) {
    return 0;
  }

  id<MTLDevice> device = (__bridge id<MTLDevice>)device_ref;
  MTLSize maxThreadsPerThreadgroup = device.maxThreadsPerThreadgroup;
  NSUInteger maxThreads = maxThreadsPerThreadgroup.width *
                          maxThreadsPerThreadgroup.height *
                          maxThreadsPerThreadgroup.depth;

  return (uint32_t)maxThreads;
}

metal_compute_pipeline_stats_t
metal_compute_pipeline_get_stats(const metal_compute_pipeline_t *pipeline) {
  metal_compute_pipeline_stats_t stats = {0};

  if (!pipeline || !pipeline->state) {
    return stats;
  }

  id<MTLComputePipelineState> state =
      (__bridge id<MTLComputePipelineState>)pipeline->state;

  /* Get max total threads per threadgroup from pipeline state */
  stats.max_total_threads_per_threadgroup =
      (uint32_t)[state maxTotalThreadsPerThreadgroup];

  /* Estimate memory */
  stats.estimated_memory_bytes = 6144; /* Base estimate for compute PSO */

  stats.creation_time_ns = 0;

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

  for (uint32_t i = 0; i < cache->count; i++) {
    metal_pipeline_cache_entry_t *entry = &cache->entries[i];

    if (entry->in_use && entry->hash == hash) {
      /* Destroy the pipeline */
      if (entry->is_compute) {
        metal_destroy_compute_pipeline(
            (metal_compute_pipeline_t *)entry->pipeline);
      } else {
        metal_destroy_render_pipeline(
            (metal_render_pipeline_t *)entry->pipeline);
      }

      /* Update stats */
      cache->stats.total_memory_bytes -= entry->memory_size;
      cache->stats.total_pipelines--;

      /* Mark as unused */
      entry->pipeline = NULL;
      entry->in_use = false;

      return true;
    }
  }

  return false; /* Not found */
}

void metal_pipeline_cache_invalidate_all(metal_pipeline_cache_t *cache) {
  if (!cache) {
    return;
  }

  /* Destroy all pipelines */
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

  /* Reset cache state */
  cache->count = 0;
  cache->stats.total_pipelines = 0;
  cache->stats.total_memory_bytes = 0;

  /* NSLog(@"Pipeline cache invalidated - all entries cleared"); */
}
