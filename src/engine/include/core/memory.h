// include/core/memory.h
//
// Purpose: REDIRECTED TO UNIFIED MEMORY ALLOCATOR - CONSOLIDATED SYSTEM
//
#ifndef CORE_MEMORY_H
#define CORE_MEMORY_H

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

// Global allocator instances for compatibility
extern void *g_persistent_allocator;
extern void *g_temp_allocator;

// Additional compatibility macros for legacy code
#define MALLOC_PERSISTENT(size) memory_alloc(size)
#define REALLOC_PERSISTENT(ptr, size) memory_realloc(ptr, size)
#define CALLOC_PERSISTENT(count, size) memory_calloc(count, size)

// Allocator compatibility functions
static inline void *allocator_alloc(void *allocator, size_t size) {
    return memory_alloc(size);
}

static inline void allocator_free(void *allocator, void *ptr) {
    memory_free(ptr);
}

#endif // CORE_MEMORY_H
