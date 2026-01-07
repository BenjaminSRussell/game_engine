/*
 * froxel_grid.c
 * Froxel volume math and management
 */

#include "lighting/volumetric/froxel_grid.h"
#include "include/math/vec3.h"
#include "include/math/mat4.h"
#include <include/math/math.h>
#include <string.h>

void froxel_grid_init_with_config(froxel_grid_t* grid, const froxel_config_t* config) {
    memset(grid, 0, sizeof(froxel_grid_t));
    
    grid->width = config->width;
    grid->height = config->height;
    grid->depth = config->depth;
    grid->near_plane = config->near_plane;
    grid->far_plane = config->far_plane;
    grid->slice_distribution = config->slice_distribution;
    
    grid->resolution_scale = 1.0f;
    grid->effective_width = config->width;
    grid->effective_height = config->height;
    
    // Z slice mapping (exponential/logarithmic distribution)
    // Blend between linear (0.0) and logarithmic (1.0) based on slice_distribution
    grid->z_log_scale = (float)config->depth / logf(config->far_plane / config->near_plane);
    grid->z_log_bias = logf(config->near_plane);
    
    // Initialize bounds
    grid->bounds_min = vec3_zero();
    grid->bounds_max = vec3_zero();
}

void froxel_grid_init(froxel_grid_t* grid, uint32_t w, uint32_t h, uint32_t d, float near_p, float far_p) {
    froxel_config_t config = {
        .width = w,
        .height = h,
        .depth = d,
        .near_plane = near_p,
        .far_plane = far_p,
        .adaptive_slicing = false,
        .slice_distribution = 1.0f  // Full logarithmic by default
    };
    froxel_grid_init_with_config(grid, &config);
}

void froxel_grid_set_resolution_scale(froxel_grid_t* grid, float scale) {
    grid->resolution_scale = scale;
    grid->effective_width = (uint32_t)(grid->width * scale);
    grid->effective_height = (uint32_t)(grid->height * scale);
    
    // Ensure at least 1x1
    if (grid->effective_width < 1) grid->effective_width = 1;
    if (grid->effective_height < 1) grid->effective_height = 1;
}

void froxel_grid_update_planes(froxel_grid_t* grid, float near_p, float far_p) {
    grid->near_plane = near_p;
    grid->far_plane = far_p;
    
    // Recalculate Z mapping
    grid->z_log_scale = (float)grid->depth / logf(far_p / near_p);
    grid->z_log_bias = logf(near_p);
}

float froxel_grid_z_to_slice(const froxel_grid_t* grid, float z) {
    if (z <= grid->near_plane) return 0.0f;
    if (z >= grid->far_plane) return 1.0f;
    
    // Logarithmic distribution
    float slice = logf(z / grid->near_plane) * grid->z_log_scale;
    return slice / (float)grid->depth;
}

float froxel_grid_slice_to_z(const froxel_grid_t* grid, float slice) {
    // slice is [0, 1]
    if (slice <= 0.0f) return grid->near_plane;
    if (slice >= 1.0f) return grid->far_plane;
    return grid->near_plane * powf(grid->far_plane / grid->near_plane, slice);
}

bool froxel_grid_screen_to_froxel(const froxel_grid_t* grid, float screen_x, float screen_y, 
                                   float view_z, uint32_t* out_x, uint32_t* out_y, uint32_t* out_z) {
    // Screen coordinates are [0,1]
    if (screen_x < 0.0f || screen_x > 1.0f || screen_y < 0.0f || screen_y > 1.0f)
        return false;
    
    if (view_z < grid->near_plane || view_z > grid->far_plane)
        return false;
    
    *out_x = (uint32_t)(screen_x * grid->effective_width);
    *out_y = (uint32_t)(screen_y * grid->effective_height);
    
    float slice = froxel_grid_z_to_slice(grid, view_z);
    *out_z = (uint32_t)(slice * grid->depth);
    
    // Clamp to bounds
    if (*out_x >= grid->effective_width) *out_x = grid->effective_width - 1;
    if (*out_y >= grid->effective_height) *out_y = grid->effective_height - 1;
    if (*out_z >= grid->depth) *out_z = grid->depth - 1;
    
    return true;
}

void froxel_grid_froxel_to_screen(const froxel_grid_t* grid, uint32_t x, uint32_t y, uint32_t z,
                                   float* out_screen_x, float* out_screen_y, float* out_view_z) {
    // Get center of froxel
    *out_screen_x = ((float)x + 0.5f) / (float)grid->effective_width;
    *out_screen_y = ((float)y + 0.5f) / (float)grid->effective_height;
    
    float slice = ((float)z + 0.5f) / (float)grid->depth;
    *out_view_z = froxel_grid_slice_to_z(grid, slice);
}

vec3_t froxel_grid_get_world_pos(const froxel_grid_t* grid, uint32_t x, uint32_t y, uint32_t z, 
                                  mat4_t inv_view_proj) {
    float screen_x, screen_y, view_z;
    froxel_grid_froxel_to_screen(grid, x, y, z, &screen_x, &screen_y, &view_z);
    
    // Convert to NDC [-1, 1]
    float ndc_x = screen_x * 2.0f - 1.0f;
    float ndc_y = screen_y * 2.0f - 1.0f;
    
    // Convert view Z to NDC Z (assuming standard perspective projection [0,1] depth range)
    float n = grid->near_plane;
    float f = grid->far_plane;
    float ndc_z = f / (f - n) - (f * n) / (view_z * (f - n));
    
    // Transform to world space
    vec4_t ndc = {ndc_x, ndc_y, ndc_z, 1.0f};
    vec4_t world = mat4_mul_vec4(inv_view_proj, ndc);
    
    // Perspective divide
    if (fabsf(world.w) > 0.0001f) {
        return vec3_set(world.x / world.w, world.y / world.w, world.z / world.w);
    }
    
    return vec3_zero();
}

void froxel_grid_get_froxel_bounds(const froxel_grid_t* grid, uint32_t x, uint32_t y, uint32_t z,
                                    vec3_t* out_min, vec3_t* out_max) {
    // Get min corner (in screen space and view Z)
    float min_screen_x = (float)x / (float)grid->effective_width;
    float min_screen_y = (float)y / (float)grid->effective_height;
    float min_slice = (float)z / (float)grid->depth;
    float min_z = froxel_grid_slice_to_z(grid, min_slice);
    
    // Get max corner
    float max_screen_x = (float)(x + 1) / (float)grid->effective_width;
    float max_screen_y = (float)(y + 1) / (float)grid->effective_height;
    float max_slice = (float)(z + 1) / (float)grid->depth;
    float max_z = froxel_grid_slice_to_z(grid, max_slice);
    
    // In view space, the froxel is a frustum slice
    // For simplicity, we'll use screen-space bounds and Z range
    *out_min = vec3_set(min_screen_x, min_screen_y, min_z);
    *out_max = vec3_set(max_screen_x, max_screen_y, max_z);
}

bool froxel_grid_point_in_froxel(const froxel_grid_t* grid, uint32_t x, uint32_t y, uint32_t z,
                                  vec3_t view_pos) {
    vec3_t min, max;
    froxel_grid_get_froxel_bounds(grid, x, y, z, &min, &max);
    
    // Simple AABB test (simplified, actual test would need screen-space projection)
    return (view_pos.z >= min.z && view_pos.z <= max.z);
}

bool froxel_grid_sphere_intersects_froxel(const froxel_grid_t* grid, uint32_t x, uint32_t y, uint32_t z,
                                           vec3_t sphere_center, float sphere_radius) {
    vec3_t min, max;
    froxel_grid_get_froxel_bounds(grid, x, y, z, &min, &max);
    
    // Find closest point on AABB to sphere center
    float closest_z = fmaxf(min.z, fminf(sphere_center.z, max.z));
    
    // Simple Z-only test for now (proper test would be more complex)
    float dist_z = closest_z - sphere_center.z;
    return (dist_z * dist_z <= sphere_radius * sphere_radius);
}

uint32_t froxel_grid_count(const froxel_grid_t* grid) {
    return grid->effective_width * grid->effective_height * grid->depth;
}

uint32_t froxel_grid_index(const froxel_grid_t* grid, uint32_t x, uint32_t y, uint32_t z) {
    return z * (grid->effective_width * grid->effective_height) + y * grid->effective_width + x;
}

void froxel_grid_coords_from_index(const froxel_grid_t* grid, uint32_t index,
                                    uint32_t* out_x, uint32_t* out_y, uint32_t* out_z) {
    uint32_t slice_size = grid->effective_width * grid->effective_height;
    *out_z = index / slice_size;
    uint32_t remainder = index % slice_size;
    *out_y = remainder / grid->effective_width;
    *out_x = remainder % grid->effective_width;
}
