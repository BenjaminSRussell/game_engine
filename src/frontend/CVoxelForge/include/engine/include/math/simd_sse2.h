#ifndef SIMD_SSE2_H
#define SIMD_SSE2_H

#include "../math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

// SSE2 optimized vector operations (fallback for older CPUs)
Vec3 vec3_add_sse2(Vec3 a, Vec3 b);
Vec3 vec3_sub_sse2(Vec3 a, Vec3 b);
Vec3 vec3_mul_sse2(Vec3 v, f32 s);
f32 vec3_dot_sse2(Vec3 a, Vec3 b);
Vec3 vec3_cross_sse2(Vec3 a, Vec3 b);
Vec3 vec3_normalize_sse2(Vec3 v);
Vec3 vec3_lerp_sse2(Vec3 a, Vec3 b, f32 t);
Vec3 vec3_scale_sse2(Vec3 a, Vec3 b);
Vec3 vec3_min_sse2(Vec3 a, Vec3 b);
Vec3 vec3_max_sse2(Vec3 a, Vec3 b);

// Batch operations using SSE2
void vec3_add_batch_sse2(const Vec3 *a, const Vec3 *b, Vec3 *result, size_t count);
void vec3_normalize_batch_sse2(const Vec3 *input, Vec3 *output, size_t count);

// SSE2 optimized matrix operations
void mat4_mul_vec3_sse2(const f32 *matrix, const Vec3 *vectors, Vec3 *result, size_t count);

// SSE2 optimized vector magnitude calculations
void vec3_length_batch_sse2(const Vec3 *vectors, f32 *lengths, size_t count);
void vec3_distance_batch_sse2(const Vec3 *a, const Vec3 *b, f32 *distances, size_t count);

// Advanced vector operations
Vec3 vec3_reflect_sse2(Vec3 v, Vec3 normal);
Vec3 vec3_project_sse2(Vec3 v, Vec3 onto);
Vec3 vec3_reject_sse2(Vec3 v, Vec3 from);
f32 vec3_angle_sse2(Vec3 a, Vec3 b);
Vec3 vec3_slerp_sse2(Vec3 a, Vec3 b, f32 t);

#ifdef __cplusplus
}
#endif

#endif // SIMD_SSE2_H
