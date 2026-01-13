#pragma once

#include "../math/types.h"
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

// Check if we're being included from memory.h
#ifndef UNIFIED_MEMORY_ALLOCATOR_MAIN

// ============================================================================
// UNIFIED MEMORY ALLOCATOR - CONSOLIDATING ALL MEMORY SYSTEMS
// ============================================================================

// Memory allocation strategies
typedef enum {
  MEMORY_STRATEGY_DEFAULT = 0, // Use default malloc/free
  MEMORY_STRATEGY_POOL,        // Use memory pools for small allocations
  MEMORY_STRATEGY_STACK,       // Use stack allocator for temporary data
  MEMORY_STRATEGY_ARENA,       // Use arena allocator for bulk allocations
  MEMORY_STRATEGY_TRACKED,     // Use tracked allocations with debugging
  MEMORY_STRATEGY_COUNT
} MemoryStrategy;

// Memory allocation flags
typedef enum {
  MEMORY_FLAG_NONE = 0,
  MEMORY_FLAG_ZERO = 1 << 0,  // Zero-initialize memory
  MEMORY_FLAG_ALIGN = 1 << 1, // Align to cache line
  MEMORY_FLAG_GUARD = 1 << 2, // Add guard pages for corruption detection
  MEMORY_FLAG_TRACK = 1 << 3, // Track allocation for debugging
  MEMORY_FLAG_TEMP = 1 << 4,  // Temporary allocation (auto-cleanup)
  MEMORY_FLAG_COUNT
} MemoryFlags;

// Memory allocation metadata
typedef struct {
  void *ptr;
  size_t size;
  size_t actual_size; // Including headers and alignment
  const char *file;
  int line;
  const char *function;
  MemoryStrategy strategy;
  MemoryFlags flags;
  u64 timestamp;
  u32 allocation_id;
  struct AllocationMetadata *next;
  struct AllocationMetadata *prev;
} AllocationMetadata;

// Memory pool configuration
typedef struct {
  size_t block_size;
  u32 block_count;
  u32 initial_blocks;
  bool auto_expand;
  size_t max_size;
  MemoryStrategy backing_strategy;
} MemoryPoolConfig;

// Memory pool structure
typedef struct MemoryPool {
  void *buffer;
  size_t buffer_size;
  size_t block_size;
  u32 total_blocks;
  u32 free_blocks;
  u32 *free_list;
  u32 free_head;
  pthread_mutex_t mutex;
  MemoryPoolConfig config;
  struct MemoryPool *next;
  bool initialized;
} MemoryPool;

// Stack allocator structure
typedef struct StackAllocator {
  void *buffer;
  size_t buffer_size;
  size_t current_offset;
  size_t peak_offset;
  pthread_mutex_t mutex;
  void *markers[64]; // Stack markers for rollback
  u32 marker_count;
  bool initialized;
} StackAllocator;

// Arena allocator structure
typedef struct ArenaAllocator {
  void **blocks;
  size_t *block_sizes;
  u32 block_count;
  u32 current_block;
  size_t current_offset;
  size_t total_allocated;
  pthread_mutex_t mutex;
  bool initialized;
} ArenaAllocator;

// Memory statistics
typedef struct {
  u64 total_allocations;
  u64 total_deallocations;
  u64 current_allocations;
  size_t total_allocated;
  size_t peak_allocated;
  size_t fragmentation_ratio;
  u64 allocation_failures;
  u64 guard_page_hits;
  u64 double_frees_detected;
  u64 use_after_free_detected;
  size_t pool_usage[16]; // Per-pool usage statistics
  size_t stack_usage;    // Stack allocator usage
  size_t arena_usage;    // Arena allocator usage
} MemoryStats;

// Memory limits and policies
typedef struct {
  size_t global_limit;             // Global memory limit
  size_t per_pool_limit;           // Per-pool memory limit
  size_t per_stack_limit;          // Per-stack memory limit
  size_t per_arena_limit;          // Per-arena memory limit
  u32 max_allocations;             // Maximum number of allocations
  bool enable_guard_pages;         // Enable guard page protection
  bool enable_canaries;            // Enable canary values
  bool enable_stack_traces;        // Enable call stack tracking
  u32 stack_trace_depth;           // Maximum stack trace depth
  bool enable_leak_detection;      // Enable automatic leak detection
  bool enable_fragmentation_check; // Enable fragmentation monitoring
} MemoryPolicy;

// Memory allocator context
typedef struct {
  // Allocation tracking
  AllocationMetadata *allocations;
  pthread_mutex_t allocations_mutex;

  // Memory pools
  MemoryPool *pools;
  pthread_mutex_t pools_mutex;

  // Stack allocators
  StackAllocator *stacks;
  pthread_mutex_t stacks_mutex;

  // Arena allocators
  ArenaAllocator *arenas;
  pthread_mutex_t arenas_mutex;

  // Configuration and statistics
  MemoryPolicy policy;
  MemoryStats stats;
  pthread_mutex_t stats_mutex;

  // Global state
  bool initialized;
  u32 next_allocation_id;
  pthread_mutex_t global_mutex;
} UnifiedMemoryAllocator;

// ============================================================================
// UNIFIED MEMORY ALLOCATOR API
// ============================================================================

// Lifecycle
bool unified_memory_init(const MemoryPolicy *policy);
void unified_memory_shutdown(void);
void unified_memory_free(void *ptr);
void unified_memory_get_stats(MemoryStats *stats);
void unified_memory_print_stats(void);
void unified_memory_get_policy(MemoryPolicy *policy);
void unified_memory_set_policy(const MemoryPolicy *policy);

// Core allocation functions
void *unified_memory_alloc(size_t size, MemoryStrategy strategy,
                           MemoryFlags flags, const char *file, int line,
                           const char *function);
void *unified_memory_realloc(void *ptr, size_t new_size,
                             MemoryStrategy strategy, MemoryFlags flags,
                             const char *file, int line, const char *function);
void unified_memory_pool_destroy(MemoryPool *pool);
void *unified_memory_pool_alloc(MemoryPool *pool, MemoryFlags flags);
void unified_memory_pool_free(MemoryPool *pool, void *ptr);

// Stack allocator for temporary allocations
typedef struct {
  void *stack_memory;
  u32 stack_size;
  u32 current_offset;
  u32 peak_offset;
  bool initialized;
} MemoryStack;

MemoryStack *unified_memory_stack_create(u32 size);
void unified_memory_stack_destroy(MemoryStack *stack);
void *unified_memory_stack_alloc(MemoryStack *stack, u32 size, MemoryFlags flags);
void unified_memory_stack_reset(MemoryStack *stack);

// Arena allocator for bulk allocations
typedef struct {
  void *arena_memory;
  u32 arena_size;
  u32 current_offset;
  u32 peak_offset;
  bool initialized;
} MemoryArena;

MemoryArena *unified_memory_arena_create(u32 size);
void unified_memory_arena_destroy(MemoryArena *arena);
void *unified_memory_arena_alloc(MemoryArena *arena, u32 size, MemoryFlags flags);
void unified_memory_arena_reset(MemoryArena *arena);

// ============================================================================
// MEMORY TRACKING AND DEBUGGING
// ============================================================================

typedef struct {
  u64 total_allocated;
  u64 total_freed;
  u32 allocation_count;
  u32 peak_allocations;
  u64 peak_memory_usage;
  bool initialized;
  pthread_mutex_t mutex;
} MemoryTracker;

extern MemoryTracker g_memory_tracker;

void memory_tracker_init(u32 initial_capacity);
void memory_tracker_shutdown(void);
void memory_tracker_report(void);

// Memory limits and enforcement
void memory_set_limit(u64 limit_bytes);
void memory_set_enforcement(bool enabled);
u64 memory_get_limit(void);
bool memory_is_enforcement_enabled(void);
bool memory_check_limit(u64 requested_size);

// ============================================================================
// COMPATIBILITY MACROS
// ============================================================================

// Convenience macros for common usage
#define MALLOC(size) memory_alloc(size)
#define CALLOC(count, size) memory_calloc(count, size)
#define REALLOC(ptr, size) memory_realloc(ptr, size)
#define FREE(ptr) memory_free(ptr)

// Pool allocator macros
#define pool_create(block_size, block_count) unified_memory_pool_create(block_size, block_count)
#define pool_destroy(pool) unified_memory_pool_destroy(pool)
#define pool_alloc(pool) unified_memory_pool_alloc(pool, MEMORY_FLAG_NONE)
#define pool_free(pool, ptr) unified_memory_pool_free(pool, ptr)

// Object pool macros
#define object_pool_create(type, count) pool_create(sizeof(type), count)
#define object_pool_destroy(pool) pool_destroy(pool)
#define object_pool_alloc(pool) pool_alloc(pool)
#define object_pool_free(pool, ptr) pool_free(pool, ptr)

// Stack allocator macros
#define stack_create(size) unified_memory_stack_create(size)
#define stack_destroy(stack) unified_memory_stack_destroy(stack)
#define stack_alloc(stack, size) unified_memory_stack_alloc(stack, size, MEMORY_FLAG_NONE)
#define stack_reset(stack) unified_memory_stack_reset(stack)

// Arena allocator macros
#define arena_create(size) unified_memory_arena_create(size)
#define arena_destroy(arena) unified_memory_arena_destroy(arena)
#define arena_alloc(arena, size) unified_memory_arena_alloc(arena, size, MEMORY_FLAG_NONE)
#define arena_reset(arena) unified_memory_arena_reset(arena)

// Legacy aliases
#define core_alloc(size) MALLOC(size)
#define core_free(ptr) FREE(ptr)

// Compatibility with existing pool allocator
#define memory_pool_create(block_size, block_count) pool_create(block_size, block_count)
#define memory_pool_destroy(pool) pool_destroy(pool)
#define memory_pool_alloc(pool, size) pool_alloc(pool)
#define memory_pool_free(pool, ptr) pool_free(pool, ptr)

// Compatibility with object pool (remove duplicate macros)
// #define object_pool_alloc(pool) object_pool_alloc(pool)  // Removed - causes recursion
// #define object_pool_free(pool, ptr) object_pool_free(pool, ptr)  // Removed - causes recursion

// Compatibility with stack allocator
#define stack_allocator_create(size) unified_memory_stack_create(size)
#define stack_allocator_destroy(stack) unified_memory_stack_destroy(stack)
#define stack_allocator_alloc(stack, size)                                     \
  unified_memory_stack_alloc(stack, size, MEMORY_FLAG_NONE)
#define stack_allocator_reset(stack) unified_memory_stack_reset(stack)

#else // UNIFIED_MEMORY_ALLOCATOR_MAIN

// Forward declarations for when included from memory.h
typedef struct {
    u64 total_allocated;
    u64 total_freed;
    u32 allocation_count;
    u32 peak_allocations;
    u64 peak_memory_usage;
    bool initialized;
    pthread_mutex_t mutex;
} MemoryStats;

// When included from memory.h, only provide the function declarations
void *memory_alloc(u32 size, const char *file, u32 line);
void *memory_calloc(u32 count, u32 size, const char *file, u32 line);
void *memory_realloc(void *ptr, u32 new_size, const char *file, u32 line);
void memory_free(void *ptr);
MemoryStats unified_memory_get_stats(void);

#endif // UNIFIED_MEMORY_ALLOCATOR_MAIN
