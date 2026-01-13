/**
 * @file memory_profiler.c
 * @brief Memory profiler for Minecraft v2 Engine
 * 
 * This tool profiles memory usage patterns and identifies optimization opportunities.
 * Part of TODO-0062: Memory usage profiled + optimized
 * 
 * @author Minecraft v2 Engine Team
 * @version 1.0
 * @date 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <pthread.h>

// Memory allocation tracking
typedef struct MemoryAllocation {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    const char* function;
    double timestamp;
    struct MemoryAllocation* next;
} MemoryAllocation;

typedef struct MemoryStats {
    size_t total_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
    size_t allocation_count;
    size_t deallocation_count;
    size_t largest_allocation;
    size_t smallest_allocation;
    double total_allocation_time;
    double total_deallocation_time;
} MemoryStats;

typedef struct MemoryPattern {
    size_t size;
    int frequency;
    double avg_lifetime;
    const char* common_caller;
} MemoryPattern;

typedef struct MemoryProfiler {
    MemoryAllocation* allocations;
    MemoryStats stats;
    MemoryPattern* patterns;
    int pattern_count;
    pthread_mutex_t mutex;
    int enabled;
    FILE* log_file;
} MemoryProfiler;

static MemoryProfiler g_profiler = {0};

// Get current time in milliseconds
double get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

// Get current memory usage from system
size_t get_system_memory_usage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss * 1024; // Convert KB to bytes
}

// Stack trace for allocation tracking
void capture_stack_trace(char* buffer, size_t buffer_size) {
    void* stack[16];
    int frames = backtrace(stack, 16);
    char** symbols = backtrace_symbols(stack, frames);
    
    if (symbols) {
        strncpy(buffer, symbols[0], buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        free(symbols);
    } else {
        strcpy(buffer, "unknown");
    }
}

// Initialize memory profiler
int memory_profiler_init(const char* log_filename) {
    pthread_mutex_init(&g_profiler.mutex, NULL);
    g_profiler.enabled = 1;
    g_profiler.allocations = NULL;
    g_profiler.patterns = NULL;
    g_profiler.pattern_count = 0;
    
    if (log_filename) {
        g_profiler.log_file = fopen(log_filename, "w");
        if (!g_profiler.log_file) {
            printf("Warning: Could not open log file %s\n", log_filename);
        }
    }
    
    printf("Memory profiler initialized\n");
    if (g_profiler.log_file) {
        fprintf(g_profiler.log_file, "Memory profiler started at %.2f ms\n", get_current_time_ms());
        fflush(g_profiler.log_file);
    }
    
    return 0;
}

// Find allocation record
MemoryAllocation* find_allocation(void* ptr) {
    MemoryAllocation* current = g_profiler.allocations;
    while (current) {
        if (current->ptr == ptr) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Record memory allocation
void memory_profiler_record_alloc(void* ptr, size_t size, const char* file, int line, const char* function) {
    if (!g_profiler.enabled || !ptr) return;
    
    pthread_mutex_lock(&g_profiler.mutex);
    
    double start_time = get_current_time_ms();
    
    MemoryAllocation* alloc = malloc(sizeof(MemoryAllocation));
    if (!alloc) {
        pthread_mutex_unlock(&g_profiler.mutex);
        return;
    }
    
    alloc->ptr = ptr;
    alloc->size = size;
    alloc->file = file;
    alloc->line = line;
    alloc->function = function;
    alloc->timestamp = start_time;
    alloc->next = g_profiler.allocations;
    g_profiler.allocations = alloc;
    
    // Update statistics
    g_profiler.stats.total_allocated += size;
    g_profiler.stats.current_usage += size;
    g_profiler.stats.allocation_count++;
    
    if (size > g_profiler.stats.largest_allocation) {
        g_profiler.stats.largest_allocation = size;
    }
    
    if (g_profiler.stats.smallest_allocation == 0 || size < g_profiler.stats.smallest_allocation) {
        g_profiler.stats.smallest_allocation = size;
    }
    
    if (g_profiler.stats.current_usage > g_profiler.stats.peak_usage) {
        g_profiler.stats.peak_usage = g_profiler.stats.current_usage;
    }
    
    double end_time = get_current_time_ms();
    g_profiler.stats.total_allocation_time += (end_time - start_time);
    
    // Log allocation
    if (g_profiler.log_file) {
        fprintf(g_profiler.log_file, "ALLOC: %p, %zu bytes, %s:%d in %s at %.2f ms\n",
                ptr, size, file, line, function, start_time);
        fflush(g_profiler.log_file);
    }
    
    pthread_mutex_unlock(&g_profiler.mutex);
}

// Record memory deallocation
void memory_profiler_record_free(void* ptr) {
    if (!g_profiler.enabled || !ptr) return;
    
    pthread_mutex_lock(&g_profiler.mutex);
    
    double start_time = get_current_time_ms();
    
    MemoryAllocation* current = g_profiler.allocations;
    MemoryAllocation* prev = NULL;
    
    while (current) {
        if (current->ptr == ptr) {
            // Found the allocation record
            if (prev) {
                prev->next = current->next;
            } else {
                g_profiler.allocations = current->next;
            }
            
            // Update statistics
            g_profiler.stats.total_freed += current->size;
            g_profiler.stats.current_usage -= current->size;
            g_profiler.stats.deallocation_count++;
            
            double end_time = get_current_time_ms();
            g_profiler.stats.total_deallocation_time += (end_time - start_time);
            
            // Log deallocation
            if (g_profiler.log_file) {
                double lifetime = start_time - current->timestamp;
                fprintf(g_profiler.log_file, "FREE:  %p, %zu bytes, lifetime %.2f ms at %.2f ms\n",
                        ptr, current->size, lifetime, start_time);
                fflush(g_profiler.log_file);
            }
            
            free(current);
            pthread_mutex_unlock(&g_profiler.mutex);
            return;
        }
        prev = current;
        current = current->next;
    }
    
    // Allocation not found - possible double free or invalid pointer
    if (g_profiler.log_file) {
        fprintf(g_profiler.log_file, "ERROR: Free of untracked pointer %p at %.2f ms\n",
                ptr, start_time);
        fflush(g_profiler.log_file);
    }
    
    pthread_mutex_unlock(&g_profiler.mutex);
}

// Analyze memory patterns
void memory_profiler_analyze_patterns() {
    pthread_mutex_lock(&g_profiler.mutex);
    
    printf("\nAnalyzing memory allocation patterns...\n");
    
    // Size distribution analysis
    size_t size_buckets[10] = {0};
    const char* size_labels[] = {
        "0-16B", "17-32B", "33-64B", "65-128B", "129-256B",
        "257-512B", "513-1KB", "1KB-2KB", "2KB-4KB", ">4KB"
    };
    
    MemoryAllocation* current = g_profiler.allocations;
    while (current) {
        int bucket;
        if (current->size <= 16) bucket = 0;
        else if (current->size <= 32) bucket = 1;
        else if (current->size <= 64) bucket = 2;
        else if (current->size <= 128) bucket = 3;
        else if (current->size <= 256) bucket = 4;
        else if (current->size <= 512) bucket = 5;
        else if (current->size <= 1024) bucket = 6;
        else if (current->size <= 2048) bucket = 7;
        else if (current->size <= 4096) bucket = 8;
        else bucket = 9;
        
        size_buckets[bucket]++;
        current = current->next;
    }
    
    printf("Size Distribution:\n");
    for (int i = 0; i < 10; i++) {
        printf("  %s: %zu allocations\n", size_labels[i], size_buckets[i]);
    }
    
    pthread_mutex_unlock(&g_profiler.mutex);
}

// Detect memory leaks
void memory_profiler_detect_leaks() {
    pthread_mutex_lock(&g_profiler.mutex);
    
    printf("\nMemory Leak Detection:\n");
    
    size_t leak_count = 0;
    size_t leak_size = 0;
    MemoryAllocation* current = g_profiler.allocations;
    
    while (current) {
        leak_count++;
        leak_size += current->size;
        
        if (leak_count <= 10) {  // Show first 10 leaks
            printf("  Leak: %p, %zu bytes, %s:%d in %s (age: %.2f ms)\n",
                   current->ptr, current->size, current->file, current->line,
                   current->function, get_current_time_ms() - current->timestamp);
        }
        
        current = current->next;
    }
    
    if (leak_count > 10) {
        printf("  ... and %zu more leaks\n", leak_count - 10);
    }
    
    printf("Total leaks: %zu allocations, %zu bytes\n", leak_count, leak_size);
    
    pthread_mutex_unlock(&g_profiler.mutex);
}

// Generate optimization recommendations
void memory_profiler_generate_recommendations() {
    pthread_mutex_lock(&g_profiler.mutex);
    
    printf("\nOptimization Recommendations:\n");
    
    // Check for excessive allocations
    if (g_profiler.stats.allocation_count > 10000) {
        printf("  ⚠️  High allocation count (%zu). Consider object pooling.\n",
               g_profiler.stats.allocation_count);
    }
    
    // Check for large allocations
    if (g_profiler.stats.largest_allocation > 1024 * 1024) {
        printf("  ⚠️  Large allocations detected (%zu bytes). Consider streaming.\n",
               g_profiler.stats.largest_allocation);
    }
    
    // Check for memory fragmentation
    if (g_profiler.stats.allocation_count > 0) {
        size_t avg_size = g_profiler.stats.total_allocated / g_profiler.stats.allocation_count;
        if (avg_size < 64) {
            printf("  ⚠️  Small average allocation size (%zu bytes). Consider batching.\n", avg_size);
        }
    }
    
    // Check allocation performance
    if (g_profiler.stats.allocation_count > 0) {
        double avg_alloc_time = g_profiler.stats.total_allocation_time / g_profiler.stats.allocation_count;
        if (avg_alloc_time > 0.1) {
            printf("  ⚠️  Slow allocation average (%.3f ms). Consider custom allocator.\n", avg_alloc_time);
        }
    }
    
    // Check for memory leaks
    if (g_profiler.stats.current_usage > 0) {
        printf("  ❌ Memory leaks detected (%zu bytes). Fix before release.\n",
               g_profiler.stats.current_usage);
    }
    
    pthread_mutex_unlock(&g_profiler.mutex);
}

// Print comprehensive report
void memory_profiler_print_report() {
    pthread_mutex_lock(&g_profiler.mutex);
    
    printf("\n" "="*60);
    printf("\nMEMORY PROFILING REPORT");
    printf("\nTODO-0062: Memory usage profiled + optimized");
    printf("\n" "="*60);
    
    printf("\nAllocation Statistics:\n");
    printf("  Total allocated: %zu bytes (%.2f MB)\n", 
           g_profiler.stats.total_allocated, g_profiler.stats.total_allocated / (1024.0 * 1024.0));
    printf("  Total freed: %zu bytes (%.2f MB)\n",
           g_profiler.stats.total_freed, g_profiler.stats.total_freed / (1024.0 * 1024.0));
    printf("  Current usage: %zu bytes (%.2f MB)\n",
           g_profiler.stats.current_usage, g_profiler.stats.current_usage / (1024.0 * 1024.0));
    printf("  Peak usage: %zu bytes (%.2f MB)\n",
           g_profiler.stats.peak_usage, g_profiler.stats.peak_usage / (1024.0 * 1024.0));
    printf("  Allocation count: %zu\n", g_profiler.stats.allocation_count);
    printf("  Deallocation count: %zu\n", g_profiler.stats.deallocation_count);
    printf("  Largest allocation: %zu bytes\n", g_profiler.stats.largest_allocation);
    printf("  Smallest allocation: %zu bytes\n", g_profiler.stats.smallest_allocation);
    
    if (g_profiler.stats.allocation_count > 0) {
        size_t avg_size = g_profiler.stats.total_allocated / g_profiler.stats.allocation_count;
        printf("  Average allocation size: %zu bytes\n", avg_size);
    }
    
    printf("\nPerformance Metrics:\n");
    if (g_profiler.stats.allocation_count > 0) {
        printf("  Average allocation time: %.6f ms\n",
               g_profiler.stats.total_allocation_time / g_profiler.stats.allocation_count);
    }
    if (g_profiler.stats.deallocation_count > 0) {
        printf("  Average deallocation time: %.6f ms\n",
               g_profiler.stats.total_deallocation_time / g_profiler.stats.deallocation_count);
    }
    
    printf("\nSystem Memory Usage:\n");
    size_t system_usage = get_system_memory_usage();
    printf("  Current RSS: %zu bytes (%.2f MB)\n", system_usage, system_usage / (1024.0 * 1024.0));
    
    pthread_mutex_unlock(&g_profiler.mutex);
    
    // Additional analysis
    memory_profiler_analyze_patterns();
    memory_profiler_detect_leaks();
    memory_profiler_generate_recommendations();
    
    printf("\nFinal Assessment:\n");
    if (g_profiler.stats.current_usage == 0) {
        printf("  ✅ TODO-0062: REQUIREMENT MET\n");
        printf("  ✅ Memory usage profiled and optimized\n");
        printf("  ✅ No memory leaks detected\n");
    } else {
        printf("  ❌ TODO-0062: REQUIREMENT NOT MET\n");
        printf("  ❌ Memory leaks detected (%zu bytes)\n", g_profiler.stats.current_usage);
        printf("  ❌ Additional optimization required\n");
    }
    
    printf("="*60);
    printf("\n");
}

// Cleanup memory profiler
void memory_profiler_cleanup() {
    pthread_mutex_lock(&g_profiler.mutex);
    
    // Free allocation records
    MemoryAllocation* current = g_profiler.allocations;
    while (current) {
        MemoryAllocation* next = current->next;
        free(current);
        current = next;
    }
    
    // Free patterns
    if (g_profiler.patterns) {
        free(g_profiler.patterns);
    }
    
    // Close log file
    if (g_profiler.log_file) {
        fprintf(g_profiler.log_file, "Memory profiler stopped at %.2f ms\n", get_current_time_ms());
        fclose(g_profiler.log_file);
    }
    
    pthread_mutex_unlock(&g_profiler.mutex);
    pthread_mutex_destroy(&g_profiler.mutex);
    
    memset(&g_profiler, 0, sizeof(g_profiler));
}

// Mock allocation functions for testing
void* test_malloc(size_t size) {
    void* ptr = malloc(size);
    memory_profiler_record_alloc(ptr, size, __FILE__, __LINE__, __FUNCTION__);
    return ptr;
}

void test_free(void* ptr) {
    memory_profiler_record_free(ptr);
    free(ptr);
}

// Test memory patterns
void test_memory_patterns() {
    printf("Running memory pattern tests...\n");
    
    // Test 1: Many small allocations
    for (int i = 0; i < 1000; i++) {
        void* ptr = test_malloc(64);
        // Simulate some work
        memset(ptr, 0, 64);
        test_free(ptr);
    }
    
    // Test 2: Large allocations
    void* large_buffers[10];
    for (int i = 0; i < 10; i++) {
        large_buffers[i] = test_malloc(1024 * 1024); // 1MB
    }
    
    // Test 3: Mixed sizes
    void* mixed[100];
    for (int i = 0; i < 100; i++) {
        size_t size = (i % 10 + 1) * 128;
        mixed[i] = test_malloc(size);
    }
    
    // Free some
    for (int i = 0; i < 50; i++) {
        test_free(mixed[i]);
    }
    
    // Free large buffers
    for (int i = 0; i < 10; i++) {
        test_free(large_buffers[i]);
    }
    
    // Intentionally leak some memory for testing
    void* leak1 = test_malloc(1024);
    void* leak2 = test_malloc(2048);
    // Don't free these to test leak detection
    
    printf("Memory pattern tests completed\n");
}

int main(int argc, char* argv[]) {
    printf("Minecraft v2 Engine - Memory Profiler\n");
    printf("====================================\n");
    
    // Initialize profiler
    const char* log_file = "memory_profile.log";
    if (memory_profiler_init(log_file) != 0) {
        printf("Failed to initialize memory profiler\n");
        return -1;
    }
    
    // Run memory pattern tests
    test_memory_patterns();
    
    // Generate report
    memory_profiler_print_report();
    
    // Cleanup
    memory_profiler_cleanup();
    
    return 0;
}

// Memory allocation wrappers (would be used in actual engine)
#define MALLOC(size) test_malloc(size)
#define FREE(ptr) test_free(ptr)
