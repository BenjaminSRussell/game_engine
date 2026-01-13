#ifndef MEMORY_H
#define MEMORY_H

#include "math/types.h"

// Memory tags
typedef enum {
    MEMORY_TAG_UNKNOWN,
    MEMORY_TAG_ARRAY,
    MEMORY_TAG_LINEAR_ALLOCATOR,
    MEMORY_TAG_DARRAY,
    MEMORY_TAG_DICT,
    MEMORY_TAG_RING_QUEUE,
    MEMORY_TAG_BST,
    MEMORY_TAG_STRING,
    MEMORY_TAG_ENGINE,
    MEMORY_TAG_JOB,
    MEMORY_TAG_TEXTURE,
    MEMORY_TAG_MATERIAL,
    MEMORY_TAG_RENDERER,
    MEMORY_TAG_GAME,
    MEMORY_TAG_TRANSFORM,
    MEMORY_TAG_ENTITY,
    MEMORY_TAG_ENTITY_NODE,
    MEMORY_TAG_SCENE,
    MEMORY_TAG_TEMP,
    MEMORY_TAG_PERSISTENT,
    MEMORY_TAG_ASSET,
    MEMORY_TAG_AUDIO,
    MEMORY_TAG_PHYSICS,
    MEMORY_TAG_AI,
    MEMORY_TAG_NETWORK,
    MEMORY_TAG_UI,
    MEMORY_TAG_GAMEPLAY,
    MEMORY_TAG_GEOMETRY,
    MEMORY_TAG_COUNT
} MemoryTag;

// Redirect to the unified memory allocator that consolidates all memory systems
#include "memory/unified_memory_allocator.h"

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
// memory_alloc/free are macros in unified_memory_allocator.h, so we don't declare them here unless we undef

// Memory limits
void memory_set_limit(u64 limit_bytes);
void memory_set_enforcement(bool enabled);
u64 memory_get_limit(void);
bool memory_is_enforcement_enabled(void);
bool memory_check_limit(u64 requested_size);

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

// Vulkan allocator context for tracking driver allocations
typedef struct {
  u64 driver_allocations_count;
  u64 driver_allocated_bytes;
  u64 driver_freed_bytes;
  u64 driver_realloc_count;
  u32 alignment_check_failures;
  bool enabled;
} VulkanAllocatorStats;

// Get VkAllocationCallbacks structure
void *memory_get_vulkan_callbacks(void);

// Vulkan allocator statistics
void memory_get_vulkan_stats(VulkanAllocatorStats *stats);
void memory_print_vulkan_stats(void);

#endif // MEMORY_H
