// include/core/memory.h
//
// Purpose: Defines a comprehensive memory management and tracking system for
// the game engine. This header provides tools for monitoring memory
// allocations, optimizing memory usage through object pooling and stack
// allocation, and debugging memory-related issues like leaks. It centralizes
// memory operations, offering a safer and more performant alternative to raw
// `malloc`/`free`.
//
// Public APIs:
// - `MemoryAlloc`: Internal structure to track individual allocations (pointer,
// size, file, line).
// - `MemoryTracker`: Global structure to manage all tracked allocations,
// total/peak usage, and active count.
// - `g_memory_tracker`: An external global instance of the `MemoryTracker`.
// - `memory_tracker_init`, `memory_tracker_shutdown`: Lifecycle functions for
// the memory tracker.
// - `memory_alloc`, `memory_calloc`, `memory_realloc`, `memory_free`:
// Instrumented allocation/deallocation functions.
// - `MALLOC`, `CALLOC`, `REALLOC`, `FREE`: Convenience macros to automatically
// pass file and line information.
// - `memory_tracker_report`: Generates a detailed report of current memory
// allocations.
// - `ObjectPool`: Structure for managing a pool of fixed-size objects for fast
// allocation/deallocation.
// - `object_pool_create`, `object_pool_destroy`, `object_pool_allocate`,
// `object_pool_free`: API for object pool management.
// - `StackAllocator`: Structure for managing a stack-based memory allocator for
// temporary data.
// - `stack_allocator_create`, `stack_allocator_destroy`,
// `stack_allocator_alloc`, `stack_allocator_reset`,
// `stack_allocator_end_frame`: API for stack allocator management.
// - `memory_check_leaks`, `memory_dump_allocations`: Functions for detecting
// and reporting memory leaks.
// - `memory_get_total_used`, `memory_get_peak_used`, `memory_print_stats`:
// Functions for querying and printing memory usage statistics.
// - `memory_named_region_begin`, `memory_named_region_end`: For profiling
// memory usage in specific code sections.
//
// Ownership: The `MemoryTracker` manages the metadata for all instrumented
// allocations. `ObjectPool` and `StackAllocator` instances directly own their
// respective memory blocks. Users are responsible for properly initializing and
// shutting down these systems.
//
// Invariants:
// - All allocations should go through the `MALLOC`/`CALLOC`/`REALLOC` macros to
// enable tracking.
// - Every `MALLOC`/`CALLOC`/`REALLOC` must have a corresponding `FREE` to
// prevent leaks.
// - `ObjectPool` and `StackAllocator` are designed for specific use cases
// (fixed-size objects, temporary data)
//   and should be used appropriately.
// - The `g_memory_tracker` should be initialized once at application start and
// shut down at exit.
//
#ifndef MEMORY_H
#define MEMORY_H

#include "../common.h"
#include <stdint.h>

// Memory tagging system
typedef enum {
  MEMORY_TAG_DEFAULT = 0,
  MEMORY_TAG_TEMP,
  MEMORY_TAG_PERSISTENT,
  MEMORY_TAG_ASSET,
  MEMORY_TAG_RENDERER,
  MEMORY_TAG_AUDIO,
  MEMORY_TAG_PHYSICS,
  MEMORY_TAG_AI,
  MEMORY_TAG_NETWORK,
  MEMORY_TAG_UI,
  MEMORY_TAG_GAMEPLAY,
  MEMORY_TAG_GEOMETRY,
  MEMORY_TAG_COUNT
} MemoryTag;

// Advanced allocation tracking with stack traces
typedef struct {
  void *ptr;
  u32 size;
  const char *file;
  u32 line;
  MemoryTag tag;
  u64 allocation_id;
  void *stack_trace[16]; // Stack trace for leak detection
  u32 stack_depth;
  u64 timestamp;
} MemoryAlloc;

typedef struct {
  MemoryAlloc *allocations;
  u32 count;
  u32 capacity;
  u64 total_allocated;
  u64 total_freed;
  u64 peak_allocated;
  u32 active_allocations;
  u64 memory_limit;
  bool enforce_limits;

  // Enhanced tracking
  u64 allocation_counter;
  u64 tag_limits[MEMORY_TAG_COUNT];
  u64 tag_usage[MEMORY_TAG_COUNT];
  bool advanced_leak_detection;
  bool stack_trace_enabled;

  // Performance metrics
  u64 total_alloc_time_ns;
  u64 total_free_time_ns;
  u64 allocation_count;
} MemoryTracker;

extern MemoryTracker g_memory_tracker;

// Memory tracking
void memory_tracker_init(u32 initial_capacity);
void memory_tracker_shutdown(void);
void *memory_alloc(u32 size, const char *file, u32 line);
void *memory_calloc(u32 count, u32 size, const char *file, u32 line);
void *memory_realloc(void *ptr, u32 new_size, const char *file, u32 line);
void memory_free(void *ptr);
void memory_tracker_report(void);

// Memory limits
void memory_set_limit(u64 limit_bytes);
void memory_set_enforcement(bool enabled);
u64 memory_get_limit(void);
bool memory_is_enforcement_enabled(void);
bool memory_check_limit(u64 requested_size);

// Convenience macros
#define MALLOC(size) memory_alloc(size, __FILE__, __LINE__)
#define CALLOC(count, size) memory_calloc(count, size, __FILE__, __LINE__)
#define REALLOC(ptr, size) memory_realloc(ptr, size, __FILE__, __LINE__)
#define FREE(ptr) memory_free(ptr)

// Legacy aliases
#define core_alloc(size) MALLOC(size)
#define core_realloc(ptr, size) REALLOC(ptr, size)
#define core_free(ptr) FREE(ptr)

// Object pool for fast allocation/deallocation
typedef struct {
  void *objects;
  bool *free_list;
  u32 object_size;
  u32 capacity;
  u32 count;
} ObjectPool;

ObjectPool *object_pool_create(u32 object_size, u32 capacity);
void object_pool_destroy(ObjectPool *pool);
void *object_pool_allocate(ObjectPool *pool);
void object_pool_free(ObjectPool *pool, void *object);
void object_pool_reset(ObjectPool *pool);
u32 object_pool_get_available(ObjectPool *pool);

// Stack allocator for temporary allocations
typedef struct {
  void *base;
  u32 size;
  u32 offset;
  u32 frame_offset;
  u32 peak_usage;
} StackAllocator;

// Linear allocator for ultra-fast per-frame allocations
typedef struct {
  void *base;
  u32 size;
  u32 offset;
  u32 peak_usage;
  u32 buffer_count;
  u32 current_buffer;
  void **buffers; // Double-buffering for multi-threading
} LinearAllocator;

// Buddy allocator for zero-fragmentation variable-size allocations
typedef struct BuddyBlock {
  struct BuddyBlock *left;
  struct BuddyBlock *right;
  struct BuddyBlock *parent;
  u32 size;
  u32 level;
  bool is_free;
  bool is_split;
} BuddyBlock;

typedef struct {
  void *memory_pool;
  u32 pool_size;
  u32 min_block_size;
  u32 max_block_size;
  u32 max_level;
  BuddyBlock *root;
  BuddyBlock **free_lists; // Array of free lists for each level
  u32 total_blocks;
  u32 free_blocks;
} BuddyAllocator;

// Custom allocator interface
typedef struct Allocator Allocator;
typedef struct {
  void *(*alloc)(Allocator *allocator, u32 size, u32 alignment);
  void *(*realloc)(Allocator *allocator, void *ptr, u32 new_size);
  void (*free)(Allocator *allocator, void *ptr);
  void (*reset)(Allocator *allocator);
  u64 (*get_usage)(Allocator *allocator);
  u64 (*get_capacity)(Allocator *allocator);
  void (*get_stats)(Allocator *allocator, char *buffer, u32 buffer_size);
} AllocatorVTable;

typedef struct Allocator {
  const AllocatorVTable *vtable;
  const char *name;
  MemoryTag tag;
  Allocator *fallback; // Fallback allocator
  void *impl;          // Implementation-specific data
} Allocator;

// Memory profiler visualization data
typedef struct {
  u64 timestamp;
  u64 total_usage;
  u64 tag_usage[MEMORY_TAG_COUNT];
  u32 allocation_count;
  u32 free_count;
} MemorySnapshot;

typedef struct {
  MemorySnapshot *snapshots;
  u32 snapshot_count;
  u32 max_snapshots;
  u32 current_index;
  bool recording;
} MemoryProfiler;

StackAllocator *stack_allocator_create(u32 size);
void stack_allocator_destroy(StackAllocator *alloc);
void *stack_allocator_alloc(StackAllocator *alloc, u32 size);
void stack_allocator_reset(StackAllocator *alloc);
void stack_allocator_end_frame(StackAllocator *alloc);

// Memory statistics and pool management
typedef struct {
  u32 pool_id;
  u32 object_size;
  u32 capacity;
  u32 allocated;
  u32 available;
  f32 fragmentation;
} PoolStats;

typedef struct {
  char module_name[64];
  u64 total_allocated;
  u64 total_freed;
  u32 allocation_count;
  u32 active_allocations;
  u64 peak_usage;
} ModuleStats;

PoolStats *memory_get_pool_stats(u32 *out_pool_count);
ModuleStats *memory_get_module_stats(u32 *out_module_count);
u64 memory_get_total_used(void);
u64 memory_get_peak_used(void);
void memory_print_stats(void);
void memory_print_module_stats(void);
void memory_named_region_begin(const char *name);
void memory_named_region_end(void);

// Leak detection
void memory_check_leaks(void);
void memory_dump_allocations(void);

// Linear allocator API
LinearAllocator *linear_allocator_create(u32 size, u32 buffer_count);
void linear_allocator_destroy(LinearAllocator *alloc);
void *linear_allocator_alloc(LinearAllocator *alloc, u32 size, u32 alignment);
void linear_allocator_reset(LinearAllocator *alloc);
void linear_allocator_reset_current(LinearAllocator *alloc);
void linear_allocator_swap_buffers(LinearAllocator *alloc);

// Buddy allocator API
BuddyAllocator *buddy_allocator_create(u32 min_block_size, u32 max_block_size);
void buddy_allocator_destroy(BuddyAllocator *alloc);
void *buddy_allocator_alloc(BuddyAllocator *alloc, u32 size);
void buddy_allocator_free(BuddyAllocator *alloc, void *ptr);
void buddy_allocator_print_stats(BuddyAllocator *alloc);

// SIMD memory operations
void *simd_memcpy(void *dst, const void *src, u32 size);
void *simd_memset(void *dst, u8 value, u32 size);
int simd_memcmp(const void *ptr1, const void *ptr2, u32 size);
bool simd_is_supported(void);

// Memory tagging API
void memory_set_tag_limit(MemoryTag tag, u64 limit);
u64 memory_get_tag_usage(MemoryTag tag);
u64 memory_get_tag_limit(MemoryTag tag);
void memory_print_tag_stats(void);
void *memory_alloc_tagged(u32 size, MemoryTag tag, const char *file, u32 line);

// Custom allocator interface
Allocator *allocator_create_custom(const AllocatorVTable *vtable,
                                   const char *name, MemoryTag tag,
                                   Allocator *fallback);
void allocator_destroy(Allocator *allocator);
void *allocator_alloc(Allocator *allocator, u32 size, u32 alignment);
void allocator_free(Allocator *allocator, void *ptr);
void allocator_reset(Allocator *allocator);
void allocator_get_stats(Allocator *allocator, char *buffer, u32 buffer_size);

// Memory profiler visualization
void memory_profiler_init(u32 max_snapshots);
void memory_profiler_shutdown(void);
void memory_profiler_start_recording(void);
void memory_profiler_stop_recording(void);
void memory_profiler_capture_snapshot(void);
MemorySnapshot *memory_profiler_get_snapshots(u32 *count);
void memory_profiler_export_timeline(const char *filename);

// Advanced leak detection
void memory_enable_advanced_leak_detection(bool enable);
void memory_enable_stack_traces(bool enable);
void memory_capture_stack_trace(void **buffer, u32 max_depth, u32 *out_depth);
char **memory_resolve_stack_trace(void **addresses, u32 depth);
void memory_free_resolved_stack_trace(char **symbols);

// Enhanced allocation macros with tagging
#define MALLOC_TAGGED(size, tag)                                               \
  memory_alloc_tagged(size, tag, __FILE__, __LINE__)
#define MALLOC_TEMP(size) MALLOC_TAGGED(size, MEMORY_TAG_TEMP)
#define MALLOC_PERSISTENT(size) MALLOC_TAGGED(size, MEMORY_TAG_PERSISTENT)
#define MALLOC_ASSET(size) MALLOC_TAGGED(size, MEMORY_TAG_ASSET)
#define MALLOC_RENDERER(size) MALLOC_TAGGED(size, MEMORY_TAG_RENDERER)
#define MALLOC_AUDIO(size) MALLOC_TAGGED(size, MEMORY_TAG_AUDIO)
#define MALLOC_PHYSICS(size) MALLOC_TAGGED(size, MEMORY_TAG_PHYSICS)
#define MALLOC_AI(size) MALLOC_TAGGED(size, MEMORY_TAG_AI)
#define MALLOC_NETWORK(size) MALLOC_TAGGED(size, MEMORY_TAG_NETWORK)
#define MALLOC_UI(size) MALLOC_TAGGED(size, MEMORY_TAG_UI)
#define MALLOC_GAMEPLAY(size) MALLOC_TAGGED(size, MEMORY_TAG_GAMEPLAY)
#define MALLOC_GEOMETRY(size) MALLOC_TAGGED(size, MEMORY_TAG_GEOMETRY)

// ============================================================================
// Vulkan Memory Integration
// ============================================================================
// Unified memory tracking for Vulkan driver allocations via VkAllocationCallbacks

// Vulkan allocator context for tracking driver allocations
typedef struct {
  u64 driver_allocations_count;
  u64 driver_allocated_bytes;
  u64 driver_freed_bytes;
  u64 driver_realloc_count;
  u32 alignment_check_failures;
  bool enabled;
} VulkanAllocatorStats;

// Initialize Vulkan allocation system (internal)
// Callbacks are exposed via memory_get_vulkan_callbacks()

// Get VkAllocationCallbacks structure (opaque pointer to avoid Vulkan dependency in this header)
void* memory_get_vulkan_callbacks(void);

// Vulkan allocator statistics
void memory_get_vulkan_stats(VulkanAllocatorStats* stats);
void memory_print_vulkan_stats(void);

#endif
