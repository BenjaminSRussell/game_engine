/*
 * visibility_buffer.c
 * Visibility buffer pass
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "rendering/visibility/visibility_buffer.h"
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

typedef struct rendering_visibility_buffer_internal {
    uint32_t id;
    uint32_t flags;
    ResourceHandle buffer_handle;
    ResourceHandle pipeline_handle;
    bool initialized;
    bool dirty;
    uint32_t width;
    uint32_t height;
} rendering_visibility_buffer_internal_t;

typedef struct rendering_visibility_buffer_context {
    rendering_visibility_buffer_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} rendering_visibility_buffer_context_t;

static rendering_visibility_buffer_context_t g_visibility_buffer_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void rendering_visibility_buffer_cleanup_internal(rendering_visibility_buffer_internal_t* item) {
    if (!item) return;
    item->buffer_handle = INVALID_HANDLE;
    item->pipeline_handle = INVALID_HANDLE;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_visibility_buffer_init(void) {
    if (g_visibility_buffer_ctx.initialized) {
        return 0; // Already initialized
    }

    g_visibility_buffer_ctx.capacity = 256;
    g_visibility_buffer_ctx.items = calloc(g_visibility_buffer_ctx.capacity, sizeof(rendering_visibility_buffer_internal_t));
    if (!g_visibility_buffer_ctx.items) {
        return -1;
    }

    g_visibility_buffer_ctx.count = 0;
    g_visibility_buffer_ctx.initialized = true;

    return 0;
}

void rendering_visibility_buffer_shutdown(void) {
    if (!g_visibility_buffer_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_visibility_buffer_ctx.count; i++) {
        rendering_visibility_buffer_cleanup_internal(&g_visibility_buffer_ctx.items[i]);
    }

    free(g_visibility_buffer_ctx.items);
    g_visibility_buffer_ctx.items = NULL;
    g_visibility_buffer_ctx.count = 0;
    g_visibility_buffer_ctx.capacity = 0;
    g_visibility_buffer_ctx.initialized = false;
}

int rendering_visibility_buffer_create(rendering_visibility_buffer_handle_t* out_handle, const rendering_visibility_buffer_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_visibility_buffer_ctx.initialized) {
        return -2;
    }

    if (g_visibility_buffer_ctx.count >= g_visibility_buffer_ctx.capacity) {
        uint32_t new_capacity = g_visibility_buffer_ctx.capacity * 2;
        rendering_visibility_buffer_internal_t* new_items = realloc(g_visibility_buffer_ctx.items, new_capacity * sizeof(rendering_visibility_buffer_internal_t));
        if (!new_items) return -3;
        g_visibility_buffer_ctx.items = new_items;
        g_visibility_buffer_ctx.capacity = new_capacity;
    }

    uint32_t index = g_visibility_buffer_ctx.count++;
    rendering_visibility_buffer_internal_t* item = &g_visibility_buffer_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->width = desc->width;
    item->height = desc->height;
    item->buffer_handle = INVALID_HANDLE;
    item->pipeline_handle = INVALID_HANDLE;
    item->initialized = true;
    item->dirty = true;

    out_handle->id = index;
    return 0;
}

void rendering_visibility_buffer_destroy(rendering_visibility_buffer_handle_t handle) {
    if (handle.id >= g_visibility_buffer_ctx.count) {
        return;
    }

    rendering_visibility_buffer_cleanup_internal(&g_visibility_buffer_ctx.items[handle.id]);
}

int rendering_visibility_buffer_update(rendering_visibility_buffer_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_visibility_buffer_ctx.count) {
        return -1;
    }

    rendering_visibility_buffer_internal_t* item = &g_visibility_buffer_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool rendering_visibility_buffer_is_valid(rendering_visibility_buffer_handle_t handle) {
    if (handle.id >= g_visibility_buffer_ctx.count) {
        return false;
    }
    return g_visibility_buffer_ctx.items[handle.id].initialized;
}

int rendering_visibility_buffer_get_info(rendering_visibility_buffer_handle_t handle, rendering_visibility_buffer_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_visibility_buffer_ctx.count) {
        return -2;
    }

    const rendering_visibility_buffer_internal_t* item = &g_visibility_buffer_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void rendering_visibility_buffer_mark_dirty(rendering_visibility_buffer_handle_t handle) {
    if (handle.id < g_visibility_buffer_ctx.count) {
        g_visibility_buffer_ctx.items[handle.id].dirty = true;
    }
}

int rendering_visibility_buffer_process_pending(void) {
    if (!g_visibility_buffer_ctx.initialized) return 0;

    int processed = 0;
    for (uint32_t i = 0; i < g_visibility_buffer_ctx.count; i++) {
        rendering_visibility_buffer_internal_t* item = &g_visibility_buffer_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Update visibility buffer resources
            // Dispatch/Draw call for visibility rendering
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t rendering_visibility_buffer_get_count(void) {
    return g_visibility_buffer_ctx.count;
}

size_t rendering_visibility_buffer_get_memory_usage(void) {
    size_t total = sizeof(rendering_visibility_buffer_context_t);
    total += g_visibility_buffer_ctx.capacity * sizeof(rendering_visibility_buffer_internal_t);
    return total;
}

void rendering_visibility_buffer_debug_print(void) {
    if (!g_visibility_buffer_ctx.initialized) return;
    
    printf("Visibility Buffer Context:\n");
    printf("  Count: %u/%u\n", g_visibility_buffer_ctx.count, g_visibility_buffer_ctx.capacity);
}

/* End of visibility_buffer.c */
