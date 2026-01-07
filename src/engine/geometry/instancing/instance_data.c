/*
 * instance_data.c
 * Instance data management implementation
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "instance_data.h"
#include "../../rendering/3d_rendering/backend/metal/mtl_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

typedef struct instance_data_state {
    bool initialized;
    
    // Statistics
    uint32_t total_buffers;
    uint32_t total_instances;
    size_t total_memory_bytes;
    size_t peak_memory_bytes;
    uint32_t total_updates;
    
    // Active buffers list (for tracking)
    instance_buffer_t** buffers;
    uint32_t buffer_count;
    uint32_t buffer_capacity;
} instance_data_state_t;

static instance_data_state_t g_instance_state = {0};

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int instance_data_init(void) {
    if (g_instance_state.initialized) {
        return 0; // Already initialized
    }
    
    memset(&g_instance_state, 0, sizeof(instance_data_state_t));
    
    // Allocate buffer tracking array
    g_instance_state.buffer_capacity = 64;
    g_instance_state.buffers = (instance_buffer_t**)calloc(
        g_instance_state.buffer_capacity, sizeof(instance_buffer_t*));
    
    if (!g_instance_state.buffers) {
        return -1;
    }
    
    g_instance_state.initialized = true;
    printf("[Instance Data] System initialized\n");
    
    return 0;
}

void instance_data_shutdown(void) {
    if (!g_instance_state.initialized) {
        return;
    }
    
    // Destroy all remaining buffers
    for (uint32_t i = 0; i < g_instance_state.buffer_count; i++) {
        if (g_instance_state.buffers[i]) {
            instance_buffer_destroy(g_instance_state.buffers[i]);
        }
    }
    
    free(g_instance_state.buffers);
    
    printf("[Instance Data] System shutdown - %u buffers created, %zu peak memory\n",
           g_instance_state.total_buffers, g_instance_state.peak_memory_bytes);
    
    memset(&g_instance_state, 0, sizeof(instance_data_state_t));
}

/* ============================================================================
 * INSTANCE BUFFER MANAGEMENT
 * ============================================================================ */

instance_buffer_t* instance_buffer_create(metal_device_t* device, 
                                          const instance_buffer_desc_t* desc) {
    if (!g_instance_state.initialized) {
        fprintf(stderr, "[Instance Data] System not initialized\n");
        return NULL;
    }
    
    if (!device || !desc || desc->max_instances == 0) {
        fprintf(stderr, "[Instance Data] Invalid parameters\n");
        return NULL;
    }
    
    // Allocate instance buffer structure
    instance_buffer_t* buffer = (instance_buffer_t*)calloc(1, sizeof(instance_buffer_t));
    if (!buffer) {
        return NULL;
    }
    
    // Setup parameters
    buffer->max_instances = desc->max_instances;
    buffer->instance_count = desc->initial_count;
    buffer->instance_stride = sizeof(instance_data_t);
    buffer->allow_resize = desc->allow_resize;
    buffer->gpu_writable = desc->gpu_writable;
    buffer->total_bytes = buffer->max_instances * buffer->instance_stride;
    
    if (desc->label) {
        strncpy(buffer->label, desc->label, sizeof(buffer->label) - 1);
    } else {
        snprintf(buffer->label, sizeof(buffer->label), "InstanceBuffer_%p", (void*)buffer);
    }
    
    // Create Metal buffer
    metal_buffer_desc_t metal_desc = {0};
    metal_desc.size = buffer->total_bytes;
    metal_desc.storage_mode = METAL_STORAGE_SHARED; // CPU/GPU accessible
    metal_desc.usage = METAL_BUFFER_USAGE_VERTEX | METAL_BUFFER_USAGE_STORAGE;
    metal_desc.initial_data = (const void*)desc->initial_data;
    metal_desc.label = buffer->label;
    
    buffer->buffer = metal_buffer_create(device, &metal_desc);
    if (!buffer->buffer) {
        fprintf(stderr, "[Instance Data] Failed to create Metal buffer\n");
        free(buffer);
        return NULL;
    }
    
    // Track buffer
    if (g_instance_state.buffer_count >= g_instance_state.buffer_capacity) {
        // Grow buffer array
        uint32_t new_capacity = g_instance_state.buffer_capacity * 2;
        instance_buffer_t** new_buffers = (instance_buffer_t**)realloc(
            g_instance_state.buffers, new_capacity * sizeof(instance_buffer_t*));
        if (new_buffers) {
            g_instance_state.buffers = new_buffers;
            g_instance_state.buffer_capacity = new_capacity;
        }
    }
    
    if (g_instance_state.buffer_count < g_instance_state.buffer_capacity) {
        g_instance_state.buffers[g_instance_state.buffer_count++] = buffer;
    }
    
    // Update statistics
    g_instance_state.total_buffers++;
    g_instance_state.total_instances += buffer->instance_count;
    g_instance_state.total_memory_bytes += buffer->total_bytes;
    if (g_instance_state.total_memory_bytes > g_instance_state.peak_memory_bytes) {
        g_instance_state.peak_memory_bytes = g_instance_state.total_memory_bytes;
    }
    
    printf("[Instance Data] Created buffer '%s': %u instances, %zu bytes\n",
           buffer->label, buffer->max_instances, buffer->total_bytes);
    
    return buffer;
}

void instance_buffer_destroy(instance_buffer_t* buffer) {
    if (!buffer) {
        return;
    }
    
    // Destroy Metal buffer
    if (buffer->buffer) {
        metal_buffer_destroy(buffer->buffer);
    }
    
    // Remove from tracking
    for (uint32_t i = 0; i < g_instance_state.buffer_count; i++) {
        if (g_instance_state.buffers[i] == buffer) {
            // Shift remaining buffers
            for (uint32_t j = i; j < g_instance_state.buffer_count - 1; j++) {
                g_instance_state.buffers[j] = g_instance_state.buffers[j + 1];
            }
            g_instance_state.buffer_count--;
            break;
        }
    }
    
    // Update statistics
    g_instance_state.total_instances -= buffer->instance_count;
    g_instance_state.total_memory_bytes -= buffer->total_bytes;
    
    printf("[Instance Data] Destroyed buffer '%s'\n", buffer->label);
    
    free(buffer);
}

int instance_buffer_update(instance_buffer_t* buffer,
                          const instance_data_t* instances,
                          uint32_t count,
                          uint32_t offset) {
    if (!buffer || !instances || count == 0) {
        return -1;
    }
    
    if (offset + count > buffer->max_instances) {
        fprintf(stderr, "[Instance Data] Update out of bounds: offset=%u, count=%u, max=%u\n",
                offset, count, buffer->max_instances);
        return -1;
    }
    
    // Calculate byte offset and size
    size_t byte_offset = offset * buffer->instance_stride;
    size_t byte_size = count * buffer->instance_stride;
    
    // Update Metal buffer
    metal_buffer_update(buffer->buffer, instances, byte_size, byte_offset);
    
    // Update instance count if needed
    uint32_t new_count = offset + count;
    if (new_count > buffer->instance_count) {
        g_instance_state.total_instances += (new_count - buffer->instance_count);
        buffer->instance_count = new_count;
    }
    
    buffer->update_count++;
    g_instance_state.total_updates++;
    
    return 0;
}

int instance_buffer_update_single(instance_buffer_t* buffer,
                                  const instance_data_t* instance,
                                  uint32_t index) {
    return instance_buffer_update(buffer, instance, 1, index);
}

int instance_buffer_resize(instance_buffer_t* buffer, uint32_t new_capacity) {
    if (!buffer || !buffer->allow_resize) {
        fprintf(stderr, "[Instance Data] Buffer resize not allowed\n");
        return -1;
    }
    
    if (new_capacity <= buffer->max_instances) {
        return 0; // No need to resize
    }
    
    fprintf(stderr, "[Instance Data] Buffer resizing not yet implemented\n");
    // TODO: Implement resize by creating new buffer and copying data
    return -1;
}

uint32_t instance_buffer_get_count(const instance_buffer_t* buffer) {
    return buffer ? buffer->instance_count : 0;
}

void instance_buffer_set_count(instance_buffer_t* buffer, uint32_t count) {
    if (buffer && count <= buffer->max_instances) {
        int32_t delta = (int32_t)count - (int32_t)buffer->instance_count;
        buffer->instance_count = count;
        g_instance_state.total_instances += delta;
    }
}

metal_buffer_t* instance_buffer_get_metal_buffer(instance_buffer_t* buffer) {
    return buffer ? buffer->buffer : NULL;
}

bool instance_buffer_validate(const instance_buffer_t* buffer) {
    if (!buffer) {
        return false;
    }
    
    if (!buffer->buffer) {
        return false;
    }
    
    if (buffer->instance_count > buffer->max_instances) {
        return false;
    }
    
    if (buffer->instance_stride != sizeof(instance_data_t)) {
        return false;
    }
    
    return metal_buffer_validate(buffer->buffer);
}

/* ============================================================================
 * INSTANCE POOL MANAGEMENT
 * ============================================================================ */

instance_pool_t* instance_pool_create(const instance_pool_desc_t* desc) {
    if (!desc || desc->initial_capacity == 0) {
        return NULL;
    }
    
    instance_pool_t* pool = (instance_pool_t*)calloc(1, sizeof(instance_pool_t));
    if (!pool) {
        return NULL;
    }
    
    pool->capacity = desc->initial_capacity;
    pool->growth_factor = desc->growth_factor > 0 ? desc->growth_factor : 2;
    pool->alignment = desc->alignment > 0 ? desc->alignment : 16;
    
    // Allocate memory pool
    size_t total_size = pool->capacity * sizeof(instance_data_t);
    pool->memory = aligned_alloc(pool->alignment, total_size);
    if (!pool->memory) {
        free(pool);
        return NULL;
    }
    
    // Allocate free list
    pool->free_list = (uint32_t*)calloc(pool->capacity, sizeof(uint32_t));
    if (!pool->free_list) {
        free(pool->memory);
        free(pool);
        return NULL;
    }
    
    // Initialize free list
    for (uint32_t i = 0; i < pool->capacity; i++) {
        pool->free_list[i] = i;
    }
    pool->free_count = pool->capacity;
    
    if (desc->label) {
        strncpy(pool->label, desc->label, sizeof(pool->label) - 1);
    }
    
    printf("[Instance Pool] Created pool '%s': %u capacity\n", 
           pool->label, pool->capacity);
    
    return pool;
}

void instance_pool_destroy(instance_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    printf("[Instance Pool] Destroyed pool '%s': %u/%u peak usage\n",
           pool->label, pool->peak_allocated, pool->capacity);
    
    free(pool->free_list);
    free(pool->memory);
    free(pool);
}

int instance_pool_alloc(instance_pool_t* pool, uint32_t count, uint32_t* out_index) {
    if (!pool || count == 0 || !out_index) {
        return -1;
    }
    
    if (pool->free_count < count) {
        fprintf(stderr, "[Instance Pool] Insufficient space: requested=%u, available=%u\n",
                count, pool->free_count);
        return -1;
    }
    
    // Allocate from free list
    *out_index = pool->free_list[pool->free_count - count];
    pool->free_count -= count;
    pool->allocated += count;
    pool->total_allocations++;
    
    if (pool->allocated > pool->peak_allocated) {
        pool->peak_allocated = pool->allocated;
    }
    
    return 0;
}

int instance_pool_free(instance_pool_t* pool, uint32_t index, uint32_t count) {
    if (!pool || count == 0) {
        return -1;
    }
    
    if (index + count > pool->capacity) {
        return -1;
    }
    
    // Return to free list
    for (uint32_t i = 0; i < count; i++) {
        pool->free_list[pool->free_count++] = index + i;
    }
    
    pool->allocated -= count;
    
    return 0;
}

void instance_pool_reset(instance_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    // Reinitialize free list
    for (uint32_t i = 0; i < pool->capacity; i++) {
        pool->free_list[i] = i;
    }
    pool->free_count = pool->capacity;
    pool->allocated = 0;
}

bool instance_pool_can_allocate(const instance_pool_t* pool, uint32_t count) {
    return pool && pool->free_count >= count;
}

/* ============================================================================
 * UTILITIES & STATISTICS
 * ============================================================================ */

instance_buffer_stats_t instance_buffer_get_stats(void) {
    instance_buffer_stats_t stats = {0};
    
    if (g_instance_state.initialized) {
        stats.total_buffers = g_instance_state.total_buffers;
        stats.total_instances = g_instance_state.total_instances;
        stats.total_memory_bytes = g_instance_state.total_memory_bytes;
        stats.peak_memory_bytes = g_instance_state.peak_memory_bytes;
        stats.total_updates = g_instance_state.total_updates;
    }
    
    return stats;
}

size_t instance_data_calculate_size(uint32_t instance_count) {
    return instance_count * sizeof(instance_data_t);
}

bool instance_data_validate(const instance_data_t* instance) {
    if (!instance) {
        return false;
    }
    
    // Basic validation
    // Check for NaN/Inf in transform matrix
    for (int i = 0; i < 16; i++) {
        float val = instance->transform.matrix[i];
        if (val != val || val == INFINITY || val == -INFINITY) {
            return false;
        }
    }
    
    return true;
}

void instance_buffer_debug_print(const instance_buffer_t* buffer) {
    if (!buffer) {
        return;
    }
    
    printf("=== Instance Buffer: %s ===\n", buffer->label);
    printf("  Instances: %u / %u\n", buffer->instance_count, buffer->max_instances);
    printf("  Stride: %zu bytes\n", buffer->instance_stride);
    printf("  Total Size: %zu bytes\n", buffer->total_bytes);
    printf("  Updates: %u\n", buffer->update_count);
    printf("  Resizable: %s\n", buffer->allow_resize ? "Yes" : "No");
    printf("  GPU Writable: %s\n", buffer->gpu_writable ? "Yes" : "No");
}

void instance_pool_debug_print(const instance_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    printf("=== Instance Pool: %s ===\n", pool->label);
    printf("  Capacity: %u\n", pool->capacity);
    printf("  Allocated: %u\n", pool->allocated);
    printf("  Free: %u\n", pool->free_count);
    printf("  Peak: %u\n", pool->peak_allocated);
    printf("  Total Allocations: %u\n", pool->total_allocations);
    printf("  Utilization: %.1f%%\n", 
           (float)pool->allocated / pool->capacity * 100.0f);
}
