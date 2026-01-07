/*
 * instance_data_gpu.c
 * GPU instance buffer
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "instance_data_gpu.h"
#include "../../3d_rendering.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_instance_data_gpu_internal {
    uint32_t id;
    uint32_t flags;
    ResourceHandle transform_buffer;
    ResourceHandle culling_buffer;
    bool initialized;
    bool dirty;
} rendering_instance_data_gpu_internal_t;

typedef struct rendering_instance_data_gpu_context {
    rendering_instance_data_gpu_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} rendering_instance_data_gpu_context_t;

static rendering_instance_data_gpu_context_t g_instance_data_gpu_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void rendering_instance_data_gpu_cleanup_internal(rendering_instance_data_gpu_internal_t* item) {
    if (!item) return;
    item->transform_buffer = INVALID_HANDLE;
    item->culling_buffer = INVALID_HANDLE;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_instance_data_gpu_init(void) {
    if (g_instance_data_gpu_ctx.initialized) {
        return 0;
    }

    g_instance_data_gpu_ctx.capacity = 256;
    g_instance_data_gpu_ctx.items = calloc(g_instance_data_gpu_ctx.capacity, sizeof(rendering_instance_data_gpu_internal_t));
    if (!g_instance_data_gpu_ctx.items) {
        return -1;
    }

    g_instance_data_gpu_ctx.count = 0;
    g_instance_data_gpu_ctx.initialized = true;

    return 0;
}

void rendering_instance_data_gpu_shutdown(void) {
    if (!g_instance_data_gpu_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_instance_data_gpu_ctx.count; i++) {
        rendering_instance_data_gpu_cleanup_internal(&g_instance_data_gpu_ctx.items[i]);
    }

    free(g_instance_data_gpu_ctx.items);
    g_instance_data_gpu_ctx.items = NULL;
    g_instance_data_gpu_ctx.count = 0;
    g_instance_data_gpu_ctx.capacity = 0;
    g_instance_data_gpu_ctx.initialized = false;
}

int rendering_instance_data_gpu_create(rendering_instance_data_gpu_handle_t* out_handle, const rendering_instance_data_gpu_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_instance_data_gpu_ctx.initialized) {
        return -2;
    }

    if (g_instance_data_gpu_ctx.count >= g_instance_data_gpu_ctx.capacity) {
        uint32_t new_capacity = g_instance_data_gpu_ctx.capacity * 2;
        rendering_instance_data_gpu_internal_t* new_items = realloc(g_instance_data_gpu_ctx.items, new_capacity * sizeof(rendering_instance_data_gpu_internal_t));
        if (!new_items) return -3;
        g_instance_data_gpu_ctx.items = new_items;
        g_instance_data_gpu_ctx.capacity = new_capacity;
    }

    uint32_t index = g_instance_data_gpu_ctx.count++;
    rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->transform_buffer = INVALID_HANDLE;
    item->culling_buffer = INVALID_HANDLE;

    out_handle->id = index;
    return 0;
}

void rendering_instance_data_gpu_destroy(rendering_instance_data_gpu_handle_t handle) {
    if (handle.id >= g_instance_data_gpu_ctx.count) {
        return;
    }

    rendering_instance_data_gpu_cleanup_internal(&g_instance_data_gpu_ctx.items[handle.id]);
}

int rendering_instance_data_gpu_update(rendering_instance_data_gpu_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_instance_data_gpu_ctx.count) {
        return -1;
    }

    rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool rendering_instance_data_gpu_is_valid(rendering_instance_data_gpu_handle_t handle) {
    if (handle.id >= g_instance_data_gpu_ctx.count) {
        return false;
    }
    return g_instance_data_gpu_ctx.items[handle.id].initialized;
}

int rendering_instance_data_gpu_get_info(rendering_instance_data_gpu_handle_t handle, rendering_instance_data_gpu_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_instance_data_gpu_ctx.count) {
        return -2;
    }

    const rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void rendering_instance_data_gpu_mark_dirty(rendering_instance_data_gpu_handle_t handle) {
    if (handle.id < g_instance_data_gpu_ctx.count) {
        g_instance_data_gpu_ctx.items[handle.id].dirty = true;
    }
}

int rendering_instance_data_gpu_process_pending(void) {
    if (!g_instance_data_gpu_ctx.initialized) return 0;

    int processed = 0;
    for (uint32_t i = 0; i < g_instance_data_gpu_ctx.count; i++) {
        rendering_instance_data_gpu_internal_t* item = &g_instance_data_gpu_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Update instance transform and culling data on GPU
            // Stream instance LOD selection data
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t rendering_instance_data_gpu_get_count(void) {
    return g_instance_data_gpu_ctx.count;
}

size_t rendering_instance_data_gpu_get_memory_usage(void) {
    size_t total = sizeof(rendering_instance_data_gpu_context_t);
    total += g_instance_data_gpu_ctx.capacity * sizeof(rendering_instance_data_gpu_internal_t);
    return total;
}

void rendering_instance_data_gpu_debug_print(void) {
    if (!g_instance_data_gpu_ctx.initialized) return;
    
    printf("Instance Data GPU Status:\n");
    printf("  Count: %u / %u\n", g_instance_data_gpu_ctx.count, g_instance_data_gpu_ctx.capacity);
}

/* End of instance_data_gpu.c */
