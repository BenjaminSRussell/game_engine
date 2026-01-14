/**
 * @file vec3.h
 * @brief 3D Vector mathematics
 */

#ifndef VOXELFORGE_MATH_VEC3_H
#define VOXELFORGE_MATH_VEC3_H

#include "Core/Public/core_types.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Types
// ============================================================================

typedef struct Vec3 {
  f32 x, y, z;
} Vec3;

typedef struct Vec3i {
  i32 x, y, z;
} Vec3i;

// ============================================================================
// Construction
// ============================================================================

VF_INLINE Vec3 vec3_create(f32 x, f32 y, f32 z) { return (Vec3){x, y, z}; }

VF_INLINE Vec3 vec3_zero(void) { return (Vec3){0.0f, 0.0f, 0.0f}; }

VF_INLINE Vec3 vec3_one(void) { return (Vec3){1.0f, 1.0f, 1.0f}; }

VF_INLINE Vec3 vec3_splat(f32 v) { return (Vec3){v, v, v}; }

VF_INLINE Vec3 vec3_up(void) { return (Vec3){0.0f, 1.0f, 0.0f}; }

VF_INLINE Vec3 vec3_right(void) { return (Vec3){1.0f, 0.0f, 0.0f}; }

VF_INLINE Vec3 vec3_forward(void) { return (Vec3){0.0f, 0.0f, -1.0f}; }

// ============================================================================
// Arithmetic
// ============================================================================

VF_INLINE Vec3 vec3_add(Vec3 a, Vec3 b) {
  return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

VF_INLINE Vec3 vec3_sub(Vec3 a, Vec3 b) {
  return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

VF_INLINE Vec3 vec3_mul(Vec3 a, Vec3 b) {
  return (Vec3){a.x * b.x, a.y * b.y, a.z * b.z};
}

VF_INLINE Vec3 vec3_div(Vec3 a, Vec3 b) {
  return (Vec3){a.x / b.x, a.y / b.y, a.z / b.z};
}

VF_INLINE Vec3 vec3_scale(Vec3 v, f32 s) {
  return (Vec3){v.x * s, v.y * s, v.z * s};
}

VF_INLINE Vec3 vec3_neg(Vec3 v) { return (Vec3){-v.x, -v.y, -v.z}; }

// ============================================================================
// Vector Operations
// ============================================================================

VF_INLINE f32 vec3_dot(Vec3 a, Vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

VF_INLINE Vec3 vec3_cross(Vec3 a, Vec3 b) {
  return (Vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x};
}

VF_INLINE f32 vec3_length_sq(Vec3 v) { return vec3_dot(v, v); }

VF_INLINE f32 vec3_length(Vec3 v) { return sqrtf(vec3_length_sq(v)); }

VF_INLINE Vec3 vec3_normalize(Vec3 v) {
  f32 len = vec3_length(v);
  if (len > 0.0001f) {
    return vec3_scale(v, 1.0f / len);
  }
  return vec3_zero();
}

VF_INLINE f32 vec3_distance(Vec3 a, Vec3 b) {
  return vec3_length(vec3_sub(b, a));
}

VF_INLINE Vec3 vec3_reflect(Vec3 v, Vec3 n) {
  f32 d = 2.0f * vec3_dot(v, n);
  return vec3_sub(v, vec3_scale(n, d));
}

// ============================================================================
// Interpolation
// ============================================================================

VF_INLINE Vec3 vec3_lerp(Vec3 a, Vec3 b, f32 t) {
  return (Vec3){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t};
}

// ============================================================================
// Utility
// ============================================================================

VF_INLINE Vec3 vec3_min(Vec3 a, Vec3 b) {
  return (Vec3){a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y,
                a.z < b.z ? a.z : b.z};
}

VF_INLINE Vec3 vec3_max(Vec3 a, Vec3 b) {
  return (Vec3){a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y,
                a.z > b.z ? a.z : b.z};
}

VF_INLINE Vec3 vec3_clamp(Vec3 v, Vec3 lo, Vec3 hi) {
  return vec3_min(vec3_max(v, lo), hi);
}

VF_INLINE Vec3 vec3_abs(Vec3 v) {
  return (Vec3){fabsf(v.x), fabsf(v.y), fabsf(v.z)};
}

// ============================================================================
// Comparison
// ============================================================================

VF_INLINE b8 vec3_equals(Vec3 a, Vec3 b, f32 epsilon) {
  return fabsf(a.x - b.x) < epsilon && fabsf(a.y - b.y) < epsilon &&
         fabsf(a.z - b.z) < epsilon;
}

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_MATH_VEC3_H
