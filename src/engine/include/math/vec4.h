#ifndef VEC4_H
#define VEC4_H

#include <common.h>

// Use unified type from core/math/types.h
typedef vec4 Vec4;

static inline Vec4 vec4_create(f32 x, f32 y, f32 z, f32 w) {
  Vec4 v = {x, y, z, w};
  return v;
}

static inline Vec4 vec4_lerp(Vec4 a, Vec4 b, f32 t) {
  return vec4_create(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                     a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}

#endif // VEC4_H
