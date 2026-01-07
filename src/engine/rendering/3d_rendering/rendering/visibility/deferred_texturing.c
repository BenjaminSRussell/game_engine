/*
 * deferred_texturing.c
 * Deferred texture sampling
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "deferred_texturing.h"
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

typedef struct rendering_deferred_texturing_internal {
    uint32_t id;
    uint32_t flags;
    ResourceHandle pipeline_handle;
    ResourceHandle fullscreen_quad;
    bool initialized;
    bool dirty;
    uint32_t width; 
    uint32_t height;
} rendering_deferred_texturing_internal_t;

typedef struct rendering_deferred_texturing_context {
    rendering_deferred_texturing_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} rendering_deferred_texturing_context_t;

static rendering_deferred_texturing_context_t g_deferred_texturing_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void rendering_deferred_texturing_cleanup_internal(rendering_deferred_texturing_internal_t* item) {
    if (!item) return;
    item->pipeline_handle = INVALID_HANDLE;
    item->fullscreen_quad = INVALID_HANDLE;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_deferred_texturing_init(void) {
    if (g_deferred_texturing_ctx.initialized) {
        return 0; // Already initialized
    }

    g_deferred_texturing_ctx.capacity = 256;
    g_deferred_texturing_ctx.items = calloc(g_deferred_texturing_ctx.capacity, sizeof(rendering_deferred_texturing_internal_t));
    if (!g_deferred_texturing_ctx.items) {
        return -1;
    }

    g_deferred_texturing_ctx.count = 0;
    g_deferred_texturing_ctx.initialized = true;

    return 0;
}

void rendering_deferred_texturing_shutdown(void) {
    if (!g_deferred_texturing_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_deferred_texturing_ctx.count; i++) {
        rendering_deferred_texturing_cleanup_internal(&g_deferred_texturing_ctx.items[i]);
    }

    free(g_deferred_texturing_ctx.items);
    g_deferred_texturing_ctx.items = NULL;
    g_deferred_texturing_ctx.count = 0;
    g_deferred_texturing_ctx.capacity = 0;
    g_deferred_texturing_ctx.initialized = false;
}

int rendering_deferred_texturing_create(rendering_deferred_texturing_handle_t* out_handle, const rendering_deferred_texturing_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_deferred_texturing_ctx.initialized) {
        return -2;
    }

    if (g_deferred_texturing_ctx.count >= g_deferred_texturing_ctx.capacity) {
        uint32_t new_capacity = g_deferred_texturing_ctx.capacity * 2;
        rendering_deferred_texturing_internal_t* new_items = realloc(g_deferred_texturing_ctx.items, new_capacity * sizeof(rendering_deferred_texturing_internal_t));
        if (!new_items) return -3;
        g_deferred_texturing_ctx.items = new_items;
        g_deferred_texturing_ctx.capacity = new_capacity;
    }

    uint32_t index = g_deferred_texturing_ctx.count++;
    rendering_deferred_texturing_internal_t* item = &g_deferred_texturing_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->pipeline_handle = INVALID_HANDLE;
    item->fullscreen_quad = INVALID_HANDLE;
    item->initialized = true;
    item->dirty = true;
    item->width = 1920; 
    item->height = 1080;

    out_handle->id = index;
    return 0;
}

void rendering_deferred_texturing_destroy(rendering_deferred_texturing_handle_t handle) {
    if (handle.id >= g_deferred_texturing_ctx.count) {
        return;
    }

    rendering_deferred_texturing_cleanup_internal(&g_deferred_texturing_ctx.items[handle.id]);
}

int rendering_deferred_texturing_update(rendering_deferred_texturing_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_deferred_texturing_ctx.count) {
        return -1;
    }

    rendering_deferred_texturing_internal_t* item = &g_deferred_texturing_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool rendering_deferred_texturing_is_valid(rendering_deferred_texturing_handle_t handle) {
    if (handle.id >= g_deferred_texturing_ctx.count) {
        return false;
    }
    return g_deferred_texturing_ctx.items[handle.id].initialized;
}

int rendering_deferred_texturing_get_info(rendering_deferred_texturing_handle_t handle, rendering_deferred_texturing_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_deferred_texturing_ctx.count) {
        return -2;
    }

    const rendering_deferred_texturing_internal_t* item = &g_deferred_texturing_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void rendering_deferred_texturing_mark_dirty(rendering_deferred_texturing_handle_t handle) {
    if (handle.id < g_deferred_texturing_ctx.count) {
        g_deferred_texturing_ctx.items[handle.id].dirty = true;
    }
}

int rendering_deferred_texturing_process_pending(void) {
    if (!g_deferred_texturing_ctx.initialized) return 0;

    int processed = 0;
    for (uint32_t i = 0; i < g_deferred_texturing_ctx.count; i++) {
        rendering_deferred_texturing_internal_t* item = &g_deferred_texturing_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Bind visibility buffer, index/vertex buffers, and material textures
            // Execute fullscreen pass to shade pixels based on visibility ID
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t rendering_deferred_texturing_get_count(void) {
    return g_deferred_texturing_ctx.count;
}

size_t rendering_deferred_texturing_get_memory_usage(void) {
    size_t total = sizeof(rendering_deferred_texturing_context_t);
    total += g_deferred_texturing_ctx.capacity * sizeof(rendering_deferred_texturing_internal_t);
    return total;
}

void rendering_deferred_texturing_debug_print(void) {
    if (!g_deferred_texturing_ctx.initialized) return;
    
    printf("Deferred Texturing Context:\n");
    printf("  Count: %u/%u\n", g_deferred_texturing_ctx.count, g_deferred_texturing_ctx.capacity);
}

/* End of deferred_texturing.c */
