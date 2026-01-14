#include "memory_private.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#define GET_TIMESTAMP() GetTickCount64()
#define MEMORY_ALIGN(ptr, alignment) _aligned_malloc((size), (alignment))
#define MEMORY_FREE_ALIGNED(ptr) _aligned_free(ptr)
#else
#include <sys/mman.h>
#include <unistd.h>
#define GET_TIMESTAMP() ((uint64_t)time(NULL) * 1000)
#define MEMORY_ALIGN(ptr, alignment) aligned_alloc((alignment), (size))
#define MEMORY_FREE_ALIGNED(ptr) free(ptr)
#endif

// Forward Declarations from memory_tracking.c
void add_allocation_to_tracking(unified_memory_allocation_t *allocation);
unified_memory_allocation_t *find_allocation_metadata(void *ptr);
void remove_allocation_from_tracking(unified_memory_allocation_t *allocation);
unified_memory_allocation_t *
create_allocation_metadata(void *ptr, size_t size,
                           unified_memory_strategy_t strategy,
                           unified_memory_flags_t flags, const char *file,
                           int line, const char *function, const char *type);
void update_stats_allocation(size_t size, unified_memory_strategy_t strategy,
                             unified_memory_flags_t flags);
void update_stats_deallocation(size_t size);

// Core Implementation

uint64_t get_timestamp_ms(void) { return GET_TIMESTAMP(); }

void unified_memory_init(void) {
  if (g_unified_memory.initialized)
    return;

  pthread_mutex_init(&g_unified_memory.global_mutex, NULL);
  pthread_mutex_init(&g_unified_memory.allocations_mutex, NULL);
  pthread_mutex_init(&g_unified_memory.pools_mutex, NULL);
  pthread_mutex_init(&g_unified_memory.arena_mutex, NULL);
  pthread_mutex_init(&g_unified_memory.stack_mutex, NULL);
  pthread_mutex_init(&g_unified_memory.profiles_mutex, NULL);
  pthread_mutex_init(&g_unified_memory.boundary_mutex, NULL);

  // Initialize default configuration
  g_unified_memory.config.global_memory_limit = 0;  // No limit
  g_unified_memory.config.per_allocation_limit = 0; // No limit
  g_unified_memory.config.enable_tracking = true;
  g_unified_memory.config.enable_guard_pages = false;
  g_unified_memory.config.enable_leak_detection = true;
  g_unified_memory.config.enable_corruption_detection = false;
  g_unified_memory.config.enable_profiling = false;
  g_unified_memory.config.allocation_limit = 0;
  g_unified_memory.config.fragmentation_threshold = 0.5f;
  g_unified_memory.config.auto_compact = false;

  memset(&g_unified_memory.stats, 0, sizeof(g_unified_memory.stats));

  g_unified_memory.initialized = true;
  g_unified_memory.next_allocation_id = 1;

  printf("Unified memory system initialized\n");
}

void unified_memory_shutdown(void) {
  if (!g_unified_memory.initialized)
    return;

  // Report memory leaks
  if (g_unified_memory.config.enable_leak_detection) {
    unified_memory_dump_leaks();
  }

  // Free all tracked allocations
  pthread_mutex_lock(&g_unified_memory.allocations_mutex);
  unified_memory_allocation_t *current = g_unified_memory.allocations;
  while (current) {
    unified_memory_allocation_t *next = current->next;
    free(current->ptr);
    free(current);
    current = next;
  }
  g_unified_memory.allocations = NULL;
  pthread_mutex_unlock(&g_unified_memory.allocations_mutex);

  // Cleanup profiles
  pthread_mutex_lock(&g_unified_memory.profiles_mutex);
  if (g_unified_memory.profiles) {
    free(g_unified_memory.profiles);
    g_unified_memory.profiles = NULL;
  }
  g_unified_memory.profile_count = 0;
  pthread_mutex_unlock(&g_unified_memory.profiles_mutex);

  // Destroy mutexes
  pthread_mutex_destroy(&g_unified_memory.global_mutex);
  pthread_mutex_destroy(&g_unified_memory.allocations_mutex);
  pthread_mutex_destroy(&g_unified_memory.pools_mutex);
  pthread_mutex_destroy(&g_unified_memory.arena_mutex);
  pthread_mutex_destroy(&g_unified_memory.stack_mutex);
  pthread_mutex_destroy(&g_unified_memory.profiles_mutex);
  pthread_mutex_destroy(&g_unified_memory.boundary_mutex);

  g_unified_memory.initialized = false;

  printf("Unified memory system shutdown\n");
}

/* ============================================================================
 * CORE ALLOCATION FUNCTIONS
 * ============================================================================
 */

// Undefine macros to avoid conflict with function definitions
#undef unified_memory_alloc
#undef unified_memory_realloc
// Also undefine standard library macros to prevent infinite recursion
// (since this file implements the allocator using the real malloc)
#undef malloc
#undef free
#undef calloc
#undef realloc

void *unified_memory_alloc(size_t size, unified_memory_strategy_t strategy,
                           unified_memory_flags_t flags, const char *file,
                           int line, const char *function, const char *type) {
  if (!g_unified_memory.initialized) {
    unified_memory_init();
  }

  if (size == 0) {
    // REPORT_ERROR(ERROR_INVALID_PARAMETER, "Zero size allocation requested");
    return NULL;
  }

  // Check global memory limit
  if (g_unified_memory.config.global_memory_limit > 0) {
    if (g_unified_memory.stats.current_allocated + size >
        g_unified_memory.config.global_memory_limit) {
      // REPORT_ERROR(ERROR_OUT_OF_MEMORY, "Global memory limit exceeded");
      g_unified_memory.stats.allocation_failures++;
      return NULL;
    }
  }

  // Check per-allocation limit
  if (g_unified_memory.config.per_allocation_limit > 0 &&
      size > g_unified_memory.config.per_allocation_limit) {
    // REPORT_ERROR(ERROR_OUT_OF_MEMORY, "Per-allocation size limit exceeded");
    g_unified_memory.stats.allocation_failures++;
    return NULL;
  }

  void *ptr = NULL;
  size_t actual_size = size;

  // Apply alignment if requested
  if (flags & UNIFIED_MEMORY_FLAG_ALIGN) {
    actual_size = (size + 63) & ~63; // Align to 64-byte boundary
  }

  // Allocate based on strategy
  switch (strategy) {
  case UNIFIED_MEMORY_STRATEGY_DEFAULT:
  case UNIFIED_MEMORY_STRATEGY_TRACKED:
    if (flags & UNIFIED_MEMORY_FLAG_ALIGN) {
      // ptr = MEMORY_ALIGN(actual_size, 64); // Need standard C wrapper or fix
      // macro usage Simple fallback for now
      if (posix_memalign(&ptr, 64, actual_size) != 0)
        ptr = NULL;
    } else {
      ptr = malloc(actual_size);
    }
    break;

  case UNIFIED_MEMORY_STRATEGY_POOLED:
    // TODO: Implement memory pool allocation
    ptr = malloc(actual_size);
    break;

  case UNIFIED_MEMORY_STRATEGY_ARENA:
    // TODO: Implement arena allocation
    ptr = malloc(actual_size);
    break;

  case UNIFIED_MEMORY_STRATEGY_STACK:
    // TODO: Implement stack allocation
    ptr = malloc(actual_size);
    break;

  case UNIFIED_MEMORY_STRATEGY_GPU:
    // TODO: Implement GPU allocation
    ptr = malloc(actual_size);
    break;

  default:
    ptr = malloc(actual_size);
    break;
  }

  if (!ptr) {
    // REPORT_ERROR(ERROR_OUT_OF_MEMORY, "Memory allocation failed");
    g_unified_memory.stats.allocation_failures++;
    return NULL;
  }

  // Zero-initialize if requested
  if (flags & UNIFIED_MEMORY_FLAG_ZERO) {
    memset(ptr, 0, size);
  }

  // Track allocation if enabled
  if (g_unified_memory.config.enable_tracking ||
      (flags & UNIFIED_MEMORY_FLAG_TRACK)) {
    unified_memory_allocation_t *metadata = create_allocation_metadata(
        ptr, size, strategy, flags, file, line, function, type);
    if (metadata) {
      add_allocation_to_tracking(metadata);
    }
  }

  update_stats_allocation(size, strategy, flags);

  return ptr;
}

void *unified_memory_realloc(void *ptr, size_t new_size,
                             unified_memory_strategy_t strategy,
                             unified_memory_flags_t flags, const char *file,
                             int line, const char *function, const char *type) {
  if (!ptr) {
    return unified_memory_alloc(new_size, strategy, flags, file, line, function,
                                type);
  }

  if (new_size == 0) {
    unified_memory_free(ptr, file, line, function);
    return NULL;
  }

  unified_memory_allocation_t *metadata = find_allocation_metadata(ptr);
  size_t old_size = metadata ? metadata->size : 0;

  void *new_ptr = unified_memory_alloc(new_size, strategy, flags, file, line,
                                       function, type);
  if (!new_ptr) {
    return NULL;
  }

  // Copy old data
  size_t copy_size = (old_size < new_size) ? old_size : new_size;
  memcpy(new_ptr, ptr, copy_size);

  // Free old pointer
  unified_memory_free(ptr, file, line, function);

  g_unified_memory.stats.reallocations++;

  return new_ptr;
}

void unified_memory_free(void *ptr, const char *file, int line,
                         const char *function) {
  (void)file;
  (void)line;
  (void)function;
  if (!ptr)
    return;

  unified_memory_allocation_t *metadata = find_allocation_metadata(ptr);
  size_t size = metadata ? metadata->size : 0;

  // Remove from tracking
  if (metadata) {
    remove_allocation_from_tracking(metadata);
    free(metadata);
  }

  // Free based on strategy
  // Note: For now, we use standard free for all strategies
  free(ptr);

  update_stats_deallocation(size);
}

void unified_memory_get_context(unified_memory_stats_t *stats) {
  // Legacy wrapper if needed, or mapped to get_stats
  unified_memory_get_stats(stats);
}

void unified_memory_get_config(unified_memory_config_t *config) {
  if (!config)
    return;
  pthread_mutex_lock(&g_unified_memory.global_mutex);
  *config = g_unified_memory.config;
  pthread_mutex_unlock(&g_unified_memory.global_mutex);
}

void unified_memory_set_config(const unified_memory_config_t *config) {
  if (!config)
    return;
  pthread_mutex_lock(&g_unified_memory.global_mutex);
  g_unified_memory.config = *config;
  pthread_mutex_unlock(&g_unified_memory.global_mutex);
}

void unified_memory_get_stats(unified_memory_stats_t *stats) {
  if (!stats)
    return;
  pthread_mutex_lock(&g_unified_memory.global_mutex);
  *stats = g_unified_memory.stats;
  pthread_mutex_unlock(&g_unified_memory.global_mutex);
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

bool unified_memory_validate_pointer(void *ptr) {
  if (!ptr)
    return false;
  unified_memory_allocation_t *metadata = find_allocation_metadata(ptr);
  return metadata != NULL;
}
