/*
 * cascade_stabilization.c
 * Temporal stabilization logic 
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "cascade_stabilization.h"
#include <math.h>

/* ============================================================================
 * IMPLEMENTATION
 * ============================================================================ */

void cascade_stabilize_matrix(float* view_proj_matrix, uint32_t shadow_map_size) {
    if (!view_proj_matrix || shadow_map_size == 0) return;
    
    // Extract shadow map translation from view-proj matrix (assuming orthographic)
    // For orthographic, m[12], m[13], m[14] are translations
    
    // We need to snap the translation to texel increments
    // 1 texel in world space = world_width / shadow_map_size
    // But working in NDC/Projection space? 
    // Usually easier to snap the camera position *before* building the view matrix. 
    // Here we might be adjusting the *projection* matrix or the final product.
    
    // Standard approach:
    // 1. Transform origin (0,0,0) by view_proj to get current shadow origin in clip space
    // 2. Find offset needed to align to pixel center
    // 3. Apply offset to matrix
    
    float shadow_origin_x = view_proj_matrix[12];
    float shadow_origin_y = view_proj_matrix[13];
    
    float shadow_map_size_f = (float)shadow_map_size;
    
    // Clip space is [-1, 1], so width is 2
    // Texel size in clip space = 2.0 / shadow_map_size
    float texel_size = 2.0f / shadow_map_size_f;
    
    // Quantize origin to texel size
    float start_x = roundf(shadow_origin_x / texel_size) * texel_size;
    float start_y = roundf(shadow_origin_y / texel_size) * texel_size;
    
    // Apply offset
    view_proj_matrix[12] = start_x;
    view_proj_matrix[13] = start_y;
}

void cascade_apply_camera_rounding(float* camera_pos, float shadow_unit_per_texel) {
    if (!camera_pos || shadow_unit_per_texel <= 0.0001f) return;
    
    // Snap camera position to grid steps
    camera_pos[0] = roundf(camera_pos[0] / shadow_unit_per_texel) * shadow_unit_per_texel;
    camera_pos[1] = roundf(camera_pos[1] / shadow_unit_per_texel) * shadow_unit_per_texel;
    camera_pos[2] = roundf(camera_pos[2] / shadow_unit_per_texel) * shadow_unit_per_texel;
}
