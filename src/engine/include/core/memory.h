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

#endif // MEMORY_H
