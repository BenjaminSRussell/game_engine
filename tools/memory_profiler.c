/*
 * memory_profiler.c
 * Memory profiling and leak detection tool for Phase 5 implementation
 *
 * Provides comprehensive memory analysis, leak detection, and optimization metrics
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <signal.h>
#include <execinfo.h>

#include "gpu_cull_compute.h"
#include "cull_output_buffer.h"
#include "cull_statistics.h"

/* ============================================================================
 * MEMORY TRACKING STRUCTURES
 * ============================================================================ */

typedef struct memory_allocation {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    const char* function;
    uint64_t timestamp;
    struct memory_allocation* next;
} memory_allocation_t;

typedef struct memory_stats {
    size_t total_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
    uint64_t allocation_count;
    uint64_t free_count;
    uint64_t leak_count;
} memory_stats_t;

/* ============================================================================
 * GLOBAL MEMORY TRACKER
 * ============================================================================ */

static struct {
    memory_allocation_t* allocations;
    pthread_mutex_t mutex;
    memory_stats_t stats;
    bool tracking_enabled;
    FILE* leak_log;
} memory_tracker = {0};

/* ============================================================================
 * MEMORY TRACKING FUNCTIONS
 * ============================================================================ */

static void memory_tracker_init(void) {
    pthread_mutex_init(&memory_tracker.mutex, NULL);
    memset(&memory_tracker.stats, 0, sizeof(memory_tracker.stats));
    memory_tracker.allocations = NULL;
    memory_tracker.tracking_enabled = true;
    memory_tracker.leak_log = fopen("memory_leaks.log", "w");
    
    if (memory_tracker.leak_log) {
        fprintf(memory_tracker.leak_log, "Memory Leak Detection Log\n");
        fprintf(memory_tracker.leak_log, "=============================\n\n");
    }
}

static void memory_tracker_shutdown(void) {
    pthread_mutex_lock(&memory_tracker.mutex);
    
    // Report remaining allocations (leaks)
    if (memory_tracker.leak_log) {
        memory_allocation_t* current = memory_tracker.allocations;
        while (current) {
            fprintf(memory_tracker.leak_log, 
                   "LEAK: %zu bytes at %p allocated in %s:%d (%s)\n",
                   current->size, current->ptr, 
                   current->file ? current->file : "unknown",
                   current->line, 
                   current->function ? current->function : "unknown");
            current = current->next;
        }
        
        fprintf(memory_tracker.leak_log, "\n=== Memory Statistics ===\n");
        fprintf(memory_tracker.leak_log, "Total allocated: %zu bytes\n", memory_tracker.stats.total_allocated);
        fprintf(memory_tracker.leak_log, "Total freed: %zu bytes\n", memory_tracker.stats.total_freed);
        fprintf(memory_tracker.leak_log, "Current usage: %zu bytes\n", memory_tracker.stats.current_usage);
        fprintf(memory_tracker.leak_log, "Peak usage: %zu bytes\n", memory_tracker.stats.peak_usage);
        fprintf(memory_tracker.leak_log, "Allocation count: %llu\n", (unsigned long long)memory_tracker.stats.allocation_count);
        fprintf(memory_tracker.leak_log, "Free count: %llu\n", (unsigned long long)memory_tracker.stats.free_count);
        fprintf(memory_tracker.leak_log, "Leak count: %llu\n", (unsigned long long)memory_tracker.stats.leak_count);
        
        fclose(memory_tracker.leak_log);
    }
    
    pthread_mutex_unlock(&memory_tracker.mutex);
    pthread_mutex_destroy(&memory_tracker.mutex);
}

static void track_allocation(void* ptr, size_t size, const char* file, int line, const char* function) {
    if (!memory_tracker.tracking_enabled || !ptr) return;
    
    pthread_mutex_lock(&memory_tracker.mutex);
    
    memory_allocation_t* alloc = malloc(sizeof(memory_allocation_t));
    if (alloc) {
        alloc->ptr = ptr;
        alloc->size = size;
        alloc->file = file;
        alloc->line = line;
        alloc->function = function;
        alloc->timestamp = time(NULL);
        alloc->next = memory_tracker.allocations;
        
        memory_tracker.allocations = alloc;
        
        // Update statistics
        memory_tracker.stats.total_allocated += size;
        memory_tracker.stats.current_usage += size;
        memory_tracker.stats.allocation_count++;
        
        if (memory_tracker.stats.current_usage > memory_tracker.stats.peak_usage) {
            memory_tracker.stats.peak_usage = memory_tracker.stats.current_usage;
        }
    }
    
    pthread_mutex_unlock(&memory_tracker.mutex);
}

static void track_deallocation(void* ptr) {
    if (!memory_tracker.tracking_enabled || !ptr) return;
    
    pthread_mutex_lock(&memory_tracker.mutex);
    
    memory_allocation_t** current = &memory_tracker.allocations;
    while (*current) {
        if ((*current)->ptr == ptr) {
            memory_allocation_t* to_remove = *current;
            *current = to_remove->next;
            
            // Update statistics
            memory_tracker.stats.total_freed += to_remove->size;
            memory_tracker.stats.current_usage -= to_remove->size;
            memory_tracker.stats.free_count++;
            
            free(to_remove);
            break;
        }
        current = &(*current)->next;
    }
    
    pthread_mutex_unlock(&memory_tracker.mutex);
}

/* ============================================================================
 * CUSTOM MEMORY ALLOCATORS
 * ============================================================================ */

void* tracked_malloc(size_t size, const char* file, int line, const char* function) {
    void* ptr = malloc(size);
    if (ptr) {
        track_allocation(ptr, size, file, line, function);
    }
    return ptr;
}

void* tracked_calloc(size_t nmemb, size_t size, const char* file, int line, const char* function) {
    void* ptr = calloc(nmemb, size);
    if (ptr) {
        track_allocation(ptr, nmemb * size, file, line, function);
    }
    return ptr;
}

void* tracked_realloc(void* ptr, size_t size, const char* file, int line, const char* function) {
    if (ptr) {
        track_deallocation(ptr);
    }
    
    void* new_ptr = realloc(ptr, size);
    if (new_ptr) {
        track_allocation(new_ptr, size, file, line, function);
    }
    return new_ptr;
}

void tracked_free(void* ptr) {
    if (ptr) {
        track_deallocation(ptr);
        free(ptr);
    }
}

// Macros for easier usage
#define MALLOC(size) tracked_malloc(size, __FILE__, __LINE__, __FUNCTION__)
#define CALLOC(nmemb, size) tracked_calloc(nmemb, size, __FILE__, __LINE__, __FUNCTION__)
#define REALLOC(ptr, size) tracked_realloc(ptr, size, __FILE__, __LINE__, __FUNCTION__)
#define FREE(ptr) tracked_free(ptr)

/* ============================================================================
 * MEMORY PROFILING TESTS
 * ============================================================================ */

static void test_basic_memory_tracking(void) {
    printf("=== Basic Memory Tracking Test ===\n");
    
    memory_tracker_init();
    
    // Test allocations
    void* ptr1 = MALLOC(1024);
    void* ptr2 = MALLOC(2048);
    void* ptr3 = MALLOC(512);
    
    printf("Allocated 3 blocks: 1024, 2048, 512 bytes\n");
    printf("Current usage: %zu bytes\n", memory_tracker.stats.current_usage);
    printf("Peak usage: %zu bytes\n", memory_tracker.stats.peak_usage);
    
    // Test deallocations
    FREE(ptr2);
    printf("Freed 2048 byte block\n");
    printf("Current usage: %zu bytes\n", memory_tracker.stats.current_usage);
    
    FREE(ptr1);
    FREE(ptr3);
    printf("Freed remaining blocks\n");
    printf("Current usage: %zu bytes\n", memory_tracker.stats.current_usage);
    
    memory_tracker_shutdown();
    printf("Basic tracking test completed\n\n");
}

static void test_leak_detection(void) {
    printf("=== Leak Detection Test ===\n");
    
    memory_tracker_init();
    
    // Intentional leaks for testing
    void* leak1 = MALLOC(1024);
    void* leak2 = MALLOC(2048);
    void* leak3 = MALLOC(512);
    
    printf("Created intentional leaks: 1024, 2048, 512 bytes\n");
    printf("Current usage: %zu bytes\n", memory_tracker.stats.current_usage);
    
    // Free some but not all
    FREE(leak2);
    printf("Freed 2048 byte block, leaving 2 leaks\n");
    
    memory_tracker_shutdown();
    printf("Leak detection test completed - check memory_leaks.log\n\n");
}

static void test_gpu_culling_memory_usage(void) {
    printf("=== GPU Culling Memory Usage Test ===\n");
    
    memory_tracker_init();
    
    // Initialize GPU culling system
    culling_gpu_cull_compute_init();
    
    size_t initial_memory = culling_gpu_cull_compute_get_memory_usage();
    printf("Initial GPU culling memory: %zu bytes\n", initial_memory);
    
    // Create multiple instances
    const int num_instances = 100;
    culling_gpu_cull_compute_handle_t handles[num_instances];
    
    for (int i = 0; i < num_instances; i++) {
        culling_gpu_cull_compute_desc_t desc = {.flags = i % 8};
        culling_gpu_cull_compute_create(&handles[i], &desc);
    }
    
    size_t after_creation = culling_gpu_cull_compute_get_memory_usage();
    printf("Memory after creating %d instances: %zu bytes\n", num_instances, after_creation);
    printf("Memory per instance: %.2f bytes\n", 
           (double)(after_creation - initial_memory) / num_instances);
    
    // Perform operations
    for (int i = 0; i < num_instances; i++) {
        uint32_t objects = 100 + (i * 10);
        culling_gpu_cull_compute_update(handles[i], &objects, sizeof(objects));
    }
    culling_gpu_cull_compute_process_pending();
    
    size_t after_operations = culling_gpu_cull_compute_get_memory_usage();
    printf("Memory after operations: %zu bytes\n", after_operations);
    
    // Cleanup
    for (int i = 0; i < num_instances; i++) {
        culling_gpu_cull_compute_destroy(handles[i]);
    }
    
    size_t after_cleanup = culling_gpu_cull_compute_get_memory_usage();
    printf("Memory after cleanup: %zu bytes\n", after_cleanup);
    
    culling_gpu_cull_compute_shutdown();
    memory_tracker_shutdown();
    
    printf("GPU culling memory test completed\n\n");
}

static void test_memory_stress(void) {
    printf("=== Memory Stress Test ===\n");
    
    memory_tracker_init();
    
    const int num_cycles = 10;
    const int allocations_per_cycle = 1000;
    const size_t allocation_size = 1024;
    
    void** allocations = malloc(sizeof(void*) * allocations_per_cycle);
    
    for (int cycle = 0; cycle < num_cycles; cycle++) {
        printf("Cycle %d/%d: Allocating %d blocks of %zu bytes each\n", 
               cycle + 1, num_cycles, allocations_per_cycle, allocation_size);
        
        // Allocate
        for (int i = 0; i < allocations_per_cycle; i++) {
            allocations[i] = MALLOC(allocation_size + (i % 1024));  // Variable sizes
        }
        
        printf("  Peak usage: %zu bytes\n", memory_tracker.stats.peak_usage);
        
        // Random deallocations
        for (int i = 0; i < allocations_per_cycle / 2; i++) {
            int idx = rand() % allocations_per_cycle;
            if (allocations[idx]) {
                FREE(allocations[idx]);
                allocations[idx] = NULL;
            }
        }
        
        printf("  After partial free: %zu bytes\n", memory_tracker.stats.current_usage);
        
        // Free remaining
        for (int i = 0; i < allocations_per_cycle; i++) {
            if (allocations[i]) {
                FREE(allocations[i]);
                allocations[i] = NULL;
            }
        }
        
        printf("  After full free: %zu bytes\n", memory_tracker.stats.current_usage);
    }
    
    free(allocations);
    memory_tracker_shutdown();
    
    printf("Memory stress test completed\n\n");
}

/* ============================================================================
 * MEMORY ANALYSIS REPORT
 * ============================================================================ */

static void generate_memory_report(void) {
    printf("=== Memory Analysis Report ===\n");
    
    memory_tracker_init();
    
    // Simulate typical usage patterns
    printf("Simulating typical engine usage patterns...\n");
    
    // Phase 1: Initialization
    culling_gpu_cull_compute_init();
    printf("Phase 1 - System initialized\n");
    
    // Phase 2: Resource creation
    const int resources = 50;
    culling_gpu_cull_compute_handle_t handles[resources];
    for (int i = 0; i < resources; i++) {
        culling_gpu_cull_compute_desc_t desc = {.flags = i % 4};
        culling_gpu_cull_compute_create(&handles[i], &desc);
    }
    printf("Phase 2 - %d resources created\n", resources);
    
    // Phase 3: Active usage
    for (int frame = 0; frame < 100; frame++) {
        for (int i = 0; i < resources; i++) {
            uint32_t objects = 100 + (frame % 500);
            culling_gpu_cull_compute_update(handles[i], &objects, sizeof(objects));
        }
        culling_gpu_cull_compute_process_pending();
        
        if (frame % 25 == 0) {
            printf("Phase 3 - Frame %d processed\n", frame);
        }
    }
    
    // Phase 4: Cleanup
    for (int i = 0; i < resources; i++) {
        culling_gpu_cull_compute_destroy(handles[i]);
    }
    culling_gpu_cull_compute_shutdown();
    printf("Phase 4 - System shutdown\n");
    
    memory_tracker_shutdown();
    
    printf("\nMemory analysis completed - check memory_leaks.log for detailed report\n");
    printf("Review the statistics to identify optimization opportunities\n\n");
}

/* ============================================================================
 * MAIN PROFILER RUNNER
 * ============================================================================ */

int main(int argc, char* argv[]) {
    printf("=== Memory Profiler Suite ===\n");
    printf("Phase 5 Implementation Memory Analysis\n\n");
    
    // Run all memory tests
    test_basic_memory_tracking();
    test_leak_detection();
    test_gpu_culling_memory_usage();
    test_memory_stress();
    generate_memory_report();
    
    printf("=== Memory Profiling Summary ===\n");
    printf("All memory profiling tests completed.\n");
    printf("Check memory_leaks.log for detailed leak detection results.\n");
    printf("Review memory usage patterns for optimization opportunities.\n");
    
    return 0;
}
