// math/vec3.h
//
// Purpose: Defines the `Vec3` structure for 3D vectors and provides a
// comprehensive API for common 3D linear algebra operations. This header is
// fundamental for representing and manipulating positions, directions,
// velocities, and other 3D quantities within the game engine. It includes basic
// arithmetic, dot and cross products, length calculations, normalization,
// linear interpolation, and conversions to/from integer block positions, with
// optimizations for SIMD-friendly alignment.
//
// Public APIs:
// - `Vec3`: A union representing a 3-component floating-point vector, allowing
// access
//   via named members (`x`, `y`, `z`) or a 1D array (`data`). It is 16-byte
//   aligned to potentially leverage SIMD instructions.
// - `vec3`: A convenience constructor for creating a `Vec3` from three float
// values.
// - `vec3_zero`, `vec3_one`: Returns zero and one vectors, respectively.
// - `vec3_add`, `vec3_sub`, `vec3_mul`, `vec3_div`, `vec3_scale`: Basic vector
// arithmetic
//   (addition, subtraction, scalar multiplication/division, component-wise
//   multiplication).
// - `vec3_dot`, `vec3_cross`: Calculates the dot and cross products of two
// vectors.
// - `vec3_length_sq`, `vec3_length`: Calculates the squared Euclidean length
// and Euclidean length of a vector.
// - `vec3_normalize`: Returns a normalized (unit length) version of the vector.
// - `vec3_lerp`: Performs linear interpolation between two vectors.
// - `vec3_min`, `vec3_max`: Returns a vector with component-wise minimum or
// maximum values.
// - `vec3_from_block_pos`, `vec3_to_block_pos`: Converts between integer block
// coordinates
//   and `Vec3` floating-point coordinates.
//
// Ownership: `Vec3` instances are value types. Functions return new `Vec3`
// instances but do not manage dynamic memory.
//
// Invariants:
// - Vectors are assumed to be 3-component floating-point vectors.
// - `EPSILON` (from `common.h`) is used to handle floating-point precision
// issues,
//   especially in normalization.
// - The `K_ALIGN(16)` attribute suggests optimization for 16-byte boundaries.
//
#ifndef VEC3_H
#define VEC3_H

#include <common.h>
#include <math.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// Vector validation structure
typedef struct {
  bool has_nan;
  bool has_inf;
  bool is_valid;
} Vec3Validation;

// Vector statistics structure
typedef struct {
  u64 operations_count;
  u64 simd_operations_count;
  u64 cache_hits;
  u64 cache_misses;
  f64 total_operation_time;
} Vec3Statistics;

// Vector precision enum
typedef enum { VEC3_PRECISION_SINGLE, VEC3_PRECISION_DOUBLE } Vec3Precision;

// Use unified type from core/math/types.h
typedef vec3 Vec3;

typedef Vec3 vec3_t;

// Vector operations (all inline for performance)
INLINE Vec3 vec3_create(f32 x, f32 y, f32 z) {
  Vec3 v = {x, y, z};
  return v;
}

INLINE Vec3 vec3_zero(void) { return vec3_create(0.0f, 0.0f, 0.0f); }

INLINE Vec3 vec3_one(void) { return vec3_create(1.0f, 1.0f, 1.0f); }

INLINE Vec3 vec3_add(Vec3 a, Vec3 b) {
  return vec3_create(a.x + b.x, a.y + b.y, a.z + b.z);
}

INLINE Vec3 vec3_sub(Vec3 a, Vec3 b) {
  return vec3_create(a.x - b.x, a.y - b.y, a.z - b.z);
}

INLINE Vec3 vec3_mul(Vec3 v, f32 s) {
  return vec3_create(v.x * s, v.y * s, v.z * s);
}

INLINE Vec3 vec3_div(Vec3 v, f32 s) {
  f32 inv = 1.0f / s;
  return vec3_create(v.x * inv, v.y * inv, v.z * inv);
}

INLINE Vec3 vec3_scale(Vec3 a, Vec3 b) {
  return vec3_create(a.x * b.x, a.y * b.y, a.z * b.z);
}

INLINE f32 vec3_dot(Vec3 a, Vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

INLINE Vec3 vec3_cross(Vec3 a, Vec3 b) {
  return vec3_create(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                     a.x * b.y - a.y * b.x);
}

INLINE f32 vec3_length_sq(Vec3 v) { return vec3_dot(v, v); }

INLINE f32 vec3_length(Vec3 v) { return sqrtf(vec3_length_sq(v)); }

INLINE f32 vec3_distance(Vec3 a, Vec3 b) { return vec3_length(vec3_sub(a, b)); }

INLINE f32 vec3_distance_sq(Vec3 a, Vec3 b) {
  return vec3_length_sq(vec3_sub(a, b));
}

INLINE Vec3 vec3_normalize(Vec3 v) {
  f32 len = vec3_length(v);
  if (len < EPSILON)
    return vec3_zero();
  return vec3_div(v, len);
}

INLINE Vec3 vec3_lerp(Vec3 a, Vec3 b, f32 t) {
  return vec3_add(a, vec3_mul(vec3_sub(b, a), t));
}

INLINE Vec3 vec3_min(Vec3 a, Vec3 b) {
  return vec3_create(MIN(a.x, b.x), MIN(a.y, b.y), MIN(a.z, b.z));
}

INLINE Vec3 vec3_max(Vec3 a, Vec3 b) {
  return vec3_create(MAX(a.x, b.x), MAX(a.y, b.y), MAX(a.z, b.z));
}

// Block position helpers
INLINE Vec3 vec3_from_block_pos(i32 x, i32 y, i32 z) {
  return vec3_create((f32)x, (f32)y, (f32)z);
}

INLINE void vec3_to_block_pos(Vec3 v, i32 *x, i32 *y, i32 *z) {
  *x = (i32)floorf(v.x);
  *y = (i32)floorf(v.y);
  *z = (i32)floorf(v.z);
}

// Aliases for compatibility
INLINE Vec3 vec3_subtract(Vec3 a, Vec3 b) { return vec3_sub(a, b); }
INLINE f32 vec3_length_squared(Vec3 v) { return vec3_length_sq(v); }

// Direction helpers
INLINE Vec3 vec3_up(void) { return vec3_create(0.0f, 1.0f, 0.0f); }
INLINE Vec3 vec3_down(void) { return vec3_create(0.0f, -1.0f, 0.0f); }
INLINE Vec3 vec3_left(void) { return vec3_create(-1.0f, 0.0f, 0.0f); }
INLINE Vec3 vec3_right(void) { return vec3_create(1.0f, 0.0f, 0.0f); }
INLINE Vec3 vec3_forward(void) { return vec3_create(0.0f, 0.0f, 1.0f); }
INLINE Vec3 vec3_back(void) { return vec3_create(0.0f, 0.0f, -1.0f); }

// Additional utility functions
f32 vec3_angle_between(Vec3 a, Vec3 b);
f32 vec3_angle_between_normalized(Vec3 a, Vec3 b);
Vec3 vec3_reflect(Vec3 v, Vec3 normal);
Vec3 vec3_project(Vec3 v, Vec3 onto);
Vec3 vec3_project_plane(Vec3 v, Vec3 normal);
Vec3 vec3_orthogonal(Vec3 v, Vec3 reference);
bool vec3_is_equal(Vec3 a, Vec3 b, f32 tolerance);
bool vec3_is_zero(Vec3 v, f32 tolerance);
Vec3 vec3_abs(Vec3 v);
Vec3 vec3_clamp(Vec3 v, Vec3 min, Vec3 max);
Vec3 vec3_slerp(Vec3 a, Vec3 b, f32 t);
Vec3 vec3_nlerp(Vec3 a, Vec3 b, f32 t);
Vec3 vec3_face_forward(Vec3 n, Vec3 v, Vec3 reference);
Vec3 vec3_negate(Vec3 v);
Vec3 vec3_sign(Vec3 v);
Vec3 vec3_step(Vec3 edge, Vec3 v);
Vec3 vec3_smoothstep(Vec3 edge0, Vec3 edge1, Vec3 v);

// SIMD support functions
bool vec3_has_avx2_support(void);
bool vec3_has_sse2_support(void);

// Optimized versions that automatically use SIMD when available
Vec3 vec3_add_optimized(Vec3 a, Vec3 b);
Vec3 vec3_sub_optimized(Vec3 a, Vec3 b);
Vec3 vec3_mul_optimized(Vec3 v, f32 s);
f32 vec3_dot_optimized(Vec3 a, Vec3 b);
Vec3 vec3_cross_optimized(Vec3 a, Vec3 b);
Vec3 vec3_normalize_optimized(Vec3 v);
Vec3 vec3_lerp_optimized(Vec3 a, Vec3 b, f32 t);

// Vector validation system
Vec3Validation vec3_validate(Vec3 v);
bool vec3_is_valid(Vec3 v);
bool vec3_has_nan(Vec3 v);
bool vec3_has_inf(Vec3 v);
Vec3 vec3_sanitize(Vec3 v);

// Vector cache system
bool vec3_cache_get(Vec3 v, Vec3 *out);
void vec3_cache_put(Vec3 v);
void vec3_cache_clear(void);

// Vector statistics system
void vec3_stats_reset(void);
Vec3Statistics vec3_stats_get(void);
void vec3_stats_increment_operation(bool used_simd);

// Precision system
void vec3_set_precision(Vec3Precision precision);
Vec3Precision vec3_get_precision(void);

// Debug visualization helpers
const char *vec3_to_string(Vec3 v, char *buffer, size_t buffer_size);
void vec3_print(Vec3 v);

// Batch operations
void vec3_add_batch_avx2(const Vec3 *a, const Vec3 *b, Vec3 *result,
                         size_t count);
void vec3_normalize_batch_avx2(const Vec3 *input, Vec3 *output, size_t count);
// void vec3_transform_batch_avx2(const Vec3 *input, const Mat4 *transform, Vec3
// *output, size_t count); // Moved to mat4.h to avoid circular dependency
void vec3_distance_batch(const Vec3 *a, const Vec3 *b, f32 *distances,
                         size_t count);
void vec3_angle_batch(const Vec3 *a, const Vec3 *b, f32 *angles, size_t count);

#endif // VEC3_H
