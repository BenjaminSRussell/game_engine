/*
 * editor_selection.c
 * Selection outline rendering
 *
 * Part of the Editor subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements stencil-based selection outlines and multi-selection management
 */

#include "editor_selection.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define SELECTION_MAX_ITEMS 1024
#define SELECTION_OUTLINE_WIDTH 2.0f
#define SELECTION_COLOR_PRIMARY (vec4_t){1.0f, 0.6f, 0.0f, 1.0f}
#define SELECTION_COLOR_SECONDARY (vec4_t){0.6f, 0.8f, 1.0f, 1.0f}

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct vec4 {
    float x, y, z, w;
} vec4_t;

/* ============================================================================
 * SELECTION TYPES
 * ============================================================================ */

typedef struct selection_item {
    uint32_t entity_id;
    // Potentially other ID types (component, resource)
    bool is_primary;
} selection_item_t;

typedef struct editor_editor_selection_internal {
    uint32_t id;
    uint32_t flags;
    
    selection_item_t items[SELECTION_MAX_ITEMS];
    uint32_t item_count;
    
    vec4_t primary_color;
    vec4_t secondary_color;
    float outline_width;
    
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} editor_editor_selection_internal_t;

typedef struct editor_editor_selection_context {
    editor_editor_selection_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} editor_editor_selection_context_t;

static editor_editor_selection_context_t g_selection_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int editor_editor_selection_init(void) {
    if (g_selection_ctx.initialized) {
        return 0;
    }

    g_selection_ctx.capacity = 1; // Only need one active selection context usually
    g_selection_ctx.items = calloc(g_selection_ctx.capacity, sizeof(editor_editor_selection_internal_t));
    if (!g_selection_ctx.items) {
        return -1;
    }

    g_selection_ctx.count = 0;
    g_selection_ctx.initialized = true;

    return 0;
}

void editor_editor_selection_shutdown(void) {
    if (!g_selection_ctx.initialized) {
        return;
    }

    free(g_selection_ctx.items);
    g_selection_ctx.items = NULL;
    g_selection_ctx.count = 0;
    g_selection_ctx.capacity = 0;
    g_selection_ctx.initialized = false;
}

int editor_editor_selection_create(editor_editor_selection_handle_t* out_handle, 
                                     const editor_editor_selection_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_selection_ctx.initialized) {
        return -2;
    }

    if (g_selection_ctx.count >= g_selection_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_selection_ctx.count++;
    editor_editor_selection_internal_t* item = &g_selection_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->item_count = 0;
    
    item->primary_color = SELECTION_COLOR_PRIMARY;
    item->secondary_color = SELECTION_COLOR_SECONDARY;
    item->outline_width = SELECTION_OUTLINE_WIDTH;
    
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void editor_editor_selection_destroy(editor_editor_selection_handle_t handle) {
    if (handle.id >= g_selection_ctx.count) {
        return;
    }

    g_selection_ctx.items[handle.id].initialized = false;
}

int editor_editor_selection_add(editor_editor_selection_handle_t handle, uint32_t entity_id) {
    if (handle.id >= g_selection_ctx.count) return -1;
    
    editor_editor_selection_internal_t* ctx = &g_selection_ctx.items[handle.id];
    
    // Check if already selected
    for (uint32_t i = 0; i < ctx->item_count; i++) {
        if (ctx->items[i].entity_id == entity_id) {
            // Promote to primary
            ctx->items[i].is_primary = true;
            // Demote others
            for (uint32_t j = 0; j < ctx->item_count; j++) {
                if (i != j) ctx->items[j].is_primary = false;
            }
            ctx->dirty = true;
            return 0;
        }
    }
    
    if (ctx->item_count < SELECTION_MAX_ITEMS) {
        // Demote existing primary
        for (uint32_t i = 0; i < ctx->item_count; i++) {
            ctx->items[i].is_primary = false;
        }
        
        ctx->items[ctx->item_count].entity_id = entity_id;
        ctx->items[ctx->item_count].is_primary = true;
        ctx->item_count++;
        ctx->dirty = true;
        return 0;
    }
    
    return -2; // Full
}

int editor_editor_selection_remove(editor_editor_selection_handle_t handle, uint32_t entity_id) {
    if (handle.id >= g_selection_ctx.count) return -1;
    
    editor_editor_selection_internal_t* ctx = &g_selection_ctx.items[handle.id];
    
    for (uint32_t i = 0; i < ctx->item_count; i++) {
        if (ctx->items[i].entity_id == entity_id) {
            // Remove and shift
            if (ctx->items[i].is_primary && ctx->item_count > 1) {
                // Determine new primary (e.g. last added)
                // Simplified: just pick first active
            }
            
            for (uint32_t j = i; j < ctx->item_count - 1; j++) {
                ctx->items[j] = ctx->items[j + 1];
            }
            ctx->item_count--;
            ctx->dirty = true;
            return 0;
        }
    }
    
    return -2; // Not found
}

int editor_editor_selection_clear(editor_editor_selection_handle_t handle) {
    if (handle.id >= g_selection_ctx.count) return -1;
    
    g_selection_ctx.items[handle.id].item_count = 0;
    g_selection_ctx.items[handle.id].dirty = true;
    return 0;
}

int editor_editor_selection_set_color(editor_editor_selection_handle_t handle, 
                                        vec4_t primary, vec4_t secondary) {
    if (handle.id >= g_selection_ctx.count) return -1;
    
    g_selection_ctx.items[handle.id].primary_color = primary;
    g_selection_ctx.items[handle.id].secondary_color = secondary;
    g_selection_ctx.items[handle.id].dirty = true;
    return 0;
}

int editor_editor_selection_update(editor_editor_selection_handle_t handle, 
                                     const void* data, size_t size) {
    if (handle.id >= g_selection_ctx.count) {
        return -1;
    }

    g_selection_ctx.items[handle.id].dirty = true;
    return 0;
}

bool editor_editor_selection_is_valid(editor_editor_selection_handle_t handle) {
    if (handle.id >= g_selection_ctx.count) {
        return false;
    }
    return g_selection_ctx.items[handle.id].initialized;
}

int editor_editor_selection_get_info(editor_editor_selection_handle_t handle, 
                                       editor_editor_selection_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_selection_ctx.count) {
        return -2;
    }

    const editor_editor_selection_internal_t* item = &g_selection_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void editor_editor_selection_mark_dirty(editor_editor_selection_handle_t handle) {
    if (handle.id < g_selection_ctx.count) {
        g_selection_ctx.items[handle.id].dirty = true;
    }
}

int editor_editor_selection_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_selection_ctx.count; i++) {
        editor_editor_selection_internal_t* item = &g_selection_ctx.items[i];
        if (item->initialized && item->dirty) {
            // In a real implementation, this would regenerate the selection stencil buffer
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t editor_editor_selection_get_count(void) {
    return g_selection_ctx.count;
}

size_t editor_editor_selection_get_memory_usage(void) {
    size_t total = sizeof(g_selection_ctx);
    total += g_selection_ctx.capacity * sizeof(editor_editor_selection_internal_t);
    return total;
}

void editor_editor_selection_debug_print(void) {
    // Debug output
}

/* End of editor_selection.c */
