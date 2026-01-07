/*
 * buffer_pool.c
 * Buffer pooling implementation
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "rendering/gpu_driven/buffer_pool.h"
#include "backend/metal/mtl_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static uint32_t g_next_pool_id = 1;

/* ============================================================================
 * POOL MANAGEMENT
 * ============================================================================ */

buffer_pool_t* buffer_pool_create(const buffer_pool_desc_t* desc) {
    if (!desc || desc->initial_capacity == 0 || desc->buffer_size == 0) {
        return NULL;
    }
    
    buffer_pool_t* pool = (buffer_pool_t*)calloc(1, sizeof(buffer_pool_t));
    if (!pool) {
        return NULL;
    }
    
    pool->capacity = desc->initial_capacity;
    pool->buffer_size = desc->buffer_size;
    pool->max_capacity = desc->max_capacity > 0 ? desc->max_capacity : UINT32_MAX;
    pool->allow_growth = desc->allow_growth;
    pool->pool_id = g_next_pool_id++;
    
    if (desc->label) {
        strncpy(pool->label, desc->label, sizeof(pool->label) - 1);
    } else {
        snprintf(pool->label, sizeof(pool->label), "BufferPool_%u", pool->pool_id);
    }
    
    // Allocate arrays
    pool->buffers = (metal_buffer_t**)calloc(pool->capacity, sizeof(metal_buffer_t*));
    pool->generations = (uint64_t*)calloc(pool->capacity, sizeof(uint64_t));
    pool->free_list = (uint32_t*)calloc(pool->capacity, sizeof(uint32_t));
    
    if (!pool->buffers || !pool->generations || !pool->free_list) {
        buffer_pool_destroy(pool);
        return NULL;
    }
    
    // Initialize free list
    for (uint32_t i = 0; i < pool->capacity; i++) {
        pool->free_list[i] = i;
    }
    pool->free_count = pool->capacity;
    
    printf("[Buffer Pool] Created '%s': %u buffers of %zu bytes\n",
           pool->label, pool->capacity, pool->buffer_size);
    
    return pool;
}

void buffer_pool_destroy(buffer_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    // Destroy all buffers
    for (uint32_t i = 0; i < pool->capacity; i++) {
        if (pool->buffers && pool->buffers[i]) {
            metal_buffer_destroy(pool->buffers[i]);
        }
    }
    
    free(pool->buffers);
    free(pool->generations);
    free(pool->free_list);
    
    printf("[Buffer Pool] Destroyed '%s'\n", pool->label);
    free(pool);
}

void buffer_pool_reset(buffer_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    // Reset free list
    for (uint32_t i = 0; i < pool->capacity; i++) {
        pool->free_list[i] = i;
        pool->generations[i]++;  // Invalidate old handles
    }
    
    pool->free_count = pool->capacity;
    pool->allocated = 0;
    
    printf("[Buffer Pool] Reset '%s'\n", pool->label);
}

/* ============================================================================
 * BUFFER ALLOCATION
 * ============================================================================ */

int buffer_pool_allocate(
    buffer_pool_t* pool,
    pooled_buffer_handle_t* out_handle,
    metal_buffer_t** out_buffer) {
    
    if (!pool || !out_handle || !out_buffer) {
        return -1;
    }
    
    // Check if pool has free buffers
    if (pool->free_count == 0) {
        // Try to grow pool
        if (pool->allow_growth && pool->capacity < pool->max_capacity) {
            uint32_t new_capacity = pool->capacity * 2;
            if (new_capacity > pool->max_capacity) {
                new_capacity = pool->max_capacity;
            }
            
            // Reallocate arrays
            metal_buffer_t** new_buffers = (metal_buffer_t**)realloc(
                pool->buffers, new_capacity * sizeof(metal_buffer_t*));
            uint64_t* new_generations = (uint64_t*)realloc(
                pool->generations, new_capacity * sizeof(uint64_t));
            uint32_t* new_free_list = (uint32_t*)realloc(
                pool->free_list, new_capacity * sizeof(uint32_t));
            
            if (!new_buffers || !new_generations || !new_free_list) {
                fprintf(stderr, "[Buffer Pool] Failed to grow pool\n");
                return -1;
            }
            
            pool->buffers = new_buffers;
            pool->generations = new_generations;
            pool->free_list = new_free_list;
            
            // Initialize new slots
            for (uint32_t i = pool->capacity; i < new_capacity; i++) {
                pool->buffers[i] = NULL;
                pool->generations[i] = 0;
                pool->free_list[pool->free_count++] = i;
            }
            
            pool->capacity = new_capacity;
            
            printf("[Buffer Pool] Grew '%s' to %u buffers\n", pool->label, new_capacity);
        } else {
            fprintf(stderr, "[Buffer Pool] Pool '%s' exhausted\n", pool->label);
            return -1;
        }
    }
    
    // Get buffer from free list
    uint32_t index = pool->free_list[--pool->free_count];
    
    // Lazy create buffer if needed
    if (!pool->buffers[index]) {
        metal_buffer_desc_t buffer_desc = {0};
        buffer_desc.size = pool->buffer_size;
        buffer_desc.storage_mode = METAL_STORAGE_SHARED;
        buffer_desc.usage = METAL_BUFFER_USAGE_STORAGE;
        
        char label[128];
        snprintf(label, sizeof(label), "%s_Buffer_%u", pool->label, index);
        buffer_desc.label = label;
        
        pool->buffers[index] = metal_buffer_create(NULL, &buffer_desc);
        if (!pool->buffers[index]) {
            // Return to free list
            pool->free_list[pool->free_count++] = index;
            return -1;
        }
    }
    
    // Fill output
    out_handle->pool_id = pool->pool_id;
    out_handle->buffer_index = index;
    out_handle->generation = pool->generations[index];
    *out_buffer = pool->buffers[index];
    
    pool->allocated++;
    pool->total_allocations++;
    
    if (pool->allocated > pool->peak_allocated) {
        pool->peak_allocated = pool->allocated;
    }
    
    return 0;
}

int buffer_pool_free(
    buffer_pool_t* pool,
    pooled_buffer_handle_t handle) {
    
    if (!pool) {
        return -1;
    }
    
    // Validate handle
    if (handle.pool_id != pool->pool_id ||
        handle.buffer_index >= pool->capacity ||
        handle.generation != pool->generations[handle.buffer_index]) {
        fprintf(stderr, "[Buffer Pool] Invalid handle\n");
        return -1;
    }
    
    // Return to free list
    pool->free_list[pool->free_count++] = handle.buffer_index;
    pool->allocated--;
    
    // Increment generation to invalidate handle
    pool->generations[handle.buffer_index]++;
    
    return 0;
}

metal_buffer_t* buffer_pool_get_buffer(
    buffer_pool_t* pool,
    pooled_buffer_handle_t handle) {
    
    if (!pool || !buffer_pool_is_valid(pool, handle)) {
        return NULL;
    }
    
    return pool->buffers[handle.buffer_index];
}

bool buffer_pool_is_valid(
    const buffer_pool_t* pool,
    pooled_buffer_handle_t handle) {
    
    if (!pool || handle.pool_id != pool->pool_id) {
        return false;
    }
    
    if (handle.buffer_index >= pool->capacity) {
        return false;
    }
    
    if (handle.generation != pool->generations[handle.buffer_index]) {
        return false;
    }
    
    return true;
}

/* ============================================================================
 * STATISTICS
 * ============================================================================ */

buffer_pool_stats_t buffer_pool_get_stats(const buffer_pool_t* pool) {
    buffer_pool_stats_t stats = {0};
    
    if (pool) {
        stats.capacity = pool->capacity;
        stats.allocated = pool->allocated;
        stats.peak_allocated = pool->peak_allocated;
        stats.total_allocations = pool->total_allocations;
        stats.total_memory_bytes = pool->capacity * pool->buffer_size;
        stats.utilization = pool->capacity > 0 ? 
            ((float)pool->allocated / pool->capacity) : 0.0f;
    }
    
    return stats;
}

void buffer_pool_debug_print(const buffer_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    printf("=== Buffer Pool: %s ===\n", pool->label);
    printf("  Capacity: %u (max: %u)\n", pool->capacity, pool->max_capacity);
    printf("  Allocated: %u\n", pool->allocated);
    printf("  Free: %u\n", pool->free_count);
    printf("  Peak: %u\n", pool->peak_allocated);
    printf("  Total Allocations: %u\n", pool->total_allocations);
    printf("  Utilization: %.1f%%\n", (float)pool->allocated / pool->capacity * 100.0f);
    printf("  Memory: %.2f MB\n", (pool->capacity * pool->buffer_size) / (1024.0f * 1024.0f));
    printf("  Growth: %s\n", pool->allow_growth ? "Enabled" : "Disabled");
}
