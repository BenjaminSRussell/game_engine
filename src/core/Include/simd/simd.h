#ifndef SIMD_H
#define SIMD_H

#if defined(__APPLE__)
#include_next <simd/simd.h>
#else
#include "../math/mat4.h"
#include "../math/types.h"
#include "../math/vec3.h"
#include "../math/vec4.h"

typedef vec3 simd_float3;
typedef vec4 simd_float4;
typedef mat4 simd_float4x4;

static inline simd_float3 simd_make_float3(float x, float y, float z) {
  return vec3_create(x, y, z);
}

static inline float simd_dot(simd_float3 a, simd_float3 b) {
  return vec3_dot(a, b);
}

static inline simd_float3 simd_normalize(simd_float3 a) {
  return vec3_normalize(a);
}

static inline simd_float3 simd_add(simd_float3 a, simd_float3 b) {
  return vec3_add(a, b);
}

static inline simd_float3 simd_sub(simd_float3 a, simd_float3 b) {
  return vec3_sub(a, b);
}

static inline simd_float3 simd_mul(simd_float3 a, float s) {
  return vec3_mul(a, s);
}

static inline simd_float3 simd_min(simd_float3 a, simd_float3 b) {
  return vec3_min(a, b);
}

static inline simd_float3 simd_max(simd_float3 a, simd_float3 b) {
  return vec3_max(a, b);
}

static inline float simd_distance(simd_float3 a, simd_float3 b) {
  return vec3_distance(a, b);
}

static inline float simd_length(simd_float3 a) { return vec3_length(a); }

static inline simd_float3 simd_cross(simd_float3 a, simd_float3 b) {
  return vec3_cross(a, b);
}

static inline simd_float4 simd_mul_matrix_vector(simd_float4x4 m,
                                                 simd_float4 v) {
  return mat4_mul_vec4(m, v);
}
#endif

#endif // SIMD_H
