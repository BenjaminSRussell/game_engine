#ifndef SIMD_H
#define SIMD_H

/*
 * Portable stand-in for <simd/simd.h> used only by targets that compile the
 * atmosphere LUT sources. Linux CI has no Apple simd headers. Operators match
 * the clang/gcc ext_vector types those sources already use.
 */

#include <math.h>

typedef float simd_float2 __attribute__((ext_vector_type(2)));
typedef float simd_float3 __attribute__((ext_vector_type(3)));
typedef float simd_float4 __attribute__((ext_vector_type(4)));

typedef struct {
  simd_float4 columns[4];
} simd_float4x4;

static inline float simd_dot(simd_float3 a, simd_float3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float simd_length(simd_float3 a) {
  return sqrtf(simd_dot(a, a));
}

#endif /* SIMD_H */
