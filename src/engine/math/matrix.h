#pragma once

#include "engine/include/common.h"
#include <math/mat4.h>

/**
 * =================================================================================================
 *                                 MATRIX OPERATIONS
 * =================================================================================================
 *
 * Purpose: Convenience header that aggregates matrix-related functionality.
 * Provides common matrix operations for 3D graphics and linear algebra.
 */

/* ===== MATRIX DECOMPOSITION ===== */

/**
 * Extract translation component from transformation matrix
 */
Vec3 matrix_get_translation(Mat4 *m);

/**
 * Extract scale component from transformation matrix
 */
Vec3 matrix_get_scale(Mat4 *m);

/**
 * Extract rotation (euler angles) from transformation matrix
 */
Vec3 matrix_get_rotation(Mat4 *m);

/* ===== COMMON TRANSFORMATIONS ===== */

/**
 * Create translation matrix
 */
Mat4 matrix_translate(Vec3 offset);

/**
 * Create rotation matrix from euler angles (in radians)
 */
Mat4 matrix_rotate_euler(Vec3 euler);

/**
 * Create rotation matrix around X axis
 */
Mat4 matrix_rotate_x(f32 angle);

/**
 * Create rotation matrix around Y axis
 */
Mat4 matrix_rotate_y(f32 angle);

/**
 * Create rotation matrix around Z axis
 */
Mat4 matrix_rotate_z(f32 angle);

/**
 * Create scale matrix
 */
Mat4 matrix_scale(Vec3 scale);

/**
 * Create perspective projection matrix
 */
Mat4 matrix_perspective(f32 fov, f32 aspect, f32 near, f32 far);

/**
 * Create orthographic projection matrix
 */
Mat4 matrix_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);

/**
 * Create look-at view matrix
 */
Mat4 matrix_look_at(Vec3 eye, Vec3 target, Vec3 up);

#endif /* MATRIX_H */
