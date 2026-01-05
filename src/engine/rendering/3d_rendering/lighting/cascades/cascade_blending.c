/*
 * cascade_blending.c
 * Blend between cascades
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "cascade_blending.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct cascade_blending_context {
    float blend_range;  // Percentage of cascade range to blend
    bool enabled;
    bool initialized;
} cascade_blending_context_t;

static cascade_blending_context_t g_blending_ctx = {0};

/* ============================================================================
 * CASCADE BLENDING
 * ============================================================================ */

float lighting_cascade_compute_blend_factor(float view_depth, float split_near, float split_far) {
    if (!g_blending_ctx.enabled) {
        return 0.0f;
    }
    
    float range = split_far - split_near;
    float blend_distance = range * g_blending_ctx.blend_range;
    
    // Blend in the last portion of the cascade
    float blend_start = split_far - blend_distance;
    
    if (view_depth < blend_start) {
        return 0.0f;  // No blending
    }
    
    if (view_depth >= split_far) {
        return 1.0f;  // Full blend to next cascade
    }
    
    // Linear blend
    float factor = (view_depth - blend_start) / blend_distance;
    return fmaxf(0.0f, fminf(1.0f, factor));
}

float lighting_cascade_blend_shadows(float shadow0, float shadow1, float blend_factor) {
    // Linear interpolation between two shadow values
    return shadow0 * (1.0f - blend_factor) + shadow1 * blend_factor;
}

void lighting_cascade_set_blend_range(float range) {
    if (g_blending_ctx.initialized) {
        g_blending_ctx.blend_range = fmaxf(0.0f, fminf(1.0f, range));
    }
}

float lighting_cascade_get_blend_range(void) {
    return g_blending_ctx.blend_range;
}

void lighting_cascade_enable_blending(bool enabled) {
    if (g_blending_ctx.initialized) {
        g_blending_ctx.enabled = enabled;
    }
}

bool lighting_cascade_is_blending_enabled(void) {
    return g_blending_ctx.enabled;
}

/* ============================================================================
 * PUBLIC API (Compatibility)
 * ============================================================================ */

int lighting_cascade_blending_init(void) {
    if (g_blending_ctx.initialized) {
        return 0;
    }
    
    g_blending_ctx.blend_range = 0.1f;  // 10% of cascade range
    g_blending_ctx.enabled = true;
    g_blending_ctx.initialized = true;
    
    return 0;
}

void lighting_cascade_blending_shutdown(void) {
    if (!g_blending_ctx.initialized) {
        return;
    }
    
    g_blending_ctx.initialized = false;
}

int lighting_cascade_blending_create(lighting_cascade_blending_handle_t* out_handle, 
                                     const lighting_cascade_blending_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    out_handle->id = 0;
    return 0;
}

void lighting_cascade_blending_destroy(lighting_cascade_blending_handle_t handle) {
    (void)handle;
}

int lighting_cascade_blending_update(lighting_cascade_blending_handle_t handle, 
                                     const void* data, size_t size) {
    (void)handle; (void)data; (void)size;
    return 0;
}

bool lighting_cascade_blending_is_valid(lighting_cascade_blending_handle_t handle) {
    (void)handle;
    return g_blending_ctx.initialized;
}

int lighting_cascade_blending_get_info(lighting_cascade_blending_handle_t handle, 
                                       lighting_cascade_blending_info_t* out_info) {
    if (!out_info) return -1;
    out_info->id = handle.id;
    out_info->flags = 0;
    out_info->initialized = g_blending_ctx.initialized;
    return 0;
}

void lighting_cascade_blending_mark_dirty(lighting_cascade_blending_handle_t handle) {
    (void)handle;
}

int lighting_cascade_blending_process_pending(void) {
    return 0;
}

uint32_t lighting_cascade_blending_get_count(void) {
    return g_blending_ctx.initialized ? 1 : 0;
}

size_t lighting_cascade_blending_get_memory_usage(void) {
    return sizeof(cascade_blending_context_t);
}

void lighting_cascade_blending_debug_print(void) {
    // Debug output
}

/* End of cascade_blending.c */
