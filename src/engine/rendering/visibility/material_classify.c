/*
 * material_classify.c
 * Material classification
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "rendering/visibility/material_classify.h"
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

typedef struct rendering_material_classify_internal {
    uint32_t id;
    uint32_t flags;
    ResourceHandle material_buffer;
    ResourceHandle compute_pipeline;
    bool initialized;
    bool dirty;
    uint32_t group_size_x;
    uint32_t group_size_y;
} rendering_material_classify_internal_t;

typedef struct rendering_material_classify_context {
    rendering_material_classify_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} rendering_material_classify_context_t;

static rendering_material_classify_context_t g_material_classify_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void rendering_material_classify_cleanup_internal(rendering_material_classify_internal_t* item) {
    if (!item) return;
    item->material_buffer = INVALID_HANDLE;
    item->compute_pipeline = INVALID_HANDLE;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_material_classify_init(void) {
    if (g_material_classify_ctx.initialized) {
        return 0; // Already initialized
    }

    g_material_classify_ctx.capacity = 256;
    g_material_classify_ctx.items = calloc(g_material_classify_ctx.capacity, sizeof(rendering_material_classify_internal_t));
    if (!g_material_classify_ctx.items) {
        return -1;
    }

    g_material_classify_ctx.count = 0;
    g_material_classify_ctx.initialized = true;

    return 0;
}

void rendering_material_classify_shutdown(void) {
    if (!g_material_classify_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_material_classify_ctx.count; i++) {
        rendering_material_classify_cleanup_internal(&g_material_classify_ctx.items[i]);
    }

    free(g_material_classify_ctx.items);
    g_material_classify_ctx.items = NULL;
    g_material_classify_ctx.count = 0;
    g_material_classify_ctx.capacity = 0;
    g_material_classify_ctx.initialized = false;
}

int rendering_material_classify_create(rendering_material_classify_handle_t* out_handle, const rendering_material_classify_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_material_classify_ctx.initialized) {
        return -2;
    }

    if (g_material_classify_ctx.count >= g_material_classify_ctx.capacity) {
        uint32_t new_capacity = g_material_classify_ctx.capacity * 2;
        rendering_material_classify_internal_t* new_items = realloc(g_material_classify_ctx.items, new_capacity * sizeof(rendering_material_classify_internal_t));
        if (!new_items) return -3;
        g_material_classify_ctx.items = new_items;
        g_material_classify_ctx.capacity = new_capacity;
    }

    uint32_t index = g_material_classify_ctx.count++;
    rendering_material_classify_internal_t* item = &g_material_classify_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->material_buffer = INVALID_HANDLE;
    item->compute_pipeline = INVALID_HANDLE;
    item->initialized = true;
    item->dirty = true;
    item->group_size_x = 8;
    item->group_size_y = 8;

    out_handle->id = index;
    return 0;
}

void rendering_material_classify_destroy(rendering_material_classify_handle_t handle) {
    if (handle.id >= g_material_classify_ctx.count) {
        return;
    }

    rendering_material_classify_cleanup_internal(&g_material_classify_ctx.items[handle.id]);
}

int rendering_material_classify_update(rendering_material_classify_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_material_classify_ctx.count) {
        return -1;
    }

    rendering_material_classify_internal_t* item = &g_material_classify_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool rendering_material_classify_is_valid(rendering_material_classify_handle_t handle) {
    if (handle.id >= g_material_classify_ctx.count) {
        return false;
    }
    return g_material_classify_ctx.items[handle.id].initialized;
}

int rendering_material_classify_get_info(rendering_material_classify_handle_t handle, rendering_material_classify_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_material_classify_ctx.count) {
        return -2;
    }

    const rendering_material_classify_internal_t* item = &g_material_classify_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void rendering_material_classify_mark_dirty(rendering_material_classify_handle_t handle) {
    if (handle.id < g_material_classify_ctx.count) {
        g_material_classify_ctx.items[handle.id].dirty = true;
    }
}

int rendering_material_classify_process_pending(void) {
    if (!g_material_classify_ctx.initialized) return 0;

    int processed = 0;
    for (uint32_t i = 0; i < g_material_classify_ctx.count; i++) {
        rendering_material_classify_internal_t* item = &g_material_classify_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Dispatch compute shader for material classification and counting
            // Reads visibility buffer, outputs material counts/offsets
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t rendering_material_classify_get_count(void) {
    return g_material_classify_ctx.count;
}

size_t rendering_material_classify_get_memory_usage(void) {
    size_t total = sizeof(rendering_material_classify_context_t);
    total += g_material_classify_ctx.capacity * sizeof(rendering_material_classify_internal_t);
    return total;
}

void rendering_material_classify_debug_print(void) {
    if (!g_material_classify_ctx.initialized) return;
    
    printf("Material Classify Context:\n");
    printf("  Count: %u/%u\n", g_material_classify_ctx.count, g_material_classify_ctx.capacity);
}

/* End of material_classify.c */
