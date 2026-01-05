/*
 * cascade_selection.c
 * Runtime cascade selection
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "cascade_selection.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct cascade_selection_context {
    float cascade_splits[8];
    uint32_t cascade_count;
    bool initialized;
} cascade_selection_context_t;

static cascade_selection_context_t g_selection_ctx = {0};

/* ============================================================================
 * CASCADE SELECTION
 * ============================================================================ */

uint32_t lighting_cascade_select_cascade(float view_depth) {
    if (!g_selection_ctx.initialized || g_selection_ctx.cascade_count == 0) {
        return 0;
    }
    
    // Find which cascade this depth falls into
    for (uint32_t i = 0; i < g_selection_ctx.cascade_count; i++) {
        if (view_depth < g_selection_ctx.cascade_splits[i + 1]) {
            return i;
        }
    }
    
    // If beyond all cascades, use the last one
    return g_selection_ctx.cascade_count - 1;
}

uint32_t lighting_cascade_select_cascade_vec3(const float* view_pos) {
    if (!view_pos) return 0;
    
    // Assume view_pos.z is the depth in view space
    float depth = fabsf(view_pos[2]);
    return lighting_cascade_select_cascade(depth);
}

void lighting_cascade_set_splits(const float* splits, uint32_t count) {
    if (!g_selection_ctx.initialized || !splits || count == 0 || count > 8) {
        return;
    }
    
    g_selection_ctx.cascade_count = count;
    memcpy(g_selection_ctx.cascade_splits, splits, sizeof(float) * (count + 1));
}

float lighting_cascade_get_split_distance(uint32_t cascade_index) {
    if (cascade_index >= g_selection_ctx.cascade_count) {
        return 0.0f;
    }
    
    return g_selection_ctx.cascade_splits[cascade_index + 1];
}

bool lighting_cascade_is_in_range(float view_depth, uint32_t cascade_index) {
    if (cascade_index >= g_selection_ctx.cascade_count) {
        return false;
    }
    
    return view_depth >= g_selection_ctx.cascade_splits[cascade_index] &&
           view_depth < g_selection_ctx.cascade_splits[cascade_index + 1];
}

/* ============================================================================
 * PUBLIC API (Compatibility)
 * ============================================================================ */

int lighting_cascade_selection_init(void) {
    if (g_selection_ctx.initialized) {
        return 0;
    }
    
    g_selection_ctx.cascade_count = 4;
    memset(g_selection_ctx.cascade_splits, 0, sizeof(g_selection_ctx.cascade_splits));
    g_selection_ctx.initialized = true;
    
    return 0;
}

void lighting_cascade_selection_shutdown(void) {
    if (!g_selection_ctx.initialized) {
        return;
    }
    
    g_selection_ctx.initialized = false;
}

int lighting_cascade_selection_create(lighting_cascade_selection_handle_t* out_handle, 
                                      const lighting_cascade_selection_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    out_handle->id = 0;
    return 0;
}

void lighting_cascade_selection_destroy(lighting_cascade_selection_handle_t handle) {
    (void)handle;
}

int lighting_cascade_selection_update(lighting_cascade_selection_handle_t handle, 
                                      const void* data, size_t size) {
    (void)handle; (void)data; (void)size;
    return 0;
}

bool lighting_cascade_selection_is_valid(lighting_cascade_selection_handle_t handle) {
    (void)handle;
    return g_selection_ctx.initialized;
}

int lighting_cascade_selection_get_info(lighting_cascade_selection_handle_t handle, 
                                        lighting_cascade_selection_info_t* out_info) {
    if (!out_info) return -1;
    out_info->id = handle.id;
    out_info->flags = 0;
    out_info->initialized = g_selection_ctx.initialized;
    return 0;
}

void lighting_cascade_selection_mark_dirty(lighting_cascade_selection_handle_t handle) {
    (void)handle;
}

int lighting_cascade_selection_process_pending(void) {
    return 0;
}

uint32_t lighting_cascade_selection_get_count(void) {
    return g_selection_ctx.cascade_count;
}

size_t lighting_cascade_selection_get_memory_usage(void) {
    return sizeof(cascade_selection_context_t);
}

void lighting_cascade_selection_debug_print(void) {
    // Debug output
}

/* End of cascade_selection.c */
