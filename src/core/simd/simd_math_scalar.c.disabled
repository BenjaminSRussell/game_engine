/**
 * SIMD Math - Scalar Fallback Implementation
 * 
 * Provides reference implementation for platforms without SIMD support.
 * Also serves as ground truth for testing SIMD implementations.
 */

#include "core/simd/simd_math.h"
#include <include/math/math.h>
#include <string.h>

// Only compile if no SIMD available
#if !defined(SIMD_SSE2) && !defined(SIMD_AVX2) && !defined(SIMD_NEON)

void v4f_add(v4f *dst, const v4f *a, const v4f *b) {
    dst->x = a->x + b->x;
    dst->y = a->y + b->y;
    dst->z = a->z + b->z;
    dst->w = a->w + b->w;
}

void v4f_sub(v4f *dst, const v4f *a, const v4f *b) {
    dst->x = a->x - b->x;
    dst->y = a->y - b->y;
    dst->z = a->z - b->z;
    dst->w = a->w - b->w;
}

void v4f_scale(v4f *dst, const v4f *v, float s) {
    dst->x = v->x * s;
    dst->y = v->y * s;
    dst->z = v->z * s;
    dst->w = v->w * s;
}

float v4f_dot(const v4f *a, const v4f *b) {
    return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

void v4f_cross(v4f *dst, const v4f *a, const v4f *b) {
    // 3D cross product only (ignore w)
    float x = a->y * b->z - a->z * b->y;
    float y = a->z * b->x - a->x * b->z;
    float z = a->x * b->y - a->y * b->x;
    
    dst->x = x;
    dst->y = y;
    dst->z = z;
    dst->w = 0.0f;
}

float v4f_length(const v4f *v) {
    return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

void v4f_normalize(v4f *dst, const v4f *v) {
    float len = v4f_length(v);
    if (len > 1e-6f) {
        float inv_len = 1.0f / len;
        v4f_scale(dst, v, inv_len);
    } else {
        v4f_zero(dst);
    }
}

void v4f_mul(v4f *dst, const v4f *a, const v4f *b) {
    dst->x = a->x * b->x;
    dst->y = a->y * b->y;
    dst->z = a->z * b->z;
    dst->w = a->w * b->w;
}

void v4f_add_batch(v4f *dst, const v4f *a, const v4f *b, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        v4f_add(&dst[i], &a[i], &b[i]);
    }
}

void v4f_scale_batch(v4f *dst, const v4f *v, float s, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        v4f_scale(&dst[i], &v[i], s);
    }
}

void m33f_transform(v4f *dst, const m33f *M, const v4f *v) {
    // Matrix-vector multiply: dst = M * v
    // M is column-major, so each column is a basis vector
    dst->x = M->col0.x * v->x + M->col1.x * v->y + M->col2.x * v->z;
    dst->y = M->col0.y * v->x + M->col1.y * v->y + M->col2.y * v->z;
    dst->z = M->col0.z * v->x + M->col1.z * v->y + M->col2.z * v->z;
    dst->w = 0.0f;
}

void m33f_mul(m33f *dst, const m33f *A, const m33f *B) {
    // Matrix multiply: C = A * B
    // Each column of C is A * (column of B)
    m33f_transform(&dst->col0, A, &B->col0);
    m33f_transform(&dst->col1, A, &B->col1);
    m33f_transform(&dst->col2, A, &B->col2);
}

void m33f_transpose(m33f *dst, const m33f *M) {
    dst->col0.x = M->col0.x;
    dst->col0.y = M->col1.x;
    dst->col0.z = M->col2.x;
    
    dst->col1.x = M->col0.y;
    dst->col1.y = M->col1.y;
    dst->col1.z = M->col2.y;
    
    dst->col2.x = M->col0.z;
    dst->col2.y = M->col1.z;
    dst->col2.z = M->col2.z;
    
    // Clear w components
    dst->col0.w = dst->col1.w = dst->col2.w = 0.0f;
}

void m33f_from_axis_angle(m33f *dst, const v4f *axis, float angle_rad) {
    // Rodrigues' rotation formula
    float c = cosf(angle_rad);
    float s = sinf(angle_rad);
    float t = 1.0f - c;
    
    v4f n;
    v4f_normalize(&n, axis);
    
    float x = n.x, y = n.y, z = n.z;
    
    dst->col0.x = t*x*x + c;
    dst->col0.y = t*x*y + s*z;
    dst->col0.z = t*x*z - s*y;
    dst->col0.w = 0.0f;
    
    dst->col1.x = t*x*y - s*z;
    dst->col1.y = t*y*y + c;
    dst->col1.z = t*y*z + s*x;
    dst->col1.w = 0.0f;
    
    dst->col2.x = t*x*z + s*y;
    dst->col2.y = t*y*z - s*x;
    dst->col2.z = t*z*z + c;
    dst->col2.w = 0.0f;
}

#endif // !SIMD
