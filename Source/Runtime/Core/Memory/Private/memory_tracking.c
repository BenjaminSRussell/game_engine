#include "memory_private.h"
#include <stdio.h>
#include <stdlib.h>

// Undefine macros to ensure we use valid system allocators for metadata
// and prevent infinite recursion where tracking tracks itself.
#undef malloc
#undef free
#undef calloc
#undef realloc

// Global state instance
unified_memory_state_t g_unified_memory = {0};

// Tracking Implementation

void add_allocation_to_tracking(unified_memory_allocation_t *allocation) {
  if (!allocation)
    return;

  pthread_mutex_lock(&g_unified_memory.allocations_mutex);

  // Add to front of linked list
  allocation->next = g_unified_memory.allocations;
  if (g_unified_memory.allocations) {
    g_unified_memory.allocations->prev = allocation;
  }
  g_unified_memory.allocations = allocation;

  pthread_mutex_unlock(&g_unified_memory.allocations_mutex);
}

unified_memory_allocation_t *find_allocation_metadata(void *ptr) {
  pthread_mutex_lock(&g_unified_memory.allocations_mutex);

  unified_memory_allocation_t *current = g_unified_memory.allocations;
  while (current) {
    if (current->ptr == ptr) {
      pthread_mutex_unlock(&g_unified_memory.allocations_mutex);
      return current;
    }
    current = current->next;
  }

  pthread_mutex_unlock(&g_unified_memory.allocations_mutex);
  return NULL;
}

void remove_allocation_from_tracking(unified_memory_allocation_t *allocation) {
  if (!allocation)
    return;

  pthread_mutex_lock(&g_unified_memory.allocations_mutex);

  if (allocation->prev) {
    allocation->prev->next = allocation->next;
  } else {
    g_unified_memory.allocations = allocation->next;
  }

  if (allocation->next) {
    allocation->next->prev = allocation->prev;
  }

  pthread_mutex_unlock(&g_unified_memory.allocations_mutex);
}

unified_memory_allocation_t *
create_allocation_metadata(void *ptr, size_t size,
                           unified_memory_strategy_t strategy,
                           unified_memory_flags_t flags, const char *file,
                           int line, const char *function, const char *type) {
  unified_memory_allocation_t *alloc =
      malloc(sizeof(unified_memory_allocation_t));
  if (!alloc)
    return NULL;

  alloc->ptr = ptr;
  alloc->size = size;
  alloc->actual_size = size; // TODO: Calculate actual size including headers
  alloc->file = file;
  alloc->line = line;
  alloc->function = function;
  alloc->type = type;
  alloc->strategy = strategy;
  alloc->flags = flags;
  alloc->timestamp = get_timestamp_ms();
  alloc->allocation_id = g_unified_memory.next_allocation_id++;
  alloc->next = NULL;
  alloc->prev = NULL;

  return alloc;
}

// Stats Updates

void update_stats_allocation(size_t size, unified_memory_strategy_t strategy,
                             unified_memory_flags_t flags) {
  pthread_mutex_lock(&g_unified_memory.global_mutex);

  g_unified_memory.stats.total_allocations++;
  g_unified_memory.stats.current_allocations++;
  g_unified_memory.stats.total_allocated += size;
  g_unified_memory.stats.current_allocated += size;

  if (g_unified_memory.stats.current_allocated >
      g_unified_memory.stats.peak_allocated) {
    g_unified_memory.stats.peak_allocated =
        g_unified_memory.stats.current_allocated;
  }

  if (size > g_unified_memory.stats.largest_allocation) {
    g_unified_memory.stats.largest_allocation = size;
  }

  if (g_unified_memory.stats.smallest_allocation == 0 ||
      size < g_unified_memory.stats.smallest_allocation) {
    g_unified_memory.stats.smallest_allocation = size;
  }

  g_unified_memory.stats.average_allocation_size =
      (double)g_unified_memory.stats.total_allocated /
      g_unified_memory.stats.total_allocations;

  // Update strategy-specific counters
  switch (strategy) {
  case UNIFIED_MEMORY_STRATEGY_TRACKED:
    g_unified_memory.stats.tracked_allocations++;
    break;
  case UNIFIED_MEMORY_STRATEGY_POOLED:
    g_unified_memory.stats.pooled_allocations++;
    break;
  case UNIFIED_MEMORY_STRATEGY_ARENA:
    g_unified_memory.stats.arena_allocations++;
    break;
  case UNIFIED_MEMORY_STRATEGY_STACK:
    g_unified_memory.stats.stack_allocations++;
    break;
  case UNIFIED_MEMORY_STRATEGY_GPU:
    g_unified_memory.stats.gpu_allocations++;
    break;
  default:
    break;
  }

  // Update flag-specific counters
  if (flags & UNIFIED_MEMORY_FLAG_ZERO) {
    g_unified_memory.stats.zeroed_allocations++;
  }
  if (flags & UNIFIED_MEMORY_FLAG_ALIGN) {
    g_unified_memory.stats.aligned_allocations++;
  }

  pthread_mutex_unlock(&g_unified_memory.global_mutex);
}

void update_stats_deallocation(size_t size) {
  pthread_mutex_lock(&g_unified_memory.global_mutex);

  g_unified_memory.stats.total_deallocations++;
  g_unified_memory.stats.current_allocations--;
  g_unified_memory.stats.total_freed += size;
  g_unified_memory.stats.current_allocated -= size;

  // Calculate fragmentation ratio
  if (g_unified_memory.stats.total_allocated > 0) {
    g_unified_memory.stats.fragmentation_ratio =
        (float)(g_unified_memory.stats.total_allocated -
                g_unified_memory.stats.current_allocated) /
        g_unified_memory.stats.total_allocated;
  }

  pthread_mutex_unlock(&g_unified_memory.global_mutex);
}
