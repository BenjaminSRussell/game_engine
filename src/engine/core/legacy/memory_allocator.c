// src/engine/core/legacy/memory_allocator.c
//
// REDIRECTED TO UNIFIED MEMORY ALLOCATOR - CONSOLIDATED SYSTEM
// This file is kept for backwards compatibility only
// All functionality has been moved to unified_memory_allocator.c

#include "../memory/unified_memory_allocator.h"
#include <stdlib.h>
#include <string.h>

// Initialize legacy allocator - redirects to unified system
bool memory_allocator_init(void) {
    // Use default policy for legacy compatibility
    return unified_memory_init(NULL);
}

// Shutdown legacy allocator - redirects to unified system  
void memory_allocator_shutdown(void) {
    unified_memory_shutdown();
}

// Legacy allocation - redirects to unified system
void* memory_allocate(size_t size, const char* file, int line) {
    return unified_memory_alloc(size, MEMORY_STRATEGY_DEFAULT, MEMORY_FLAG_TRACK, file, line);
}

// Legacy reallocation - redirects to unified system
void* memory_reallocate(void* ptr, size_t new_size, const char* file, int line) {
    return unified_memory_realloc(ptr, new_size, MEMORY_STRATEGY_DEFAULT, MEMORY_FLAG_TRACK, file, line);
}

// Legacy deallocation - redirects to unified system
void memory_deallocate(void* ptr) {
    unified_memory_free(ptr);
}

// Get memory statistics - redirects to unified system
void memory_allocator_get_stats(size_t* total_allocated, size_t* peak_allocated, u64* allocation_count) {
    MemoryStats stats;
    unified_memory_get_stats(&stats);
    
    if (total_allocated) *total_allocated = stats.total_allocated;
    if (peak_allocated) *peak_allocated = stats.peak_allocated;
    if (allocation_count) *allocation_count = stats.total_allocations;
}

// Check for memory leaks - redirects to unified system
void memory_allocator_check_leaks(void) {
    unified_memory_check_leaks();
}

// Set memory limit - redirects to unified system
void memory_allocator_set_limit(size_t limit) {
    MemoryPolicy policy = {0};
    unified_memory_get_policy(&policy);
    policy.global_limit = limit;
    unified_memory_set_policy(&policy);
}

// Legacy compatibility - all existing code continues to work
// The unified allocator provides:
// - Memory tracking and leak detection ✓
// - Memory pools for performance ✓  
// - Stack allocators for temporary data ✓
// - Arena allocators for bulk allocations ✓
// - Guard pages and canaries for corruption detection ✓
// - Statistics and monitoring ✓
// - Fragmentation analysis ✓
// - Hot-spot detection ✓
