/*
 * gpu_scene.c
 * GPU scene representation
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "rendering/gpu_driven/gpu_scene.h"
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

typedef struct rendering_gpu_scene_internal {
    uint32_t id;
    uint32_t flags;
    ResourceHandle cluster_buffer;
    ResourceHandle instance_buffer;
    ResourceHandle material_buffer;
    bool initialized;
    bool dirty;
} rendering_gpu_scene_internal_t;

typedef struct rendering_gpu_scene_context {
    rendering_gpu_scene_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} rendering_gpu_scene_context_t;

static rendering_gpu_scene_context_t g_gpu_scene_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void rendering_gpu_scene_cleanup_internal(rendering_gpu_scene_internal_t* item) {
    if (!item) return;
    item->cluster_buffer = INVALID_HANDLE;
    item->instance_buffer = INVALID_HANDLE;
    item->material_buffer = INVALID_HANDLE;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_gpu_scene_init(void) {
    if (g_gpu_scene_ctx.initialized) {
        return 0;
    }

    g_gpu_scene_ctx.capacity = 256;
    g_gpu_scene_ctx.items = calloc(g_gpu_scene_ctx.capacity, sizeof(rendering_gpu_scene_internal_t));
    if (!g_gpu_scene_ctx.items) {
        return -1;
    }

    g_gpu_scene_ctx.count = 0;
    g_gpu_scene_ctx.initialized = true;

    return 0;
}

void rendering_gpu_scene_shutdown(void) {
    if (!g_gpu_scene_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_gpu_scene_ctx.count; i++) {
        rendering_gpu_scene_cleanup_internal(&g_gpu_scene_ctx.items[i]);
    }

    free(g_gpu_scene_ctx.items);
    g_gpu_scene_ctx.items = NULL;
    g_gpu_scene_ctx.count = 0;
    g_gpu_scene_ctx.capacity = 0;
    g_gpu_scene_ctx.initialized = false;
}

int rendering_gpu_scene_create(rendering_gpu_scene_handle_t* out_handle, const rendering_gpu_scene_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_gpu_scene_ctx.initialized) {
        return -2;
    }

    if (g_gpu_scene_ctx.count >= g_gpu_scene_ctx.capacity) {
        uint32_t new_capacity = g_gpu_scene_ctx.capacity * 2;
        rendering_gpu_scene_internal_t* new_items = realloc(g_gpu_scene_ctx.items, new_capacity * sizeof(rendering_gpu_scene_internal_t));
        if (!new_items) return -3;
        g_gpu_scene_ctx.items = new_items;
        g_gpu_scene_ctx.capacity = new_capacity;
    }

    uint32_t index = g_gpu_scene_ctx.count++;
    rendering_gpu_scene_internal_t* item = &g_gpu_scene_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->cluster_buffer = INVALID_HANDLE;
    item->instance_buffer = INVALID_HANDLE;
    item->material_buffer = INVALID_HANDLE;

    out_handle->id = index;
    return 0;
}

void rendering_gpu_scene_destroy(rendering_gpu_scene_handle_t handle) {
    if (handle.id >= g_gpu_scene_ctx.count) {
        return;
    }

    rendering_gpu_scene_cleanup_internal(&g_gpu_scene_ctx.items[handle.id]);
}

int rendering_gpu_scene_update(rendering_gpu_scene_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_gpu_scene_ctx.count) {
        return -1;
    }

    rendering_gpu_scene_internal_t* item = &g_gpu_scene_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool rendering_gpu_scene_is_valid(rendering_gpu_scene_handle_t handle) {
    if (handle.id >= g_gpu_scene_ctx.count) {
        return false;
    }
    return g_gpu_scene_ctx.items[handle.id].initialized;
}

int rendering_gpu_scene_get_info(rendering_gpu_scene_handle_t handle, rendering_gpu_scene_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_gpu_scene_ctx.count) {
        return -2;
    }

    const rendering_gpu_scene_internal_t* item = &g_gpu_scene_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void rendering_gpu_scene_mark_dirty(rendering_gpu_scene_handle_t handle) {
    if (handle.id < g_gpu_scene_ctx.count) {
        g_gpu_scene_ctx.items[handle.id].dirty = true;
    }
}

int rendering_gpu_scene_process_pending(void) {
    if (!g_gpu_scene_ctx.initialized) return 0;

    int processed = 0;
    for (uint32_t i = 0; i < g_gpu_scene_ctx.count; i++) {
        rendering_gpu_scene_internal_t* item = &g_gpu_scene_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Update GPU scene buffers from CPU side data
            // Manage cluster streaming to GPU
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t rendering_gpu_scene_get_count(void) {
    return g_gpu_scene_ctx.count;
}

size_t rendering_gpu_scene_get_memory_usage(void) {
    size_t total = sizeof(rendering_gpu_scene_context_t);
    total += g_gpu_scene_ctx.capacity * sizeof(rendering_gpu_scene_internal_t);
    return total;
}

void rendering_gpu_scene_debug_print(void) {
    if (!g_gpu_scene_ctx.initialized) return;
    
    printf("GPU Scene Status:\n");
    printf("  Count: %u / %u\n", g_gpu_scene_ctx.count, g_gpu_scene_ctx.capacity);
}

/* End of gpu_scene.c */
