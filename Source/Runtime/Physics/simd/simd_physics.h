#pragma once

#include "../math/types.h"
#include <immintrin.h>

// SIMD-Optimized Physics Core
// High-performance vectorized physics operations

// SIMD Vector Types
typedef struct {
    __m256 x, y, z, w;
} simd_vec4;

typedef struct {
    __m256 x, y, z;
} simd_vec3;

// SIMD Physics Operations
static inline simd_vec3 simd_vec3_load(const vec3* v) {
    return (simd_vec3){
        .x = _mm256_set1_ps(v->x),
        .y = _mm256_set1_ps(v->y),
        .z = _mm256_set1_ps(v->z)
    };
}

static inline vec3 simd_vec3_store(const simd_vec3* v) {
    return (vec3){
        .x = _mm256_cvtss_f32(v->x.m256_f32[0]),
        .y = _mm256_cvtss_f32(v->y.m256_f32[0]),
        .z = _mm256_cvtss_f32(v->z.m256_f32[0])
    };
}

// SIMD Cross Product
static inline simd_vec3 simd_cross(const simd_vec3* a, const simd_vec3* b) {
    __m256 ax = _mm256_permute_ps(a->x, 0xC9);
    __m256 by = _mm256_permute_ps(b->y, 0xC9);
    __m256 az = _mm256_permute_ps(a->z, 0xC9);
    __m256 bx = _mm256_permute_ps(b->x, 0xC9);
    __m256 bz = _mm256_permute_ps(b->z, 0xC9);
    
    __m256 result1 = _mm256_sub_ps(_mm256_mul_ps(ax, bz), _mm256_mul_ps(az, bx));
    __m256 result2 = _mm256_sub_ps(_mm256_mul_ps(ay, bx), _mm256_mul_ps(ax, by));
    
    return (simd_vec3){
        .x = result1,
        .y = result2,
        .z = _mm256_sub_ps(_mm256_mul_ps(ay, bz), _mm256_mul_ps(az, by))
    };
}

// SIMD Dot Product
static inline __m256 simd_dot(const simd_vec3* a, const simd_vec3* b) {
    __m256 mul = _mm256_mul_ps(a->x, b->x);
    mul = _mm256_add_ps(mul, _mm256_mul_ps(a->y, b->y));
    mul = _mm256_add_ps(mul, _mm256_mul_ps(a->z, b->z));
    return mul;
}

// SIMD Normalize
static inline simd_vec3 simd_normalize(const simd_vec3* v) {
    __m256 dot = simd_dot(v, v);
    __m256 length = _mm256_sqrt_ps(dot);
    __m256 inv_length = _mm256_div_ps(_mm256_set1_ps(1.0f), length);
    
    return (simd_vec3){
        .x = _mm256_mul_ps(v->x, inv_length),
        .y = _mm256_mul_ps(v->y, inv_length),
        .z = _mm256_mul_ps(v->z, inv_length)
    };
}

// SIMD Rigid Body Integration
typedef struct {
    simd_vec3 position;
    simd_vec3 velocity;
    simd_vec3 acceleration;
    simd_vec3 angular_velocity;
    simd_vec3 angular_acceleration;
    __m256 mass;
    __m256 inv_mass;
    __m256 inv_inertia;
} simd_rigid_body;

// Batch Physics Processing
void simd_integrate_rigid_bodies(simd_rigid_body* bodies, u32 count, f32 dt);
void simd_resolve_collisions_batch(simd_rigid_body* bodies, u32 count);
void simd_update_broadphase_batch(simd_rigid_body* bodies, u32 count);

#endif // SIMD_PHYSICS_H
