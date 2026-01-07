#ifndef SIMD_NEON_H
#define SIMD_NEON_H

#include "../math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

// ARM NEON optimized vector operations for mobile platforms
Vec3 vec3_add_neon(Vec3 a, Vec3 b);
Vec3 vec3_sub_neon(Vec3 a, Vec3 b);
Vec3 vec3_mul_neon(Vec3 v, f32 s);
f32 vec3_dot_neon(Vec3 a, Vec3 b);
Vec3 vec3_cross_neon(Vec3 a, Vec3 b);
Vec3 vec3_normalize_neon(Vec3 v);
Vec3 vec3_lerp_neon(Vec3 a, Vec3 b, f32 t);
Vec3 vec3_scale_neon(Vec3 a, Vec3 b);
Vec3 vec3_min_neon(Vec3 a, Vec3 b);
Vec3 vec3_max_neon(Vec3 a, Vec3 b);

// Batch operations using NEON
void vec3_add_batch_neon(const Vec3 *a, const Vec3 *b, Vec3 *result, size_t count);
void vec3_normalize_batch_neon(const Vec3 *input, Vec3 *output, size_t count);

// NEON optimized matrix operations
void mat4_mul_vec3_neon(const f32 *matrix, const Vec3 *vectors, Vec3 *result, size_t count);

// NEON optimized vector magnitude calculations
void vec3_length_batch_neon(const Vec3 *vectors, f32 *lengths, size_t count);
void vec3_distance_batch_neon(const Vec3 *a, const Vec3 *b, f32 *distances, size_t count);

// Advanced vector operations
Vec3 vec3_reflect_neon(Vec3 v, Vec3 normal);
Vec3 vec3_project_neon(Vec3 v, Vec3 onto);
Vec3 vec3_reject_neon(Vec3 v, Vec3 from);
f32 vec3_angle_neon(Vec3 a, Vec3 b);
Vec3 vec3_slerp_neon(Vec3 a, Vec3 b, f32 t);

// NEON capability detection
bool vec3_has_neon_support(void);

// Optimized versions that automatically use NEON when available
Vec3 vec3_add_optimized_neon(Vec3 a, Vec3 b);
Vec3 vec3_sub_optimized_neon(Vec3 a, Vec3 b);
Vec3 vec3_mul_optimized_neon(Vec3 v, f32 s);
f32 vec3_dot_optimized_neon(Vec3 a, Vec3 b);
Vec3 vec3_cross_optimized_neon(Vec3 a, Vec3 b);
Vec3 vec3_normalize_optimized_neon(Vec3 v);
Vec3 vec3_lerp_optimized_neon(Vec3 a, Vec3 b, f32 t);

#ifdef __cplusplus
}
#endif

#endif // SIMD_NEON_H
