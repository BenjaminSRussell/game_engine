/*
 * shadow_pass.c
 * Shadow rendering pass
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#include "geometry/nanite/nanite/rendering_nanite/shadow_pass.h"
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

typedef struct nanite_shadow_pass_internal {
    uint32_t id;
    uint32_t flags;
    ResourceHandle shadow_map;
    bool initialized;
    bool dirty;
} nanite_shadow_pass_internal_t;

typedef struct nanite_shadow_pass_context {
    nanite_shadow_pass_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} nanite_shadow_pass_context_t;

static nanite_shadow_pass_context_t g_shadow_pass_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void nanite_shadow_pass_cleanup_internal(nanite_shadow_pass_internal_t* item) {
    if (!item) return;
    item->shadow_map = INVALID_HANDLE;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int nanite_shadow_pass_init(void) {
    if (g_shadow_pass_ctx.initialized) {
        return 0;
    }

    g_shadow_pass_ctx.capacity = 256;
    g_shadow_pass_ctx.items = calloc(g_shadow_pass_ctx.capacity, sizeof(nanite_shadow_pass_internal_t));
    if (!g_shadow_pass_ctx.items) {
        return -1;
    }

    g_shadow_pass_ctx.count = 0;
    g_shadow_pass_ctx.initialized = true;

    return 0;
}

void nanite_shadow_pass_shutdown(void) {
    if (!g_shadow_pass_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_shadow_pass_ctx.count; i++) {
        nanite_shadow_pass_cleanup_internal(&g_shadow_pass_ctx.items[i]);
    }

    free(g_shadow_pass_ctx.items);
    g_shadow_pass_ctx.items = NULL;
    g_shadow_pass_ctx.count = 0;
    g_shadow_pass_ctx.capacity = 0;
    g_shadow_pass_ctx.initialized = false;
}

int nanite_shadow_pass_create(nanite_shadow_pass_handle_t* out_handle, const nanite_shadow_pass_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_shadow_pass_ctx.initialized) {
        return -2;
    }

    if (g_shadow_pass_ctx.count >= g_shadow_pass_ctx.capacity) {
        uint32_t new_capacity = g_shadow_pass_ctx.capacity * 2;
        nanite_shadow_pass_internal_t* new_items = realloc(g_shadow_pass_ctx.items, new_capacity * sizeof(nanite_shadow_pass_internal_t));
        if (!new_items) return -3;
        g_shadow_pass_ctx.items = new_items;
        g_shadow_pass_ctx.capacity = new_capacity;
    }

    uint32_t index = g_shadow_pass_ctx.count++;
    nanite_shadow_pass_internal_t* item = &g_shadow_pass_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->shadow_map = INVALID_HANDLE;

    out_handle->id = index;
    return 0;
}

void nanite_shadow_pass_destroy(nanite_shadow_pass_handle_t handle) {
    if (handle.id >= g_shadow_pass_ctx.count) {
        return;
    }

    nanite_shadow_pass_cleanup_internal(&g_shadow_pass_ctx.items[handle.id]);
}

int nanite_shadow_pass_update(nanite_shadow_pass_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_shadow_pass_ctx.count) {
        return -1;
    }

    nanite_shadow_pass_internal_t* item = &g_shadow_pass_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool nanite_shadow_pass_is_valid(nanite_shadow_pass_handle_t handle) {
    if (handle.id >= g_shadow_pass_ctx.count) {
        return false;
    }
    return g_shadow_pass_ctx.items[handle.id].initialized;
}

int nanite_shadow_pass_get_info(nanite_shadow_pass_handle_t handle, nanite_shadow_pass_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_shadow_pass_ctx.count) {
        return -2;
    }

    const nanite_shadow_pass_internal_t* item = &g_shadow_pass_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void nanite_shadow_pass_mark_dirty(nanite_shadow_pass_handle_t handle) {
    if (handle.id < g_shadow_pass_ctx.count) {
        g_shadow_pass_ctx.items[handle.id].dirty = true;
    }
}

int nanite_shadow_pass_process_pending(void) {
    if (!g_shadow_pass_ctx.initialized) return 0;

    int processed = 0;
    for (uint32_t i = 0; i < g_shadow_pass_ctx.count; i++) {
        nanite_shadow_pass_internal_t* item = &g_shadow_pass_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Shadow Pass (Virtual Shadow Maps):
            // 1. Cull clusters against shadow frustum / light view
            // 2. Render clusters into depth-only shadow maps
            // 3. Fast hardware rasterization for large clusters
            // 4. Software rasterization for small clusters (requires 32-bit atomicMin)
            
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t nanite_shadow_pass_get_count(void) {
    return g_shadow_pass_ctx.count;
}

size_t nanite_shadow_pass_get_memory_usage(void) {
    size_t total = sizeof(nanite_shadow_pass_context_t);
    total += g_shadow_pass_ctx.capacity * sizeof(nanite_shadow_pass_internal_t);
    return total;
}

void nanite_shadow_pass_debug_print(void) {
    if (!g_shadow_pass_ctx.initialized) return;
    
    printf("Nanite Shadow Pass Status:\n");
    printf("  Count: %u / %u\n", g_shadow_pass_ctx.count, g_shadow_pass_ctx.capacity);
}

/* End of shadow_pass.c */
