#include "resource_pool.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Internal structure for pool management
struct resource_pool_t {
    uint8_t* data;              // Dense data array
    uint32_t* generations;      // Parallel array of generations
    uint32_t* free_indices;     // Stack of free indices
    
    uint32_t element_size;
    uint32_t capacity;
    uint32_t count;
    uint32_t free_count;
    
    char name[32];
};

resource_pool_t* resource_pool_create(const resource_pool_desc_t* desc) {
    if (!desc || desc->element_size == 0 || desc->capacity == 0) return NULL;

    resource_pool_t* pool = (resource_pool_t*)calloc(1, sizeof(resource_pool_t));
    if (!pool) return NULL;

    pool->element_size = desc->element_size;
    pool->capacity = desc->capacity;
    pool->count = 0;
    
    // Allocate arrays
    pool->data = (uint8_t*)calloc(pool->capacity, pool->element_size);
    pool->generations = (uint32_t*)calloc(pool->capacity, sizeof(uint32_t));
    pool->free_indices = (uint32_t*)malloc(pool->capacity * sizeof(uint32_t));
    
    if (!pool->data || !pool->generations || !pool->free_indices) {
        resource_pool_destroy(pool);
        return NULL;
    }

    // Initialize free list (reverse order so 0 is first allocated)
    for (uint32_t i = 0; i < pool->capacity; ++i) {
        pool->free_indices[i] = pool->capacity - 1 - i;
    }
    pool->free_count = pool->capacity;

    if (desc->name) {
        strncpy(pool->name, desc->name, sizeof(pool->name) - 1);
    } else {
        strcpy(pool->name, "ResourcePool");
    }

    return pool;
}

void resource_pool_destroy(resource_pool_t* pool) {
    if (!pool) return;
    if (pool->data) free(pool->data);
    if (pool->generations) free(pool->generations);
    if (pool->free_indices) free(pool->free_indices);
    free(pool);
}

resource_handle_t resource_pool_alloc(resource_pool_t* pool) {
    if (!pool || pool->free_count == 0) {
        return resource_handle_invalid();
    }

    // Pop free index
    uint32_t index = pool->free_indices[--pool->free_count];
    
    // Increment generation (start at 1 to avoid 0 generation)
    pool->generations[index]++;
    if (pool->generations[index] == 0) pool->generations[index] = 1;

    pool->count++;
    
    // Zero out the memory slot for safety
    memset(pool->data + (index * pool->element_size), 0, pool->element_size);

    return resource_handle_create(index, pool->generations[index]);
}

bool resource_pool_free(resource_pool_t* pool, resource_handle_t handle) {
    if (!pool || !resource_handle_is_valid(handle)) return false;

    uint32_t index = resource_handle_index(handle);
    uint32_t gen = resource_handle_generation(handle);

    if (index >= pool->capacity) return false;
    
    // Check generation match
    if (pool->generations[index] != gen) {
        // Double free or stale handle access
        return false;
    }

    // Mark as free by incrementing generation immediately?
    // Or just putting back in free list.
    // Usually we increment generation on alloc, but doing it on free helps catch dangling usage sooner.
    // Let's increment on free to invalidate current handles immediately.
    // NOTE: We rely on alloc to also ensure non-zero generation.
    pool->generations[index]++;
    
    // Push to free list
    pool->free_indices[pool->free_count++] = index;
    pool->count--;

    return true;
}

void* resource_pool_get(resource_pool_t* pool, resource_handle_t handle) {
    if (!pool || !resource_handle_is_valid(handle)) return NULL;

    uint32_t index = resource_handle_index(handle);
    uint32_t gen = resource_handle_generation(handle);

    if (index >= pool->capacity) return NULL;
    if (pool->generations[index] != gen) return NULL;

    return pool->data + (index * pool->element_size);
}

void* resource_pool_get_raw(resource_pool_t* pool, uint32_t index) {
    if (!pool || index >= pool->capacity) return NULL;
    // Warning: Unsafe access, ignores generation
    return pool->data + (index * pool->element_size);
}

void resource_pool_clear(resource_pool_t* pool) {
    if (!pool) return;
    
    // Reset all
    pool->count = 0;
    pool->free_count = pool->capacity;
    memset(pool->generations, 0, pool->capacity * sizeof(uint32_t));
    
    // Reset free list
    for (uint32_t i = 0; i < pool->capacity; ++i) {
        pool->free_indices[i] = pool->capacity - 1 - i;
    }
}

uint32_t resource_pool_count(resource_pool_t* pool) {
    return pool ? pool->count : 0;
}

uint32_t resource_pool_capacity(resource_pool_t* pool) {
    return pool ? pool->capacity : 0;
}
