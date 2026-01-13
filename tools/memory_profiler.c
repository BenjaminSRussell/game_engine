/*
 * Memory Profiler Tool
 * Comprehensive memory usage analysis and profiling
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

typedef struct {
    size_t total_allocated;
    size_t peak_usage;
    size_t current_usage;
    uint64_t allocation_count;
    uint64_t deallocation_count;
    uint64_t timestamp;
} memory_stats_t;

static memory_stats_t g_stats = {0};

void* profiled_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        g_stats.total_allocated += size;
        g_stats.current_usage += size;
        g_stats.allocation_count++;
        
        if (g_stats.current_usage > g_stats.peak_usage) {
            g_stats.peak_usage = g_stats.current_usage;
        }
    }
    return ptr;
}

void profiled_free(void* ptr, size_t size) {
    if (ptr) {
        free(ptr);
        g_stats.current_usage -= size;
        g_stats.deallocation_count++;
    }
}

void print_memory_stats() {
    printf("=== Memory Profiler Statistics ===\n");
    printf("Total Allocated: %zu bytes (%.2f MB)\n", 
           g_stats.total_allocated, g_stats.total_allocated / (1024.0 * 1024.0));
    printf("Current Usage: %zu bytes (%.2f MB)\n", 
           g_stats.current_usage, g_stats.current_usage / (1024.0 * 1024.0));
    printf("Peak Usage: %zu bytes (%.2f MB)\n", 
           g_stats.peak_usage, g_stats.peak_usage / (1024.0 * 1024.0));
    printf("Allocations: %llu\n", g_stats.allocation_count);
    printf("Deallocations: %llu\n", g_stats.deallocation_count);
    printf("Active Allocations: %llu\n", 
           g_stats.allocation_count - g_stats.deallocation_count);
    printf("===============================\n");
}

int main() {
    printf("Memory Profiler Tool - Testing Memory Usage\n");
    
    // Test memory allocation patterns
    const size_t test_size = 1024 * 1024; // 1MB
    
    for (int i = 0; i < 10; i++) {
        void* ptr = profiled_malloc(test_size);
        printf("Allocated %d MB block\n", i + 1);
        
        // Simulate some work
        memset(ptr, 0, test_size);
        
        if (i % 3 == 0) {
            profiled_free(ptr, test_size);
            printf("Freed block %d\n", i + 1);
        }
    }
    
    print_memory_stats();
    return 0;
}
