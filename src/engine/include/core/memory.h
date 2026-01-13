// include/core/memory.h
//
// Purpose: REDIRECTED TO UNIFIED MEMORY ALLOCATOR - CONSOLIDATED SYSTEM
//
#ifndef MEMORY_H
#define MEMORY_H

// Redirect to unified memory allocator that consolidates all memory systems
#include "memory/unified_memory_allocator.h"

// Note: All memory management functionality has been consolidated into unified_memory_allocator.h
// This header is kept for backwards compatibility only

// Basic memory macros for compatibility (these are defined in unified allocator when UNIFIED_MEMORY_ALLOCATOR_MAIN is not set)
#define MALLOC(size) memory_alloc(size)
#define CALLOC(count, size) memory_calloc(count, size)
#define REALLOC(ptr, size) memory_realloc(ptr, size)
#define FREE(ptr) memory_free(ptr)

// Additional compatibility macros for legacy code not defined in unified allocator
#define MALLOC_PERSISTENT(size) memory_alloc(size)
#define REALLOC_PERSISTENT(ptr, size) memory_realloc(ptr, size)
#define CALLOC_PERSISTENT(count, size) memory_calloc(count, size)

// Global allocator instances for compatibility
extern void *g_persistent_allocator;
extern void *g_temp_allocator;

// Allocator compatibility functions
static inline void *allocator_alloc(void *allocator, size_t size) {
    return memory_alloc(size);
}

static inline void allocator_free(void *allocator, void *ptr) {
    memory_free(ptr);
}

#endif // MEMORY_H
