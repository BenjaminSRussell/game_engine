/*
 * visibility_buffer_nanite.c
 * Visibility buffer integration
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#include "visibility_buffer_nanite.h"
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

typedef struct nanite_visibility_buffer_nanite_internal {
    uint32_t id;
    uint32_t flags;
    ResourceHandle vis_buffer;
    uint32_t width;
    uint32_t height;
    bool initialized;
    bool dirty;
} nanite_visibility_buffer_nanite_internal_t;

typedef struct nanite_visibility_buffer_nanite_context {
    nanite_visibility_buffer_nanite_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} nanite_visibility_buffer_nanite_context_t;

static nanite_visibility_buffer_nanite_context_t g_visibility_buffer_nanite_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void nanite_visibility_buffer_nanite_cleanup_internal(nanite_visibility_buffer_nanite_internal_t* item) {
    if (!item) return;
    item->vis_buffer = INVALID_HANDLE;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int nanite_visibility_buffer_nanite_init(void) {
    if (g_visibility_buffer_nanite_ctx.initialized) {
        return 0;
    }

    g_visibility_buffer_nanite_ctx.capacity = 256;
    g_visibility_buffer_nanite_ctx.items = calloc(g_visibility_buffer_nanite_ctx.capacity, sizeof(nanite_visibility_buffer_nanite_internal_t));
    if (!g_visibility_buffer_nanite_ctx.items) {
        return -1;
    }

    g_visibility_buffer_nanite_ctx.count = 0;
    g_visibility_buffer_nanite_ctx.initialized = true;

    return 0;
}

void nanite_visibility_buffer_nanite_shutdown(void) {
    if (!g_visibility_buffer_nanite_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_visibility_buffer_nanite_ctx.count; i++) {
        nanite_visibility_buffer_nanite_cleanup_internal(&g_visibility_buffer_nanite_ctx.items[i]);
    }

    free(g_visibility_buffer_nanite_ctx.items);
    g_visibility_buffer_nanite_ctx.items = NULL;
    g_visibility_buffer_nanite_ctx.count = 0;
    g_visibility_buffer_nanite_ctx.capacity = 0;
    g_visibility_buffer_nanite_ctx.initialized = false;
}

int nanite_visibility_buffer_nanite_create(nanite_visibility_buffer_nanite_handle_t* out_handle, const nanite_visibility_buffer_nanite_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_visibility_buffer_nanite_ctx.initialized) {
        return -2;
    }

    if (g_visibility_buffer_nanite_ctx.count >= g_visibility_buffer_nanite_ctx.capacity) {
        uint32_t new_capacity = g_visibility_buffer_nanite_ctx.capacity * 2;
        nanite_visibility_buffer_nanite_internal_t* new_items = realloc(g_visibility_buffer_nanite_ctx.items, new_capacity * sizeof(nanite_visibility_buffer_nanite_internal_t));
        if (!new_items) return -3;
        g_visibility_buffer_nanite_ctx.items = new_items;
        g_visibility_buffer_nanite_ctx.capacity = new_capacity;
    }

    uint32_t index = g_visibility_buffer_nanite_ctx.count++;
    nanite_visibility_buffer_nanite_internal_t* item = &g_visibility_buffer_nanite_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->vis_buffer = INVALID_HANDLE;
    item->width = 1920;
    item->height = 1080;

    out_handle->id = index;
    return 0;
}

void nanite_visibility_buffer_nanite_destroy(nanite_visibility_buffer_nanite_handle_t handle) {
    if (handle.id >= g_visibility_buffer_nanite_ctx.count) {
        return;
    }

    nanite_visibility_buffer_nanite_cleanup_internal(&g_visibility_buffer_nanite_ctx.items[handle.id]);
}

int nanite_visibility_buffer_nanite_update(nanite_visibility_buffer_nanite_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_visibility_buffer_nanite_ctx.count) {
        return -1;
    }

    nanite_visibility_buffer_nanite_internal_t* item = &g_visibility_buffer_nanite_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool nanite_visibility_buffer_nanite_is_valid(nanite_visibility_buffer_nanite_handle_t handle) {
    if (handle.id >= g_visibility_buffer_nanite_ctx.count) {
        return false;
    }
    return g_visibility_buffer_nanite_ctx.items[handle.id].initialized;
}

int nanite_visibility_buffer_nanite_get_info(nanite_visibility_buffer_nanite_handle_t handle, nanite_visibility_buffer_nanite_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_visibility_buffer_nanite_ctx.count) {
        return -2;
    }

    const nanite_visibility_buffer_nanite_internal_t* item = &g_visibility_buffer_nanite_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void nanite_visibility_buffer_nanite_mark_dirty(nanite_visibility_buffer_nanite_handle_t handle) {
    if (handle.id < g_visibility_buffer_nanite_ctx.count) {
        g_visibility_buffer_nanite_ctx.items[handle.id].dirty = true;
    }
}

int nanite_visibility_buffer_nanite_process_pending(void) {
    if (!g_visibility_buffer_nanite_ctx.initialized) return 0;

    int processed = 0;
    for (uint32_t i = 0; i < g_visibility_buffer_nanite_ctx.count; i++) {
        nanite_visibility_buffer_nanite_internal_t* item = &g_visibility_buffer_nanite_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Encoding Scheme:
            // bits 0-6: Triangle ID within cluster (max 128 triangles)
            // bits 7-31: Cluster index (max 33 million clusters)
            // PixelValue = (cluster_index << 7) | triangle_index;
            
            // This logic is implemented in the cluster rasterization shader.
            // Setup uniform/push constant data for ID offset if needed.
            
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t nanite_visibility_buffer_nanite_get_count(void) {
    return g_visibility_buffer_nanite_ctx.count;
}

size_t nanite_visibility_buffer_nanite_get_memory_usage(void) {
    size_t total = sizeof(nanite_visibility_buffer_nanite_context_t);
    total += g_visibility_buffer_nanite_ctx.capacity * sizeof(nanite_visibility_buffer_nanite_internal_t);
    return total;
}

void nanite_visibility_buffer_nanite_debug_print(void) {
    if (!g_visibility_buffer_nanite_ctx.initialized) return;
    
    printf("Nanite Visibility Buffer Integration Status:\n");
    printf("  Count: %u / %u\n", g_visibility_buffer_nanite_ctx.count, g_visibility_buffer_nanite_ctx.capacity);
}

/* End of visibility_buffer_nanite.c */
