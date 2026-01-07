/**
 * SIMD Math Operations - Cross-Platform Abstraction
 * 
 * Provides vectorized math operations with automatic platform selection:
 * - x86: SSE2 / AVX2
 * - ARM: NEON
 * - Fallback: Scalar
 */

#ifndef SIMD_MATH_H
#define SIMD_MATH_H

#include "core/simd/simd_types.h"
#include <include/math/math.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Vector Operations
// ========================================

/**
 * Vector addition: dst = a + b
 */
void v4f_add(v4f *dst, const v4f *a, const v4f *b);

/**
 * Vector subtraction: dst = a - b
 */
void v4f_sub(v4f *dst, const v4f *a, const v4f *b);

/**
 * Vector scaling: dst = v * s
 */
void v4f_scale(v4f *dst, const v4f *v, float s);

/**
 * Dot product: returns a · b
 */
float v4f_dot(const v4f *a, const v4f *b);

/**
 * Cross product: dst = a × b (3D only, w=0)
 */
void v4f_cross(v4f *dst, const v4f *a, const v4f *b);

/**
 * Vector length: returns |v|
 */
float v4f_length(const v4f *v);

/**
 * Vector normalization: dst = v / |v|
 */
void v4f_normalize(v4f *dst, const v4f *v);

/**
 * Component-wise multiply: dst = a * b
 */
void v4f_mul(v4f *dst, const v4f *a, const v4f *b);

// ========================================
// Batch Vector Operations (SoA optimized)
// ========================================

/**
 * Add N vectors: dst[i] = a[i] + b[i] for i in [0, count)
 * Arrays must be 16-byte aligned.
 */
void v4f_add_batch(v4f *dst, const v4f *a, const v4f *b, uint32_t count);

/**
 * Scale N vectors: dst[i] = v[i] * s for i in [0, count)
 */
void v4f_scale_batch(v4f *dst, const v4f *v, float s, uint32_t count);

// ========================================
// Matrix Operations
// ========================================

/**
 * Matrix-vector multiply: dst = M * v
 */
void m33f_transform(v4f *dst, const m33f *M, const v4f *v);

/**
 * Matrix-matrix multiply: dst = A * B
 */
void m33f_mul(m33f *dst, const m33f *A, const m33f *B);

/**
 * Matrix transpose: dst = M^T
 */
void m33f_transpose(m33f *dst, const m33f *M);

/**
 * Create rotation matrix from axis and angle
 */
void m33f_from_axis_angle(m33f *dst, const v4f *axis, float angle_rad);

// ========================================
// Utility Functions
// ========================================

/**
 * Copy vector: dst = src
 */
static inline void v4f_copy(v4f *dst, const v4f *src) {
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
    dst->w = src->w;
}

/**
 * Zero vector: dst = (0, 0, 0, 0)
 */
static inline void v4f_zero(v4f *dst) {
    dst->x = dst->y = dst->z = dst->w = 0.0f;
}

/**
 * Rotate vector by quaternion
 * v' = q * v * q^-1
 */
/**
 * Rotate vector by quaternion
 * v' = v + 2 * cross(q.xyz, cross(q.xyz, v) + q.w * v)
 */
static inline void v4f_rotate_quat(v4f *dst, const v4f *v, const v4f *q) {
    float qx = q->x, qy = q->y, qz = q->z, qw = q->w;
    float vx = v->x, vy = v->y, vz = v->z;
    
    // t = 2 * cross(q.xyz, v)
    float tx = 2.0f * (qy * vz - qz * vy);
    float ty = 2.0f * (qz * vx - qx * vz);
    float tz = 2.0f * (qx * vy - qy * vx);
    
    // v' = v + q.w * t + cross(q.xyz, t)
    dst->x = vx + qw * tx + (qy * tz - qz * ty);
    dst->y = vy + qw * ty + (qz * tx - qx * tz);
    dst->z = vz + qw * tz + (qx * ty - qy * tx);
    dst->w = 0.0f;
}

#ifdef __cplusplus
}
#endif

#endif // SIMD_MATH_H
