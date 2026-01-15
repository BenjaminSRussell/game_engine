/*
 * froxel_grid.h
 * Froxel volume math and management
 */

#ifndef LIGHTING_FROXEL_GRID_H
#define LIGHTING_FROXEL_GRID_H

#include "include/math/mat4.h"
#include "include/math/vec3.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct froxel_config {
  uint32_t width;
  uint32_t height;
  uint32_t depth;
  float near_plane;
  float far_plane;
  bool adaptive_slicing;    // Enable adaptive depth slicing
  float slice_distribution; // 0.0 = linear, 1.0 = full logarithmic
} froxel_config_t;

typedef struct froxel_grid {
  uint32_t width;
  uint32_t height;
  uint32_t depth;

  float near_plane;
  float far_plane;

  // Cached values for coordinate conversion
  float z_log_scale;
  float z_log_bias;

  // Dynamic resolution support
  float resolution_scale; // 1.0 = full res, 0.5 = half res
  uint32_t effective_width;
  uint32_t effective_height;

  // Slice distribution
  float slice_distribution;

  // Bounds in view space
  vec3_t bounds_min;
  vec3_t bounds_max;
} froxel_grid_t;

/**
 * Initializes a froxel grid structure from configuration.
 */
void froxel_grid_init_with_config(froxel_grid_t *grid,
                                  const froxel_config_t *config);

/**
 * Initializes a froxel grid structure (legacy).
 */
void froxel_grid_init(froxel_grid_t *grid, uint32_t w, uint32_t h, uint32_t d,
                      float near_p, float far_p);

/**
 * Updates the froxel grid resolution scale (for dynamic resolution).
 */
void froxel_grid_set_resolution_scale(froxel_grid_t *grid, float scale);

/**
 * Updates near/far planes (e.g., when camera changes).
 */
void froxel_grid_update_planes(froxel_grid_t *grid, float near_p, float far_p);

/**
 * Converts a linear view-space Z to a normalized froxel Z index [0, 1].
 */
float froxel_grid_z_to_slice(const froxel_grid_t *grid, float z);

/**
 * Converts a froxel Z slice [0, 1] to a linear view-space Z.
 */
float froxel_grid_slice_to_z(const froxel_grid_t *grid, float slice);

/**
 * Converts screen-space coordinates (normalized [0,1]) and depth to froxel
 * indices. Returns false if out of bounds.
 */
bool froxel_grid_screen_to_froxel(const froxel_grid_t *grid, float screen_x,
                                  float screen_y, float view_z, uint32_t *out_x,
                                  uint32_t *out_y, uint32_t *out_z);

/**
 * Converts froxel indices to normalized screen coordinates [0,1] and view-space
 * Z.
 */
void froxel_grid_froxel_to_screen(const froxel_grid_t *grid, uint32_t x,
                                  uint32_t y, uint32_t z, float *out_screen_x,
                                  float *out_screen_y, float *out_view_z);

/**
 * Gets the world-space position of a froxel's center.
 */
vec3_t froxel_grid_get_world_pos(const froxel_grid_t *grid, uint32_t x,
                                 uint32_t y, uint32_t z, mat4 inv_view_proj);

/**
 * Gets the view-space bounds of a specific froxel.
 */
void froxel_grid_get_froxel_bounds(const froxel_grid_t *grid, uint32_t x,
                                   uint32_t y, uint32_t z, vec3_t *out_min,
                                   vec3_t *out_max);

/**
 * Tests if a point in view space is inside a froxel.
 */
bool froxel_grid_point_in_froxel(const froxel_grid_t *grid, uint32_t x,
                                 uint32_t y, uint32_t z, vec3_t view_pos);

/**
 * Tests if a sphere intersects with a froxel.
 */
bool froxel_grid_sphere_intersects_froxel(const froxel_grid_t *grid, uint32_t x,
                                          uint32_t y, uint32_t z,
                                          vec3_t sphere_center,
                                          float sphere_radius);

/**
 * Gets the total number of froxels in the grid.
 */
uint32_t froxel_grid_count(const froxel_grid_t *grid);

/**
 * Gets linear index from 3D froxel coordinates.
 */
uint32_t froxel_grid_index(const froxel_grid_t *grid, uint32_t x, uint32_t y,
                           uint32_t z);

/**
 * Converts linear index back to 3D coordinates.
 */
void froxel_grid_coords_from_index(const froxel_grid_t *grid, uint32_t index,
                                   uint32_t *out_x, uint32_t *out_y,
                                   uint32_t *out_z);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_FROXEL_GRID_H */
