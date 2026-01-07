/*
 * mtl_timestamp.m
 * Metal GPU timestamp query system implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#import "mtl_timestamp.h"
#import "mtl_command.h"
#import "mtl_command_internal.h"
#import <Metal/Metal.h>
#import <mach/mach_time.h>
#import <math.h>
#import <stdio.h>
#import <stdlib.h>
#import <string.h>

// ============================================================================
// Internal Structures
// ============================================================================

typedef struct metal_timestamp_query {
  uint64_t begin_timestamp;
  uint64_t end_timestamp;
  uint64_t cpu_timestamp;
  char *label;
  bool active;
  bool completed;
} metal_timestamp_query_t;

typedef struct metal_profiling_region {
  char *label;
  uint64_t sample_count;
  double min_time_ms;
  double max_time_ms;
  double total_time_ms;
  struct metal_profiling_region *next;
} metal_profiling_region_t;

struct metal_timestamp_pool {
  id<MTLDevice> device;
  id<MTLCounterSampleBuffer> sample_buffer;
  id<MTLCounterSet> timestamp_counter_set;

  metal_timestamp_query_t *queries;
  uint32_t max_queries;
  uint32_t active_query_count;
  uint32_t next_query_id;

  metal_timestamp_config_t config;
  double timer_resolution_ns;

  // Profiling statistics
  metal_profiling_region_t *regions;
};

// ============================================================================
// Helper Functions
// ============================================================================

static double mach_time_to_ns(uint64_t mach_time) {
  static mach_timebase_info_data_t timebase_info;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    mach_timebase_info(&timebase_info);
  });

  return (double)(mach_time * timebase_info.numer / timebase_info.denom);
}

static metal_profiling_region_t *
find_or_create_region(metal_timestamp_pool_t *pool, const char *label) {
  if (!pool || !label)
    return NULL;

  // Find existing region
  metal_profiling_region_t *region = pool->regions;
  while (region) {
    if (strcmp(region->label, label) == 0) {
      return region;
    }
    region = region->next;
  }

  // Create new region
  region =
      (metal_profiling_region_t *)calloc(1, sizeof(metal_profiling_region_t));
  if (!region)
    return NULL;

  region->label = strdup(label);
  region->min_time_ms = INFINITY;
  region->next = pool->regions;
  pool->regions = region;

  return region;
}

// ============================================================================
// Timestamp Pool Implementation
// ============================================================================

metal_timestamp_pool_t *
metal_timestamp_pool_create(id<MTLDevice> device,
                            const metal_timestamp_config_t *config) {
  if (!device || !config)
    return NULL;

  metal_timestamp_pool_t *pool =
      (metal_timestamp_pool_t *)calloc(1, sizeof(metal_timestamp_pool_t));
  if (!pool)
    return NULL;

  pool->device = [device retain];
  pool->config = *config;
  pool->max_queries = config->max_queries > 0 ? config->max_queries : 64;

  // Find timestamp counter set
  NSArray<id<MTLCounterSet>> *counterSets = device.counterSets;
  for (id<MTLCounterSet> counterSet in counterSets) {
    if ([counterSet.name containsString:@"timestamp"]) {
      pool->timestamp_counter_set = [counterSet retain];
      break;
    }
  }

  if (!pool->timestamp_counter_set) {
    // Fallback: use first available counter set
    if (counterSets.count > 0) {
      pool->timestamp_counter_set = [counterSets[0] retain];
    } else {
      // No counters available on this device
      [pool->device release];
      free(pool);
      return NULL;
    }
  }

  // Create counter sample buffer
  MTLCounterSampleBufferDescriptor *desc =
      [[MTLCounterSampleBufferDescriptor alloc] init];
  desc.counterSet = pool->timestamp_counter_set;
  desc.storageMode = MTLStorageModeShared;
  desc.sampleCount = pool->max_queries * 2; // Begin + end for each query
  desc.label = @"Timestamp Sample Buffer";

  NSError *error = nil;
  pool->sample_buffer = [device newCounterSampleBufferWithDescriptor:desc
                                                               error:&error];
  [desc release];

  if (!pool->sample_buffer) {
    NSLog(@"Failed to create counter sample buffer: %@", error);
    [pool->timestamp_counter_set release];
    [pool->device release];
    free(pool);
    return NULL;
  }

  // Allocate query array
  pool->queries = (metal_timestamp_query_t *)calloc(
      pool->max_queries, sizeof(metal_timestamp_query_t));
  if (!pool->queries) {
    [pool->sample_buffer release];
    [pool->timestamp_counter_set release];
    [pool->device release];
    free(pool);
    return NULL;
  }

  // Get timer resolution (typically ~1ns for modern GPUs)
  pool->timer_resolution_ns = 1.0;

  return pool;
}

uint32_t metal_timestamp_begin(metal_timestamp_pool_t *pool,
                               metal_command_buffer_t *cmd_buffer,
                               const char *label) {
  if (!pool || !cmd_buffer || !cmd_buffer->buffer)
    return UINT32_MAX;

  if (pool->active_query_count >= pool->max_queries) {
    NSLog(@"Timestamp pool exhausted!");
    return UINT32_MAX;
  }

  uint32_t query_id = pool->next_query_id;
  pool->next_query_id = (pool->next_query_id + 1) % pool->max_queries;

  metal_timestamp_query_t *query = &pool->queries[query_id];
  query->active = true;
  query->completed = false;

  if (label) {
    if (query->label)
      free(query->label);
    query->label = strdup(label);
  }

  if (pool->config.enable_cpu_correlation) {
    query->cpu_timestamp = mach_absolute_time();
  }

  // Encode timestamp sample
  NSUInteger sample_index = query_id * 2; // Begin sample
  [cmd_buffer->buffer sampleCountersInBuffer:pool->sample_buffer
                               atSampleIndex:sample_index
                                 withBarrier:YES];

  pool->active_query_count++;
  return query_id;
}

void metal_timestamp_end(metal_timestamp_pool_t *pool,
                         metal_command_buffer_t *cmd_buffer,
                         uint32_t query_id) {
  if (!pool || !cmd_buffer || !cmd_buffer->buffer ||
      query_id >= pool->max_queries)
    return;

  metal_timestamp_query_t *query = &pool->queries[query_id];
  if (!query->active)
    return;

  // Encode end timestamp sample
  NSUInteger sample_index = query_id * 2 + 1; // End sample
  [cmd_buffer->buffer sampleCountersInBuffer:pool->sample_buffer
                               atSampleIndex:sample_index
                                 withBarrier:YES];

  // Mark as completed when command buffer finishes
  __block metal_timestamp_pool_t *pool_ref = pool;
  __block uint32_t qid = query_id;

  [cmd_buffer->buffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
    if (qid < pool_ref->max_queries) {
      pool_ref->queries[qid].completed = true;
      pool_ref->active_query_count--;
    }
  }];
}

bool metal_timestamp_get_result(metal_timestamp_pool_t *pool, uint32_t query_id,
                                metal_timestamp_result_t *result) {
  if (!pool || !result || query_id >= pool->max_queries)
    return false;

  metal_timestamp_query_t *query = &pool->queries[query_id];
  if (!query->completed)
    return false;

  // Resolve counter data
  NSData *data =
      [pool->sample_buffer resolveCounterRange:NSMakeRange(query_id * 2, 2)];
  if (!data || data.length < 16)
    return false;

  const uint64_t *timestamps = (const uint64_t *)data.bytes;
  query->begin_timestamp = timestamps[0];
  query->end_timestamp = timestamps[1];

  result->gpu_timestamp_ns = query->end_timestamp - query->begin_timestamp;
  result->cpu_timestamp_ns = (uint64_t)mach_time_to_ns(query->cpu_timestamp);
  result->valid = true;

  // Update profiling statistics if labeled
  if (query->label) {
    metal_profiling_region_t *region =
        find_or_create_region(pool, query->label);
    if (region) {
      double time_ms = (double)result->gpu_timestamp_ns / 1000000.0;

      region->sample_count++;
      region->total_time_ms += time_ms;

      if (time_ms < region->min_time_ms)
        region->min_time_ms = time_ms;
      if (time_ms > region->max_time_ms)
        region->max_time_ms = time_ms;
    }
  }

  return true;
}

bool metal_timestamp_get_elapsed(metal_timestamp_pool_t *pool,
                                 uint32_t begin_query_id, uint32_t end_query_id,
                                 double *elapsed_ms) {
  if (!pool || !elapsed_ms)
    return false;

  metal_timestamp_result_t begin_result, end_result;

  if (!metal_timestamp_get_result(pool, begin_query_id, &begin_result))
    return false;
  if (!metal_timestamp_get_result(pool, end_query_id, &end_result))
    return false;

  uint64_t elapsed_ns =
      end_result.gpu_timestamp_ns - begin_result.gpu_timestamp_ns;
  *elapsed_ms = (double)elapsed_ns / 1000000.0;

  return true;
}

double metal_timestamp_get_resolution_ns(metal_timestamp_pool_t *pool) {
  return pool ? pool->timer_resolution_ns : 1.0;
}

void metal_timestamp_pool_reset(metal_timestamp_pool_t *pool) {
  if (!pool)
    return;

  for (uint32_t i = 0; i < pool->max_queries; i++) {
    pool->queries[i].active = false;
    pool->queries[i].completed = false;
  }

  pool->active_query_count = 0;
  pool->next_query_id = 0;
}

void metal_timestamp_pool_destroy(metal_timestamp_pool_t *pool) {
  if (!pool)
    return;

  if (pool->queries) {
    for (uint32_t i = 0; i < pool->max_queries; i++) {
      if (pool->queries[i].label) {
        free(pool->queries[i].label);
      }
    }
    free(pool->queries);
  }

  // Free profiling regions
  metal_profiling_region_t *region = pool->regions;
  while (region) {
    metal_profiling_region_t *next = region->next;
    if (region->label)
      free(region->label);
    free(region);
    region = next;
  }

  if (pool->sample_buffer) {
    [pool->sample_buffer release];
  }

  if (pool->timestamp_counter_set) {
    [pool->timestamp_counter_set release];
  }

  if (pool->device) {
    [pool->device release];
  }

  free(pool);
}

// ============================================================================
// Scoped Timestamp Implementation
// ============================================================================

metal_scoped_timestamp_t
metal_timestamp_scope_begin(metal_timestamp_pool_t *pool,
                            metal_command_buffer_t *cmd_buffer,
                            const char *label) {
  metal_scoped_timestamp_t scope = {0};
  scope.pool = pool;
  scope.label = label;
  scope.query_id = metal_timestamp_begin(pool, cmd_buffer, label);
  return scope;
}

void metal_timestamp_scope_end(metal_scoped_timestamp_t *scope,
                               metal_command_buffer_t *cmd_buffer,
                               metal_timestamp_result_t *result) {
  if (!scope || !scope->pool)
    return;

  metal_timestamp_end(scope->pool, cmd_buffer, scope->query_id);

  if (result) {
    metal_timestamp_get_result(scope->pool, scope->query_id, result);
  }
}

// ============================================================================
// Profiling Statistics Implementation
// ============================================================================

bool metal_timestamp_get_stats(metal_timestamp_pool_t *pool, const char *label,
                               metal_profiling_stats_t *stats) {
  if (!pool || !label || !stats)
    return false;

  metal_profiling_region_t *region = pool->regions;
  while (region) {
    if (strcmp(region->label, label) == 0) {
      stats->label = region->label;
      stats->sample_count = region->sample_count;
      stats->min_time_ms = region->min_time_ms;
      stats->max_time_ms = region->max_time_ms;
      stats->total_time_ms = region->total_time_ms;
      stats->avg_time_ms = region->sample_count > 0
                               ? region->total_time_ms / region->sample_count
                               : 0.0;
      return true;
    }
    region = region->next;
  }

  return false;
}

void metal_timestamp_print_stats(metal_timestamp_pool_t *pool) {
  if (!pool)
    return;

  printf("=== GPU Profiling Statistics ===\n");

  metal_profiling_region_t *region = pool->regions;
  while (region) {
    if (region->sample_count > 0) {
      double avg_ms = region->total_time_ms / region->sample_count;

      printf("%-30s | Samples: %6llu | Avg: %6.3fms | Min: %6.3fms | Max: "
             "%6.3fms | Total: %8.2fms\n",
             region->label, region->sample_count, avg_ms, region->min_time_ms,
             region->max_time_ms, region->total_time_ms);
    }
    region = region->next;
  }

  printf("================================\n");
}

void metal_timestamp_reset_stats(metal_timestamp_pool_t *pool) {
  if (!pool)
    return;

  metal_profiling_region_t *region = pool->regions;
  while (region) {
    region->sample_count = 0;
    region->min_time_ms = INFINITY;
    region->max_time_ms = 0.0;
    region->total_time_ms = 0.0;
    region = region->next;
  }
}
