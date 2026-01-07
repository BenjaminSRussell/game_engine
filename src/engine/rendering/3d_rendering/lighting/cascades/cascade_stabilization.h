/*
 * cascade_stabilization.h
 * Temporal stabilization
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_CASCADE_STABILIZATION_H
#define LIGHTING_CASCADE_STABILIZATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Stabilize view-projection matrix by snapping to shadow map texels
 * Prevents shimmering when camera moves.
 * 
 * @param view_proj_matrix INPUT/OUTPUT: 4x4 matrix to modify (column-major)
 * @param shadow_map_size Resolution of the shadow map side (e.g. 1024)
 */
void cascade_stabilize_matrix(float* view_proj_matrix, uint32_t shadow_map_size);

/**
 * @brief Round camera position to nearest stable grid point
 * Alternative to matrix snapping
 * 
 * @param camera_pos INPUT/OUTPUT: 3 float position array
 * @param shadow_unit_per_texel World units covered by one shadow texel
 */
void cascade_apply_camera_rounding(float* camera_pos, float shadow_unit_per_texel);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_CASCADE_STABILIZATION_H */
