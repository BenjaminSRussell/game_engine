/**
 * SIMD Math - ARM NEON Implementation
 * 
 * High-performance vectorized math operations using ARM NEON intrinsics.
 * Compile with -mfpu=neon (ARMv7) or automatically enabled on AArch64.
 */

#include "core/simd/simd_math.h"

#ifdef SIMD_NEON

#include <arm_neon.h>
#include <include/math/math_all.h>

// ========================================
// Single Vector Operations
// ========================================

void v4f_add(v4f *dst, const v4f *a, const v4f *b) {
    float32x4_t va = vld1q_f32((const float*)a);
    float32x4_t vb = vld1q_f32((const float*)b);
    float32x4_t result = vaddq_f32(va, vb);
    vst1q_f32((float*)dst, result);
}

void v4f_sub(v4f *dst, const v4f *a, const v4f *b) {
    float32x4_t va = vld1q_f32((const float*)a);
    float32x4_t vb = vld1q_f32((const float*)b);
    float32x4_t result = vsubq_f32(va, vb);
    vst1q_f32((float*)dst, result);
}

void v4f_scale(v4f *dst, const v4f *v, float s) {
    float32x4_t vv = vld1q_f32((const float*)v);
    float32x4_t vs = vdupq_n_f32(s);  // Broadcast scalar
    float32x4_t result = vmulq_f32(vv, vs);
    vst1q_f32((float*)dst, result);
}

float v4f_dot(const v4f *a, const v4f *b) {
    float32x4_t va = vld1q_f32((const float*)a);
    float32x4_t vb = vld1q_f32((const float*)b);
    
    // Component-wise multiply
    float32x4_t mul = vmulq_f32(va, vb);
    
    // Horizontal add (sum all lanes)
    float32x2_t sum = vadd_f32(vget_low_f32(mul), vget_high_f32(mul));
    sum = vpadd_f32(sum, sum);  // Pairwise add
    
    return vget_lane_f32(sum, 0);
}

void v4f_cross(v4f *dst, const v4f *a, const v4f *b) {
    // Cross product: (a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x)
    
    // Use scalar approach for NEON since permutations are complex
    // This is still faster than scalar due to FMA operations
    float ax = a->x, ay = a->y, az = a->z;
    float bx = b->x, by = b->y, bz = b->z;
    
    dst->x = ay * bz - az * by;
    dst->y = az * bx - ax * bz;
    dst->z = ax * by - ay * bx;
    dst->w = 0.0f;
}

float v4f_length(const v4f *v) {
    float32x4_t vv = vld1q_f32((const float*)v);
    float32x4_t sq = vmulq_f32(vv, vv);
    
    // Horizontal sum
    float32x2_t sum = vadd_f32(vget_low_f32(sq), vget_high_f32(sq));
    sum = vpadd_f32(sum, sum);
    
    // Square root
    float32x2_t sqrt_val = vsqrt_f32(sum);
    return vget_lane_f32(sqrt_val, 0);
}

void v4f_normalize(v4f *dst, const v4f *v) {
    float len = v4f_length(v);
    if (len > 1e-6f) {
        v4f_scale(dst, v, 1.0f / len);
    } else {
        v4f_zero(dst);
    }
}

void v4f_mul(v4f *dst, const v4f *a, const v4f *b) {
    float32x4_t va = vld1q_f32((const float*)a);
    float32x4_t vb = vld1q_f32((const float*)b);
    float32x4_t result = vmulq_f32(va, vb);
    vst1q_f32((float*)dst, result);
}

// ========================================
// Batch Operations
// ========================================

void v4f_add_batch(v4f *dst, const v4f *a, const v4f *b, uint32_t count) {
    uint32_t i = 0;
    
    // Process 4 v4f at a time (4 * 128 bits)
    for (; i + 4 <= count; i += 4) {
        float32x4_t a0 = vld1q_f32((const float*)&a[i]);
        float32x4_t a1 = vld1q_f32((const float*)&a[i + 1]);
        float32x4_t a2 = vld1q_f32((const float*)&a[i + 2]);
        float32x4_t a3 = vld1q_f32((const float*)&a[i + 3]);
        
        float32x4_t b0 = vld1q_f32((const float*)&b[i]);
        float32x4_t b1 = vld1q_f32((const float*)&b[i + 1]);
        float32x4_t b2 = vld1q_f32((const float*)&b[i + 2]);
        float32x4_t b3 = vld1q_f32((const float*)&b[i + 3]);
        
        vst1q_f32((float*)&dst[i], vaddq_f32(a0, b0));
        vst1q_f32((float*)&dst[i + 1], vaddq_f32(a1, b1));
        vst1q_f32((float*)&dst[i + 2], vaddq_f32(a2, b2));
        vst1q_f32((float*)&dst[i + 3], vaddq_f32(a3, b3));
    }
    
    // Handle remaining
    for (; i < count; i++) {
        v4f_add(&dst[i], &a[i], &b[i]);
    }
}

void v4f_scale_batch(v4f *dst, const v4f *v, float s, uint32_t count) {
    float32x4_t vs = vdupq_n_f32(s);
    uint32_t i = 0;
    
    for (; i + 4 <= count; i += 4) {
        float32x4_t v0 = vld1q_f32((const float*)&v[i]);
        float32x4_t v1 = vld1q_f32((const float*)&v[i + 1]);
        float32x4_t v2 = vld1q_f32((const float*)&v[i + 2]);
        float32x4_t v3 = vld1q_f32((const float*)&v[i + 3]);
        
        vst1q_f32((float*)&dst[i], vmulq_f32(v0, vs));
        vst1q_f32((float*)&dst[i + 1], vmulq_f32(v1, vs));
        vst1q_f32((float*)&dst[i + 2], vmulq_f32(v2, vs));
        vst1q_f32((float*)&dst[i + 3], vmulq_f32(v3, vs));
    }
    
    for (; i < count; i++) {
        v4f_scale(&dst[i], &v[i], s);
    }
}

// ========================================
// Matrix Operations
// ========================================

void m33f_transform(v4f *dst, const m33f *M, const v4f *v) {
    float32x4_t vv = vld1q_f32((const float*)v);
    
    // Load matrix columns
    float32x4_t col0 = vld1q_f32((const float*)&M->col0);
    float32x4_t col1 = vld1q_f32((const float*)&M->col1);
    float32x4_t col2 = vld1q_f32((const float*)&M->col2);
    
    // Broadcast each component
    float32x4_t vx = vdupq_n_f32(vgetq_lane_f32(vv, 0));
    float32x4_t vy = vdupq_n_f32(vgetq_lane_f32(vv, 1));
    float32x4_t vz = vdupq_n_f32(vgetq_lane_f32(vv, 2));
    
    // Multiply and accumulate
    float32x4_t result = vmulq_f32(col0, vx);
    result = vmlaq_f32(result, col1, vy);  // FMA: result += col1 * vy
    result = vmlaq_f32(result, col2, vz);
    
    // Clear w component
    result = vsetq_lane_f32(0.0f, result, 3);
    
    vst1q_f32((float*)dst, result);
}

void m33f_mul(m33f *dst, const m33f *A, const m33f *B) {
    m33f_transform(&dst->col0, A, &B->col0);
    m33f_transform(&dst->col1, A, &B->col1);
    m33f_transform(&dst->col2, A, &B->col2);
}

void m33f_transpose(m33f *dst, const m33f *M) {
    // Use scalar for simplicity (NEON transpose is complex for 3x3)
    dst->col0.x = M->col0.x;
    dst->col0.y = M->col1.x;
    dst->col0.z = M->col2.x;
    dst->col0.w = 0.0f;
    
    dst->col1.x = M->col0.y;
    dst->col1.y = M->col1.y;
    dst->col1.z = M->col2.y;
    dst->col1.w = 0.0f;
    
    dst->col2.x = M->col0.z;
    dst->col2.y = M->col1.z;
    dst->col2.z = M->col2.z;
    dst->col2.w = 0.0f;
}

void m33f_from_axis_angle(m33f *dst, const v4f *axis, float angle_rad) {
    // Rodrigues formula (scalar implementation)
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

#endif // SIMD_NEON
