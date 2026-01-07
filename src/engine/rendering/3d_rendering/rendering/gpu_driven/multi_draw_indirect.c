/*
 * multi_draw_indirect.c
 * Multi-draw indirect
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "multi_draw_indirect.h"
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

typedef struct rendering_multi_draw_indirect_internal {
    uint32_t id;
    uint32_t flags;
    ResourceHandle argument_buffer;
    ResourceHandle count_buffer;
    bool initialized;
    bool dirty;
} rendering_multi_draw_indirect_internal_t;

typedef struct rendering_multi_draw_indirect_context {
    rendering_multi_draw_indirect_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} rendering_multi_draw_indirect_context_t;

static rendering_multi_draw_indirect_context_t g_multi_draw_indirect_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void rendering_multi_draw_indirect_cleanup_internal(rendering_multi_draw_indirect_internal_t* item) {
    if (!item) return;
    item->argument_buffer = INVALID_HANDLE;
    item->count_buffer = INVALID_HANDLE;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_multi_draw_indirect_init(void) {
    if (g_multi_draw_indirect_ctx.initialized) {
        return 0;
    }

    g_multi_draw_indirect_ctx.capacity = 256;
    g_multi_draw_indirect_ctx.items = calloc(g_multi_draw_indirect_ctx.capacity, sizeof(rendering_multi_draw_indirect_internal_t));
    if (!g_multi_draw_indirect_ctx.items) {
        return -1;
    }

    g_multi_draw_indirect_ctx.count = 0;
    g_multi_draw_indirect_ctx.initialized = true;

    return 0;
}

void rendering_multi_draw_indirect_shutdown(void) {
    if (!g_multi_draw_indirect_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_multi_draw_indirect_ctx.count; i++) {
        rendering_multi_draw_indirect_cleanup_internal(&g_multi_draw_indirect_ctx.items[i]);
    }

    free(g_multi_draw_indirect_ctx.items);
    g_multi_draw_indirect_ctx.items = NULL;
    g_multi_draw_indirect_ctx.count = 0;
    g_multi_draw_indirect_ctx.capacity = 0;
    g_multi_draw_indirect_ctx.initialized = false;
}

int rendering_multi_draw_indirect_create(rendering_multi_draw_indirect_handle_t* out_handle, const rendering_multi_draw_indirect_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_multi_draw_indirect_ctx.initialized) {
        return -2;
    }

    if (g_multi_draw_indirect_ctx.count >= g_multi_draw_indirect_ctx.capacity) {
        uint32_t new_capacity = g_multi_draw_indirect_ctx.capacity * 2;
        rendering_multi_draw_indirect_internal_t* new_items = realloc(g_multi_draw_indirect_ctx.items, new_capacity * sizeof(rendering_multi_draw_indirect_internal_t));
        if (!new_items) return -3;
        g_multi_draw_indirect_ctx.items = new_items;
        g_multi_draw_indirect_ctx.capacity = new_capacity;
    }

    uint32_t index = g_multi_draw_indirect_ctx.count++;
    rendering_multi_draw_indirect_internal_t* item = &g_multi_draw_indirect_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->argument_buffer = INVALID_HANDLE;
    item->count_buffer = INVALID_HANDLE;

    out_handle->id = index;
    return 0;
}

void rendering_multi_draw_indirect_destroy(rendering_multi_draw_indirect_handle_t handle) {
    if (handle.id >= g_multi_draw_indirect_ctx.count) {
        return;
    }

    rendering_multi_draw_indirect_cleanup_internal(&g_multi_draw_indirect_ctx.items[handle.id]);
}

int rendering_multi_draw_indirect_update(rendering_multi_draw_indirect_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_multi_draw_indirect_ctx.count) {
        return -1;
    }

    rendering_multi_draw_indirect_internal_t* item = &g_multi_draw_indirect_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool rendering_multi_draw_indirect_is_valid(rendering_multi_draw_indirect_handle_t handle) {
    if (handle.id >= g_multi_draw_indirect_ctx.count) {
        return false;
    }
    return g_multi_draw_indirect_ctx.items[handle.id].initialized;
}

int rendering_multi_draw_indirect_get_info(rendering_multi_draw_indirect_handle_t handle, rendering_multi_draw_indirect_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_multi_draw_indirect_ctx.count) {
        return -2;
    }

    const rendering_multi_draw_indirect_internal_t* item = &g_multi_draw_indirect_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void rendering_multi_draw_indirect_mark_dirty(rendering_multi_draw_indirect_handle_t handle) {
    if (handle.id < g_multi_draw_indirect_ctx.count) {
        g_multi_draw_indirect_ctx.items[handle.id].dirty = true;
    }
}

int rendering_multi_draw_indirect_process_pending(void) {
    if (!g_multi_draw_indirect_ctx.initialized) return 0;

    int processed = 0;
    for (uint32_t i = 0; i < g_multi_draw_indirect_ctx.count; i++) {
        rendering_multi_draw_indirect_internal_t* item = &g_multi_draw_indirect_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Setup Multi-Draw Indirect command buffer
            // Execute GPU-driven draw submission
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t rendering_multi_draw_indirect_get_count(void) {
    return g_multi_draw_indirect_ctx.count;
}

size_t rendering_multi_draw_indirect_get_memory_usage(void) {
    size_t total = sizeof(rendering_multi_draw_indirect_context_t);
    total += g_multi_draw_indirect_ctx.capacity * sizeof(rendering_multi_draw_indirect_internal_t);
    return total;
}

void rendering_multi_draw_indirect_debug_print(void) {
    if (!g_multi_draw_indirect_ctx.initialized) return;
    
    printf("Multi-Draw Indirect Status:\n");
    printf("  Count: %u / %u\n", g_multi_draw_indirect_ctx.count, g_multi_draw_indirect_ctx.capacity);
}

/* End of multi_draw_indirect.c */
