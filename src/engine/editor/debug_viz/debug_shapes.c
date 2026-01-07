/*
 * debug_shapes.c
 * Debug shape rendering
 *
 * Part of the Debug Visualization subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements wireframe/solid shape rendering for debugging
 */

#include "editor/debug_viz/debug_shapes.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MAX_DEBUG_SHAPES 4096
#define ONE_FRAME_DURATION 0.0f
#define PI 3.14159265359f

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct vec4 {
    float x, y, z, w;
} vec4_t;

typedef struct quat {
    float x, y, z, w;
} quat_t;

typedef struct mat4 {
    float m[16];
} mat4_t;

/* ============================================================================
 * DEBUG SHAPE TYPES
 * ============================================================================ */

typedef enum shape_type {
    SHAPE_BOX = 0,
    SHAPE_SPHERE,
    SHAPE_CAPSULE,
    SHAPE_CYLINDER,
    SHAPE_CONE
} shape_type_t;

typedef struct debug_shape {
    shape_type_t type;
    vec3_t position;
    quat_t rotation;
    vec3_t scale;
    vec4_t color;
    bool wireframe;
    float duration;
} debug_shape_t;

typedef struct debugging_visualization_debug_shapes_internal {
    uint32_t id;
    uint32_t flags;
    
    debug_shape_t* shapes;
    uint32_t shape_count;
    uint32_t shape_capacity;
    
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} debugging_visualization_debug_shapes_internal_t;

typedef struct debugging_visualization_debug_shapes_context {
    debugging_visualization_debug_shapes_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} debugging_visualization_debug_shapes_context_t;

static debugging_visualization_debug_shapes_context_t g_debug_shapes_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int debugging_visualization_debug_shapes_init(void) {
    if (g_debug_shapes_ctx.initialized) {
        return 0;
    }

    g_debug_shapes_ctx.capacity = 1;
    g_debug_shapes_ctx.items = calloc(g_debug_shapes_ctx.capacity, sizeof(debugging_visualization_debug_shapes_internal_t));
    if (!g_debug_shapes_ctx.items) {
        return -1;
    }

    g_debug_shapes_ctx.count = 0;
    g_debug_shapes_ctx.initialized = true;

    return 0;
}

void debugging_visualization_debug_shapes_shutdown(void) {
    if (!g_debug_shapes_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_debug_shapes_ctx.count; i++) {
        free(g_debug_shapes_ctx.items[i].shapes);
    }
    
    free(g_debug_shapes_ctx.items);
    g_debug_shapes_ctx.items = NULL;
    g_debug_shapes_ctx.count = 0;
    g_debug_shapes_ctx.capacity = 0;
    g_debug_shapes_ctx.initialized = false;
}

int debugging_visualization_debug_shapes_create(debugging_visualization_debug_shapes_handle_t* out_handle, 
                                                  const debugging_visualization_debug_shapes_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_debug_shapes_ctx.initialized) {
        return -2;
    }

    if (g_debug_shapes_ctx.count >= g_debug_shapes_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_debug_shapes_ctx.count++;
    debugging_visualization_debug_shapes_internal_t* item = &g_debug_shapes_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->shape_capacity = MAX_DEBUG_SHAPES;
    item->shapes = calloc(item->shape_capacity, sizeof(debug_shape_t));
    item->shape_count = 0;
    
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void debugging_visualization_debug_shapes_destroy(debugging_visualization_debug_shapes_handle_t handle) {
    if (handle.id >= g_debug_shapes_ctx.count) {
        return;
    }

    free(g_debug_shapes_ctx.items[handle.id].shapes);
    g_debug_shapes_ctx.items[handle.id].initialized = false;
}

int debugging_visualization_debug_shapes_add_box(debugging_visualization_debug_shapes_handle_t handle,
                                                   vec3_t center, vec3_t half_extents,
                                                   quat_t rotation, vec4_t color, bool wireframe) {
    if (handle.id >= g_debug_shapes_ctx.count) return -1;
    debugging_visualization_debug_shapes_internal_t* item = &g_debug_shapes_ctx.items[handle.id];
    if (item->shape_count >= item->shape_capacity) return -2;
    
    item->shapes[item->shape_count++] = (debug_shape_t){
        SHAPE_BOX, center, rotation, half_extents, color, wireframe, ONE_FRAME_DURATION
    };
    item->dirty = true;
    return 0;
}

int debugging_visualization_debug_shapes_add_sphere(debugging_visualization_debug_shapes_handle_t handle,
                                                      vec3_t center, float radius,
                                                      vec4_t color, bool wireframe) {
    if (handle.id >= g_debug_shapes_ctx.count) return -1;
    debugging_visualization_debug_shapes_internal_t* item = &g_debug_shapes_ctx.items[handle.id];
    if (item->shape_count >= item->shape_capacity) return -2;
    
    item->shapes[item->shape_count++] = (debug_shape_t){
        SHAPE_SPHERE, center, (quat_t){0,0,0,1}, (vec3_t){radius, radius, radius}, color, wireframe, ONE_FRAME_DURATION
    };
    item->dirty = true;
    return 0;
}

int debugging_visualization_debug_shapes_update(debugging_visualization_debug_shapes_handle_t handle, 
                                                  const void* data, size_t size) {
    // Treat data as dt for updating durations
    if (handle.id >= g_debug_shapes_ctx.count) {
        return -1;
    }
    
    float dt = 0.016f;
    if (data && size == sizeof(float)) {
        dt = *(const float*)data;
    }

    debugging_visualization_debug_shapes_internal_t* item = &g_debug_shapes_ctx.items[handle.id];
    
    // Remove expired shapes (almost all are 1-frame duration)
    uint32_t alive = 0;
    for (uint32_t i = 0; i < item->shape_count; i++) {
        if (item->shapes[i].duration > 0.0f) {
            item->shapes[i].duration -= dt;
            if (item->shapes[i].duration > 0.0f) {
                // Keep
                if (i != alive) {
                    item->shapes[alive] = item->shapes[i];
                }
                alive++;
            }
        }
    }
    item->shape_count = alive;
    item->dirty = true;

    return 0;
}

bool debugging_visualization_debug_shapes_is_valid(debugging_visualization_debug_shapes_handle_t handle) {
    if (handle.id >= g_debug_shapes_ctx.count) {
        return false;
    }
    return g_debug_shapes_ctx.items[handle.id].initialized;
}

int debugging_visualization_debug_shapes_get_info(debugging_visualization_debug_shapes_handle_t handle, 
                                                    debugging_visualization_debug_shapes_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_debug_shapes_ctx.count) {
        return -2;
    }

    const debugging_visualization_debug_shapes_internal_t* item = &g_debug_shapes_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void debugging_visualization_debug_shapes_mark_dirty(debugging_visualization_debug_shapes_handle_t handle) {
    if (handle.id < g_debug_shapes_ctx.count) {
        g_debug_shapes_ctx.items[handle.id].dirty = true;
    }
}

int debugging_visualization_debug_shapes_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_debug_shapes_ctx.count; i++) {
        debugging_visualization_debug_shapes_internal_t* item = &g_debug_shapes_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t debugging_visualization_debug_shapes_get_count(void) {
    return g_debug_shapes_ctx.count;
}

size_t debugging_visualization_debug_shapes_get_memory_usage(void) {
    size_t total = sizeof(g_debug_shapes_ctx);
    for (uint32_t i = 0; i < g_debug_shapes_ctx.count; i++) {
        total += g_debug_shapes_ctx.items[i].shape_capacity * sizeof(debug_shape_t);
    }
    return total;
}

void debugging_visualization_debug_shapes_debug_print(void) {
    // Debug output
}

/* End of debug_shapes.c */
