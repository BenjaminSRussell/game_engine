// src/engine/rendering/memory/gpu_memory_manager.c
// GPU Memory Management System - Efficient GPU resource allocation and tracking

#include <core/logger.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "../render_pipeline.h"

// ============================================================================
// GPU Memory Management Types
// ============================================================================

typedef enum {
    MEMORY_TYPE_TEXTURE,
    MEMORY_TYPE_BUFFER,
    MEMORY_TYPE_RENDER_TARGET,
    MEMORY_TYPE_DEPTH_STENCIL,
    MEMORY_TYPE_UNIFORM_BUFFER,
    MEMORY_TYPE_VERTEX_BUFFER,
    MEMORY_TYPE_INDEX_BUFFER,
    MEMORY_TYPE_SHADER,
    MEMORY_TYPE_PIPELINE,
    MEMORY_TYPE_COUNT
} MemoryType;

typedef enum {
    MEMORY_USAGE_STATIC,    // Persistent for the entire application
    MEMORY_USAGE_DYNAMIC,   // Created and destroyed frequently
    MEMORY_USAGE_STAGING,    // CPU-to-GPU transfer
    MEMORY_USAGE_UPLOAD,     // CPU-to-GPU upload (read-only on GPU)
    MEMORY_TYPE_COUNT
} MemoryUsage;

typedef struct {
    void *resource;
    MemoryType type;
    MemoryUsage usage;
    uint32_t size;
    char name[256];
    char file_path[512];
    uint32_t line_number;
    
    // Allocation tracking
    uint64_t allocation_id;
    uint64_t allocation_time;
    bool is_active;
    
    // Memory pool tracking
    uint32_t pool_index;
    bool is_pooled;
    
    // Reference counting
    uint32_t ref_count;
    pthread_mutex_t ref_mutex;
} MemoryAllocation;

typedef struct {
    MemoryAllocation *allocations[4096];
    uint32_t allocation_count;
    uint32_t allocation_capacity;
    
    // Memory pools for different types and usage patterns
    MemoryAllocation *pools[MEMORY_TYPE_COUNT][MEMORY_TYPE_COUNT];
    uint32_t pool_sizes[MEMORY_TYPE_COUNT][MEMORY_TYPE_COUNT];
    uint32_t pool_used[MEMORY_TYPE_COUNT][MEMORY_TYPE_COUNT];
    
    // Memory budgets
    uint64_t total_budget;
    uint64_t used_memory;
    uint64_t peak_memory;
    uint64_t type_budgets[MEMORY_TYPE_COUNT];
    uint64_t type_used[MEMORY_TYPE_COUNT];
    
    // Statistics
    uint64_t total_allocations;
    uint64_t total_deallocations;
    uint64_t fragmentation_count;
    uint64_t defragmentation_time_ms;
    
    // Defragmentation
    bool needs_defragmentation;
    pthread_t defrag_thread;
    bool defrag_running;
    
    // Memory monitoring
    bool enable_monitoring;
    float memory_warning_threshold;
    float memory_critical_threshold;
    
    // Thread safety
    pthread_mutex_t global_mutex;
    
    bool initialized;
} GPUMemoryManager;

static GPUMemoryManager g_memory_manager = {0};

// ============================================================================
// Memory Pool Management
// ============================================================================

static bool initialize_memory_pools(GPUMemoryManager *manager) {
    // Set pool sizes based on typical usage patterns
    const uint32_t pool_sizes[MEMORY_TYPE_COUNT][MEMORY_TYPE_COUNT] = {
        // TEXTURE
        {64, 128, 32, 16},  // static, dynamic, staging, upload
        // BUFFER
        {256, 512, 128, 64}, // static, dynamic, staging, upload
        // RENDER_TARGET
        {16, 32, 8, 4},   // static, dynamic, staging, upload
        // DEPTH_STENCIL
        {16, 32, 8, 4},   // static, dynamic, staging, upload
        // UNIFORM_BUFFER
        {512, 1024, 256, 128}, // static, dynamic, staging, upload
        // VERTEX_BUFFER
        {1024, 2048, 512, 256}, // static, dynamic, staging, upload
        // INDEX_BUFFER
        {512, 1024, 256, 128}, // static, dynamic, staging, upload
        // SHADER
        {128, 256, 64, 32}, // static, dynamic, staging, upload
        // PIPELINE
        {64, 128, 16, 8}   // static, dynamic, staging, upload
    };
    
    for (int type = 0; type < MEMORY_TYPE_COUNT; type++) {
        for (int usage = 0; usage < MEMORY_TYPE_COUNT; usage++) {
            uint32_t pool_size = pool_sizes[type][usage];
            if (pool_size == 0) continue;
            
            manager->pools[type][usage] = calloc(pool_size, sizeof(MemoryAllocation));
            if (!manager->pools[type][usage]) {
                LOG_ERROR("Failed to allocate memory pool for type %d, usage %d", type, usage);
                return false;
            }
            
            manager->pool_sizes[type][usage] = pool_size;
            manager->pool_used[type][usage] = 0;
            
            // Initialize pool allocations
            for (uint32_t i = 0; i < pool_size; i++) {
                MemoryAllocation *alloc = &manager->pools[type][usage][i];
                alloc->type = (MemoryType)type;
                alloc->usage = (MemoryUsage)usage;
                alloc->is_pooled = true;
                alloc->is_active = false;
                alloc->ref_count = 0;
                pthread_mutex_init(&alloc->ref_mutex, NULL);
            }
        }
    }
    
    return true;
}

static MemoryAllocation* allocate_from_pool(GPUMemoryManager *manager, MemoryType type, 
                                         MemoryUsage usage, uint32_t size) {
    if (!manager->pools[type][usage] || manager->pool_used[type][usage] >= manager->pool_sizes[type][usage]) {
        return NULL; // Pool exhausted
    }
    
    // Find free allocation in pool
    for (uint32_t i = 0; i < manager->pool_sizes[type][usage]; i++) {
        MemoryAllocation *alloc = &manager->pools[type][usage][i];
        if (!alloc->is_active) {
            alloc->is_active = true;
            alloc->size = size;
            alloc->allocation_time = get_time_nanos();
            manager->pool_used[type][usage]++;
            return alloc;
        }
    }
    
    return NULL; // No free allocation found
}

static void return_to_pool(GPUMemoryManager *manager, MemoryAllocation *allocation) {
    if (!allocation || !allocation->is_pooled) return;
    
    allocation->is_active = false;
    allocation->resource = NULL;
    allocation->size = 0;
    allocation->ref_count = 0;
    
    manager->pool_used[allocation->type][allocation->usage]--;
}

// ============================================================================
// Memory Allocation API
// ============================================================================

static uint64_t generate_allocation_id(void) {
    static uint64_t next_id = 1;
    return next_id++;
}

static void* allocate_gpu_resource(MemoryType type, MemoryUsage usage, uint32_t size, 
                                   const char *name, const char *file_path, uint32_t line_number) {
    // TODO: Implement actual GPU resource allocation based on type
    // This would call the appropriate Metal/Vulkan/OpenGL functions
    
    void *resource = NULL;
    
    switch (type) {
        case MEMORY_TYPE_TEXTURE:
            // resource = create_texture(size, format, usage);
            break;
        case MEMORY_TYPE_BUFFER:
            // resource = create_buffer(size, usage);
            break;
        case MEMORY_TYPE_RENDER_TARGET:
            // resource = create_render_target(size, format);
            break;
        case MEMORY_TYPE_DEPTH_STENCIL:
            // resource = create_depth_stencil(size, format);
            break;
        case MEMORY_TYPE_UNIFORM_BUFFER:
            // resource = create_uniform_buffer(size);
            break;
        case MEMORY_TYPE_VERTEX_BUFFER:
            // resource = create_vertex_buffer(size);
            break;
        case MEMORY_TYPE_INDEX_BUFFER:
            // resource = create_index_buffer(size);
            break;
        case MEMORY_TYPE_SHADER:
            // resource = create_shader();
            break;
        case MEMORY_TYPE_PIPELINE:
            // resource = create_pipeline();
            break;
        default:
            LOG_ERROR("Unknown memory type: %d", (int)type);
            return NULL;
    }
    
    if (!resource) {
        LOG_ERROR("Failed to allocate GPU resource: type=%d, usage=%d, size=%u", (int)type, (int)usage, size);
        return NULL;
    }
    
    return resource;
}

void* gpu_memory_allocate(MemoryType type, MemoryUsage usage, uint32_t size, 
                           const char *name, const char *file_path, uint32_t line_number) {
    if (!g_memory_manager.initialized) {
        LOG_ERROR("GPU memory manager not initialized");
        return NULL;
    }
    
    pthread_mutex_lock(&g_memory_manager.global_mutex);
    
    // Check if we have enough memory
    if (g_memory_manager.used_memory + size > g_memory_manager.total_budget) {
        LOG_WARN("GPU memory budget exceeded: used=%lluMB, budget=%lluMB, requested=%uB",
                 g_memory_manager.used_memory / (1024 * 1024),
                 g_memory_manager.total_budget / (1024 * 1024),
                 size);
        
        // Try to free unused memory
        if (g_memory_manager.enable_monitoring) {
            gpu_memory_defragment();
        }
        
        // Still not enough memory
        if (g_memory_manager.used_memory + size > g_memory_manager.total_budget) {
            pthread_mutex_unlock(&g_memory_manager.global_mutex);
            LOG_ERROR("Out of GPU memory");
            return NULL;
        }
    }
    
    // Try to allocate from pool first
    MemoryAllocation *allocation = allocate_from_pool(&g_memory_manager, type, usage, size);
    bool from_pool = (allocation != NULL);
    
    // If pool allocation failed, allocate directly
    if (!allocation) {
        if (g_memory_manager.allocation_count >= g_memory_manager.allocation_capacity) {
            LOG_ERROR("Too many GPU memory allocations");
            pthread_mutex_unlock(&g_memory_manager.global_mutex);
            return NULL;
        }
        
        allocation = calloc(1, sizeof(MemoryAllocation));
        if (!allocation) {
            LOG_ERROR("Failed to allocate memory allocation tracking");
            pthread_mutex_unlock(&g_memory_manager.global_mutex);
            return NULL;
        }
        
        allocation->type = type;
        allocation->usage = usage;
        allocation->is_pooled = false;
        allocation->ref_count = 1;
        pthread_mutex_init(&allocation->ref_mutex, NULL);
        
        g_memory_manager.allocations[g_memory_manager.allocation_count++] = allocation;
    }
    
    // Allocate the actual GPU resource
    void *resource = allocate_gpu_resource(type, usage, size, name, file_path, line_number);
    if (!resource) {
        if (from_pool) {
            return_to_pool(&g_memory_manager, allocation);
        } else {
            free(allocation);
            g_memory_manager.allocation_count--;
        }
        pthread_mutex_unlock(&g_memory_manager.global_mutex);
        return NULL;
    }
    
    // Set allocation properties
    allocation->resource = resource;
    allocation->size = size;
    allocation->allocation_id = generate_allocation_id();
    allocation->allocation_time = get_time_nanos();
    
    if (name) strncpy(allocation->name, name, sizeof(allocation->name) - 1);
    if (file_path) strncpy(allocation->file_path, file_path, sizeof(allocation->file_path) - 1);
    allocation->line_number = line_number;
    
    // Update statistics
    g_memory_manager.used_memory += size;
    g_memory_manager.type_used[type] += size;
    g_memory_manager.total_allocations++;
    
    if (g_memory_manager.used_memory > g_memory_manager.peak_memory) {
        g_memory_manager.peak_memory = g_memory_manager.used_memory;
    }
    
    // Check memory thresholds
    if (g_memory_manager.enable_monitoring) {
        float usage_percent = (float)g_memory_manager.used_memory / (float)g_memory_manager.total_budget;
        
        if (usage_percent > g_memory_manager.memory_critical_threshold) {
            LOG_ERROR("Critical GPU memory usage: %.1f%% (%lluMB / %lluMB)",
                     usage_percent * 100.0f,
                     g_memory_manager.used_memory / (1024 * 1024),
                     g_memory_manager.total_budget / (1024 * 1024));
        } else if (usage_percent > g_memory_manager.memory_warning_threshold) {
            LOG_WARN("High GPU memory usage: %.1f%% (%lluMB / %lluMB)",
                   usage_percent * 100.0f,
                   g_memory_manager.used_memory / (1024 * 1024),
                   g_memory_manager.total_budget / (1024 * 1024));
        }
    }
    
    pthread_mutex_unlock(&g_memory_manager.global_mutex);
    
    LOG_DEBUG("Allocated GPU resource: type=%d, usage=%d, size=%u, name=%s, id=%llu",
             (int)type, (int)usage, size, name ? name : "unnamed", allocation->allocation_id);
    
    return resource;
}

void gpu_memory_deallocate(void *resource) {
    if (!resource || !g_memory_manager.initialized) return;
    
    pthread_mutex_lock(&g_memory_manager.global_mutex);
    
    // Find the allocation for this resource
    MemoryAllocation *allocation = NULL;
    for (uint32_t i = 0; i < g_memory_manager.allocation_count; i++) {
        if (g_memory_manager.allocations[i] && g_memory_manager.allocations[i]->resource == resource) {
            allocation = g_memory_manager.allocations[i];
            break;
        }
    }
    
    if (!allocation) {
        LOG_WARN("Attempted to deallocate unknown GPU resource: %p", resource);
        pthread_mutex_unlock(&g_memory_manager.global_mutex);
        return;
    }
    
    // Decrease reference count
    pthread_mutex_lock(&allocation->ref_mutex);
    allocation->ref_count--;
    bool should_free = (allocation->ref_count == 0);
    pthread_mutex_unlock(&allocation->ref_mutex);
    
    if (!should_free) {
        pthread_mutex_unlock(&g_memory_manager.global_mutex);
        return;
    }
    
    // Free the actual GPU resource
    // TODO: Call appropriate GPU resource destruction function
    // destroy_gpu_resource(allocation->resource, allocation->type);
    
    // Update statistics
    g_memory_manager.used_memory -= allocation->size;
    g_memory_manager.type_used[allocation->type] -= allocation->size;
    g_memory_manager.total_deallocations++;
    
    // Return to pool or free allocation
    if (allocation->is_pooled) {
        return_to_pool(&g_memory_manager, allocation);
    } else {
        // Remove from allocation list
        for (uint32_t i = 0; i < g_memory_manager.allocation_count; i++) {
            if (g_memory_manager.allocations[i] == allocation) {
                g_memory_manager.allocations[i] = g_memory_manager.allocations[g_memory_manager.allocation_count - 1];
                g_memory_manager.allocation_count--;
                break;
            }
        }
        
        free(allocation);
    }
    
    pthread_mutex_unlock(&g_memory_manager.global_mutex);
    
    LOG_DEBUG("Deallocated GPU resource: type=%d, size=%u, id=%llu",
             (int)allocation->type, allocation->size, allocation->allocation_id);
}

void* gpu_memory_add_reference(void *resource) {
    if (!resource || !g_memory_manager.initialized) return NULL;
    
    pthread_mutex_lock(&g_memory_manager.global_mutex);
    
    // Find the allocation for this resource
    MemoryAllocation *allocation = NULL;
    for (uint32_t i = 0; i < g_memory_manager.allocation_count; i++) {
        if (g_memory_manager.allocations[i] && g_memory_manager.allocations[i]->resource == resource) {
            allocation = g_memory_manager.allocations[i];
            break;
        }
    }
    
    if (!allocation) {
        LOG_WARN("Attempted to add reference to unknown GPU resource: %p", resource);
        pthread_mutex_unlock(&g_memory_manager.global_mutex);
        return NULL;
    }
    
    pthread_mutex_lock(&allocation->ref_mutex);
    allocation->ref_count++;
    pthread_mutex_unlock(&allocation->ref_mutex);
    
    pthread_mutex_unlock(&g_memory_manager.global_mutex);
    
    return resource;
}

// ============================================================================
// Memory Management API
// ============================================================================

bool gpu_memory_manager_init(uint64_t total_budget_mb, bool enable_monitoring, 
                              float warning_threshold, float critical_threshold) {
    if (g_memory_manager.initialized) {
        LOG_WARN("GPU memory manager already initialized");
        return true;
    }
    
    memset(&g_memory_manager, 0, sizeof(GPUMemoryManager));
    
    g_memory_manager.total_budget = total_budget_mb * 1024 * 1024;
    g_memory_manager.enable_monitoring = enable_monitoring;
    g_memory_manager.memory_warning_threshold = warning_threshold;
    g_memory_manager.memory_critical_threshold = critical_threshold;
    
    // Initialize mutex
    if (pthread_mutex_init(&g_memory_manager.global_mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize GPU memory manager mutex");
        return false;
    }
    
    // Initialize memory pools
    if (!initialize_memory_pools(&g_memory_manager)) {
        LOG_ERROR("Failed to initialize memory pools");
        pthread_mutex_destroy(&g_memory_manager.global_mutex);
        return false;
    }
    
    g_memory_manager.initialized = true;
    LOG_INFO("GPU memory manager initialized (budget: %llu MB, monitoring: %s)",
             total_budget_mb,
             enable_monitoring ? "yes" : "no");
    return true;
}

void gpu_memory_manager_shutdown(void) {
    if (!g_memory_manager.initialized)
        return;
    
    LOG_INFO("Shutting down GPU memory manager");
    
    // Stop defragmentation thread if running
    if (g_memory_manager.defrag_running) {
        g_memory_manager.defrag_running = false;
        pthread_join(g_memory_manager.defrag_thread, NULL);
    }
    
    // Free all allocations
    for (uint32_t i = 0; i < g_memory_manager.allocation_count; i++) {
        if (g_memory_manager.allocations[i] && g_memory_manager.allocations[i]->resource) {
            gpu_memory_deallocate(g_memory_manager.allocations[i]->resource);
        }
    }
    
    // Free memory pools
    for (int type = 0; type < MEMORY_TYPE_COUNT; type++) {
        for (int usage = 0; usage < MEMORY_TYPE_COUNT; usage++) {
            if (g_memory_manager.pools[type][usage]) {
                free(g_memory_manager.pools[type][usage]);
                g_memory_manager.pools[type][usage] = NULL;
            }
        }
    }
    
    // Free allocation array
    free(g_memory_manager.allocations);
    
    // Destroy mutex
    pthread_mutex_destroy(&g_memory_manager.global_mutex);
    
    memset(&g_memory_manager, 0, sizeof(GPUMemoryManager));
    
    LOG_INFO("GPU memory manager shutdown complete");
}

void gpu_memory_set_budget(uint64_t budget_mb) {
    if (!g_memory_manager.initialized) return;
    
    pthread_mutex_lock(&g_memory_manager.global_mutex);
    
    g_memory_manager.total_budget = budget_mb * 1024 * 1024;
    
    LOG_INFO("GPU memory budget set to %llu MB", budget_mb);
    
    pthread_mutex_unlock(&g_memory_manager.global_mutex);
}

void gpu_memory_enable_monitoring(bool enable, float warning_threshold, float critical_threshold) {
    if (!g_memory_manager.initialized) return;
    
    pthread_mutex_lock(&g_memory_manager.global_mutex);
    
    g_memory_manager.enable_monitoring = enable;
    g_memory_manager.memory_warning_threshold = warning_threshold;
    g_memory_manager.memory_critical_threshold = critical_threshold;
    
    LOG_INFO("GPU memory monitoring: %s, warning=%.1f%%, critical=%.1f%%",
             enable ? "enabled" : "disabled",
             warning_threshold * 100.0f,
             critical_threshold * 100.0f);
    
    pthread_mutex_unlock(&g_memory_manager.global_mutex);
}

void gpu_memory_defragment(void) {
    if (!g_memory_manager.initialized || g_memory_manager.defrag_running) return;
    
    // TODO: Implement GPU memory defragmentation
    // This would involve:
    // 1. Analyze memory fragmentation
    // 2. Move resources to reduce fragmentation
    // 3. Update allocation tracking
    
    LOG_DEBUG("GPU memory defragmentation requested");
}

void gpu_memory_get_stats(uint64_t *used_memory, uint64_t *total_budget, 
                          uint64_t *peak_memory, uint64_t *total_allocations) {
    if (!g_memory_manager.initialized) return;
    
    pthread_mutex_lock(&g_memory_manager.global_mutex);
    
    if (used_memory) *used_memory = g_memory_manager.used_memory;
    if (total_budget) *total_budget = g_memory_manager.total_budget;
    if (peak_memory) *peak_memory = g_memory_manager.peak_memory;
    if (total_allocations) *total_allocations = g_memory_manager.total_allocations;
    
    pthread_mutex_unlock(&g_memory_manager.global_mutex);
}

void gpu_memory_print_leaks(void) {
    if (!g_memory_manager.initialized) return;
    
    pthread_mutex_lock(&g_memory_manager.global_mutex);
    
    bool found_leaks = false;
    
    for (uint32_t i = 0; i < g_memory_manager.allocation_count; i++) {
        MemoryAllocation *alloc = g_memory_manager.allocations[i];
        if (alloc && alloc->is_active && alloc->resource) {
            LOG_ERROR("GPU memory leak detected: type=%d, size=%u, name=%s, file=%s:%u, id=%llu",
                     (int)alloc->type, alloc->size, alloc->name, alloc->file_path, alloc->line_number,
                     alloc->allocation_id);
            found_leaks = true;
        }
    }
    
    if (!found_leaks) {
        LOG_INFO("No GPU memory leaks detected");
    }
    
    pthread_mutex_unlock(&g_memory_manager.global_mutex);
}

bool gpu_memory_is_initialized(void) {
    return g_memory_manager.initialized;
}
