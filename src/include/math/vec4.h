#ifndef VEC4_H
#define VEC4_H

#include "../common.h"
#include <math.h>

typedef struct {
  f32 x, y, z, w;
} Vec4;

typedef Vec4 vec4_t;

static inline Vec4 vec4_create(f32 x, f32 y, f32 z, f32 w) {
  Vec4 v = {x, y, z, w};
  return v;
}

static inline Vec4 vec4_add(Vec4 a, Vec4 b) {
  return vec4_create(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

static inline Vec4 vec4_sub(Vec4 a, Vec4 b) {
  return vec4_create(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

static inline Vec4 vec4_scale(Vec4 a, f32 s) {
  return vec4_create(a.x * s, a.y * s, a.z * s, a.w * s);
}

static inline f32 vec4_dot(Vec4 a, Vec4 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

static inline Vec4 vec4_lerp(Vec4 a, Vec4 b, f32 t) {
  return vec4_add(a, vec4_scale(vec4_sub(b, a), t));
}

static inline f32 vec4_length(Vec4 a) {
  return sqrtf(vec4_dot(a, a));
}

static inline f32 vec4_distance(Vec4 a, Vec4 b) {
  Vec4 diff = vec4_sub(a, b);
  return vec4_length(diff);
}

static inline Vec4 vec4_zero(void) {
  return vec4_create(0.0f, 0.0f, 0.0f, 0.0f);
}

static inline Vec4 vec4_one(void) {
  return vec4_create(1.0f, 1.0f, 1.0f, 1.0f);
}

// Additional function declarations that are implemented in vec4.c
Vec4 vec4_min(Vec4 a, Vec4 b);
Vec4 vec4_max(Vec4 a, Vec4 b);
bool vec4_equals(Vec4 a, Vec4 b);
Vec4 vec4_abs(Vec4 a);
Vec4 vec4_clamp(Vec4 a, f32 min_val, f32 max_val);

#endif // VEC4_H