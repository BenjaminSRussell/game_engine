/*
 * instance_buffer.c
 * Instance Buffer Management
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "instance_buffer.h"
#include "../../backend/metal/metal/mtl_buffer.h"
#include "../../backend/metal/metal/mtl_device.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define INSTANCE_BUFFER_DEFAULT_CAPACITY 256
#define INSTANCE_BUFFER_ALIGNMENT 256  // Metal buffer alignment requirement

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct instance_buffer_internal {
    uint32_t id;
    uint32_t flags;
    metal_buffer_t* gpu_buffer;
    void* cpu_data;
    size_t capacity;
    size_t size;
    uint32_t instance_count;
    uint32_t instance_stride;
    bool initialized;
    bool dirty;
} instance_buffer_internal_t;

typedef struct instance_buffer_context {
    instance_buffer_internal_t* buffers;
    uint32_t count;
    uint32_t capacity;
    metal_device_t* device;
    bool initialized;
} instance_buffer_context_t;

static instance_buffer_context_t g_instance_buffer_ctx = {0};

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================ */

static size_t align_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static bool allocate_instance_buffer(instance_buffer_internal_t* buffer, 
                                     uint32_t instance_count,
                                     uint32_t instance_stride) {
    size_t required_size = align_size(instance_count * instance_stride, INSTANCE_BUFFER_ALIGNMENT);
    
    // Allocate CPU-side buffer
    buffer->cpu_data = malloc(required_size);
    if (!buffer->cpu_data) {
        return false;
    }
    memset(buffer->cpu_data, 0, required_size);
    
    // Create GPU buffer (Metal shared storage for CPU/GPU access)
    if (g_instance_buffer_ctx.device) {
        metal_buffer_desc_t desc = {
            .size = required_size,
            .storage_mode = METAL_STORAGE_SHARED,
            .usage = METAL_BUFFER_USAGE_VERTEX,
            .initial_data = NULL
        };
        buffer->gpu_buffer = metal_buffer_create(g_instance_buffer_ctx.device, &desc);
        if (!buffer->gpu_buffer) {
            free(buffer->cpu_data);
            return false;
        }
    }
    
    buffer->capacity = required_size;
    buffer->size = 0;
    buffer->instance_count = 0;
    buffer->instance_stride = instance_stride;
    buffer->dirty = true;
    
    return true;
}

static void free_instance_buffer(instance_buffer_internal_t* buffer) {
    if (buffer->gpu_buffer) {
        metal_buffer_destroy(buffer->gpu_buffer);
        buffer->gpu_buffer = NULL;
    }
    if (buffer->cpu_data) {
        free(buffer->cpu_data);
        buffer->cpu_data = NULL;
    }
    buffer->capacity = 0;
    buffer->size = 0;
    buffer->instance_count = 0;
}

static bool resize_instance_buffer(instance_buffer_internal_t* buffer, uint32_t new_instance_count) {
    size_t new_size = align_size(new_instance_count * buffer->instance_stride, INSTANCE_BUFFER_ALIGNMENT);
    
    if (new_size <= buffer->capacity) {
        return true;
    }
    
    // Allocate new CPU buffer
    void* new_cpu_data = malloc(new_size);
    if (!new_cpu_data) {
        return false;
    }
    
    // Copy existing data
    if (buffer->cpu_data && buffer->size > 0) {
        memcpy(new_cpu_data, buffer->cpu_data, buffer->size);
    }
    memset((uint8_t*)new_cpu_data + buffer->size, 0, new_size - buffer->size);
    
    // Create new GPU buffer
    metal_buffer_t* new_gpu_buffer = NULL;
    if (g_instance_buffer_ctx.device) {
        metal_buffer_desc_t desc = {
            .size = new_size,
            .storage_mode = METAL_STORAGE_SHARED,
            .usage = METAL_BUFFER_USAGE_VERTEX,
            .initial_data = new_cpu_data
        };
        new_gpu_buffer = metal_buffer_create(g_instance_buffer_ctx.device, &desc);
        if (!new_gpu_buffer) {
            free(new_cpu_data);
            return false;
        }
    }
    
    // Free old buffers
    if (buffer->gpu_buffer) {
        metal_buffer_destroy(buffer->gpu_buffer);
    }
    if (buffer->cpu_data) {
        free(buffer->cpu_data);
    }
    
    // Update buffer
    buffer->cpu_data = new_cpu_data;
    buffer->gpu_buffer = new_gpu_buffer;
    buffer->capacity = new_size;
    buffer->dirty = true;
    
    return true;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int geometry_instance_buffer_init(void) {
    if (g_instance_buffer_ctx.initialized) {
        return 0;
    }
    
    g_instance_buffer_ctx.capacity = INSTANCE_BUFFER_DEFAULT_CAPACITY;
    g_instance_buffer_ctx.buffers = calloc(g_instance_buffer_ctx.capacity,
                                           sizeof(instance_buffer_internal_t));
    if (!g_instance_buffer_ctx.buffers) {
        return -1;
    }
    
    // Get Metal device (would be passed in or retrieved from global context)
    // g_instance_buffer_ctx.device = metal_device_get_global();
    g_instance_buffer_ctx.device = NULL;  // TODO: Get device from context
    
    g_instance_buffer_ctx.count = 0;
    g_instance_buffer_ctx.initialized = true;
    
    return 0;
}

void geometry_instance_buffer_shutdown(void) {
    if (!g_instance_buffer_ctx.initialized) {
        return;
    }
    
    for (uint32_t i = 0; i < g_instance_buffer_ctx.count; i++) {
        free_instance_buffer(&g_instance_buffer_ctx.buffers[i]);
    }
    
    free(g_instance_buffer_ctx.buffers);
    g_instance_buffer_ctx.buffers = NULL;
    g_instance_buffer_ctx.count = 0;
    g_instance_buffer_ctx.capacity = 0;
    g_instance_buffer_ctx.device = NULL;
    g_instance_buffer_ctx.initialized = false;
}

int geometry_instance_buffer_create(geometry_instance_buffer_handle_t* out_handle,
                                    const geometry_instance_buffer_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }
    
    if (!g_instance_buffer_ctx.initialized) {
        return -2;
    }
    
    if (g_instance_buffer_ctx.count >= g_instance_buffer_ctx.capacity) {
        return -3;
    }
    
    uint32_t index = g_instance_buffer_ctx.count++;
    instance_buffer_internal_t* buffer = &g_instance_buffer_ctx.buffers[index];
    
    buffer->id = index;
    buffer->flags = desc->flags;
    buffer->initialized = true;
    buffer->dirty = true;
    
    uint32_t initial_capacity = desc->initial_capacity > 0 ? desc->initial_capacity : 128;
    uint32_t stride = desc->instance_stride > 0 ? desc->instance_stride : 64;  // Default 64 bytes
    
    if (!allocate_instance_buffer(buffer, initial_capacity, stride)) {
        buffer->initialized = false;
        g_instance_buffer_ctx.count--;
        return -4;
    }
    
    out_handle->id = index;
    return 0;
}

void geometry_instance_buffer_destroy(geometry_instance_buffer_handle_t handle) {
    if (handle.id >= g_instance_buffer_ctx.count) {
        return;
    }
    
    instance_buffer_internal_t* buffer = &g_instance_buffer_ctx.buffers[handle.id];
    free_instance_buffer(buffer);
    buffer->initialized = false;
}

int geometry_instance_buffer_update(geometry_instance_buffer_handle_t handle,
                                    const void* data, size_t size) {
    if (handle.id >= g_instance_buffer_ctx.count) {
        return -1;
    }
    
    instance_buffer_internal_t* buffer = &g_instance_buffer_ctx.buffers[handle.id];
    if (!buffer->initialized) {
        return -2;
    }
    
    if (!data || size == 0) {
        return -3;
    }
    
    // Resize if needed
    uint32_t required_instances = (size + buffer->instance_stride - 1) / buffer->instance_stride;
    if (size > buffer->capacity) {
        if (!resize_instance_buffer(buffer, required_instances)) {
            return -4;
        }
    }
    
    // Update CPU buffer
    memcpy(buffer->cpu_data, data, size);
    buffer->size = size;
    buffer->instance_count = required_instances;
    buffer->dirty = true;
    
    return 0;
}

bool geometry_instance_buffer_is_valid(geometry_instance_buffer_handle_t handle) {
    if (handle.id >= g_instance_buffer_ctx.count) {
        return false;
    }
    return g_instance_buffer_ctx.buffers[handle.id].initialized;
}

int geometry_instance_buffer_get_info(geometry_instance_buffer_handle_t handle,
                                      geometry_instance_buffer_info_t* out_info) {
    if (!out_info) {
        return -1;
    }
    
    if (handle.id >= g_instance_buffer_ctx.count) {
        return -2;
    }
    
    const instance_buffer_internal_t* buffer = &g_instance_buffer_ctx.buffers[handle.id];
    out_info->id = buffer->id;
    out_info->flags = buffer->flags;
    out_info->initialized = buffer->initialized;
    out_info->instance_count = buffer->instance_count;
    out_info->capacity = buffer->capacity;
    
    return 0;
}

void geometry_instance_buffer_mark_dirty(geometry_instance_buffer_handle_t handle) {
    if (handle.id < g_instance_buffer_ctx.count) {
        g_instance_buffer_ctx.buffers[handle.id].dirty = true;
    }
}

int geometry_instance_buffer_process_pending(void) {
    int processed = 0;
    
    for (uint32_t i = 0; i < g_instance_buffer_ctx.count; i++) {
        instance_buffer_internal_t* buffer = &g_instance_buffer_ctx.buffers[i];
        if (buffer->initialized && buffer->dirty && buffer->gpu_buffer) {
            // Upload to GPU
            metal_buffer_update(buffer->gpu_buffer, buffer->cpu_data, buffer->size, 0);
            buffer->dirty = false;
            processed++;
        }
    }
    
    return processed;
}

uint32_t geometry_instance_buffer_get_count(void) {
    return g_instance_buffer_ctx.count;
}

size_t geometry_instance_buffer_get_memory_usage(void) {
    size_t total = sizeof(g_instance_buffer_ctx);
    total += g_instance_buffer_ctx.capacity * sizeof(instance_buffer_internal_t);
    
    for (uint32_t i = 0; i < g_instance_buffer_ctx.count; i++) {
        const instance_buffer_internal_t* buffer = &g_instance_buffer_ctx.buffers[i];
        total += buffer->capacity;
    }
    
    return total;
}

void geometry_instance_buffer_debug_print(void) {
    printf("[Instance Buffer] Total buffers: %u\n", g_instance_buffer_ctx.count);
    
    for (uint32_t i = 0; i < g_instance_buffer_ctx.count; i++) {
        const instance_buffer_internal_t* buffer = &g_instance_buffer_ctx.buffers[i];
        if (buffer->initialized) {
            printf("  Buffer %u: %u instances, %zu bytes\n", 
                   i, buffer->instance_count, buffer->size);
        }
    }
}

/* End of instance_buffer.c */
