/*
 * cull_feedback.c
 * Culling feedback
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#include "cull_feedback.h"
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

typedef struct nanite_cull_feedback_internal {
    uint32_t id;
    uint32_t flags;
    ResourceHandle feedback_buffer;
    bool initialized;
    bool dirty;
    uint32_t request_count;
} nanite_cull_feedback_internal_t;

typedef struct nanite_cull_feedback_context {
    nanite_cull_feedback_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} nanite_cull_feedback_context_t;

static nanite_cull_feedback_context_t g_cull_feedback_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void nanite_cull_feedback_cleanup_internal(nanite_cull_feedback_internal_t* item) {
    if (!item) return;
    item->feedback_buffer = INVALID_HANDLE;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int nanite_cull_feedback_init(void) {
    if (g_cull_feedback_ctx.initialized) {
        return 0; // Already initialized
    }

    g_cull_feedback_ctx.capacity = 256;
    g_cull_feedback_ctx.items = calloc(g_cull_feedback_ctx.capacity, sizeof(nanite_cull_feedback_internal_t));
    if (!g_cull_feedback_ctx.items) {
        return -1;
    }

    g_cull_feedback_ctx.count = 0;
    g_cull_feedback_ctx.initialized = true;

    return 0;
}

void nanite_cull_feedback_shutdown(void) {
    if (!g_cull_feedback_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_cull_feedback_ctx.count; i++) {
        nanite_cull_feedback_cleanup_internal(&g_cull_feedback_ctx.items[i]);
    }

    free(g_cull_feedback_ctx.items);
    g_cull_feedback_ctx.items = NULL;
    g_cull_feedback_ctx.count = 0;
    g_cull_feedback_ctx.capacity = 0;
    g_cull_feedback_ctx.initialized = false;
}

int nanite_cull_feedback_create(nanite_cull_feedback_handle_t* out_handle, const nanite_cull_feedback_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_cull_feedback_ctx.initialized) {
        return -2;
    }

    if (g_cull_feedback_ctx.count >= g_cull_feedback_ctx.capacity) {
        uint32_t new_capacity = g_cull_feedback_ctx.capacity * 2;
        nanite_cull_feedback_internal_t* new_items = realloc(g_cull_feedback_ctx.items, new_capacity * sizeof(nanite_cull_feedback_internal_t));
        if (!new_items) return -3;
        g_cull_feedback_ctx.items = new_items;
        g_cull_feedback_ctx.capacity = new_capacity;
    }

    uint32_t index = g_cull_feedback_ctx.count++;
    nanite_cull_feedback_internal_t* item = &g_cull_feedback_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->feedback_buffer = INVALID_HANDLE;
    item->request_count = 0;
    item->initialized = true;
    item->dirty = true;

    out_handle->id = index;
    return 0;
}

void nanite_cull_feedback_destroy(nanite_cull_feedback_handle_t handle) {
    if (handle.id >= g_cull_feedback_ctx.count) {
        return;
    }

    nanite_cull_feedback_cleanup_internal(&g_cull_feedback_ctx.items[handle.id]);
}

int nanite_cull_feedback_update(nanite_cull_feedback_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_cull_feedback_ctx.count) {
        return -1;
    }

    nanite_cull_feedback_internal_t* item = &g_cull_feedback_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool nanite_cull_feedback_is_valid(nanite_cull_feedback_handle_t handle) {
    if (handle.id >= g_cull_feedback_ctx.count) {
        return false;
    }
    return g_cull_feedback_ctx.items[handle.id].initialized;
}

int nanite_cull_feedback_get_info(nanite_cull_feedback_handle_t handle, nanite_cull_feedback_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_cull_feedback_ctx.count) {
        return -2;
    }

    const nanite_cull_feedback_internal_t* item = &g_cull_feedback_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void nanite_cull_feedback_mark_dirty(nanite_cull_feedback_handle_t handle) {
    if (handle.id < g_cull_feedback_ctx.count) {
        g_cull_feedback_ctx.items[handle.id].dirty = true;
    }
}

int nanite_cull_feedback_process_pending(void) {
    if (!g_cull_feedback_ctx.initialized) return 0;

    int processed = 0;
    for (uint32_t i = 0; i < g_cull_feedback_ctx.count; i++) {
        nanite_cull_feedback_internal_t* item = &g_cull_feedback_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Read back feedback buffer from GPU
            // Analyze requested clusters for streaming
            // Trigger I/O requests for missing clusters
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t nanite_cull_feedback_get_count(void) {
    return g_cull_feedback_ctx.count;
}

size_t nanite_cull_feedback_get_memory_usage(void) {
    size_t total = sizeof(nanite_cull_feedback_context_t);
    total += g_cull_feedback_ctx.capacity * sizeof(nanite_cull_feedback_internal_t);
    return total;
}

void nanite_cull_feedback_debug_print(void) {
    if (!g_cull_feedback_ctx.initialized) return;
    
    printf("Nanite Cull Feedback Context:\n");
    printf("  Count: %u/%u\n", g_cull_feedback_ctx.count, g_cull_feedback_ctx.capacity);
}

/* End of cull_feedback.c */
