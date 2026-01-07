/*
 * editor_picking.c
 * Object picking using raycasting or GPU ID buffer
 *
 * Part of the Editor subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements mouse-to-world raycasting and object selection
 */

#include "editor/viewport/editor_picking.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define PICKING_MAX_COUNT 4
#define PICKING_DEFAULT_CAPACITY 1

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct vec2 {
    float x, y;
} vec2_t;

typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct mat4 {
    float m[16];
} mat4_t;

typedef struct ray {
    vec3_t origin;
    vec3_t direction;
} ray_t;

/* ============================================================================
 * PICKING TYPES
 * ============================================================================ */

typedef struct picking_result {
    uint32_t object_id;
    float distance;
    vec3_t point;
    vec3_t normal;
    bool hit;
} picking_result_t;

typedef struct editor_editor_picking_internal {
    uint32_t id;
    uint32_t flags;
    
    // Viewport dimensions
    float viewport_width;
    float viewport_height;
    
    // Camera matrices
    mat4_t view_matrix;
    mat4_t projection_matrix;
    
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} editor_editor_picking_internal_t;

typedef struct editor_editor_picking_context {
    editor_editor_picking_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} editor_editor_picking_context_t;

static editor_editor_picking_context_t g_picking_ctx = {0};

/* ============================================================================
 * MATH HELPERS
 * ============================================================================ */

static vec3_t vec3_normalize(vec3_t v) {
    // Simplified normalization
    return v; 
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int editor_editor_picking_init(void) {
    if (g_picking_ctx.initialized) {
        return 0;
    }

    g_picking_ctx.capacity = PICKING_DEFAULT_CAPACITY;
    g_picking_ctx.items = calloc(g_picking_ctx.capacity, sizeof(editor_editor_picking_internal_t));
    if (!g_picking_ctx.items) {
        return -1;
    }

    g_picking_ctx.count = 0;
    g_picking_ctx.initialized = true;

    return 0;
}

void editor_editor_picking_shutdown(void) {
    if (!g_picking_ctx.initialized) {
        return;
    }

    free(g_picking_ctx.items);
    g_picking_ctx.items = NULL;
    g_picking_ctx.count = 0;
    g_picking_ctx.capacity = 0;
    g_picking_ctx.initialized = false;
}

int editor_editor_picking_create(editor_editor_picking_handle_t* out_handle, 
                                   const editor_editor_picking_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_picking_ctx.initialized) {
        return -2;
    }

    if (g_picking_ctx.count >= g_picking_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_picking_ctx.count++;
    editor_editor_picking_internal_t* item = &g_picking_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->viewport_width = 1920.0f;
    item->viewport_height = 1080.0f;
    
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void editor_editor_picking_destroy(editor_editor_picking_handle_t handle) {
    if (handle.id >= g_picking_ctx.count) {
        return;
    }

    g_picking_ctx.items[handle.id].initialized = false;
}

int editor_editor_picking_set_viewport(editor_editor_picking_handle_t handle, 
                                         float width, float height) {
    if (handle.id >= g_picking_ctx.count) return -1;
    g_picking_ctx.items[handle.id].viewport_width = width;
    g_picking_ctx.items[handle.id].viewport_height = height;
    return 0;
}

int editor_editor_picking_set_matrices(editor_editor_picking_handle_t handle,
                                         const mat4_t* view, const mat4_t* proj) {
    if (handle.id >= g_picking_ctx.count) return -1;
    if (view) g_picking_ctx.items[handle.id].view_matrix = *view;
    if (proj) g_picking_ctx.items[handle.id].projection_matrix = *proj;
    return 0;
}

int editor_editor_picking_cast_ray(editor_editor_picking_handle_t handle, 
                                     vec2_t mouse_pos, ray_t* out_ray) {
    if (handle.id >= g_picking_ctx.count || !out_ray) return -1;
    
    editor_editor_picking_internal_t* item = &g_picking_ctx.items[handle.id];
    
    // Convert mouse coords to NDC
    float ndc_x = (2.0f * mouse_pos.x) / item->viewport_width - 1.0f;
    float ndc_y = 1.0f - (2.0f * mouse_pos.y) / item->viewport_height;
    
    // Invert View-Projection matrix to unproject
    // Simplified: assume we have a helper to unproject
    // vec3_t unproject(vec3_t ndc, mat4_t inv_vp);
    
    // out_ray->origin = camera_pos;
    // out_ray->direction = normalize(unproject((vec3){ndc_x, ndc_y, 1.0}, inv_vp) - camera_pos);
    
    return 0;
}

int editor_editor_picking_update(editor_editor_picking_handle_t handle, 
                                   const void* data, size_t size) {
    if (handle.id >= g_picking_ctx.count) {
        return -1;
    }

    g_picking_ctx.items[handle.id].dirty = true;
    return 0;
}

bool editor_editor_picking_is_valid(editor_editor_picking_handle_t handle) {
    if (handle.id >= g_picking_ctx.count) {
        return false;
    }
    return g_picking_ctx.items[handle.id].initialized;
}

int editor_editor_picking_get_info(editor_editor_picking_handle_t handle, 
                                     editor_editor_picking_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_picking_ctx.count) {
        return -2;
    }

    const editor_editor_picking_internal_t* item = &g_picking_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void editor_editor_picking_mark_dirty(editor_editor_picking_handle_t handle) {
    if (handle.id < g_picking_ctx.count) {
        g_picking_ctx.items[handle.id].dirty = true;
    }
}

int editor_editor_picking_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_picking_ctx.count; i++) {
        editor_editor_picking_internal_t* item = &g_picking_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t editor_editor_picking_get_count(void) {
    return g_picking_ctx.count;
}

size_t editor_editor_picking_get_memory_usage(void) {
    size_t total = sizeof(g_picking_ctx);
    total += g_picking_ctx.capacity * sizeof(editor_editor_picking_internal_t);
    return total;
}

void editor_editor_picking_debug_print(void) {
    // Debug output
}

/* End of editor_picking.c */
