// include/core/memory.h
//
// Purpose: REDIRECTED TO UNIFIED MEMORY ALLOCATOR - CONSOLIDATED SYSTEM
//
#ifndef MEMORY_H
#define MEMORY_H

#include "math/types.h"

// Memory tags need to be defined before including unified_memory_allocator.h
typedef enum {
    MEMORY_TAG_UNKNOWN = 0,
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

// Redirect to the unified memory allocator that consolidates all memory systems
#include "memory/unified_memory_allocator.h"

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

// Linear allocator for ultra-fast per-frame allocations
// Note: LinearAllocator struct is not defined in unified_memory_allocator.h
typedef struct {
  void *base;
  u32 size;
  u32 offset;
  u32 peak_usage;
  u32 buffer_count;
  u32 current_buffer;
  void **buffers; // Double-buffering for multi-threading
} LinearAllocator;

#endif // MEMORY_H
