#ifndef MATH_LA_VECTORS_VEC3_OPS_H
#define MATH_LA_VECTORS_VEC3_OPS_H

#include "vec3_types.h"
#include <math.h>

#define vec3(x, y, z) ((Vec3){{x, y, z, 0.0f}})

INLINE Vec3 vec3_create(f32 x, f32 y, f32 z) {
  Vec3 v;
  v.x = x;
  v.y = y;
  v.z = z;
  v._w = 0.0f;
  return v;
}

INLINE Vec3 vec3_zero(void) { return vec3_create(0.0f, 0.0f, 0.0f); }
INLINE Vec3 vec3_one(void) { return vec3_create(1.0f, 1.0f, 1.0f); }

INLINE Vec3 vec3_up(void) { return vec3_create(0.0f, 1.0f, 0.0f); }
INLINE Vec3 vec3_down(void) { return vec3_create(0.0f, -1.0f, 0.0f); }
INLINE Vec3 vec3_left(void) { return vec3_create(-1.0f, 0.0f, 0.0f); }
INLINE Vec3 vec3_right(void) { return vec3_create(1.0f, 0.0f, 0.0f); }
INLINE Vec3 vec3_forward(void) { return vec3_create(0.0f, 0.0f, 1.0f); }
INLINE Vec3 vec3_back(void) { return vec3_create(0.0f, 0.0f, -1.0f); }

INLINE Vec3 vec3_add(Vec3 a, Vec3 b) {
  return vec3_create(a.x + b.x, a.y + b.y, a.z + b.z);
}

INLINE Vec3 vec3_sub(Vec3 a, Vec3 b) {
  return vec3_create(a.x - b.x, a.y - b.y, a.z - b.z);
}

INLINE f32 vec3_dot(Vec3 a, Vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
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
  if (len < 1e-6f) // EPSILON
    return vec3_zero();
  return vec3_div(v, len);
}

INLINE Vec3 vec3_lerp(Vec3 a, Vec3 b, f32 t) {
  return vec3_add(a, vec3_mul(vec3_sub(b, a), t));
}

INLINE Vec3 vec3_min(Vec3 a, Vec3 b) {
  return vec3_create(fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z));
}

INLINE Vec3 vec3_max(Vec3 a, Vec3 b) {
  return vec3_create(fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z));
}

#endif // MATH_LA_VECTORS_VEC3_OPS_H
