// gpu_memory_validation.c
//
// Purpose: Comprehensive GPU memory allocation validation system
// Implements TODO-0041: GPU memory allocation validation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// Mock GPU memory types and structures
typedef enum {
    GPU_MEMORY_TYPE_VERTEX_BUFFER,
    GPU_MEMORY_TYPE_INDEX_BUFFER,
    GPU_MEMORY_TYPE_UNIFORM_BUFFER,
    GPU_MEMORY_TYPE_STORAGE_BUFFER,
    GPU_MEMORY_TYPE_TEXTURE_2D,
    GPU_MEMORY_TYPE_TEXTURE_3D,
    GPU_MEMORY_TYPE_RENDER_TARGET,
    GPU_MEMORY_TYPE_DEPTH_BUFFER,
    GPU_MEMORY_TYPE_COMPUTE_BUFFER,
    GPU_MEMORY_TYPE_COUNT
} GPUMemoryType;

typedef enum {
    GPU_MEMORY_USAGE_STATIC,
    GPU_MEMORY_USAGE_DYNAMIC,
    GPU_MEMORY_USAGE_STAGING,
    GPU_MEMORY_USAGE_COUNT
} GPUMemoryUsage;

typedef struct {
    void* handle;
    GPUMemoryType type;
    GPUMemoryUsage usage;
    size_t size;
    size_t alignment;
    const char* name;
    const char* file;
    int line;
    uint64_t allocation_id;
    time_t timestamp;
    bool is_mapped;
    void* mapped_pointer;
} GPUMemoryAllocation;

typedef struct {
    size_t total_allocated;
    size_t peak_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
    uint32_t allocation_count;
    uint32_t peak_allocation_count;
    uint32_t failed_allocations;
    size_t fragmentation;
    double fragmentation_ratio;
} GPUMemoryStats;

typedef struct {
    GPUMemoryAllocation* allocations;
    uint32_t capacity;
    uint32_t count;
    GPUMemoryStats stats;
    uint64_t next_allocation_id;
    bool is_initialized;
    size_t memory_budget;
    size_t warning_threshold;
    size_t critical_threshold;
} GPUMemoryManager;

// Global GPU memory manager
static GPUMemoryManager g_gpu_memory_manager = {0};

// Memory type names
static const char* MEMORY_TYPE_NAMES[GPU_MEMORY_TYPE_COUNT] = {
    "Vertex Buffer",
    "Index Buffer", 
    "Uniform Buffer",
    "Storage Buffer",
    "Texture 2D",
    "Texture 3D",
    "Render Target",
    "Depth Buffer",
    "Compute Buffer"
};

// Memory usage names
static const char* MEMORY_USAGE_NAMES[GPU_MEMORY_USAGE_COUNT] = {
    "Static",
    "Dynamic",
    "Staging"
};

// Initialize GPU memory manager
bool gpu_memory_manager_init(size_t memory_budget) {
    printf("Initializing GPU memory manager with budget: %zu MB\n", memory_budget / (1024 * 1024));
    
    g_gpu_memory_manager.capacity = 1024; // Initial capacity
    g_gpu_memory_manager.allocations = calloc(g_gpu_memory_manager.capacity, sizeof(GPUMemoryAllocation));
    if (!g_gpu_memory_manager.allocations) {
        printf("Error: Failed to allocate memory for allocation tracking\n");
        return false;
    }
    
    g_gpu_memory_manager.memory_budget = memory_budget;
    g_gpu_memory_manager.warning_threshold = memory_budget * 0.8;  // 80% warning
    g_gpu_memory_manager.critical_threshold = memory_budget * 0.95; // 95% critical
    g_gpu_memory_manager.next_allocation_id = 1;
    g_gpu_memory_manager.is_initialized = true;
    
    printf("GPU memory manager initialized successfully\n");
    return true;
}

// Find allocation by ID
static GPUMemoryAllocation* find_allocation(uint64_t allocation_id) {
    for (uint32_t i = 0; i < g_gpu_memory_manager.count; i++) {
        if (g_gpu_memory_manager.allocations[i].allocation_id == allocation_id) {
            return &g_gpu_memory_manager.allocations[i];
        }
    }
    return NULL;
}

// Add allocation to tracking
static bool add_allocation(const GPUMemoryAllocation* allocation) {
    // Expand array if needed
    if (g_gpu_memory_manager.count >= g_gpu_memory_manager.capacity) {
        uint32_t new_capacity = g_gpu_memory_manager.capacity * 2;
        GPUMemoryAllocation* new_allocations = realloc(g_gpu_memory_manager.allocations, 
                                                       new_capacity * sizeof(GPUMemoryAllocation));
        if (!new_allocations) {
            printf("Error: Failed to expand allocation tracking array\n");
            return false;
        }
        g_gpu_memory_manager.allocations = new_allocations;
        g_gpu_memory_manager.capacity = new_capacity;
    }
    
    g_gpu_memory_manager.allocations[g_gpu_memory_manager.count] = *allocation;
    g_gpu_memory_manager.count++;
    return true;
}

// Remove allocation from tracking
static bool remove_allocation(uint64_t allocation_id) {
    for (uint32_t i = 0; i < g_gpu_memory_manager.count; i++) {
        if (g_gpu_memory_manager.allocations[i].allocation_id == allocation_id) {
            // Move last element to this position
            if (i < g_gpu_memory_manager.count - 1) {
                g_gpu_memory_manager.allocations[i] = g_gpu_memory_manager.allocations[g_gpu_memory_manager.count - 1];
            }
            g_gpu_memory_manager.count--;
            return true;
        }
    }
    return false;
}

// Validate memory alignment
static bool validate_alignment(size_t size, size_t alignment) {
    if (alignment == 0) return false;
    if (alignment & (alignment - 1)) return false; // Must be power of 2
    if (size % alignment != 0) return false;
    return true;
}

// Check for memory leaks
static void check_memory_leaks(void) {
    if (g_gpu_memory_manager.count > 0) {
        printf("\n=== MEMORY LEAK DETECTED ===\n");
        printf("Outstanding allocations: %u\n", g_gpu_memory_manager.count);
        
        for (uint32_t i = 0; i < g_gpu_memory_manager.count; i++) {
            GPUMemoryAllocation* alloc = &g_gpu_memory_manager.allocations[i];
            printf("  [%zu] %s (%zu bytes) allocated at %s:%d\n",
                   alloc->allocation_id, alloc->name ? alloc->name : "Unnamed",
                   alloc->size, alloc->file ? alloc->file : "Unknown", alloc->line);
        }
    }
}

// Calculate memory fragmentation
static void calculate_fragmentation(void) {
    if (g_gpu_memory_manager.count == 0) {
        g_gpu_memory_manager.stats.fragmentation = 0;
        g_gpu_memory_manager.stats.fragmentation_ratio = 0.0;
        return;
    }
    
    // Simple fragmentation calculation based on allocation size variance
    size_t total_size = 0;
    size_t min_size = SIZE_MAX;
    size_t max_size = 0;
    
    for (uint32_t i = 0; i < g_gpu_memory_manager.count; i++) {
        size_t size = g_gpu_memory_manager.allocations[i].size;
        total_size += size;
        if (size < min_size) min_size = size;
        if (size > max_size) max_size = size;
    }
    
    size_t avg_size = total_size / g_gpu_memory_manager.count;
    g_gpu_memory_manager.stats.fragmentation = max_size - min_size;
    g_gpu_memory_manager.stats.fragmentation_ratio = (double)g_gpu_memory_manager.stats.fragmentation / avg_size;
}

// Update memory statistics
static void update_stats(size_t size, bool is_allocation) {
    if (is_allocation) {
        g_gpu_memory_manager.stats.total_allocated += size;
        g_gpu_memory_manager.stats.current_usage += size;
        g_gpu_memory_manager.stats.allocation_count++;
        
        if (g_gpu_memory_manager.stats.current_usage > g_gpu_memory_manager.stats.peak_usage) {
            g_gpu_memory_manager.stats.peak_usage = g_gpu_memory_manager.stats.current_usage;
        }
        
        if (g_gpu_memory_manager.stats.allocation_count > g_gpu_memory_manager.stats.peak_allocation_count) {
            g_gpu_memory_manager.stats.peak_allocation_count = g_gpu_memory_manager.stats.allocation_count;
        }
    } else {
        g_gpu_memory_manager.stats.total_freed += size;
        g_gpu_memory_manager.stats.current_usage -= size;
        g_gpu_memory_manager.stats.allocation_count--;
    }
    
    calculate_fragmentation();
}

// Check memory thresholds and issue warnings
static void check_memory_thresholds(void) {
    size_t current_usage = g_gpu_memory_manager.stats.current_usage;
    
    if (current_usage > g_gpu_memory_manager.critical_threshold) {
        printf("\n!!! CRITICAL MEMORY USAGE WARNING !!!\n");
        printf("Current usage: %zu MB (%.1f%% of budget)\n",
               current_usage / (1024 * 1024),
               (double)current_usage / g_gpu_memory_manager.memory_budget * 100.0);
    } else if (current_usage > g_gpu_memory_manager.warning_threshold) {
        printf("\n! Memory usage warning: %zu MB (%.1f%% of budget)\n",
               current_usage / (1024 * 1024),
               (double)current_usage / g_gpu_memory_manager.memory_budget * 100.0);
    }
}

// Mock GPU memory allocation
uint64_t gpu_memory_allocate(GPUMemoryType type, GPUMemoryUsage usage, size_t size, 
                           size_t alignment, const char* name, const char* file, int line) {
    if (!g_gpu_memory_manager.is_initialized) {
        printf("Error: GPU memory manager not initialized\n");
        return 0;
    }
    
    // Validate parameters
    if (!validate_alignment(size, alignment)) {
        printf("Error: Invalid alignment %zu for size %zu\n", alignment, size);
        return 0;
    }
    
    // Check memory budget
    if (g_gpu_memory_manager.stats.current_usage + size > g_gpu_memory_manager.memory_budget) {
        printf("Error: Memory allocation would exceed budget (%zu MB requested, %zu MB available)\n",
               size / (1024 * 1024),
               (g_gpu_memory_manager.memory_budget - g_gpu_memory_manager.stats.current_usage) / (1024 * 1024));
        g_gpu_memory_manager.stats.failed_allocations++;
        return 0;
    }
    
    // Create allocation record
    GPUMemoryAllocation allocation = {0};
    allocation.handle = malloc(size); // Mock GPU memory
    if (!allocation.handle) {
        printf("Error: Failed to allocate %zu bytes\n", size);
        g_gpu_memory_manager.stats.failed_allocations++;
        return 0;
    }
    
    allocation.type = type;
    allocation.usage = usage;
    allocation.size = size;
    allocation.alignment = alignment;
    allocation.name = name ? strdup(name) : NULL;
    allocation.file = file ? strdup(file) : NULL;
    allocation.line = line;
    allocation.allocation_id = g_gpu_memory_manager.next_allocation_id++;
    allocation.timestamp = time(NULL);
    allocation.is_mapped = false;
    
    // Add to tracking
    if (!add_allocation(&allocation)) {
        free(allocation.handle);
        if (allocation.name) free((void*)allocation.name);
        if (allocation.file) free((void*)allocation.file);
        g_gpu_memory_manager.stats.failed_allocations++;
        return 0;
    }
    
    // Update statistics
    update_stats(size, true);
    check_memory_thresholds();
    
    printf("Allocated [%zu] %s: %zu bytes (%s, %s) at %s:%d\n",
           allocation.allocation_id, name ? name : "Unnamed", size,
           MEMORY_TYPE_NAMES[type], MEMORY_USAGE_NAMES[usage], file ? file : "Unknown", line);
    
    return allocation.allocation_id;
}

// Mock GPU memory deallocation
bool gpu_memory_free(uint64_t allocation_id) {
    if (!g_gpu_memory_manager.is_initialized) {
        printf("Error: GPU memory manager not initialized\n");
        return false;
    }
    
    GPUMemoryAllocation* allocation = find_allocation(allocation_id);
    if (!allocation) {
        printf("Error: Invalid allocation ID %zu\n", allocation_id);
        return false;
    }
    
    if (allocation->is_mapped) {
        printf("Warning: Freeing mapped allocation [%zu]\n", allocation_id);
    }
    
    printf("Freed [%zu] %s: %zu bytes\n",
           allocation->allocation_id, 
           allocation->name ? allocation->name : "Unnamed",
           allocation->size);
    
    // Update statistics
    update_stats(allocation->size, false);
    
    // Clean up
    free(allocation->handle);
    if (allocation->name) free((void*)allocation->name);
    if (allocation->file) free((void*)allocation->file);
    
    // Remove from tracking
    remove_allocation(allocation_id);
    
    return true;
}

// Mock memory mapping
void* gpu_memory_map(uint64_t allocation_id) {
    GPUMemoryAllocation* allocation = find_allocation(allocation_id);
    if (!allocation) {
        printf("Error: Invalid allocation ID %zu for mapping\n", allocation_id);
        return NULL;
    }
    
    if (allocation->is_mapped) {
        printf("Warning: Allocation [%zu] is already mapped\n", allocation_id);
        return allocation->mapped_pointer;
    }
    
    allocation->is_mapped = true;
    allocation->mapped_pointer = allocation->handle; // Mock mapping
    
    printf("Mapped allocation [%zu]\n", allocation_id);
    return allocation->mapped_pointer;
}

// Mock memory unmapping
bool gpu_memory_unmap(uint64_t allocation_id) {
    GPUMemoryAllocation* allocation = find_allocation(allocation_id);
    if (!allocation) {
        printf("Error: Invalid allocation ID %zu for unmapping\n", allocation_id);
        return false;
    }
    
    if (!allocation->is_mapped) {
        printf("Warning: Allocation [%zu] is not mapped\n", allocation_id);
        return false;
    }
    
    allocation->is_mapped = false;
    allocation->mapped_pointer = NULL;
    
    printf("Unmapped allocation [%zu]\n", allocation_id);
    return true;
}

// Validate memory state
bool validate_memory_state(void) {
    printf("\n=== Memory State Validation ===\n");
    
    bool is_valid = true;
    
    // Check for invalid allocations
    for (uint32_t i = 0; i < g_gpu_memory_manager.count; i++) {
        GPUMemoryAllocation* alloc = &g_gpu_memory_manager.allocations[i];
        
        if (!alloc->handle) {
            printf("Error: Allocation [%zu] has null handle\n", alloc->allocation_id);
            is_valid = false;
        }
        
        if (alloc->size == 0) {
            printf("Error: Allocation [%zu] has zero size\n", alloc->allocation_id);
            is_valid = false;
        }
        
        if (!validate_alignment(alloc->size, alloc->alignment)) {
            printf("Error: Allocation [%zu] has invalid alignment\n", alloc->allocation_id);
            is_valid = false;
        }
    }
    
    // Check statistics consistency
    size_t calculated_usage = 0;
    for (uint32_t i = 0; i < g_gpu_memory_manager.count; i++) {
        calculated_usage += g_gpu_memory_manager.allocations[i].size;
    }
    
    if (calculated_usage != g_gpu_memory_manager.stats.current_usage) {
        printf("Error: Usage mismatch - calculated: %zu, tracked: %zu\n",
               calculated_usage, g_gpu_memory_manager.stats.current_usage);
        is_valid = false;
    }
    
    printf("Memory state validation: %s\n", is_valid ? "PASSED" : "FAILED");
    return is_valid;
}

// Generate memory report
void generate_memory_report(void) {
    printf("\n=== GPU Memory Report ===\n");
    
    // Overall statistics
    printf("Memory Budget: %zu MB\n", g_gpu_memory_manager.memory_budget / (1024 * 1024));
    printf("Current Usage: %zu MB (%.1f%%)\n", 
           g_gpu_memory_manager.stats.current_usage / (1024 * 1024),
           (double)g_gpu_memory_manager.stats.current_usage / g_gpu_memory_manager.memory_budget * 100.0);
    printf("Peak Usage: %zu MB (%.1f%%)\n",
           g_gpu_memory_manager.stats.peak_usage / (1024 * 1024),
           (double)g_gpu_memory_manager.stats.peak_usage / g_gpu_memory_manager.memory_budget * 100.0);
    printf("Total Allocated: %zu MB\n", g_gpu_memory_manager.stats.total_allocated / (1024 * 1024));
    printf("Total Freed: %zu MB\n", g_gpu_memory_manager.stats.total_freed / (1024 * 1024));
    printf("Current Allocations: %u\n", g_gpu_memory_manager.stats.allocation_count);
    printf("Peak Allocations: %u\n", g_gpu_memory_manager.stats.peak_allocation_count);
    printf("Failed Allocations: %u\n", g_gpu_memory_manager.stats.failed_allocations);
    printf("Fragmentation: %zu bytes (%.2f%%)\n", 
           g_gpu_memory_manager.stats.fragmentation,
           g_gpu_memory_manager.stats.fragmentation_ratio * 100.0);
    
    // Breakdown by type
    printf("\n--- Memory Usage by Type ---\n");
    size_t type_usage[GPU_MEMORY_TYPE_COUNT] = {0};
    uint32_t type_count[GPU_MEMORY_TYPE_COUNT] = {0};
    
    for (uint32_t i = 0; i < g_gpu_memory_manager.count; i++) {
        GPUMemoryAllocation* alloc = &g_gpu_memory_manager.allocations[i];
        type_usage[alloc->type] += alloc->size;
        type_count[alloc->type]++;
    }
    
    for (int i = 0; i < GPU_MEMORY_TYPE_COUNT; i++) {
        if (type_count[i] > 0) {
            printf("%s: %zu MB (%u allocations)\n",
                   MEMORY_TYPE_NAMES[i],
                   type_usage[i] / (1024 * 1024),
                   type_count[i]);
        }
    }
    
    // Breakdown by usage
    printf("\n--- Memory Usage by Usage Pattern ---\n");
    size_t usage_usage[GPU_MEMORY_USAGE_COUNT] = {0};
    uint32_t usage_count[GPU_MEMORY_USAGE_COUNT] = {0};
    
    for (uint32_t i = 0; i < g_gpu_memory_manager.count; i++) {
        GPUMemoryAllocation* alloc = &g_gpu_memory_manager.allocations[i];
        usage_usage[alloc->usage] += alloc->size;
        usage_count[alloc->usage]++;
    }
    
    for (int i = 0; i < GPU_MEMORY_USAGE_COUNT; i++) {
        if (usage_count[i] > 0) {
            printf("%s: %zu MB (%u allocations)\n",
                   MEMORY_USAGE_NAMES[i],
                   usage_usage[i] / (1024 * 1024),
                   usage_count[i]);
        }
    }
}

// Stress test memory allocation
bool stress_test_memory_allocation(void) {
    printf("\n=== Memory Allocation Stress Test ===\n");
    
    const int num_allocations = 1000;
    uint64_t allocation_ids[num_allocations];
    
    // Allocate many small buffers
    printf("Allocating %d small buffers...\n", num_allocations);
    for (int i = 0; i < num_allocations; i++) {
        char name[64];
        snprintf(name, sizeof(name), "StressBuffer_%d", i);
        
        size_t size = (rand() % 1024 + 1) * 1024; // 1KB to 1MB
        GPUMemoryType type = rand() % GPU_MEMORY_TYPE_COUNT;
        GPUMemoryUsage usage = rand() % GPU_MEMORY_USAGE_COUNT;
        
        allocation_ids[i] = gpu_memory_allocate(type, usage, size, 256, name, __FILE__, __LINE__);
        if (allocation_ids[i] == 0) {
            printf("Failed at allocation %d\n", i);
            break;
        }
    }
    
    printf("Created %u allocations\n", g_gpu_memory_manager.stats.allocation_count);
    
    // Validate state
    bool state_valid = validate_memory_state();
    
    // Free half of the allocations
    printf("Freeing half of the allocations...\n");
    for (int i = 0; i < num_allocations; i += 2) {
        if (allocation_ids[i] != 0) {
            gpu_memory_free(allocation_ids[i]);
        }
    }
    
    // Validate state again
    state_valid &= validate_memory_state();
    
    // Free remaining allocations
    printf("Freeing remaining allocations...\n");
    for (int i = 1; i < num_allocations; i += 2) {
        if (allocation_ids[i] != 0) {
            gpu_memory_free(allocation_ids[i]);
        }
    }
    
    // Final validation
    state_valid &= validate_memory_state();
    
    printf("Stress test %s\n", state_valid ? "PASSED" : "FAILED");
    return state_valid;
}

// Cleanup GPU memory manager
void gpu_memory_manager_cleanup(void) {
    if (!g_gpu_memory_manager.is_initialized) return;
    
    printf("Cleaning up GPU memory manager...\n");
    
    // Check for memory leaks
    check_memory_leaks();
    
    // Free remaining allocations
    for (uint32_t i = 0; i < g_gpu_memory_manager.count; i++) {
        GPUMemoryAllocation* alloc = &g_gpu_memory_manager.allocations[i];
        free(alloc->handle);
        if (alloc->name) free((void*)alloc->name);
        if (alloc->file) free((void*)alloc->file);
    }
    
    free(g_gpu_memory_manager.allocations);
    memset(&g_gpu_memory_manager, 0, sizeof(g_gpu_memory_manager));
    
    printf("GPU memory manager cleaned up\n");
}

// Main validation function
int main(void) {
    printf("=== GPU Memory Allocation Validation ===\n");
    printf("Implementing TODO-0041: GPU memory allocation validation\n\n");
    
    // Initialize with 1GB budget
    if (!gpu_memory_manager_init(1024 * 1024 * 1024)) {
        printf("Failed to initialize GPU memory manager\n");
        return 1;
    }
    
    // Test basic allocation
    printf("\n=== Basic Allocation Test ===\n");
    uint64_t alloc1 = gpu_memory_allocate(GPU_MEMORY_TYPE_VERTEX_BUFFER, GPU_MEMORY_USAGE_STATIC, 
                                         1024 * 1024, 256, "TestVertexBuffer", __FILE__, __LINE__);
    uint64_t alloc2 = gpu_memory_allocate(GPU_MEMORY_TYPE_TEXTURE_2D, GPU_MEMORY_USAGE_DYNAMIC,
                                         2048 * 2048 * 4, 256, "TestTexture", __FILE__, __LINE__);
    
    // Test mapping
    void* mapped = gpu_memory_map(alloc1);
    if (mapped) {
        gpu_memory_unmap(alloc1);
    }
    
    // Test validation
    bool validation_passed = validate_memory_state();
    
    // Test stress scenarios
    bool stress_passed = stress_test_memory_allocation();
    
    // Generate report
    generate_memory_report();
    
    // Cleanup
    gpu_memory_free(alloc1);
    gpu_memory_free(alloc2);
    gpu_memory_manager_cleanup();
    
    printf("\n=== Validation Complete ===\n");
    printf("Result: %s\n", (validation_passed && stress_passed) ? "PASSED" : "FAILED");
    
    return (validation_passed && stress_passed) ? 0 : 1;
}
