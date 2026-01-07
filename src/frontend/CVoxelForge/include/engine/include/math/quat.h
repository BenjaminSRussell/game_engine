// include/math/quat.h
//
// Purpose: Defines the `Quat` (Quaternion) structure and provides a
// comprehensive API for common quaternion operations. Quaternions are used to
// represent 3D rotations efficiently, avoiding issues like gimbal lock that can
// occur with Euler angles. This header is fundamental for handling orientations
// and rotations in the game engine's 3D math pipeline.
//
// Public APIs:
// - `Quat`: A union representing a quaternion with `w` (scalar) and `x, y, z`
// (vector) components.
//   It is 16-byte aligned for performance.
// - `quat`: A convenience constructor for creating a `Quat` from four float
// values.
// - `quat_identity`: Returns an identity quaternion (no rotation).
// - `quat_from_axis_angle`: Creates a quaternion from an axis and an angle of
// rotation.
// - `quat_from_euler`: Creates a quaternion from Euler angles (pitch, yaw,
// roll).
// - `quat_mul`: Multiplies two quaternions, representing a concatenation of
// rotations.
// - `quat_normalize`: Normalizes a quaternion to unit length.
// - `quat_rotate_vec3`: Rotates a 3D vector by the quaternion.
// - `quat_to_mat4`: Converts a quaternion to a 4x4 rotation matrix.
//
// Ownership: `Quat` instances are value types. Functions return new `Quat`
// instances or modify existing ones passed by pointer, but they do not manage
// dynamic memory.
//
// Invariants:
// - Quaternions are typically expected to be unit quaternions (normalized) for
// valid rotations.
// - Input vectors for rotation are assumed to be `Vec3` types (from `vec3.h`).
// - Rotation angles are expected in radians.
// - Euler angles for `quat_from_euler` are typically expected in a specific
// order (e.g., YXZ).
//
#ifndef QUAT_H
#define QUAT_H

#include <common.h>
#include "mat4.h"
#include "vec3.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// Quaternion validation structure
typedef struct {
    bool has_nan;
    bool has_inf;
    bool is_valid;
    bool is_normalized;
    f32 length;
} QuatValidation;

// Quaternion statistics structure
typedef struct {
    u64 operations_count;
    u64 simd_operations_count;
    u64 cache_hits;
    u64 cache_misses;
    u64 normalizations_count;
    f64 total_operation_time;
} QuatStatistics;

// Quaternion for rotations (w, x, y, z)
typedef union {
  struct {
    f32 w, x, y, z;
  };
  f32 data[4];
} ALIGN(16) Quat;

INLINE Quat quat(f32 w, f32 x, f32 y, f32 z) {
  Quat q = {w, x, y, z};
  return q;
}

INLINE Quat quat_identity(void) { return quat(1.0f, 0.0f, 0.0f, 0.0f); }

Quat quat_from_axis_angle(Vec3 axis, f32 angle);
Quat quat_look_rotation(Vec3 direction, Vec3 up);
Quat quat_from_euler(f32 pitch, f32 yaw, f32 roll);
Quat quat_mul(Quat a, Quat b);
Quat quat_normalize(Quat q);
Quat quat_conjugate(Quat q);
Quat quat_inverse(Quat q);
Vec3 quat_rotate_vec3(Quat q, Vec3 v);
Mat4 quat_to_mat4(Quat q);
Quat quat_slerp(Quat a, Quat b, f32 t);

// Additional quaternion operations
Quat quat_nlerp(Quat a, Quat b, f32 t);
f32 quat_dot(Quat a, Quat b);
f32 quat_length_sq(Quat q);
f32 quat_length(Quat q);
Quat quat_exp(Quat q);
Quat quat_log(Quat q);

// Quaternion validation system
QuatValidation quat_validate(Quat q);
bool quat_is_valid(Quat q);
bool quat_has_nan(Quat q);
bool quat_has_inf(Quat q);
bool quat_is_normalized(Quat q, f32 tolerance);
bool quat_is_equal(Quat a, Quat b, f32 tolerance);
Quat quat_sanitize(Quat q);

// Quaternion cache system
bool quat_cache_get(Quat q, Quat *out);
void quat_cache_put(Quat q);
void quat_cache_clear(void);

// Quaternion statistics system
void quat_stats_reset(void);
QuatStatistics quat_stats_get(void);
void quat_stats_increment_operation(bool used_simd);
void quat_stats_increment_normalization(void);

// SIMD support functions
bool quat_has_avx2_support(void);
bool quat_has_sse2_support(void);

// Optimized versions that automatically use SIMD when available
Quat quat_mul_optimized(Quat a, Quat b);
Quat quat_normalize_optimized(Quat q);
Vec3 quat_rotate_vec3_optimized(Quat q, Vec3 v);

// Debug visualization helpers
const char* quat_to_string(Quat q, char* buffer, size_t buffer_size);
void quat_print(Quat q);

// Batch operations
void quat_mul_batch_avx2(const Quat *a, const Quat *b, Quat *result, size_t count);
void quat_normalize_batch_avx2(const Quat *input, Quat *output, size_t count);
void quat_rotate_vec3_batch(const Quat *quaternions, const Vec3 *vectors, Vec3 *result, size_t count);

#endif // QUAT_H
