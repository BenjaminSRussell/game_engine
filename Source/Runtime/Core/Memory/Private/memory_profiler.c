#include "memory_private.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Logging macros (placeholders, should integrate with actual logging system)
#define LOG_CORE_INFO(...)                                                     \
  printf(__VA_ARGS__);                                                         \
  printf("\n")
#define LOG_CORE_WARNING(...)                                                  \
  printf(__VA_ARGS__);                                                         \
  printf("\n")

void memory_profile_start(const char *name) {
  if (!name)
    return;

  pthread_mutex_lock(&g_unified_memory.profiles_mutex);

  // Find existing profile or create new one
  memory_profile_t *profile = NULL;
  for (uint32_t i = 0; i < g_unified_memory.profile_count; i++) {
    if (strcmp(g_unified_memory.profiles[i].name, name) == 0) {
      profile = &g_unified_memory.profiles[i];
      break;
    }
  }

  if (!profile) {
    // Expand profiles array
    g_unified_memory.profiles = realloc(
        g_unified_memory.profiles,
        sizeof(memory_profile_t) * (g_unified_memory.profile_count + 1));
    profile = &g_unified_memory.profiles[g_unified_memory.profile_count];
    memset(profile, 0, sizeof(memory_profile_t));
    profile->name = strdup(name);
    g_unified_memory.profile_count++;
  }

  profile->start_time = get_timestamp_ms();
  profile->active = true;

  // Get current stats
  unified_memory_stats_t current_stats;
  unified_memory_get_stats(&current_stats);

  profile->bytes_allocated = current_stats.total_allocated;
  profile->bytes_freed = current_stats.total_freed;
  profile->allocations_count = current_stats.total_allocations;
  profile->deallocations_count = current_stats.total_deallocations;

  pthread_mutex_unlock(&g_unified_memory.profiles_mutex);
}

void memory_profile_end(const char *name) {
  if (!name)
    return;

  pthread_mutex_lock(&g_unified_memory.profiles_mutex);

  for (uint32_t i = 0; i < g_unified_memory.profile_count; i++) {
    if (strcmp(g_unified_memory.profiles[i].name, name) == 0) {
      memory_profile_t *profile = &g_unified_memory.profiles[i];
      profile->end_time = get_timestamp_ms();
      profile->active = false;

      // Calculate deltas
      unified_memory_stats_t current_stats;
      unified_memory_get_stats(&current_stats);

      profile->bytes_allocated =
          current_stats.total_allocated - profile->bytes_allocated;
      profile->bytes_freed = current_stats.total_freed - profile->bytes_freed;
      profile->allocations_count =
          current_stats.total_allocations - profile->allocations_count;
      profile->deallocations_count =
          current_stats.total_deallocations - profile->deallocations_count;

      break;
    }
  }

  pthread_mutex_unlock(&g_unified_memory.profiles_mutex);
}

void memory_profile_dump(const char *name) {
  if (!name)
    return;

  pthread_mutex_lock(&g_unified_memory.profiles_mutex);

  for (uint32_t i = 0; i < g_unified_memory.profile_count; i++) {
    if (strcmp(g_unified_memory.profiles[i].name, name) == 0) {
      memory_profile_t *profile = &g_unified_memory.profiles[i];

      LOG_CORE_INFO("=== Memory Profile: %s ===", profile->name);
      LOG_CORE_INFO("Duration: %" PRIu64 " ms",
                    profile->end_time - profile->start_time);
      LOG_CORE_INFO("Bytes allocated: %zu", profile->bytes_allocated);
      LOG_CORE_INFO("Bytes freed: %zu", profile->bytes_freed);
      LOG_CORE_INFO("Allocations: %u", profile->allocations_count);
      LOG_CORE_INFO("Deallocations: %u", profile->deallocations_count);

      break;
    }
  }

  pthread_mutex_unlock(&g_unified_memory.profiles_mutex);
}

void unified_memory_dump_stats(void) {
  unified_memory_stats_t stats;
  unified_memory_get_stats(&stats);

  LOG_CORE_INFO("=== Memory Statistics ===");
  LOG_CORE_INFO("Total allocations: %" PRIu64, stats.total_allocations);
  LOG_CORE_INFO("Total deallocations: %" PRIu64, stats.total_deallocations);
  LOG_CORE_INFO("Current allocations: %" PRIu64, stats.current_allocations);
  LOG_CORE_INFO("Total allocated: %zu bytes", stats.total_allocated);
  LOG_CORE_INFO("Total freed: %zu bytes", stats.total_freed);
  LOG_CORE_INFO("Current allocated: %zu bytes", stats.current_allocated);
  LOG_CORE_INFO("Peak allocated: %zu bytes", stats.peak_allocated);
  LOG_CORE_INFO("Allocation failures: %" PRIu64, stats.allocation_failures);
  LOG_CORE_INFO("Reallocations: %" PRIu64, stats.reallocations);
  LOG_CORE_INFO("Fragmentation ratio: %.2f%%",
                stats.fragmentation_ratio * 100.0f);
  LOG_CORE_INFO("Largest allocation: %zu bytes", stats.largest_allocation);
  LOG_CORE_INFO("Smallest allocation: %zu bytes", stats.smallest_allocation);
  LOG_CORE_INFO("Average allocation: %.2f bytes",
                stats.average_allocation_size);

  LOG_CORE_INFO("Strategy breakdown:");
  LOG_CORE_INFO("  Tracked: %" PRIu64, stats.tracked_allocations);
  LOG_CORE_INFO("  Pooled: %" PRIu64, stats.pooled_allocations);
  LOG_CORE_INFO("  Arena: %" PRIu64, stats.arena_allocations);
  LOG_CORE_INFO("  Stack: %" PRIu64, stats.stack_allocations);
  LOG_CORE_INFO("  GPU: %" PRIu64, stats.gpu_allocations);

  LOG_CORE_INFO("Flag breakdown:");
  LOG_CORE_INFO("  Zeroed: %" PRIu64, stats.zeroed_allocations);
  LOG_CORE_INFO("  Aligned: %" PRIu64, stats.aligned_allocations);
}

void unified_memory_dump_leaks(void) {
  pthread_mutex_lock(&g_unified_memory.allocations_mutex);

  if (!g_unified_memory.allocations) {
    LOG_CORE_INFO("No memory leaks detected");
    pthread_mutex_unlock(&g_unified_memory.allocations_mutex);
    return;
  }

  LOG_CORE_WARNING("=== Memory Leaks Detected ===");
  LOG_CORE_WARNING("Total leaked allocations: %zu",
                   (size_t)g_unified_memory.stats.current_allocations);

  unified_memory_allocation_t *current = g_unified_memory.allocations;
  size_t total_leaked = 0;
  uint32_t leak_count = 0;

  while (current) {
    total_leaked += current->size;
    leak_count++;

    LOG_CORE_WARNING("Leak #%u: %zu bytes at %p (%s:%d in %s) - Type: %s",
                     current->allocation_id, current->size, current->ptr,
                     current->file, current->line, current->function,
                     current->type);

    current = current->next;
  }

  LOG_CORE_WARNING("Total leaked memory: %zu bytes in %u allocations",
                   total_leaked, leak_count);

  pthread_mutex_unlock(&g_unified_memory.allocations_mutex);
}
