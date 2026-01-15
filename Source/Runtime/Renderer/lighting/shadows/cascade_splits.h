/*
 * cascade_splits.h
 * Cascade split calculation for shadow mapping
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_CASCADE_SPLITS_H
#define LIGHTING_CASCADE_SPLITS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct cascade_camera {
    float position[3];
    float forward[3];
    float up[3];
    float near_plane;
    float far_plane;
    float fov;
    float aspect_ratio;
} cascade_camera_t;

typedef struct cascade_split_info {
    float view_proj[16];
    float split_near;
    float split_far;
} cascade_split_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * @brief Calculate split planes and view-projection matrices for cascaded shadow mapping
 * 
 * @param camera View camera parameters
 * @param light_dir Directional light direction (normalized)
 * @param cascade_count Number of cascades
 * @param shadow_distance Max shadow distance
 * @param lambda Split distribution factor (0.0 = uniform, 1.0 = logarithmic)
 * @param out_cascades Array to store cascade info (must be at least cascade_count size)
 */
void cascade_splits_calculate(const cascade_camera_t* camera,
                             const float* light_dir,
                             uint32_t cascade_count,
                             float shadow_distance,
                             float lambda,
                             cascade_split_info_t* out_cascades);

/**
 * @brief Calculate a single orthographic projection matrix for a cascade
 * 
 * @param camera View camera
 * @param light_dir Light direction
 * @param near_z Cascade near plane
 * @param far_z Cascade far plane
 * @param out_matrix Output 4x4 matrix
 */
void cascade_calculate_matrix(const cascade_camera_t* camera,
                             const float* light_dir,
                             float near_z,
                             float far_z,
                             float* out_matrix);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_CASCADE_SPLITS_H */
