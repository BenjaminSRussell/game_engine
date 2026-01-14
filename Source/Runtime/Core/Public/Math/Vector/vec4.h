/**
 * @file vec4.h
 * @brief 4D Vector mathematics
 */

#ifndef VOXELFORGE_MATH_VEC4_H
#define VOXELFORGE_MATH_VEC4_H

#include "Core/Public/Math/Vector/vec3.h"
#include "Core/Public/core_types.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Types
// ============================================================================

typedef struct Vec4 {
  f32 x, y, z, w;
} Vec4;

typedef struct Vec4i {
  i32 x, y, z, w;
} Vec4i;

// ============================================================================
// Construction
// ============================================================================

VF_INLINE Vec4 vec4_create(f32 x, f32 y, f32 z, f32 w) {
  return (Vec4){x, y, z, w};
}

VF_INLINE Vec4 vec4_zero(void) { return (Vec4){0.0f, 0.0f, 0.0f, 0.0f}; }

VF_INLINE Vec4 vec4_one(void) { return (Vec4){1.0f, 1.0f, 1.0f, 1.0f}; }

VF_INLINE Vec4 vec4_splat(f32 v) { return (Vec4){v, v, v, v}; }

VF_INLINE Vec4 vec4_from_vec3(Vec3 v, f32 w) {
  return (Vec4){v.x, v.y, v.z, w};
}

VF_INLINE Vec3 vec4_to_vec3(Vec4 v) { return (Vec3){v.x, v.y, v.z}; }

// ============================================================================
// Arithmetic
// ============================================================================

VF_INLINE Vec4 vec4_add(Vec4 a, Vec4 b) {
  return (Vec4){a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

VF_INLINE Vec4 vec4_sub(Vec4 a, Vec4 b) {
  return (Vec4){a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

VF_INLINE Vec4 vec4_mul(Vec4 a, Vec4 b) {
  return (Vec4){a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}

VF_INLINE Vec4 vec4_scale(Vec4 v, f32 s) {
  return (Vec4){v.x * s, v.y * s, v.z * s, v.w * s};
}

VF_INLINE Vec4 vec4_neg(Vec4 v) { return (Vec4){-v.x, -v.y, -v.z, -v.w}; }

// ============================================================================
// Vector Operations
// ============================================================================

VF_INLINE f32 vec4_dot(Vec4 a, Vec4 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

VF_INLINE f32 vec4_length_sq(Vec4 v) { return vec4_dot(v, v); }

VF_INLINE f32 vec4_length(Vec4 v) { return sqrtf(vec4_length_sq(v)); }

VF_INLINE Vec4 vec4_normalize(Vec4 v) {
  f32 len = vec4_length(v);
  if (len > 0.0001f) {
    return vec4_scale(v, 1.0f / len);
  }
  return vec4_zero();
}

// ============================================================================
// Interpolation
// ============================================================================

VF_INLINE Vec4 vec4_lerp(Vec4 a, Vec4 b, f32 t) {
  return (Vec4){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}

// ============================================================================
// Comparison
// ============================================================================

VF_INLINE b8 vec4_equals(Vec4 a, Vec4 b, f32 epsilon) {
  return fabsf(a.x - b.x) < epsilon && fabsf(a.y - b.y) < epsilon &&
         fabsf(a.z - b.z) < epsilon && fabsf(a.w - b.w) < epsilon;
}

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_MATH_VEC4_H
