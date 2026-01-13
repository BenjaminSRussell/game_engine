// src/core/memory/memory.c
//
// REDIRECTED TO UNIFIED MEMORY ALLOCATOR - CONSOLIDATED SYSTEM
// This file is kept for backwards compatibility only
// All functionality has been moved to unified_memory_allocator.c

#include "../../engine/include/core/memory/unified_memory_allocator.h"
#include <stdlib.h>
#include <string.h>

// Initialize memory tracking system
bool memory_init(void) {
    return unified_memory_init(NULL);
}

// Shutdown memory tracking system
void memory_shutdown(void) {
    unified_memory_shutdown();
}

// Allocate memory with tracking
void* memory_alloc(u32 size, const char* file, u32 line) {
    return unified_memory_alloc(size, MEMORY_STRATEGY_TRACKED, MEMORY_FLAG_TRACK, file, line);
}

// Allocate and zero memory with tracking
void* memory_calloc(u32 count, u32 size, const char* file, u32 line) {
    void* ptr = unified_memory_alloc(count * size, MEMORY_STRATEGY_TRACKED, MEMORY_FLAG_TRACK | MEMORY_FLAG_ZERO, file, line);
    return ptr;
}

// Reallocate memory with tracking
void* memory_realloc(void* ptr, u32 new_size, const char* file, u32 line) {
    return unified_memory_realloc(ptr, new_size, MEMORY_STRATEGY_TRACKED, MEMORY_FLAG_TRACK, file, line);
}

// Free memory with tracking
void memory_free(void* ptr) {
    unified_memory_free(ptr);
}

// Set global memory limit
void memory_set_limit(u64 limit_bytes) {
    MemoryPolicy policy = {0};
    unified_memory_get_policy(&policy);
    policy.global_limit = limit_bytes;
    unified_memory_set_policy(&policy);
}

// Generate memory report
void memory_tracker_report(void) {
    unified_memory_print_stats();
}

// All memory management is now handled by the unified allocator system
// This provides: tracking, leak detection, pools, fragmentation analysis, etc.
