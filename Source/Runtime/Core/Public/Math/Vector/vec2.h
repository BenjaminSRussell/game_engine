/**
 * @file vec2.h
 * @brief 2D Vector mathematics
 */

#ifndef VOXELFORGE_MATH_VEC2_H
#define VOXELFORGE_MATH_VEC2_H

#include "Core/Public/core_types.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Types
// ============================================================================

typedef struct Vec2 {
  f32 x, y;
} Vec2;

typedef struct Vec2i {
  i32 x, y;
} Vec2i;

// ============================================================================
// Construction
// ============================================================================

VF_INLINE Vec2 vec2_create(f32 x, f32 y) { return (Vec2){x, y}; }

VF_INLINE Vec2 vec2_zero(void) { return (Vec2){0.0f, 0.0f}; }

VF_INLINE Vec2 vec2_one(void) { return (Vec2){1.0f, 1.0f}; }

VF_INLINE Vec2 vec2_splat(f32 v) { return (Vec2){v, v}; }

// ============================================================================
// Arithmetic
// ============================================================================

VF_INLINE Vec2 vec2_add(Vec2 a, Vec2 b) { return (Vec2){a.x + b.x, a.y + b.y}; }

VF_INLINE Vec2 vec2_sub(Vec2 a, Vec2 b) { return (Vec2){a.x - b.x, a.y - b.y}; }

VF_INLINE Vec2 vec2_mul(Vec2 a, Vec2 b) { return (Vec2){a.x * b.x, a.y * b.y}; }

VF_INLINE Vec2 vec2_div(Vec2 a, Vec2 b) { return (Vec2){a.x / b.x, a.y / b.y}; }

VF_INLINE Vec2 vec2_scale(Vec2 v, f32 s) { return (Vec2){v.x * s, v.y * s}; }

VF_INLINE Vec2 vec2_neg(Vec2 v) { return (Vec2){-v.x, -v.y}; }

// ============================================================================
// Vector Operations
// ============================================================================

VF_INLINE f32 vec2_dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }

VF_INLINE f32 vec2_cross(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

VF_INLINE f32 vec2_length_sq(Vec2 v) { return vec2_dot(v, v); }

VF_INLINE f32 vec2_length(Vec2 v) { return sqrtf(vec2_length_sq(v)); }

VF_INLINE Vec2 vec2_normalize(Vec2 v) {
  f32 len = vec2_length(v);
  if (len > 0.0001f) {
    return vec2_scale(v, 1.0f / len);
  }
  return vec2_zero();
}

VF_INLINE f32 vec2_distance(Vec2 a, Vec2 b) {
  return vec2_length(vec2_sub(b, a));
}

// ============================================================================
// Interpolation
// ============================================================================

VF_INLINE Vec2 vec2_lerp(Vec2 a, Vec2 b, f32 t) {
  return (Vec2){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

// ============================================================================
// Comparison
// ============================================================================

VF_INLINE b8 vec2_equals(Vec2 a, Vec2 b, f32 epsilon) {
  return fabsf(a.x - b.x) < epsilon && fabsf(a.y - b.y) < epsilon;
}

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_MATH_VEC2_H
