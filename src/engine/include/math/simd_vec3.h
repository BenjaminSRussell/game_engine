#ifndef SIMD_VEC3_H
#define SIMD_VEC3_H

#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

// AVX2 optimized vector operations
Vec3 vec3_add_avx2(Vec3 a, Vec3 b);
Vec3 vec3_sub_avx2(Vec3 a, Vec3 b);
Vec3 vec3_mul_avx2(Vec3 v, f32 s);
f32 vec3_dot_avx2(Vec3 a, Vec3 b);
Vec3 vec3_cross_avx2(Vec3 a, Vec3 b);
Vec3 vec3_normalize_avx2(Vec3 v);
Vec3 vec3_lerp_avx2(Vec3 a, Vec3 b, f32 t);
Vec3 vec3_scale_avx2(Vec3 a, Vec3 b);
Vec3 vec3_min_avx2(Vec3 a, Vec3 b);
Vec3 vec3_max_avx2(Vec3 a, Vec3 b);

// Batch operations for multiple vectors
void vec3_add_batch_avx2(const Vec3 *a, const Vec3 *b, Vec3 *result, size_t count);
void vec3_normalize_batch_avx2(const Vec3 *input, Vec3 *output, size_t count);

// SIMD capability detection
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

#ifdef __cplusplus
}
#endif

#endif // SIMD_VEC3_H
