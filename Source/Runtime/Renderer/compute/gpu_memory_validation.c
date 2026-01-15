#include "gpu_memory_validation.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// Internal structures
struct gpu_memory_allocation_t {
    uint64_t id;
    uint64_t size;
    uint64_t aligned_size;
    gpu_memory_type_t type;
    gpu_memory_flags_t flags;
    gpu_memory_heap_t heap;
    char* name;
    char* file;
    uint32_t line;
    uint64_t timestamp;
    void* device_pointer;
    void* host_pointer;
    bool is_mapped;
    uint32_t ref_count;
    uint8_t* guard_start;
    uint8_t* guard_end;
};

struct gpu_memory_pool_t {
    uint64_t pool_size;
    uint64_t block_size;
    gpu_memory_type_t type;
    uint8_t* memory;
    uint64_t* free_blocks;
    uint32_t free_count;
    uint32_t total_blocks;
    pthread_mutex_t mutex;
};

struct gpu_memory_allocator_t {
    bool is_initialized;
    pthread_mutex_t mutex;
    
    // Memory heaps
    uint64_t heap_sizes[GPU_MEMORY_HEAP_COUNT];
    uint64_t heap_usage[GPU_MEMORY_HEAP_COUNT];
    uint64_t heap_budgets[GPU_MEMORY_HEAP_COUNT];
    
    // Allocation tracking
    gpu_memory_allocation_t** allocations;
    uint32_t allocation_count;
    uint32_t allocation_capacity;
    uint64_t next_allocation_id;
    
    // Memory pools
    gpu_memory_pool_t** pools;
    uint32_t pool_count;
    uint32_t pool_capacity;
    
    // Statistics
    gpu_memory_allocator_stats_t stats;
    
    // Guard pattern for corruption detection
    static const uint8_t GUARD_PATTERN[16] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
                                               0xFE, 0xED, 0xFA, 0xCE, 0xDE, 0xAD, 0xC0, 0xDE};
};

static const uint8_t GUARD_PATTERN[16] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
                                          0xFE, 0xED, 0xFA, 0xCE, 0xDE, 0xAD, 0xC0, 0xDE};

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint64_t align_size(uint64_t size, uint64_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static bool validate_guard_pattern(const uint8_t* guard) {
    return memcmp(guard, GUARD_PATTERN, sizeof(GUARD_PATTERN)) == 0;
}

gpu_memory_error_t gpu_memory_allocator_init(gpu_memory_allocator_t** allocator, uint64_t total_memory) {
    if (!allocator) {
        return GPU_MEMORY_ERROR_INVALID_PARAM;
    }
    
    gpu_memory_allocator_t* new_allocator = (gpu_memory_allocator_t*)calloc(1, sizeof(gpu_memory_allocator_t));
    if (!new_allocator) {
        return GPU_MEMORY_ERROR_OUT_OF_MEMORY;
    }
    
    if (pthread_mutex_init(&new_allocator->mutex, NULL) != 0) {
        free(new_allocator);
        return GPU_MEMORY_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize heaps
    new_allocator->heap_sizes[GPU_MEMORY_HEAP_DEVICE_LOCAL] = total_memory * 0.7;
    new_allocator->heap_sizes[GPU_MEMORY_HEAP_HOST_VISIBLE] = total_memory * 0.2;
    new_allocator->heap_sizes[GPU_MEMORY_HEAP_HOST_COHERENT] = total_memory * 0.1;
    new_allocator->heap_sizes[GPU_MEMORY_HEAP_HOST_CACHED] = total_memory * 0.05;
    new_allocator->heap_sizes[GPU_MEMORY_HEAP_LAZILY_ALLOCATED] = total_memory * 0.1;
    
    for (int i = 0; i < GPU_MEMORY_HEAP_COUNT; i++) {
        new_allocator->heap_budgets[i] = new_allocator->heap_sizes[i];
    }
    
    new_allocator->allocation_capacity = 1024;
    new_allocator->allocations = (gpu_memory_allocation_t**)calloc(new_allocator->allocation_capacity, sizeof(gpu_memory_allocation_t*));
    
    if (!new_allocator->allocations) {
        pthread_mutex_destroy(&new_allocator->mutex);
        free(new_allocator);
        return GPU_MEMORY_ERROR_OUT_OF_MEMORY;
    }
    
    new_allocator->is_initialized = true;
    *allocator = new_allocator;
    
    return GPU_MEMORY_SUCCESS;
}

void gpu_memory_allocator_shutdown(gpu_memory_allocator_t* allocator) {
    if (!allocator) {
        return;
    }
    
    pthread_mutex_lock(&allocator->mutex);
    
    if (!allocator->is_initialized) {
        pthread_mutex_unlock(&allocator->mutex);
        return;
    }
    
    // Check for leaks
    if (allocator->allocation_count > 0) {
        printf("Warning: %u memory leaks detected\n", allocator->allocation_count);
        gpu_memory_debug_print_allocations(allocator);
    }
    
    // Free all allocations
    for (uint32_t i = 0; i < allocator->allocation_count; i++) {
        gpu_memory_allocation_t* allocation = allocator->allocations[i];
        free(allocation->name);
        free(allocation->file);
        free(allocation->guard_start);
        free(allocation->guard_end);
        free(allocation);
    }
    
    // Free all pools
    for (uint32_t i = 0; i < allocator->pool_count; i++) {
        gpu_memory_pool_t* pool = allocator->pools[i];
        pthread_mutex_destroy(&pool->mutex);
        free(pool->memory);
        free(pool->free_blocks);
        free(pool);
    }
    
    free(allocator->allocations);
    free(allocator->pools);
    
    allocator->is_initialized = false;
    pthread_mutex_unlock(&allocator->mutex);
    pthread_mutex_destroy(&allocator->mutex);
    
    free(allocator);
}

gpu_memory_error_t gpu_memory_allocate(gpu_memory_allocator_t* allocator, 
                                      const gpu_memory_alloc_desc_t* desc, 
                                      gpu_memory_allocation_t** allocation) {
    if (!allocator || !desc || !allocation) {
        return GPU_MEMORY_ERROR_INVALID_PARAM;
    }
    
    if (!allocator->is_initialized) {
        return GPU_MEMORY_ERROR_NOT_INITIALIZED;
    }
    
    uint64_t start_time = get_timestamp_ns();
    
    pthread_mutex_lock(&allocator->mutex);
    
    // Check alignment
    if (desc->alignment == 0 || (desc->alignment & (desc->alignment - 1)) != 0) {
        pthread_mutex_unlock(&allocator->mutex);
        return GPU_MEMORY_ERROR_INVALID_ALIGNMENT;
    }
    
    // Calculate aligned size
    uint64_t aligned_size = align_size(desc->size, desc->alignment);
    
    // Check heap budget
    if (allocator->heap_usage[desc->preferred_heap] + aligned_size > allocator->heap_budgets[desc->preferred_heap]) {
        allocator->stats.failed_allocations++;
        pthread_mutex_unlock(&allocator->mutex);
        return GPU_MEMORY_ERROR_OUT_OF_MEMORY;
    }
    
    // Check capacity
    if (allocator->allocation_count >= allocator->allocation_capacity) {
        uint32_t new_capacity = allocator->allocation_capacity * 2;
        gpu_memory_allocation_t** new_allocations = (gpu_memory_allocation_t**)realloc(allocator->allocations, new_capacity * sizeof(gpu_memory_allocation_t*));
        if (!new_allocations) {
            allocator->stats.failed_allocations++;
            pthread_mutex_unlock(&allocator->mutex);
            return GPU_MEMORY_ERROR_OUT_OF_MEMORY;
        }
        allocator->allocations = new_allocations;
        allocator->allocation_capacity = new_capacity;
    }
    
    // Create allocation
    gpu_memory_allocation_t* new_allocation = (gpu_memory_allocation_t*)calloc(1, sizeof(gpu_memory_allocation_t));
    if (!new_allocation) {
        allocator->stats.failed_allocations++;
        pthread_mutex_unlock(&allocator->mutex);
        return GPU_MEMORY_ERROR_OUT_OF_MEMORY;
    }
    
    new_allocation->id = allocator->next_allocation_id++;
    new_allocation->size = desc->size;
    new_allocation->aligned_size = aligned_size;
    new_allocation->type = desc->type;
    new_allocation->flags = desc->flags;
    new_allocation->heap = desc->preferred_heap;
    new_allocation->line = desc->line;
    new_allocation->timestamp = get_timestamp_ns();
    new_allocation->ref_count = 1;
    
    // Copy strings
    new_allocation->name = desc->name ? strdup(desc->name) : strdup("unnamed");
    new_allocation->file = desc->file ? strdup(desc->file) : strdup("unknown");
    
    if (!new_allocation->name || !new_allocation->file) {
        free(new_allocation->name);
        free(new_allocation->file);
        free(new_allocation);
        allocator->stats.failed_allocations++;
        pthread_mutex_unlock(&allocator->mutex);
        return GPU_MEMORY_ERROR_OUT_OF_MEMORY;
    }
    
    // Create guard patterns
    new_allocation->guard_start = malloc(sizeof(GUARD_PATTERN));
    new_allocation->guard_end = malloc(sizeof(GUARD_PATTERN));
    
    if (!new_allocation->guard_start || !new_allocation->guard_end) {
        free(new_allocation->name);
        free(new_allocation->file);
        free(new_allocation->guard_start);
        free(new_allocation->guard_end);
        free(new_allocation);
        allocator->stats.failed_allocations++;
        pthread_mutex_unlock(&allocator->mutex);
        return GPU_MEMORY_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(new_allocation->guard_start, GUARD_PATTERN, sizeof(GUARD_PATTERN));
    memcpy(new_allocation->guard_end, GUARD_PATTERN, sizeof(GUARD_PATTERN));
    
    // Simulate device pointer allocation
    new_allocation->device_pointer = malloc(aligned_size);
    if (!new_allocation->device_pointer) {
        free(new_allocation->name);
        free(new_allocation->file);
        free(new_allocation->guard_start);
        free(new_allocation->guard_end);
        free(new_allocation);
        allocator->stats.failed_allocations++;
        pthread_mutex_unlock(&allocator->mutex);
        return GPU_MEMORY_ERROR_OUT_OF_MEMORY;
    }
    
    // Update heap usage
    allocator->heap_usage[desc->preferred_heap] += aligned_size;
    
    // Add to allocator
    allocator->allocations[allocator->allocation_count++] = new_allocation;
    
    // Update statistics
    allocator->stats.total_allocated += aligned_size;
    allocator->stats.current_usage += aligned_size;
    allocator->stats.total_allocations++;
    allocator->stats.active_allocations++;
    allocator->stats.total_allocation_time_ns += (get_timestamp_ns() - start_time);
    
    if (allocator->stats.current_usage > allocator->stats.peak_usage) {
        allocator->stats.peak_usage = allocator->stats.current_usage;
    }
    
    *allocation = new_allocation;
    pthread_mutex_unlock(&allocator->mutex);
    
    return GPU_MEMORY_SUCCESS;
}

gpu_memory_error_t gpu_memory_free(gpu_memory_allocator_t* allocator, gpu_memory_allocation_t* allocation) {
    if (!allocator || !allocation) {
        return GPU_MEMORY_ERROR_INVALID_PARAM;
    }
    
    uint64_t start_time = get_timestamp_ns();
    
    pthread_mutex_lock(&allocator->mutex);
    
    // Find allocation
    bool found = false;
    for (uint32_t i = 0; i < allocator->allocation_count; i++) {
        if (allocator->allocations[i] == allocation) {
            found = true;
            
            // Validate guard patterns
            if (!validate_guard_pattern(allocation->guard_start) || !validate_guard_pattern(allocation->guard_end)) {
                printf("Memory corruption detected in allocation %s (%s:%u)\n", allocation->name, allocation->file, allocation->line);
            }
            
            // Update heap usage
            allocator->heap_usage[allocation->heap] -= allocation->aligned_size;
            
            // Update statistics
            allocator->stats.total_freed += allocation->aligned_size;
            allocator->stats.current_usage -= allocation->aligned_size;
            allocator->stats.active_allocations--;
            allocator->stats.total_deallocation_time_ns += (get_timestamp_ns() - start_time);
            
            // Remove from array
            for (uint32_t j = i; j < allocator->allocation_count - 1; j++) {
                allocator->allocations[j] = allocator->allocations[j + 1];
            }
            allocator->allocation_count--;
            
            // Free resources
            free(allocation->name);
            free(allocation->file);
            free(allocation->guard_start);
            free(allocation->guard_end);
            free(allocation->device_pointer);
            free(allocation);
            
            break;
        }
    }
    
    pthread_mutex_unlock(&allocator->mutex);
    
    if (!found) {
        return GPU_MEMORY_ERROR_NOT_FOUND;
    }
    
    return GPU_MEMORY_SUCCESS;
}

gpu_memory_error_t gpu_memory_validate_allocation(gpu_memory_allocator_t* allocator,
                                                gpu_memory_allocation_t* allocation) {
    if (!allocator || !allocation) {
        return GPU_MEMORY_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&allocator->mutex);
    
    // Validate guard patterns
    if (!validate_guard_pattern(allocation->guard_start) || !validate_guard_pattern(allocation->guard_end)) {
        pthread_mutex_unlock(&allocator->mutex);
        return GPU_MEMORY_ERROR_ALLOCATION_FAILED; // Corruption detected
    }
    
    pthread_mutex_unlock(&allocator->mutex);
    
    return GPU_MEMORY_SUCCESS;
}

gpu_memory_error_t gpu_memory_get_statistics(gpu_memory_allocator_t* allocator,
                                            gpu_memory_allocator_stats_t* stats) {
    if (!allocator || !stats) {
        return GPU_MEMORY_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&allocator->mutex);
    *stats = allocator->stats;
    
    // Update heap statistics
    for (int i = 0; i < GPU_MEMORY_HEAP_COUNT; i++) {
        stats->heap_stats[i].total_size = allocator->heap_sizes[i];
        stats->heap_stats[i].used_size = allocator->heap_usage[i];
        stats->heap_stats[i].free_size = allocator->heap_sizes[i] - allocator->heap_usage[i];
        stats->heap_stats[i].allocation_count = 0; // Would need to count per-heap allocations
        
        if (stats->heap_stats[i].total_size > 0) {
            stats->heap_stats[i].fragmentation_ratio = 
                (double)(stats->heap_stats[i].free_size) / stats->heap_stats[i].total_size;
        }
    }
    
    pthread_mutex_unlock(&allocator->mutex);
    
    return GPU_MEMORY_SUCCESS;
}

gpu_memory_error_t gpu_memory_debug_print_allocations(gpu_memory_allocator_t* allocator) {
    if (!allocator) {
        return GPU_MEMORY_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&allocator->mutex);
    
    printf("=== GPU Memory Allocations ===\n");
    printf("Total allocations: %u\n", allocator->allocation_count);
    printf("Total memory used: %llu bytes\n", (unsigned long long)allocator->stats.current_usage);
    
    for (uint32_t i = 0; i < allocator->allocation_count; i++) {
        gpu_memory_allocation_t* allocation = allocator->allocations[i];
        printf("ID: %llu, Size: %llu, Type: %d, Name: %s (%s:%u)\n",
               (unsigned long long)allocation->id,
               (unsigned long long)allocation->size,
               allocation->type,
               allocation->name,
               allocation->file,
               allocation->line);
    }
    
    pthread_mutex_unlock(&allocator->mutex);
    
    return GPU_MEMORY_SUCCESS;
}

const char* gpu_memory_error_string(gpu_memory_error_t error) {
    switch (error) {
        case GPU_MEMORY_SUCCESS: return "Success";
        case GPU_MEMORY_ERROR_INVALID_PARAM: return "Invalid parameter";
        case GPU_MEMORY_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case GPU_MEMORY_ERROR_INVALID_ALIGNMENT: return "Invalid alignment";
        case GPU_MEMORY_ERROR_INVALID_HEAP: return "Invalid heap";
        case GPU_MEMORY_ERROR_ALLOCATION_FAILED: return "Allocation failed";
        case GPU_MEMORY_ERROR_NOT_FOUND: return "Not found";
        case GPU_MEMORY_ERROR_ALREADY_MAPPED: return "Already mapped";
        case GPU_MEMORY_ERROR_NOT_MAPPED: return "Not mapped";
        case GPU_MEMORY_ERROR_INVALID_FLAGS: return "Invalid flags";
        case GPU_MEMORY_ERROR_NOT_INITIALIZED: return "Not initialized";
        default: return "Unknown error";
    }
}
