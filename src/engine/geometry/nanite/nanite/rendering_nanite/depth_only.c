/*
 * depth_only.c
 * Depth-only pass
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#include "geometry/nanite/nanite/rendering_nanite/depth_only.h"
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

typedef struct nanite_depth_only_internal {
    uint32_t id;
    uint32_t flags;
    ResourceHandle depth_target;
    bool initialized;
    bool dirty;
} nanite_depth_only_internal_t;

typedef struct nanite_depth_only_context {
    nanite_depth_only_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} nanite_depth_only_context_t;

static nanite_depth_only_context_t g_depth_only_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void nanite_depth_only_cleanup_internal(nanite_depth_only_internal_t* item) {
    if (!item) return;
    item->depth_target = INVALID_HANDLE;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int nanite_depth_only_init(void) {
    if (g_depth_only_ctx.initialized) {
        return 0;
    }

    g_depth_only_ctx.capacity = 256;
    g_depth_only_ctx.items = calloc(g_depth_only_ctx.capacity, sizeof(nanite_depth_only_internal_t));
    if (!g_depth_only_ctx.items) {
        return -1;
    }

    g_depth_only_ctx.count = 0;
    g_depth_only_ctx.initialized = true;

    return 0;
}

void nanite_depth_only_shutdown(void) {
    if (!g_depth_only_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_depth_only_ctx.count; i++) {
        nanite_depth_only_cleanup_internal(&g_depth_only_ctx.items[i]);
    }

    free(g_depth_only_ctx.items);
    g_depth_only_ctx.items = NULL;
    g_depth_only_ctx.count = 0;
    g_depth_only_ctx.capacity = 0;
    g_depth_only_ctx.initialized = false;
}

int nanite_depth_only_create(nanite_depth_only_handle_t* out_handle, const nanite_depth_only_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_depth_only_ctx.initialized) {
        return -2;
    }

    if (g_depth_only_ctx.count >= g_depth_only_ctx.capacity) {
        uint32_t new_capacity = g_depth_only_ctx.capacity * 2;
        nanite_depth_only_internal_t* new_items = realloc(g_depth_only_ctx.items, new_capacity * sizeof(nanite_depth_only_internal_t));
        if (!new_items) return -3;
        g_depth_only_ctx.items = new_items;
        g_depth_only_ctx.capacity = new_capacity;
    }

    uint32_t index = g_depth_only_ctx.count++;
    nanite_depth_only_internal_t* item = &g_depth_only_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->depth_target = INVALID_HANDLE;

    out_handle->id = index;
    return 0;
}

void nanite_depth_only_destroy(nanite_depth_only_handle_t handle) {
    if (handle.id >= g_depth_only_ctx.count) {
        return;
    }

    nanite_depth_only_cleanup_internal(&g_depth_only_ctx.items[handle.id]);
}

int nanite_depth_only_update(nanite_depth_only_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_depth_only_ctx.count) {
        return -1;
    }

    nanite_depth_only_internal_t* item = &g_depth_only_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool nanite_depth_only_is_valid(nanite_depth_only_handle_t handle) {
    if (handle.id >= g_depth_only_ctx.count) {
        return false;
    }
    return g_depth_only_ctx.items[handle.id].initialized;
}

int nanite_depth_only_get_info(nanite_depth_only_handle_t handle, nanite_depth_only_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_depth_only_ctx.count) {
        return -2;
    }

    const nanite_depth_only_internal_t* item = &g_depth_only_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void nanite_depth_only_mark_dirty(nanite_depth_only_handle_t handle) {
    if (handle.id < g_depth_only_ctx.count) {
        g_depth_only_ctx.items[handle.id].dirty = true;
    }
}

int nanite_depth_only_process_pending(void) {
    if (!g_depth_only_ctx.initialized) return 0;

    int processed = 0;
    for (uint32_t i = 0; i < g_depth_only_ctx.count; i++) {
        nanite_depth_only_internal_t* item = &g_depth_only_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Depth-Only Pass (First Culling Pass):
            // 1. Render "previously visible" clusters to populate depth buffer
            // 2. Used to build Hierarchical Z-Buffer (HzB) for occlusion culling
            // 3. No color/visibility buffer writes in this pass
            
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t nanite_depth_only_get_count(void) {
    return g_depth_only_ctx.count;
}

size_t nanite_depth_only_get_memory_usage(void) {
    size_t total = sizeof(nanite_depth_only_context_t);
    total += g_depth_only_ctx.capacity * sizeof(nanite_depth_only_internal_t);
    return total;
}

void nanite_depth_only_debug_print(void) {
    if (!g_depth_only_ctx.initialized) return;
    
    printf("Nanite Depth-Only Pass Status:\n");
    printf("  Count: %u / %u\n", g_depth_only_ctx.count, g_depth_only_ctx.capacity);
}

/* End of depth_only.c */
