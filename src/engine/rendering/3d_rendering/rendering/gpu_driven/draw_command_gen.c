/*
 * draw_command_gen.c
 * Draw command generation
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "draw_command_gen.h"
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

typedef struct rendering_draw_command_gen_internal {
    uint32_t id;
    uint32_t flags;
    ResourceHandle indirect_buffer;
    ResourceHandle count_buffer;
    bool initialized;
    bool dirty;
} rendering_draw_command_gen_internal_t;

typedef struct rendering_draw_command_gen_context {
    rendering_draw_command_gen_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} rendering_draw_command_gen_context_t;

static rendering_draw_command_gen_context_t g_draw_command_gen_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void rendering_draw_command_gen_cleanup_internal(rendering_draw_command_gen_internal_t* item) {
    if (!item) return;
    item->indirect_buffer = INVALID_HANDLE;
    item->count_buffer = INVALID_HANDLE;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_draw_command_gen_init(void) {
    if (g_draw_command_gen_ctx.initialized) {
        return 0;
    }

    g_draw_command_gen_ctx.capacity = 256;
    g_draw_command_gen_ctx.items = calloc(g_draw_command_gen_ctx.capacity, sizeof(rendering_draw_command_gen_internal_t));
    if (!g_draw_command_gen_ctx.items) {
        return -1;
    }

    g_draw_command_gen_ctx.count = 0;
    g_draw_command_gen_ctx.initialized = true;

    return 0;
}

void rendering_draw_command_gen_shutdown(void) {
    if (!g_draw_command_gen_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_draw_command_gen_ctx.count; i++) {
        rendering_draw_command_gen_cleanup_internal(&g_draw_command_gen_ctx.items[i]);
    }

    free(g_draw_command_gen_ctx.items);
    g_draw_command_gen_ctx.items = NULL;
    g_draw_command_gen_ctx.count = 0;
    g_draw_command_gen_ctx.capacity = 0;
    g_draw_command_gen_ctx.initialized = false;
}

int rendering_draw_command_gen_create(rendering_draw_command_gen_handle_t* out_handle, const rendering_draw_command_gen_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_draw_command_gen_ctx.initialized) {
        return -2;
    }

    if (g_draw_command_gen_ctx.count >= g_draw_command_gen_ctx.capacity) {
        uint32_t new_capacity = g_draw_command_gen_ctx.capacity * 2;
        rendering_draw_command_gen_internal_t* new_items = realloc(g_draw_command_gen_ctx.items, new_capacity * sizeof(rendering_draw_command_gen_internal_t));
        if (!new_items) return -3;
        g_draw_command_gen_ctx.items = new_items;
        g_draw_command_gen_ctx.capacity = new_capacity;
    }

    uint32_t index = g_draw_command_gen_ctx.count++;
    rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->indirect_buffer = INVALID_HANDLE;
    item->count_buffer = INVALID_HANDLE;

    out_handle->id = index;
    return 0;
}

void rendering_draw_command_gen_destroy(rendering_draw_command_gen_handle_t handle) {
    if (handle.id >= g_draw_command_gen_ctx.count) {
        return;
    }

    rendering_draw_command_gen_cleanup_internal(&g_draw_command_gen_ctx.items[handle.id]);
}

int rendering_draw_command_gen_update(rendering_draw_command_gen_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_draw_command_gen_ctx.count) {
        return -1;
    }

    rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool rendering_draw_command_gen_is_valid(rendering_draw_command_gen_handle_t handle) {
    if (handle.id >= g_draw_command_gen_ctx.count) {
        return false;
    }
    return g_draw_command_gen_ctx.items[handle.id].initialized;
}

int rendering_draw_command_gen_get_info(rendering_draw_command_gen_handle_t handle, rendering_draw_command_gen_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_draw_command_gen_ctx.count) {
        return -2;
    }

    const rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void rendering_draw_command_gen_mark_dirty(rendering_draw_command_gen_handle_t handle) {
    if (handle.id < g_draw_command_gen_ctx.count) {
        g_draw_command_gen_ctx.items[handle.id].dirty = true;
    }
}

int rendering_draw_command_gen_process_pending(void) {
    if (!g_draw_command_gen_ctx.initialized) return 0;

    int processed = 0;
    for (uint32_t i = 0; i < g_draw_command_gen_ctx.count; i++) {
        rendering_draw_command_gen_internal_t* item = &g_draw_command_gen_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Dispatch compute shader to generate indirect draw commands
            // Compaction of visible clusters into draw streams
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t rendering_draw_command_gen_get_count(void) {
    return g_draw_command_gen_ctx.count;
}

size_t rendering_draw_command_gen_get_memory_usage(void) {
    size_t total = sizeof(rendering_draw_command_gen_context_t);
    total += g_draw_command_gen_ctx.capacity * sizeof(rendering_draw_command_gen_internal_t);
    return total;
}

void rendering_draw_command_gen_debug_print(void) {
    if (!g_draw_command_gen_ctx.initialized) return;
    
    printf("Draw Command Generation Status:\n");
    printf("  Count: %u / %u\n", g_draw_command_gen_ctx.count, g_draw_command_gen_ctx.capacity);
}

/* End of draw_command_gen.c */
