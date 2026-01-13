// include/core/memory.h
//
// Purpose: REDIRECTED TO UNIFIED MEMORY ALLOCATOR - CONSOLIDATED SYSTEM
//
#ifndef MEMORY_H
#define MEMORY_H

// Redirect to the unified memory allocator that consolidates all memory systems
#include "memory/unified_memory_allocator.h"

// Note: All memory management functionality has been consolidated into unified_memory_allocator.h
// This header is kept for backwards compatibility only

// Legacy compatibility - all existing code continues to work
// The unified allocator provides:
// - Memory tracking and leak detection
// - Memory pools for performance
// - Stack allocators for temporary data
// - Arena allocators for bulk allocations
// - Guard pages and canaries for corruption detection
// - Statistics and monitoring
// - Fragmentation analysis
// - Hot-spot detection

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
    MEMORY_TAG_MATERIAL_INSTANCE,
    MEMORY_TAG_RENDERER,
    MEMORY_TAG_GAME,
    MEMORY_TAG_TRANSFORM,
    MEMORY_TAG_ENTITY,
    MEMORY_TAG_ENTITY_NODE,
    MEMORY_TAG_SCENE,
    MEMORY_TAG_RESOURCE,
    MEMORY_TAG_VULKAN,
    MEMORY_TAG_DIRECT3D,
    MEMORY_TAG_OPENGL,
    MEMORY_TAG_GPU_BUFFER,
    MEMORY_TAG_SHADER,
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
// memory_alloc/free replaced by unified_memory_allocator macros
void memory_tracker_report(void);

// Memory limits
void memory_set_limit(u64 limit_bytes);
void memory_set_enforcement(bool enabled);
u64 memory_get_limit(void);
bool memory_is_enforcement_enabled(void);
bool memory_check_limit(u64 requested_size);

// Convenience macros (MALLOC etc provided by unified_memory_allocator.h)

// Legacy aliases
#define core_alloc(size) MALLOC(size)
#define core_realloc(ptr, size) REALLOC(ptr, size)
#define core_free(ptr) FREE(ptr)

// Legacy ObjectPool alias
typedef MemoryPool ObjectPool;

#endif
