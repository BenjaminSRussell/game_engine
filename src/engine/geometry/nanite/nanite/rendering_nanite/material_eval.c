/*
 * material_eval.c
 * Material evaluation
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#include "geometry/nanite/nanite/rendering_nanite/material_eval.h"
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

typedef struct nanite_material_eval_internal {
    uint32_t id;
    uint32_t flags;
    ResourceHandle material_buffer;
    bool initialized;
    bool dirty;
} nanite_material_eval_internal_t;

typedef struct nanite_material_eval_context {
    nanite_material_eval_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} nanite_material_eval_context_t;

static nanite_material_eval_context_t g_material_eval_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void nanite_material_eval_cleanup_internal(nanite_material_eval_internal_t* item) {
    if (!item) return;
    item->material_buffer = INVALID_HANDLE;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int nanite_material_eval_init(void) {
    if (g_material_eval_ctx.initialized) {
        return 0;
    }

    g_material_eval_ctx.capacity = 256;
    g_material_eval_ctx.items = calloc(g_material_eval_ctx.capacity, sizeof(nanite_material_eval_internal_t));
    if (!g_material_eval_ctx.items) {
        return -1;
    }

    g_material_eval_ctx.count = 0;
    g_material_eval_ctx.initialized = true;

    return 0;
}

void nanite_material_eval_shutdown(void) {
    if (!g_material_eval_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_material_eval_ctx.count; i++) {
        nanite_material_eval_cleanup_internal(&g_material_eval_ctx.items[i]);
    }

    free(g_material_eval_ctx.items);
    g_material_eval_ctx.items = NULL;
    g_material_eval_ctx.count = 0;
    g_material_eval_ctx.capacity = 0;
    g_material_eval_ctx.initialized = false;
}

int nanite_material_eval_create(nanite_material_eval_handle_t* out_handle, const nanite_material_eval_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_material_eval_ctx.initialized) {
        return -2;
    }

    if (g_material_eval_ctx.count >= g_material_eval_ctx.capacity) {
        uint32_t new_capacity = g_material_eval_ctx.capacity * 2;
        nanite_material_eval_internal_t* new_items = realloc(g_material_eval_ctx.items, new_capacity * sizeof(nanite_material_eval_internal_t));
        if (!new_items) return -3;
        g_material_eval_ctx.items = new_items;
        g_material_eval_ctx.capacity = new_capacity;
    }

    uint32_t index = g_material_eval_ctx.count++;
    nanite_material_eval_internal_t* item = &g_material_eval_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->material_buffer = INVALID_HANDLE;

    out_handle->id = index;
    return 0;
}

void nanite_material_eval_destroy(nanite_material_eval_handle_t handle) {
    if (handle.id >= g_material_eval_ctx.count) {
        return;
    }

    nanite_material_eval_cleanup_internal(&g_material_eval_ctx.items[handle.id]);
}

int nanite_material_eval_update(nanite_material_eval_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_material_eval_ctx.count) {
        return -1;
    }

    nanite_material_eval_internal_t* item = &g_material_eval_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool nanite_material_eval_is_valid(nanite_material_eval_handle_t handle) {
    if (handle.id >= g_material_eval_ctx.count) {
        return false;
    }
    return g_material_eval_ctx.items[handle.id].initialized;
}

int nanite_material_eval_get_info(nanite_material_eval_handle_t handle, nanite_material_eval_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_material_eval_ctx.count) {
        return -2;
    }

    const nanite_material_eval_internal_t* item = &g_material_eval_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void nanite_material_eval_mark_dirty(nanite_material_eval_handle_t handle) {
    if (handle.id < g_material_eval_ctx.count) {
        g_material_eval_ctx.items[handle.id].dirty = true;
    }
}

int nanite_material_eval_process_pending(void) {
    if (!g_material_eval_ctx.initialized) return 0;

    int processed = 0;
    for (uint32_t i = 0; i < g_material_eval_ctx.count; i++) {
        nanite_material_eval_internal_t* item = &g_material_eval_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Material Evaluation (Deferred Texturing):
            // 1. Read visibility buffer (cluster_index, triangle_index)
            // 2. Fetch cluster data and triangle indices
            // 3. Interpolate vertex attributes (UVs, normals, tangents) using barycentrics
            // 4. Sample textures and evaluate material graph
            // 5. Output to G-Buffer or final lighting buffer
            
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t nanite_material_eval_get_count(void) {
    return g_material_eval_ctx.count;
}

size_t nanite_material_eval_get_memory_usage(void) {
    size_t total = sizeof(nanite_material_eval_context_t);
    total += g_material_eval_ctx.capacity * sizeof(nanite_material_eval_internal_t);
    return total;
}

void nanite_material_eval_debug_print(void) {
    if (!g_material_eval_ctx.initialized) return;
    
    printf("Nanite Material Evaluation Status:\n");
    printf("  Count: %u / %u\n", g_material_eval_ctx.count, g_material_eval_ctx.capacity);
}

/* End of material_eval.c */
