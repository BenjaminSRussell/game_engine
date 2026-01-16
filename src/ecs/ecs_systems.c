#include "core/memory/memory_macros.h"
#include "core/memory/unified_memory_allocator.h"
#include "ecs/ecs.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// SYSTEM MANAGEMENT
// ============================================================================

// System storage in world
typedef struct {
  System **systems;
  u32 count;
  u32 capacity;
} SystemData;

System *ecs_system_create(World *world, const char *name, SystemFunc execute,
                          const QueryDesc *query) {
  if (!world || !name || !execute) {
    LOG_ERROR("[ECS] Invalid parameters for system creation");
    return NULL;
  }

  // Initialize system storage if needed
  if (!world->systems) {
    SystemData *sd = ALLOC_TYPE_ZERO(SystemData);
    sd->capacity = world->config.max_systems;
    sd->systems = ALLOC_ARRAY_ZERO(System *, sd->capacity);
    world->systems = sd;
  }

  SystemData *sd = (SystemData *)world->systems;

  if (sd->count >= sd->capacity) {
    LOG_ERROR("[ECS] Max systems reached (%u)", sd->capacity);
    return NULL;
  }

  // Allocate system
  System *system = ALLOC_TYPE_ZERO(System);
  system->name = strdup(name);
  system->execute = execute;

  // Copy query descriptor
  if (query) {
    system->query = *query;

    // Deep copy component arrays
    if (query->all_count > 0) {
      system->query.all_components =
          ALLOC_ARRAY(ComponentType, query->all_count);
      memcpy(system->query.all_components, query->all_components,
             query->all_count * sizeof(ComponentType));
    }
    if (query->any_count > 0) {
      system->query.any_components =
          ALLOC_ARRAY(ComponentType, query->any_count);
      memcpy(system->query.any_components, query->any_components,
             query->any_count * sizeof(ComponentType));
    }
    if (query->none_count > 0) {
      system->query.none_components =
          ALLOC_ARRAY(ComponentType, query->none_count);
      memcpy(system->query.none_components, query->none_components,
             query->none_count * sizeof(ComponentType));
    }
  }

  // Default configuration
  system->priority = 100;
  system->enabled = true;
  system->parallel = false;
  system->thread_affinity = (u32)-1;
  system->execution_time = 0.0;
  system->execution_count = 0;

  // Add to world
  sd->systems[sd->count++] = system;
  world->system_count++;

  LOG_INFO("[ECS] System created: %s (priority: %u)", name, system->priority);
  return system;
}

void ecs_system_destroy(World *world, System *system) {
  if (!world || !system)
    return;

  SystemData *sd = (SystemData *)world->systems;
  if (!sd)
    return;

  // Find and remove system
  for (u32 i = 0; i < sd->count; i++) {
    if (sd->systems[i] == system) {
      // Free query arrays
      if (system->query.all_components)
        FREE(system->query.all_components);
      if (system->query.any_components)
        FREE(system->query.any_components);
      if (system->query.none_components)
        FREE(system->query.none_components);

      // Free name
      if (system->name)
        free((void *)system->name);

      // Free system
      FREE(system);

      // Swap-remove
      sd->systems[i] = sd->systems[--sd->count];
      world->system_count--;
      return;
    }
  }
}

void ecs_system_set_priority(World *world, System *system, u32 priority) {
  if (system) {
    system->priority = priority;
    // Trigger re-sort on next update
  }
}

void ecs_system_set_parallel(World *world, System *system, bool parallel) {
  if (system) {
    system->parallel = parallel;
  }
}

void ecs_system_set_enabled(World *world, System *system, bool enabled) {
  if (system) {
    system->enabled = enabled;
  }
}

// ============================================================================
// SYSTEM EXECUTION
// ============================================================================

// Comparison function for system priority sorting
static int compare_system_priority(const void *a, const void *b) {
  const System *sys_a = *(const System **)a;
  const System *sys_b = *(const System **)b;

  // Lower priority number = higher priority (executes first)
  if (sys_a->priority < sys_b->priority)
    return -1;
  if (sys_a->priority > sys_b->priority)
    return 1;
  return 0;
}

void ecs_world_rebuild_execution_order(World *world) {
  if (!world || !world->systems)
    return;

  SystemData *sd = (SystemData *)world->systems;
  if (sd->count == 0)
    return;

  // Sort systems by priority
  qsort(sd->systems, sd->count, sizeof(System *), compare_system_priority);

  LOG_INFO("[ECS] Execution order rebuilt (%u systems)", sd->count);
}

static inline u64 get_time_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
}

void ecs_world_execute_systems(World *world, f32 delta_time) {
  if (!world || !world->systems)
    return;

  SystemData *sd = (SystemData *)world->systems;
  if (sd->count == 0)
    return;

  // Execute systems in priority order
  for (u32 i = 0; i < sd->count; i++) {
    System *sys = sd->systems[i];

    if (!sys->enabled)
      continue;

    // Skip parallel systems for now (Phase 4.3)
    if (sys->parallel)
      continue;

    // Create system context
    SystemContext ctx = {.world = world,
                         .query = NULL, // System creates its own query
                         .delta_time = delta_time,
                         .user_data = NULL};

    // Measure execution time
    u64 start_time = get_time_ns();

    // Execute system
    sys->execute(&ctx);

    u64 end_time = get_time_ns();

    // Update statistics
    f64 execution_time_ms = (end_time - start_time) / 1000000.0;
    sys->execution_time = execution_time_ms;
    sys->execution_count++;
  }
}

void ecs_world_update(World *world, f32 delta_time) {
  if (!world)
    return;

  // Execute all systems
  ecs_world_execute_systems(world, delta_time);
}

// ============================================================================
// PROFILING AND DEBUGGING
// ============================================================================

void ecs_world_print_performance_report(World *world) {
  if (!world || !world->systems) {
    printf("No systems to report\n");
    return;
  }

  SystemData *sd = (SystemData *)world->systems;

  printf("\n=== ECS Performance Report ===\n");
  printf("Total Systems: %u\n\n", sd->count);

  printf("%-30s %10s %10s %10s\n", "System", "Calls", "Last (ms)", "Avg (ms)");
  printf("%-30s %10s %10s %10s\n", "------", "-----", "---------", "--------");

  for (u32 i = 0; i < sd->count; i++) {
    System *sys = sd->systems[i];
    f64 avg_time = sys->execution_count > 0 ? sys->execution_time : 0.0;

    printf("%-30s %10u %10.3f %10.3f\n", sys->name, sys->execution_count,
           sys->execution_time, avg_time);
  }

  printf("==============================\n\n");
}

WorldStats ecs_world_get_stats(World *world) {
  WorldStats stats = {0};

  if (!world)
    return stats;

  stats.entity_count = world->entity_count;
  stats.component_type_count = world->component_count;
  stats.system_count = world->system_count;

  // Count archetypes
  if (world->archetypes) {
    ArchetypeData *ad = (ArchetypeData *)world->archetypes;
    stats.archetype_count = ad->count;
  }

  return stats;
}

void ecs_world_print_stats(World *world) {
  WorldStats stats = ecs_world_get_stats(world);

  printf("\n=== ECS World Statistics ===\n");
  printf("Entities: %u\n", stats.entity_count);
  printf("Archetypes: %u\n", stats.archetype_count);
  printf("Component Types: %u\n", stats.component_type_count);
  printf("Systems: %u\n", stats.system_count);
  printf("===========================\n\n");
}
