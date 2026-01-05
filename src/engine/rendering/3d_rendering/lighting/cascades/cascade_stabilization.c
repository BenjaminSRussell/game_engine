/*
 * cascade_stabilization.c
 * Stabilize cascade movement (prevent shimmering)
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "cascade_stabilization.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct mat4 {
    float m[16];
} mat4_t;

typedef struct cascade_stabilization_context {
    bool texel_snapping_enabled;
    bool initialized;
} cascade_stabilization_context_t;

static cascade_stabilization_context_t g_stabilization_ctx = {0};

/* ============================================================================
 * MATRIX HELPERS
 * ============================================================================ */

static void mat4_identity(mat4_t* mat) {
    memset(mat->m, 0, sizeof(mat->m));
    mat->m[0] = mat->m[5] = mat->m[10] = mat->m[15] = 1.0f;
}

/* ============================================================================
 * CASCADE STABILIZATION
 * ============================================================================ */

void lighting_cascade_stabilize_matrix(float* view_proj_matrix, uint32_t shadow_map_size) {
    if (!g_stabilization_ctx.texel_snapping_enabled || !view_proj_matrix) {
        return;
    }
    
    // Transform origin to shadow space
    float shadow_origin[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float transformed[4];
    
    // Matrix-vector multiply (simplified for origin)
    transformed[0] = view_proj_matrix[12];
    transformed[1] = view_proj_matrix[13];
    transformed[2] = view_proj_matrix[14];
    transformed[3] = view_proj_matrix[15];
    
    // Perspective divide
    if (fabsf(transformed[3]) > 0.0001f) {
        transformed[0] /= transformed[3];
        transformed[1] /= transformed[3];
    }
    
    // Convert to texel space
    transformed[0] *= (float)shadow_map_size / 2.0f;
    transformed[1] *= (float)shadow_map_size / 2.0f;
    
    // Round to nearest texel
    transformed[0] = floorf(transformed[0]);
    transformed[1] = floorf(transformed[1]);
    
    // Convert back to normalized space
    transformed[0] /= (float)shadow_map_size / 2.0f;
    transformed[1] /= (float)shadow_map_size / 2.0f;
    
    // Compute offset
    float offset_x = transformed[0] - (view_proj_matrix[12] / view_proj_matrix[15]);
    float offset_y = transformed[1] - (view_proj_matrix[13] / view_proj_matrix[15]);
    
    // Apply offset to translation component
    view_proj_matrix[12] += offset_x * view_proj_matrix[15];
    view_proj_matrix[13] += offset_y * view_proj_matrix[15];
}

void lighting_cascade_snap_to_texels(float* shadow_matrix, uint32_t shadow_map_size, 
                                     float texel_size) {
    if (!shadow_matrix) return;
    
    // Snap translation to texel grid
    float texel_world_size = texel_size;
    
    shadow_matrix[12] = floorf(shadow_matrix[12] / texel_world_size) * texel_world_size;
    shadow_matrix[13] = floorf(shadow_matrix[13] / texel_world_size) * texel_world_size;
    shadow_matrix[14] = floorf(shadow_matrix[14] / texel_world_size) * texel_world_size;
}

void lighting_cascade_enable_stabilization(bool enabled) {
    if (g_stabilization_ctx.initialized) {
        g_stabilization_ctx.texel_snapping_enabled = enabled;
    }
}

bool lighting_cascade_is_stabilization_enabled(void) {
    return g_stabilization_ctx.texel_snapping_enabled;
}

void lighting_cascade_compute_stable_bounds(const float* camera_pos, const float* light_dir,
                                            float cascade_radius, float* out_center) {
    if (!camera_pos || !light_dir || !out_center) return;
    
    // Round camera position to world grid
    float grid_size = cascade_radius * 2.0f / 1024.0f;  // Assuming 1024 shadow map
    
    out_center[0] = floorf(camera_pos[0] / grid_size) * grid_size;
    out_center[1] = floorf(camera_pos[1] / grid_size) * grid_size;
    out_center[2] = floorf(camera_pos[2] / grid_size) * grid_size;
}

/* ============================================================================
 * PUBLIC API (Compatibility)
 * ============================================================================ */

int lighting_cascade_stabilization_init(void) {
    if (g_stabilization_ctx.initialized) {
        return 0;
    }
    
    g_stabilization_ctx.texel_snapping_enabled = true;
    g_stabilization_ctx.initialized = true;
    
    return 0;
}

void lighting_cascade_stabilization_shutdown(void) {
    if (!g_stabilization_ctx.initialized) {
        return;
    }
    
    g_stabilization_ctx.initialized = false;
}

int lighting_cascade_stabilization_create(lighting_cascade_stabilization_handle_t* out_handle, 
                                          const lighting_cascade_stabilization_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    out_handle->id = 0;
    return 0;
}

void lighting_cascade_stabilization_destroy(lighting_cascade_stabilization_handle_t handle) {
    (void)handle;
}

int lighting_cascade_stabilization_update(lighting_cascade_stabilization_handle_t handle, 
                                          const void* data, size_t size) {
    (void)handle; (void)data; (void)size;
    return 0;
}

bool lighting_cascade_stabilization_is_valid(lighting_cascade_stabilization_handle_t handle) {
    (void)handle;
    return g_stabilization_ctx.initialized;
}

int lighting_cascade_stabilization_get_info(lighting_cascade_stabilization_handle_t handle, 
                                            lighting_cascade_stabilization_info_t* out_info) {
    if (!out_info) return -1;
    out_info->id = handle.id;
    out_info->flags = 0;
    out_info->initialized = g_stabilization_ctx.initialized;
    return 0;
}

void lighting_cascade_stabilization_mark_dirty(lighting_cascade_stabilization_handle_t handle) {
    (void)handle;
}

int lighting_cascade_stabilization_process_pending(void) {
    return 0;
}

uint32_t lighting_cascade_stabilization_get_count(void) {
    return g_stabilization_ctx.initialized ? 1 : 0;
}

size_t lighting_cascade_stabilization_get_memory_usage(void) {
    return sizeof(cascade_stabilization_context_t);
}

void lighting_cascade_stabilization_debug_print(void) {
    // Debug output
}

/* End of cascade_stabilization.c */
