/*
 * gpu_instance_data.c
 * GPU-side instance storage and buffer management
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "gpu_instance_data.h"
#include "../../math/vec3.h"
#include "../../math/mat4.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define GPU_INSTANCE_DATA_DEFAULT_CAPACITY 1024
#define GPU_INSTANCE_MAX_INSTANCES 65536
#define GPU_INSTANCE_BUFFER_ALIGNMENT 256

/* ============================================================================
 * TYPES
 * ============================================================================ */

// GPU-aligned instance data (matches shader layout)
typedef struct ALIGN(16) gpu_instance {
    mat4_t transform;          // 64 bytes
    vec4_t color_scale;        // 16 bytes
    uint32_t material_id;      // 4 bytes
    uint32_t lod_index;        // 4 bytes
    uint32_t custom_data0;     // 4 bytes
    uint32_t custom_data1;     // 4 bytes
} gpu_instance_t;              // Total: 96 bytes

typedef struct gpu_instance_buffer {
    gpu_instance_t* cpu_buffer;
    void* gpu_buffer_handle;   // Opaque GPU buffer handle
    uint32_t capacity;
    uint32_t count;
    bool dirty;
} gpu_instance_buffer_t;

typedef struct geometry_gpu_instance_data_internal {
    uint32_t id;
    uint32_t flags;
    gpu_instance_buffer_t buffer;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} geometry_gpu_instance_data_internal_t;

typedef struct geometry_gpu_instance_data_context {
    geometry_gpu_instance_data_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    size_t total_gpu_memory;
    bool initialized;
} geometry_gpu_instance_data_context_t;

static geometry_gpu_instance_data_context_t g_gpu_instance_data_ctx = {0};

/* ============================================================================
 * PRIVATE HELPER FUNCTIONS
 * ============================================================================ */

static bool allocate_gpu_buffer(gpu_instance_buffer_t* buffer, uint32_t capacity) {
    buffer->cpu_buffer = aligned_alloc(GPU_INSTANCE_BUFFER_ALIGNMENT, 
                                       capacity * sizeof(gpu_instance_t));
    if (!buffer->cpu_buffer) {
        return false;
    }
    
    memset(buffer->cpu_buffer, 0, capacity * sizeof(gpu_instance_t));
    buffer->capacity = capacity;
    buffer->count = 0;
    buffer->dirty = true;
    
    // TODO: Allocate actual GPU buffer via graphics API
    buffer->gpu_buffer_handle = NULL;
    
    return true;
}

static void free_gpu_buffer(gpu_instance_buffer_t* buffer) {
    if (buffer->cpu_buffer) {
        free(buffer->cpu_buffer);
        buffer->cpu_buffer = NULL;
    }
    
    // TODO: Free GPU buffer
    buffer->gpu_buffer_handle = NULL;
    buffer->capacity = 0;
    buffer->count = 0;
}

static bool resize_gpu_buffer(gpu_instance_buffer_t* buffer, uint32_t new_capacity) {
    if (new_capacity <= buffer->capacity) {
        return true;
    }
    
    gpu_instance_t* new_buffer = aligned_alloc(GPU_INSTANCE_BUFFER_ALIGNMENT,
                                                new_capacity * sizeof(gpu_instance_t));
    if (!new_buffer) {
        return false;
    }
    
    if (buffer->cpu_buffer && buffer->count > 0) {
        memcpy(new_buffer, buffer->cpu_buffer, buffer->count * sizeof(gpu_instance_t));
    }
    
    free(buffer->cpu_buffer);
    buffer->cpu_buffer = new_buffer;
    buffer->capacity = new_capacity;
    buffer->dirty = true;
    
    return true;
}

static int upload_to_gpu(gpu_instance_buffer_t* buffer) {
    if (!buffer->dirty || buffer->count == 0) {
        return 0;
    }
    
    // TODO: Upload to GPU via graphics API
    // For now, just mark as clean
    buffer->dirty = false;
    
    return 0;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool geometry_gpu_instance_data_validate(const geometry_gpu_instance_data_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->buffer.cpu_buffer) return false;
    return true;
}

static void geometry_gpu_instance_data_cleanup_internal(geometry_gpu_instance_data_internal_t* item) {
    if (!item) return;
    
    free_gpu_buffer(&item->buffer);
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int geometry_gpu_instance_data_init(void) {
    if (g_gpu_instance_data_ctx.initialized) {
        return 0;
    }
    
    g_gpu_instance_data_ctx.capacity = GPU_INSTANCE_DATA_DEFAULT_CAPACITY;
    g_gpu_instance_data_ctx.items = calloc(g_gpu_instance_data_ctx.capacity,
                                           sizeof(geometry_gpu_instance_data_internal_t));
    if (!g_gpu_instance_data_ctx.items) {
        return -1;
    }
    
    g_gpu_instance_data_ctx.count = 0;
    g_gpu_instance_data_ctx.total_gpu_memory = 0;
    g_gpu_instance_data_ctx.initialized = true;
    
    return 0;
}

void geometry_gpu_instance_data_shutdown(void) {
    if (!g_gpu_instance_data_ctx.initialized) {
        return;
    }
    
    for (uint32_t i = 0; i < g_gpu_instance_data_ctx.count; i++) {
        geometry_gpu_instance_data_cleanup_internal(&g_gpu_instance_data_ctx.items[i]);
    }
    
    free(g_gpu_instance_data_ctx.items);
    g_gpu_instance_data_ctx.items = NULL;
    g_gpu_instance_data_ctx.count = 0;
    g_gpu_instance_data_ctx.capacity = 0;
    g_gpu_instance_data_ctx.total_gpu_memory = 0;
    g_gpu_instance_data_ctx.initialized = false;
}

int geometry_gpu_instance_data_create(geometry_gpu_instance_data_handle_t* out_handle,
                                       const geometry_gpu_instance_data_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }
    
    if (!g_gpu_instance_data_ctx.initialized) {
        return -2;
    }
    
    if (g_gpu_instance_data_ctx.count >= g_gpu_instance_data_ctx.capacity) {
        return -3;
    }
    
    uint32_t index = g_gpu_instance_data_ctx.count++;
    geometry_gpu_instance_data_internal_t* item = &g_gpu_instance_data_ctx.items[index];
    
    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    uint32_t initial_capacity = desc->initial_capacity > 0 ? desc->initial_capacity : 256;
    if (!allocate_gpu_buffer(&item->buffer, initial_capacity)) {
        return -4;
    }
    
    g_gpu_instance_data_ctx.total_gpu_memory += initial_capacity * sizeof(gpu_instance_t);
    
    out_handle->id = index;
    return 0;
}

void geometry_gpu_instance_data_destroy(geometry_gpu_instance_data_handle_t handle) {
    if (handle.id >= g_gpu_instance_data_ctx.count) {
        return;
    }
    
    geometry_gpu_instance_data_internal_t* item = &g_gpu_instance_data_ctx.items[handle.id];
    g_gpu_instance_data_ctx.total_gpu_memory -= item->buffer.capacity * sizeof(gpu_instance_t);
    
    geometry_gpu_instance_data_cleanup_internal(item);
}

int geometry_gpu_instance_data_update(geometry_gpu_instance_data_handle_t handle,
                                       const void* data, size_t size) {
    if (handle.id >= g_gpu_instance_data_ctx.count) {
        return -1;
    }
    
    geometry_gpu_instance_data_internal_t* item = &g_gpu_instance_data_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    uint32_t instance_count = size / sizeof(gpu_instance_t);
    if (instance_count == 0) {
        return -3;
    }
    
    // Resize buffer if needed
    if (instance_count > item->buffer.capacity) {
        uint32_t new_capacity = instance_count * 2;
        if (!resize_gpu_buffer(&item->buffer, new_capacity)) {
            return -4;
        }
        
        g_gpu_instance_data_ctx.total_gpu_memory += 
            (new_capacity - item->buffer.capacity) * sizeof(gpu_instance_t);
    }
    
    // Copy instance data
    memcpy(item->buffer.cpu_buffer, data, size);
    item->buffer.count = instance_count;
    item->buffer.dirty = true;
    item->dirty = true;
    
    return 0;
}

bool geometry_gpu_instance_data_is_valid(geometry_gpu_instance_data_handle_t handle) {
    if (handle.id >= g_gpu_instance_data_ctx.count) {
        return false;
    }
    return g_gpu_instance_data_ctx.items[handle.id].initialized;
}

int geometry_gpu_instance_data_get_info(geometry_gpu_instance_data_handle_t handle,
                                         geometry_gpu_instance_data_info_t* out_info) {
    if (!out_info) {
        return -1;
    }
    
    if (handle.id >= g_gpu_instance_data_ctx.count) {
        return -2;
    }
    
    const geometry_gpu_instance_data_internal_t* item = &g_gpu_instance_data_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->instance_count = item->buffer.count;
    out_info->capacity = item->buffer.capacity;
    
    return 0;
}

void geometry_gpu_instance_data_mark_dirty(geometry_gpu_instance_data_handle_t handle) {
    if (handle.id < g_gpu_instance_data_ctx.count) {
        g_gpu_instance_data_ctx.items[handle.id].dirty = true;
        g_gpu_instance_data_ctx.items[handle.id].buffer.dirty = true;
    }
}

int geometry_gpu_instance_data_process_pending(void) {
    int processed = 0;
    
    for (uint32_t i = 0; i < g_gpu_instance_data_ctx.count; i++) {
        geometry_gpu_instance_data_internal_t* item = &g_gpu_instance_data_ctx.items[i];
        if (item->initialized && item->buffer.dirty) {
            if (upload_to_gpu(&item->buffer) == 0) {
                item->dirty = false;
                processed++;
            }
        }
    }
    
    return processed;
}

uint32_t geometry_gpu_instance_data_get_count(void) {
    return g_gpu_instance_data_ctx.count;
}

size_t geometry_gpu_instance_data_get_memory_usage(void) {
    return g_gpu_instance_data_ctx.total_gpu_memory;
}

void geometry_gpu_instance_data_debug_print(void) {
    printf("[GPU Instance Data] Total buffers: %u\n", g_gpu_instance_data_ctx.count);
    printf("[GPU Instance Data] Total GPU memory: %zu bytes (%.2f MB)\n",
           g_gpu_instance_data_ctx.total_gpu_memory,
           g_gpu_instance_data_ctx.total_gpu_memory / (1024.0f * 1024.0f));
    
    for (uint32_t i = 0; i < g_gpu_instance_data_ctx.count; i++) {
        const geometry_gpu_instance_data_internal_t* item = &g_gpu_instance_data_ctx.items[i];
        if (item->initialized) {
            printf("  Buffer %u: %u/%u instances (%.1f%% full)\n",
                   i, item->buffer.count, item->buffer.capacity,
                   100.0f * item->buffer.count / item->buffer.capacity);
        }
    }
}

/* End of gpu_instance_data.c */
