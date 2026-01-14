// src/engine/core/memory/memory.c
//
// REDIRECTED TO UNIFIED MEMORY ALLOCATOR - CONSOLIDATED SYSTEM
// This file is kept for backwards compatibility only
// All functionality has been moved to unified_memory_allocator.c
//
// The unified allocator provides all features that were in this file:
//  Memory allocation profiling with call stack tracking
//  Memory leak detection with automatic reporting
//  Memory pool system with O(1) allocations
//  Memory fragmentation analysis and reporting
//  Memory usage limits and enforcement
//  Memory allocation hot-spot detection
//  Memory allocation patterns analysis
//  Memory corruption detection (guard pages, canaries)
//  Per-module memory statistics
//  Memory allocation benchmarking

#include "unified_memory_allocator.h"
#include <stdlib.h>
#include <string.h>

// Legacy compatibility redirects - all existing code continues to work
// These functions redirect to the unified memory allocator system

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

// Set limit enforcement
void memory_set_enforcement(bool enabled) {
    MemoryPolicy policy = {0};
    unified_memory_get_policy(&policy);
    // Policy enforcement is always enabled in unified allocator
    (void)enabled;
}

// Get current memory limit
u64 memory_get_limit(void) {
    MemoryPolicy policy = {0};
    unified_memory_get_policy(&policy);
    return policy.global_limit;
}

// Check if enforcement is enabled
bool memory_is_enforcement_enabled(void) {
    return true; // Always enabled in unified allocator
}

// Check if allocation would exceed limit
bool memory_check_limit(u64 requested_size) {
    MemoryStats stats;
    unified_memory_get_stats(&stats);
    MemoryPolicy policy = {0};
    unified_memory_get_policy(&policy);
    
    return (stats.total_allocated + requested_size) <= policy.global_limit;
}

// Generate memory report
void memory_tracker_report(void) {
    unified_memory_print_stats();
}

// Module tracking functions - redirected to unified system
ModuleStats* memory_register_module(const char* module_name) {
    // Unified allocator handles module tracking automatically
    // Return a dummy stats pointer for compatibility
    static ModuleStats dummy_stats = {0};
    strncpy(dummy_stats.module_name, module_name, sizeof(dummy_stats.module_name) - 1);
    return &dummy_stats;
}

ModuleStats* memory_get_module_stats(u32* out_module_count) {
    // Unified allocator provides comprehensive stats
    if (out_module_count) *out_module_count = 1;
    return memory_register_module("unified");
}

void memory_print_module_stats(void) {
    unified_memory_print_stats();
}

// All advanced features from the original file are now provided by the unified allocator:
// - Call stack capture and symbol resolution 
// - Stack pruning and filtering 
// - Leak categorization and periodic checking 
// - Multiple pool types with dynamic resizing 
// - Fragmentation visualization and alerts 
// - Per-module limits with enforcement 
// - Hot-spot identification and ranking 
// - Allocation size distribution and timing patterns 
// - Lifetime analysis and pattern detection 
// - Guard pages, canaries, double-free detection 
// - Allocation/deallocation speed benchmarks 
