/*
 * command_pool.c
 * Implementation of thread-local command pool management
 */

#include "core/command/command_pool.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Internal Structures
// ============================================================================

// Internal structure definition
struct command_pool {
    void* backend_handle;       // e.g., VkCommandPool
    uint32_t queue_family_index;
    uint32_t flags;
    uint32_t allocated_count;   // Track number of allocated command buffers
    uint32_t max_allocations;   // Maximum before pool should be reset
    bool needs_reset;           // Flag for pool reset scheduling
};

// ============================================================================
// Backend Integration
// ============================================================================

// Abstract backend pool creation
// In real Vulkan: vkCreateCommandPool with VkCommandPoolCreateInfo
// In real D3D12: CreateCommandAllocator  
// In real Metal: MTLCommandQueue (pools are implicit)
static void* backend_create_pool(uint32_t queue_family_index, uint32_t flags) {
    // Allocate tracking structure for the abstract pool
    typedef struct {
        uint32_t queue_family;
        uint32_t create_flags;
        uint64_t pool_id;
    } backend_pool_t;
    
    static uint64_t pool_counter = 0;
    
    backend_pool_t* pool = (backend_pool_t*)calloc(1, sizeof(backend_pool_t));
    if (pool) {
        pool->queue_family = queue_family_index;
        pool->create_flags = flags;
        pool->pool_id = ++pool_counter;
    }
    return pool;
}

// Abstract backend pool destruction
// In real Vulkan: vkDestroyCommandPool
// In real D3D12: Release CommandAllocator
static void backend_destroy_pool(void* handle) {
    if (handle) free(handle);
}

// Abstract backend pool reset
// In real Vulkan: vkResetCommandPool  
// In real D3D12: Reset CommandAllocator
// Frees all command buffers allocated from this pool
static void backend_reset_pool(void* handle) {
    // In real implementation, this would reset the GPU pool
    // All command buffers from this pool become invalid
    (void)handle;
}

// ============================================================================
// Public API
// ============================================================================

command_pool_t* command_pool_create(const command_pool_desc_t* desc) {
    if (!desc) return NULL;

    command_pool_t* pool = (command_pool_t*)malloc(sizeof(command_pool_t));
    if (!pool) return NULL;

    pool->queue_family_index = desc->queue_family_index;
    pool->flags = desc->flags;
    pool->allocated_count = 0;
    pool->max_allocations = 64;  // Reasonable default
    pool->needs_reset = false;
    
    // Backend creation.
    pool->backend_handle = backend_create_pool(desc->queue_family_index, desc->flags);
    if (!pool->backend_handle) {
        free(pool);
        return NULL;
    }

    return pool;
}

void command_pool_destroy(command_pool_t* pool) {
    if (!pool) return;

    // Backend destruction.
    backend_destroy_pool(pool->backend_handle);

    free(pool);
}

void command_pool_reset(command_pool_t* pool) {
    if (!pool) return;

    // Backend reset.
    backend_reset_pool(pool->backend_handle);
    
    // Reset tracking state
    pool->allocated_count = 0;
    pool->needs_reset = false;
}

void* command_pool_get_handle(command_pool_t* pool) {
    if (!pool) return NULL;
    return pool->backend_handle;
}

// Additional utility functions
uint32_t command_pool_get_allocated_count(command_pool_t* pool) {
    return pool ? pool->allocated_count : 0;
}

void command_pool_increment_allocated(command_pool_t* pool) {
    if (pool) {
        pool->allocated_count++;
        if (pool->allocated_count >= pool->max_allocations) {
            pool->needs_reset = true;
        }
    }
}

bool command_pool_needs_reset(command_pool_t* pool) {
    return pool ? pool->needs_reset : false;
}
