#include "core/memory/frame_allocator.h"
#include "core/memory/memory_macros.h"
#include "core/thread_pool.h"
#include "core/threading/parallel_utils.h"
#include "ecs/ecs.h"
#include "engine/include/core/logger.h"
#include <string.h>

// ============================================================================
// PARALLEL SYSTEM EXECUTION
// ============================================================================

// Job data for parallel system execution
typedef struct {
  System *system;
  World *world;
  f32 delta_time;
  u32 start_index;
  u32 end_index;
} ParallelSystemJob;

// Execute a batch of entities for a system
static bool execute_system_batch(void *data) {
  ParallelSystemJob *job = (ParallelSystemJob *)data;

  // Create system context
  SystemContext ctx = {.world = job->world,
                       .query = NULL,
                       .delta_time = job->delta_time,
                       .user_data = NULL};

  // Execute system (it will process its own query)
  job->system->execute(&ctx);

  return true;
}

void ecs_world_execute_parallel(World *world, f32 delta_time) {
  if (!world || !world->systems)
    return;

  SystemData *sd = (SystemData *)world->systems;
  if (sd->count == 0)
    return;

  ThreadPool *pool = thread_pool_get_global();
  if (!pool) {
    // No thread pool, fall back to serial execution
    ecs_world_execute_systems(world, delta_time);
    return;
  }

  // Separate serial and parallel systems
  u32 serial_count = 0;
  u32 parallel_count = 0;

  for (u32 i = 0; i < sd->count; i++) {
    if (sd->systems[i]->parallel) {
      parallel_count++;
    } else {
      serial_count++;
    }
  }

  // Execute serial systems first (in priority order)
  for (u32 i = 0; i < sd->count; i++) {
    System *sys = sd->systems[i];
    if (!sys->enabled || sys->parallel)
      continue;

    SystemContext ctx = {.world = world,
                         .query = NULL,
                         .delta_time = delta_time,
                         .user_data = NULL};

    u64 start_time = get_time_ns();
    sys->execute(&ctx);
    u64 end_time = get_time_ns();

    sys->execution_time = (end_time - start_time) / 1000000.0;
    sys->execution_count++;
  }

  // Execute parallel systems concurrently
  if (parallel_count > 0) {
    ParallelSystemJob *jobs =
        FRAME_ALLOC_ARRAY(ParallelSystemJob, parallel_count);
    u32 *job_ids = FRAME_ALLOC_ARRAY(u32, parallel_count);

    u32 job_idx = 0;
    for (u32 i = 0; i < sd->count; i++) {
      System *sys = sd->systems[i];
      if (!sys->enabled || !sys->parallel)
        continue;

      jobs[job_idx].system = sys;
      jobs[job_idx].world = world;
      jobs[job_idx].delta_time = delta_time;
      jobs[job_idx].start_index = 0;
      jobs[job_idx].end_index = 0;

      char job_name[64];
      snprintf(job_name, sizeof(job_name), "System:%s", sys->name);

      u64 start_time = get_time_ns();
      job_ids[job_idx] = thread_pool_submit(pool, execute_system_batch,
                                            &jobs[job_idx], job_name);

      job_idx++;
    }

    // Wait for all parallel systems to complete
    for (u32 i = 0; i < parallel_count; i++) {
      thread_pool_wait_for_job(pool, job_ids[i], 10000); // 10 second timeout

      // Update statistics
      jobs[i].system->execution_count++;
    }
  }
}

// ============================================================================
// PARALLEL QUERY ITERATION
// ============================================================================

typedef struct {
  Query *query;
  void (*process_func)(Entity entity, void **components, void *user_data);
  void *user_data;
  World *world;
} ParallelQueryContext;

// Process a single entity in parallel
static void process_entity_parallel(u32 index, void *user_data) {
  ParallelQueryContext *ctx = (ParallelQueryContext *)user_data;

  // Note: This is a simplified version
  // In a real implementation, we'd need to:
  // 1. Map index to archetype + entity
  // 2. Get component pointers
  // 3. Call process_func

  // For now, this demonstrates the structure
  // Full implementation would require archetype iteration
}

void ecs_query_foreach_parallel(World *world, Query *query,
                                void (*process_func)(Entity, void **, void *),
                                void *user_data) {
  if (!world || !query || !process_func)
    return;

  // Count total entities in query
  u32 total_entities = ecs_query_count(query);
  if (total_entities == 0)
    return;

  // Create context
  ParallelQueryContext ctx = {.query = query,
                              .process_func = process_func,
                              .user_data = user_data,
                              .world = world};

  // Execute in parallel
  parallel_for(0, total_entities, process_entity_parallel, &ctx);
}

// ============================================================================
// PARALLEL COMPONENT OPERATIONS
// ============================================================================

typedef struct {
  void *component_array;
  size_t component_size;
  void (*update_func)(void *component, f32 delta_time);
  f32 delta_time;
} ParallelComponentContext;

static void update_component_batch(u32 index, void *user_data) {
  ParallelComponentContext *ctx = (ParallelComponentContext *)user_data;

  void *component = (u8 *)ctx->component_array + (index * ctx->component_size);
  ctx->update_func(component, ctx->delta_time);
}

void ecs_update_components_parallel(void *component_array,
                                    size_t component_size, u32 count,
                                    f32 delta_time,
                                    void (*update_func)(void *, f32)) {
  if (!component_array || !update_func || count == 0)
    return;

  ParallelComponentContext ctx = {.component_array = component_array,
                                  .component_size = component_size,
                                  .update_func = update_func,
                                  .delta_time = delta_time};

  parallel_for(0, count, update_component_batch, &ctx);
}

// ============================================================================
// WORKER THREAD CONFIGURATION
// ============================================================================

void ecs_world_set_worker_threads(World *world, u32 thread_count) {
  if (!world)
    return;

  // Initialize thread pool if not already done
  if (!thread_pool_get_global()) {
    thread_pool_init(thread_count);
    LOG_INFO("[ECS] Thread pool initialized with %u threads", thread_count);
  }
}

// ============================================================================
// QUERY COUNT (Helper)
// ============================================================================

u32 ecs_query_count(Query *query) {
  if (!query)
    return 0;

  u32 total = 0;
  for (u32 i = 0; i < query->archetype_count; i++) {
    Archetype *arch = query->matching_archetypes[i];
    if (arch) {
      // Cast to SimpleArchetype to access entity_count
      SimpleArchetype *simple_arch = (SimpleArchetype *)arch;
      total += simple_arch->entity_count;
    }
  }

  return total;
}

u32 ecs_query_archetype_count(Query *query) {
  return query ? query->archetype_count : 0;
}

void ecs_query_reset(Query *query) {
  if (!query)
    return;
  query->current_archetype = 0;
  query->current_entity = 0;
}
