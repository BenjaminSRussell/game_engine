#ifndef UNIFIED_MEMORY_H
#define UNIFIED_MEMORY_H

/*
 * Unified Memory Management System
 * Consolidates all memory allocation patterns across the engine
 * Provides migration macros for existing code and standardized APIs
 */

#include <stddef.h>
// #include "memory_utils.h" // Removed or moved to private context if not
// needed for public API types
// #include <core/error_handling.h> // Commented out - not strictly needed for
// public API
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration for error handling
typedef struct error_context error_context_t;

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * MEMORY ALLOCATION STRATEGIES
 * ============================================================================
 */

typedef enum {
  UNIFIED_MEMORY_STRATEGY_DEFAULT = 0, // Use standard malloc/free
  UNIFIED_MEMORY_STRATEGY_TRACKED,     // Use tracked allocations
  UNIFIED_MEMORY_STRATEGY_POOLED,      // Use memory pools
  UNIFIED_MEMORY_STRATEGY_ARENA,       // Use arena allocator
  UNIFIED_MEMORY_STRATEGY_STACK,       // Use stack allocator
  UNIFIED_MEMORY_STRATEGY_GPU,         // GPU memory allocation
  UNIFIED_MEMORY_STRATEGY_COUNT
} unified_memory_strategy_t;

/* ============================================================================
 * MEMORY FLAGS
 * ============================================================================
 */

typedef enum {
  UNIFIED_MEMORY_FLAG_NONE = 0,
  UNIFIED_MEMORY_FLAG_ZERO = 1 << 0,       // Zero-initialize memory
  UNIFIED_MEMORY_FLAG_ALIGN = 1 << 1,      // Align to cache line
  UNIFIED_MEMORY_FLAG_GUARD = 1 << 2,      // Add guard pages
  UNIFIED_MEMORY_FLAG_TRACK = 1 << 3,      // Track allocation
  UNIFIED_MEMORY_FLAG_TEMP = 1 << 4,       // Temporary allocation
  UNIFIED_MEMORY_FLAG_PERSISTENT = 1 << 5, // Persistent allocation
  UNIFIED_MEMORY_FLAG_READONLY = 1 << 6,   // Read-only allocation
  UNIFIED_MEMORY_FLAG_COUNT
} unified_memory_flags_t;

/* ============================================================================
 * MEMORY ALLOCATION METADATA
 * ============================================================================
 */

// Opaque handle for allocations
typedef struct unified_memory_allocation unified_memory_allocation_t;

/* ============================================================================
 * MEMORY STATISTICS
 * ============================================================================
 */

typedef struct {
  uint64_t total_allocations;
  uint64_t total_deallocations;
  uint64_t current_allocations;
  size_t total_allocated;
  size_t total_freed;
  size_t current_allocated;
  size_t peak_allocated;
  uint64_t allocation_failures;
  uint64_t reallocations;
  uint64_t zeroed_allocations;
  uint64_t aligned_allocations;
  uint64_t tracked_allocations;
  uint64_t pooled_allocations;
  uint64_t arena_allocations;
  uint64_t stack_allocations;
  uint64_t gpu_allocations;
  float fragmentation_ratio;
  size_t largest_allocation;
  size_t smallest_allocation;
  double average_allocation_size;
} unified_memory_stats_t;

/* ============================================================================
 * UNIFIED MEMORY API
 * ============================================================================
 */

// Core allocation functions
void *unified_memory_alloc(size_t size, unified_memory_strategy_t strategy,
                           unified_memory_flags_t flags, const char *file,
                           int line, const char *function, const char *type);

void *unified_memory_realloc(void *ptr, size_t new_size,
                             unified_memory_strategy_t strategy,
                             unified_memory_flags_t flags, const char *file,
                             int line, const char *function, const char *type);

void unified_memory_free(void *ptr, const char *file, int line,
                         const char *function);

void *unified_memory_calloc(size_t count, size_t size,
                            unified_memory_strategy_t strategy,
                            unified_memory_flags_t flags, const char *file,
                            int line, const char *function, const char *type);

char *unified_memory_strdup(const char *str, unified_memory_strategy_t strategy,
                            unified_memory_flags_t flags, const char *file,
                            int line, const char *function);

// Memory management functions
void unified_memory_init(void);
void unified_memory_shutdown(void);
void unified_memory_reset_stats(void);
void unified_memory_get_stats(unified_memory_stats_t *stats);
void unified_memory_dump_stats(void);
void unified_memory_dump_leaks(void);

// Strategy-specific functions
void *unified_memory_pool_alloc(size_t block_size, size_t block_count,
                                unified_memory_flags_t flags, const char *file,
                                int line, const char *function);
void unified_memory_pool_free(void *ptr);
void *unified_memory_arena_alloc(size_t size, unified_memory_flags_t flags,
                                 const char *file, int line,
                                 const char *function);
void unified_memory_arena_reset(void);
void *unified_memory_stack_alloc(size_t size, unified_memory_flags_t flags,
                                 const char *file, int line,
                                 const char *function);
void unified_memory_stack_reset(void);

// Memory validation and debugging
bool unified_memory_validate_pointer(void *ptr);
bool unified_memory_check_corruption(void *ptr);
void unified_memory_mark_readonly(void *ptr);
void unified_memory_mark_readwrite(void *ptr);
void unified_memory_set_guard_pages(void *ptr, size_t size);

/* ============================================================================
 * CONVENIENCE MACROS - Standardized allocation patterns
 * ============================================================================
 */

// Basic allocation macros
#define UNIFIED_FREE(ptr) unified_memory_free(ptr, __FILE__, __LINE__, __func__)
#define UNIFIED_ALLOC(size)                                                    \
  unified_memory_alloc(size, UNIFIED_MEMORY_STRATEGY_DEFAULT,                  \
                       UNIFIED_MEMORY_FLAG_NONE, __FILE__, __LINE__, __func__, \
                       "unknown")

#define UNIFIED_ALLOC_ZERO(size)                                               \
  unified_memory_alloc(size, UNIFIED_MEMORY_STRATEGY_DEFAULT,                  \
                       UNIFIED_MEMORY_FLAG_ZERO, __FILE__, __LINE__, __func__, \
                       "zeroed")

#define UNIFIED_ALLOC_TRACKED(size, type)                                      \
  unified_memory_alloc(size, UNIFIED_MEMORY_STRATEGY_TRACKED,                  \
                       UNIFIED_MEMORY_FLAG_TRACK, __FILE__, __LINE__,          \
                       __func__, type)

#define UNIFIED_ALLOC_ALIGNED(size, alignment)                                 \
  unified_memory_alloc(size, UNIFIED_MEMORY_STRATEGY_DEFAULT,                  \
                       UNIFIED_MEMORY_FLAG_ALIGN, __FILE__, __LINE__,          \
                       __func__, "aligned")

#define UNIFIED_ALLOC_SAFE(size, type)                                         \
  unified_memory_alloc(size, UNIFIED_MEMORY_STRATEGY_TRACKED,                  \
                       UNIFIED_MEMORY_FLAG_TRACK | UNIFIED_MEMORY_FLAG_GUARD,  \
                       __FILE__, __LINE__, __func__, type)

// Type-safe allocation macros
#define UNIFIED_NEW(type)                                                      \
  ((type *)unified_memory_alloc(sizeof(type), UNIFIED_MEMORY_STRATEGY_TRACKED, \
                                UNIFIED_MEMORY_FLAG_ZERO, __FILE__, __LINE__,  \
                                __func__, #type))

#define UNIFIED_NEW_ARRAY(type, count)                                         \
  ((type *)unified_memory_alloc(                                               \
      sizeof(type) * (count), UNIFIED_MEMORY_STRATEGY_TRACKED,                 \
      UNIFIED_MEMORY_FLAG_ZERO, __FILE__, __LINE__, __func__, #type "[]"))

#define UNIFIED_NEW_PERSISTENT(type)                                           \
  ((type *)unified_memory_alloc(sizeof(type), UNIFIED_MEMORY_STRATEGY_TRACKED, \
                                UNIFIED_MEMORY_FLAG_TRACK |                    \
                                    UNIFIED_MEMORY_FLAG_PERSISTENT,            \
                                __FILE__, __LINE__, __func__, #type))

#define UNIFIED_VALIDATE_POINTER(ptr)                                          \
  do {                                                                         \
    if (!unified_memory_validate_pointer(ptr)) {                               \
      REPORT_ERROR(ERROR_INVALID_PARAMETER, "Invalid pointer detected");       \
    }                                                                          \
  } while (0)

#define VALIDATE_POINTER_RETURN(ptr, error_code)                               \
  do {                                                                         \
    if (!unified_memory_validate_pointer(ptr)) {                               \
      REPORT_ERROR(ERROR_INVALID_PARAMETER, "Invalid pointer detected");       \
      return error_code;                                                       \
    }                                                                          \
  } while (0)

// Reallocation macros
#define UNIFIED_REALLOC(ptr, new_size)                                         \
  unified_memory_realloc(ptr, new_size, UNIFIED_MEMORY_STRATEGY_DEFAULT,       \
                         UNIFIED_MEMORY_FLAG_NONE, __FILE__, __LINE__,         \
                         __func__, "realloc")

// (Removed duplicates)

/* ============================================================================
 * PERFORMANCE MACROS - Memory optimization helpers
 * ============================================================================
 */

#define MEMORY_POOL_ALLOC(type, pool_size)                                     \
  unified_memory_pool_alloc(sizeof(type), pool_size, UNIFIED_MEMORY_FLAG_ZERO, \
                            __FILE__, __LINE__, __func__)

#define MEMORY_ARENA_ALLOC(size)                                               \
  unified_memory_arena_alloc(size, UNIFIED_MEMORY_FLAG_ZERO, __FILE__,         \
                             __LINE__, __func__)

#define MEMORY_STACK_ALLOC(size)                                               \
  unified_memory_stack_alloc(size, UNIFIED_MEMORY_FLAG_TEMP, __FILE__,         \
                             __LINE__, __func__)

#define MEMORY_STACK_RESET() unified_memory_stack_reset()
#define MEMORY_ARENA_RESET() unified_memory_arena_reset()

/* ============================================================================
 * DEBUG MACROS - Development-time memory checking
 * ============================================================================
 */

#ifdef DEBUG_BUILD
#define DEBUG_ALLOC(size, type) UNIFIED_ALLOC_TRACKED(size, type)
#define DEBUG_FREE(ptr)                                                        \
  do {                                                                         \
    UNIFIED_VALIDATE_POINTER(ptr);                                             \
    UNIFIED_FREE(ptr);                                                         \
  } while (0)
#define DEBUG_REALLOC(ptr, size, type) UNIFIED_REALLOC_TRACKED(ptr, size, type)
#define DEBUG_STRDUP(str) UNIFIED_STRDUP(str)
#define DEBUG_MEMORY_DUMP() unified_memory_dump_stats()
#define DEBUG_MEMORY_LEAKS() unified_memory_dump_leaks()
#else
#define DEBUG_ALLOC(size, type) UNIFIED_ALLOC(size)
#define DEBUG_FREE(ptr) UNIFIED_FREE(ptr)
#define DEBUG_REALLOC(ptr, size, type) UNIFIED_REALLOC(ptr, size)
#define DEBUG_STRDUP(str) UNIFIED_STRDUP(str)
#define DEBUG_MEMORY_DUMP() ((void)0)
#define DEBUG_MEMORY_LEAKS() ((void)0)
#endif

/* ============================================================================
 * THREAD-SAFETY MACROS
 * ============================================================================
 */

#ifdef THREAD_SAFE_MEMORY
#define THREAD_SAFE_ALLOC(size) UNIFIED_ALLOC_TRACKED(size, "thread_safe")
#define THREAD_SAFE_FREE(ptr) UNIFIED_FREE(ptr)
#define THREAD_SAFE_REALLOC(ptr, size)                                         \
  UNIFIED_REALLOC_TRACKED(ptr, size, "thread_safe")
#else
#define THREAD_SAFE_ALLOC(size) UNIFIED_ALLOC(size)
#define THREAD_SAFE_FREE(ptr) UNIFIED_FREE(ptr)
#define THREAD_SAFE_REALLOC(ptr, size) UNIFIED_REALLOC(ptr, size)
#endif

/* ============================================================================
 * MEMORY BOUNDARY SUPPORT
 * ============================================================================
 */

typedef struct memory_boundary {
  unified_memory_stats_t start_stats;
  bool enabled;
  void (*error_handler)(error_context_t *error, void *user_data);
  void *user_data;
} memory_boundary_t;

memory_boundary_t *memory_boundary_create(void);
void memory_boundary_destroy(memory_boundary_t *boundary);
void memory_boundary_enter(memory_boundary_t *boundary);
void memory_boundary_exit(memory_boundary_t *boundary);
void memory_boundary_get_stats(memory_boundary_t *boundary,
                               unified_memory_stats_t *stats);
void memory_boundary_set_handler(memory_boundary_t *boundary,
                                 void (*handler)(error_context_t *, void *),
                                 void *user_data);

#define MEMORY_BOUNDARY(boundary)                                              \
  memory_boundary_enter(boundary);                                             \
  if (memory_boundary_get_last_error(boundary) == NULL)

/* ============================================================================
 * MEMORY PROFILING SUPPORT
 * ============================================================================
 */

typedef struct memory_profile {
  const char *name;
  uint64_t start_time;
  uint64_t end_time;
  size_t bytes_allocated;
  size_t bytes_freed;
  uint32_t allocations_count;
  uint32_t deallocations_count;
  bool active;
} memory_profile_t;

void memory_profile_start(const char *name);
void memory_profile_end(const char *name);
void memory_profile_dump(const char *name);
void memory_profile_dump_all(void);

#define MEMORY_PROFILE_START(name) memory_profile_start(name)
#define MEMORY_PROFILE_END(name) memory_profile_end(name)

/* ============================================================================
 * COMPATIBILITY LAYER - Legacy system integration
 * ============================================================================
 */

// For systems that still use old memory management
#define LEGACY_ALLOC(size) UNIFIED_ALLOC(size)
#define LEGACY_FREE(ptr) UNIFIED_FREE(ptr)
#define LEGACY_REALLOC(ptr, size) UNIFIED_REALLOC(ptr, size)
#define LEGACY_CALLOC(count, size) UNIFIED_CALLOC(count, size)

// Integration with existing memory systems
void unified_memory_integrate_memory_utils(void);
void unified_memory_integrate_unified_allocator(void);
void unified_memory_integrate_legacy_systems(void);

/* ============================================================================
 * CONFIGURATION AND POLICY
 * ============================================================================
 */

typedef struct {
  size_t global_memory_limit;
  size_t per_allocation_limit;
  bool enable_tracking;
  bool enable_guard_pages;
  bool enable_leak_detection;
  bool enable_corruption_detection;
  bool enable_profiling;
  uint32_t allocation_limit;
  float fragmentation_threshold;
  bool auto_compact;
} unified_memory_config_t;

void unified_memory_set_config(const unified_memory_config_t *config);
void unified_memory_get_config(unified_memory_config_t *config);
void unified_memory_enable_leak_detection(bool enable);
void unified_memory_enable_corruption_detection(bool enable);
void unified_memory_enable_profiling(bool enable);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

// Memory operations with bounds checking
bool unified_memory_safe_copy(void *dest, size_t dest_size, const void *src,
                              size_t src_size);
bool unified_memory_safe_set(void *dest, size_t dest_size, int value,
                             size_t count);
int unified_memory_safe_compare(const void *mem1, size_t size1,
                                const void *mem2, size_t size2);

// Memory alignment utilities
void *unified_memory_align(void *ptr, size_t alignment);
size_t unified_memory_aligned_size(size_t size, size_t alignment);
bool unified_memory_is_aligned(void *ptr, size_t alignment);

// Memory information utilities
size_t unified_memory_get_size(void *ptr);
unified_memory_strategy_t unified_memory_get_strategy(void *ptr);
unified_memory_flags_t unified_memory_get_flags(void *ptr);
const char *unified_memory_get_type(void *ptr);
uint64_t unified_memory_get_timestamp(void *ptr);

// Memory debugging utilities
void unified_memory_print_allocation(void *ptr);
void unified_memory_print_all_allocations(void);
void unified_memory_print_allocations_by_type(const char *type);
void unified_memory_print_allocations_by_strategy(
    unified_memory_strategy_t strategy);

#ifdef __cplusplus
}
#endif

#endif /* UNIFIED_MEMORY_H */
