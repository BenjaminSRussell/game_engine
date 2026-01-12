// math/mat4.h
//
// Purpose: Defines the `Mat4` structure for 4x4 matrices and provides a
// comprehensive API for common 3D linear algebra operations. This header is
// crucial for handling transformations (translation, rotation, scaling), camera
// projections (perspective, orthographic), and view matrices (look-at) within
// the game engine. Matrices are stored in column-major order, consistent with
// OpenGL conventions.
//
// Public APIs:
// - `Mat4`: A union representing a 4x4 floating-point matrix, allowing access
//   via a 1D array (`m`), 2D array (`data`), or named members (`m00`, `m10`,
//   etc.). It is 16-byte aligned for performance.
// - `mat4_identity`: Returns an identity matrix.
// - `mat4_zero`: Returns a zero matrix.
// - `mat4_mul`: Multiplies two 4x4 matrices.
// - `mat4_translate`, `mat4_scale`: Creates transformation matrices for
// translation and scaling.
// - `mat4_rotate_x`, `mat4_rotate_y`, `mat4_rotate_z`, `mat4_rotate`: Creates
// rotation matrices
//   around an axis or arbitrary axis-angle.
// - `mat4_look_at`: Creates a view matrix for a camera, looking from an eye
// position at a target.
// - `mat4_perspective`, `mat4_ortho`: Creates projection matrices for
// perspective and orthographic views.
// - `mat4_inverse`: Computes the inverse of a matrix.
// - `mat4_transform_vec3`: Transforms a 3D vector by the matrix (assumes w=0).
// - `mat4_transform_point`: Transforms a 3D point by the matrix (assumes w=1).
//
// Ownership: `Mat4` instances are value types. Functions return new `Mat4`
// instances or modify existing ones passed by pointer, but they do not manage
// dynamic memory.
//
// Invariants:
// - Matrices are assumed to be 4x4 floating-point matrices.
// - Input vectors for transformations are assumed to be `Vec3` types (from
// `vec3.h`).
// - Rotation angles are expected in radians.
// - Operations like inversion may fail for singular matrices.
//
#ifndef MAT4_H
#define MAT4_H

#include "../common.h"
#include "vec3.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// Matrix validation structure
typedef struct {
  bool has_nan;
  bool has_inf;
  bool is_valid;
  bool is_orthogonal;
  bool is_identity;
} Mat4Validation;

// Matrix statistics structure
typedef struct {
  u64 operations_count;
  u64 simd_operations_count;
  u64 cache_hits;
  u64 cache_misses;
  f64 total_operation_time;
} Mat4Statistics;

// Matrix decomposition structure
typedef struct {
  Vec3 translation;
  Vec3 rotation; // Euler angles in radians
  Vec3 scale;
} Mat4Decomposition;

// Column-major 4x4 matrix (OpenGL style)
typedef union Mat4 {
  f32 m[16];
  f32 data[4][4];
  struct {
    f32 m00, m10, m20, m30;
    f32 m01, m11, m21, m31;
    f32 m02, m12, m22, m32;
    f32 m03, m13, m23, m33;
  };
} K_ALIGN(16) Mat4;

INLINE Mat4 mat4_identity(void) {
  Mat4 m = {0};
  m.m00 = m.m11 = m.m22 = m.m33 = 1.0f;
  return m;
}

INLINE Mat4 mat4_zero(void) {
  Mat4 m = {0};
  return m;
}

Mat4 mat4_mul(Mat4 a, Mat4 b);
Mat4 mat4_translate(Vec3 v);
Mat4 mat4_scale(Vec3 v);
Mat4 mat4_rotate_x(f32 angle);
Mat4 mat4_rotate_y(f32 angle);
Mat4 mat4_rotate_z(f32 angle);
Mat4 mat4_rotate(Vec3 axis, f32 angle);
Mat4 mat4_look_at(Vec3 eye, Vec3 target, Vec3 up);
Mat4 mat4_perspective(f32 fov, f32 aspect, f32 near, f32 far);
Mat4 mat4_ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);
Mat4 mat4_inverse(Mat4 m);
Vec3 mat4_transform_vec3(Mat4 m, Vec3 v);
Vec3 mat4_transform_point(Mat4 m, Vec3 v);

// Matrix decomposition system
Mat4Decomposition mat4_decompose(Mat4 m);
Mat4 mat4_compose(Mat4Decomposition decomp);

// Matrix interpolation system
Mat4 mat4_lerp(Mat4 a, Mat4 b, f32 t);
Mat4 mat4_slerp(Mat4 a, Mat4 b, f32 t);

// Matrix validation system
Mat4Validation mat4_validate(Mat4 m);
bool mat4_is_valid(Mat4 m);
bool mat4_has_nan(Mat4 m);
bool mat4_has_inf(Mat4 m);
bool mat4_is_equal(Mat4 a, Mat4 b, f32 tolerance);
Mat4 mat4_sanitize(Mat4 m);

// Matrix cache system
bool mat4_cache_get(Mat4 m, Mat4 *out);
void mat4_cache_put(Mat4 m);
void mat4_cache_clear(void);

// Matrix statistics system
void mat4_stats_reset(void);
Mat4Statistics mat4_stats_get(void);
void mat4_stats_increment_operation(bool used_simd);

// Additional matrix operations
f32 mat4_determinant(Mat4 m);
Mat4 mat4_transpose(Mat4 m);

// SIMD support functions
bool mat4_has_avx2_support(void);
bool mat4_has_sse2_support(void);

// Optimized versions that automatically use SIMD when available
Mat4 mat4_mul_optimized(Mat4 a, Mat4 b);
Vec3 mat4_transform_vec3_optimized(Mat4 m, Vec3 v);
Vec3 mat4_transform_point_optimized(Mat4 m, Vec3 v);

// Debug visualization helpers
const char *mat4_to_string(Mat4 m, char *buffer, size_t buffer_size);
void mat4_print(Mat4 m);

// Batch operations
void mat4_mul_batch_avx2(const Mat4 *a, const Mat4 *b, Mat4 *result,
                         size_t count);
void mat4_transform_vec3_batch(const Mat4 *matrices, const Vec3 *vectors,
                               Vec3 *result, size_t count);
void mat4_transform_point_batch(const Mat4 *matrices, const Vec3 *points,
                                Vec3 *result, size_t count);

#endif // MAT4_H
