/*
 * software_raster.c
 * Software rasterization
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#include "geometry/nanite/nanite/rendering_nanite/software_raster.h"
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

typedef struct nanite_software_raster_internal {
    uint32_t id;
    uint32_t flags;
    ResourceHandle output_buffer;
    bool initialized;
    bool dirty;
    uint32_t width;
    uint32_t height;
} nanite_software_raster_internal_t;

typedef struct nanite_software_raster_context {
    nanite_software_raster_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} nanite_software_raster_context_t;

static nanite_software_raster_context_t g_software_raster_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void nanite_software_raster_cleanup_internal(nanite_software_raster_internal_t* item) {
    if (!item) return;
    item->output_buffer = INVALID_HANDLE;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int nanite_software_raster_init(void) {
    if (g_software_raster_ctx.initialized) {
        return 0; // Already initialized
    }

    g_software_raster_ctx.capacity = 256;
    g_software_raster_ctx.items = calloc(g_software_raster_ctx.capacity, sizeof(nanite_software_raster_internal_t));
    if (!g_software_raster_ctx.items) {
        return -1;
    }

    g_software_raster_ctx.count = 0;
    g_software_raster_ctx.initialized = true;

    return 0;
}

void nanite_software_raster_shutdown(void) {
    if (!g_software_raster_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_software_raster_ctx.count; i++) {
        nanite_software_raster_cleanup_internal(&g_software_raster_ctx.items[i]);
    }

    free(g_software_raster_ctx.items);
    g_software_raster_ctx.items = NULL;
    g_software_raster_ctx.count = 0;
    g_software_raster_ctx.capacity = 0;
    g_software_raster_ctx.initialized = false;
}

int nanite_software_raster_create(nanite_software_raster_handle_t* out_handle, const nanite_software_raster_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_software_raster_ctx.initialized) {
        return -2;
    }

    if (g_software_raster_ctx.count >= g_software_raster_ctx.capacity) {
        uint32_t new_capacity = g_software_raster_ctx.capacity * 2;
        nanite_software_raster_internal_t* new_items = realloc(g_software_raster_ctx.items, new_capacity * sizeof(nanite_software_raster_internal_t));
        if (!new_items) return -3;
        g_software_raster_ctx.items = new_items;
        g_software_raster_ctx.capacity = new_capacity;
    }

    uint32_t index = g_software_raster_ctx.count++;
    nanite_software_raster_internal_t* item = &g_software_raster_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->output_buffer = INVALID_HANDLE;
    item->width = 1920; 
    item->height = 1080;
    item->initialized = true;
    item->dirty = true;

    out_handle->id = index;
    return 0;
}

void nanite_software_raster_destroy(nanite_software_raster_handle_t handle) {
    if (handle.id >= g_software_raster_ctx.count) {
        return;
    }

    nanite_software_raster_cleanup_internal(&g_software_raster_ctx.items[handle.id]);
}

int nanite_software_raster_update(nanite_software_raster_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_software_raster_ctx.count) {
        return -1;
    }

    nanite_software_raster_internal_t* item = &g_software_raster_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool nanite_software_raster_is_valid(nanite_software_raster_handle_t handle) {
    if (handle.id >= g_software_raster_ctx.count) {
        return false;
    }
    return g_software_raster_ctx.items[handle.id].initialized;
}

int nanite_software_raster_get_info(nanite_software_raster_handle_t handle, nanite_software_raster_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_software_raster_ctx.count) {
        return -2;
    }

    const nanite_software_raster_internal_t* item = &g_software_raster_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void nanite_software_raster_mark_dirty(nanite_software_raster_handle_t handle) {
    if (handle.id < g_software_raster_ctx.count) {
        g_software_raster_ctx.items[handle.id].dirty = true;
    }
}

int nanite_software_raster_process_pending(void) {
    if (!g_software_raster_ctx.initialized) return 0;

    int processed = 0;
    for (uint32_t i = 0; i < g_software_raster_ctx.count; i++) {
        nanite_software_raster_internal_t* item = &g_software_raster_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Software Rasterization Path:
            // 1. Process micro-triangles with compute shader
            // 2. Perform manual depth test using 64-bit atomics (Depth | ID)
            // 3. Output to Visibility Buffer
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t nanite_software_raster_get_count(void) {
    return g_software_raster_ctx.count;
}

size_t nanite_software_raster_get_memory_usage(void) {
    size_t total = sizeof(nanite_software_raster_context_t);
    total += g_software_raster_ctx.capacity * sizeof(nanite_software_raster_internal_t);
    return total;
}

void nanite_software_raster_debug_print(void) {
    if (!g_software_raster_ctx.initialized) return;
    
    printf("Nanite Software Raster Context:\n");
    printf("  Count: %u/%u\n", g_software_raster_ctx.count, g_software_raster_ctx.capacity);
}

/* End of software_raster.c */
