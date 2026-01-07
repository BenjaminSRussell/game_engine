/*
 * debug_lines.c
 * Debug line rendering
 *
 * Part of the Debug Visualization subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements immediate mode line drawing
 */

#include "editor/debug_viz/debug_lines.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MAX_DEBUG_LINES 16384
#define DEBUG_LINES_DEFAULT_CAPACITY 1

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct vec4 {
    float x, y, z, w;
} vec4_t;

/* ============================================================================
 * DEBUG LINE TYPES
 * ============================================================================ */

typedef struct debug_line {
    vec3_t start;
    vec3_t end;
    vec4_t color;
    float duration;     // >0: persistent for N seconds, 0: one frame
} debug_line_t;

typedef struct debugging_visualization_debug_lines_internal {
    uint32_t id;
    uint32_t flags;
    
    debug_line_t* lines;
    uint32_t line_count;
    uint32_t line_capacity;
    
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} debugging_visualization_debug_lines_internal_t;

typedef struct debugging_visualization_debug_lines_context {
    debugging_visualization_debug_lines_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} debugging_visualization_debug_lines_context_t;

static debugging_visualization_debug_lines_context_t g_debug_lines_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int debugging_visualization_debug_lines_init(void) {
    if (g_debug_lines_ctx.initialized) {
        return 0;
    }

    g_debug_lines_ctx.capacity = 1;
    g_debug_lines_ctx.items = calloc(g_debug_lines_ctx.capacity, sizeof(debugging_visualization_debug_lines_internal_t));
    if (!g_debug_lines_ctx.items) {
        return -1;
    }

    g_debug_lines_ctx.count = 0;
    g_debug_lines_ctx.initialized = true;

    return 0;
}

void debugging_visualization_debug_lines_shutdown(void) {
    if (!g_debug_lines_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_debug_lines_ctx.count; i++) {
        free(g_debug_lines_ctx.items[i].lines);
    }
    
    free(g_debug_lines_ctx.items);
    g_debug_lines_ctx.items = NULL;
    g_debug_lines_ctx.count = 0;
    g_debug_lines_ctx.capacity = 0;
    g_debug_lines_ctx.initialized = false;
}

int debugging_visualization_debug_lines_create(debugging_visualization_debug_lines_handle_t* out_handle, 
                                                 const debugging_visualization_debug_lines_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_debug_lines_ctx.initialized) {
        return -2;
    }

    if (g_debug_lines_ctx.count >= g_debug_lines_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_debug_lines_ctx.count++;
    debugging_visualization_debug_lines_internal_t* item = &g_debug_lines_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->line_capacity = MAX_DEBUG_LINES;
    item->lines = calloc(item->line_capacity, sizeof(debug_line_t));
    item->line_count = 0;
    
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void debugging_visualization_debug_lines_destroy(debugging_visualization_debug_lines_handle_t handle) {
    if (handle.id >= g_debug_lines_ctx.count) {
        return;
    }

    free(g_debug_lines_ctx.items[handle.id].lines);
    g_debug_lines_ctx.items[handle.id].initialized = false;
}

int debugging_visualization_debug_lines_add_line(debugging_visualization_debug_lines_handle_t handle,
                                                   vec3_t start, vec3_t end, vec4_t color, float duration) {
    if (handle.id >= g_debug_lines_ctx.count) return -1;
    
    debugging_visualization_debug_lines_internal_t* item = &g_debug_lines_ctx.items[handle.id];
    
    if (item->line_count < item->line_capacity) {
        item->lines[item->line_count++] = (debug_line_t){start, end, color, duration};
        item->dirty = true;
        return 0;
    }
    
    return -2; // Full
}

int debugging_visualization_debug_lines_update(debugging_visualization_debug_lines_handle_t handle, 
                                                 const void* data, size_t size) {
    // Treat data as dt for updating durations
    if (handle.id >= g_debug_lines_ctx.count) {
        return -1;
    }
    
    float dt = 0.016f;
    if (data && size == sizeof(float)) {
        dt = *(const float*)data;
    }

    debugging_visualization_debug_lines_internal_t* item = &g_debug_lines_ctx.items[handle.id];
    
    // Remove expired lines
    uint32_t alive = 0;
    for (uint32_t i = 0; i < item->line_count; i++) {
        if (item->lines[i].duration > 0.0f) {
            item->lines[i].duration -= dt;
            if (item->lines[i].duration > 0.0f) {
                // Keep
                if (i != alive) {
                    item->lines[alive] = item->lines[i];
                }
                alive++;
            }
        }
    }
    item->line_count = alive;
    item->dirty = true;

    return 0;
}

bool debugging_visualization_debug_lines_is_valid(debugging_visualization_debug_lines_handle_t handle) {
    if (handle.id >= g_debug_lines_ctx.count) {
        return false;
    }
    return g_debug_lines_ctx.items[handle.id].initialized;
}

int debugging_visualization_debug_lines_get_info(debugging_visualization_debug_lines_handle_t handle, 
                                                   debugging_visualization_debug_lines_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_debug_lines_ctx.count) {
        return -2;
    }

    const debugging_visualization_debug_lines_internal_t* item = &g_debug_lines_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void debugging_visualization_debug_lines_mark_dirty(debugging_visualization_debug_lines_handle_t handle) {
    if (handle.id < g_debug_lines_ctx.count) {
        g_debug_lines_ctx.items[handle.id].dirty = true;
    }
}

int debugging_visualization_debug_lines_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_debug_lines_ctx.count; i++) {
        debugging_visualization_debug_lines_internal_t* item = &g_debug_lines_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t debugging_visualization_debug_lines_get_count(void) {
    return g_debug_lines_ctx.count;
}

size_t debugging_visualization_debug_lines_get_memory_usage(void) {
    size_t total = sizeof(g_debug_lines_ctx);
    for (uint32_t i = 0; i < g_debug_lines_ctx.count; i++) {
        total += g_debug_lines_ctx.items[i].line_capacity * sizeof(debug_line_t);
    }
    return total;
}

void debugging_visualization_debug_lines_debug_print(void) {
    // Debug output
}

/* End of debug_lines.c */
