/**
 * SIMD Math - x86 SSE2/AVX2 Implementation
 * 
 * High-performance vectorized math operations using Intel intrinsics.
 * Compile with -msse2 or -mavx2 flags.
 */

#include "simd_math.h"

#if defined(SIMD_SSE2) || defined(SIMD_AVX2)

#include <immintrin.h>
#include <xmmintrin.h>  // SSE
#include <emmintrin.h>  // SSE2

// ========================================
// Single Vector Operations (SSE2)
// ========================================

void v4f_add(v4f *dst, const v4f *a, const v4f *b) {
    __m128 va = _mm_load_ps((const float*)a);
    __m128 vb = _mm_load_ps((const float*)b);
    __m128 result = _mm_add_ps(va, vb);
    _mm_store_ps((float*)dst, result);
}

void v4f_sub(v4f *dst, const v4f *a, const v4f *b) {
    __m128 va = _mm_load_ps((const float*)a);
    __m128 vb = _mm_load_ps((const float*)b);
    __m128 result = _mm_sub_ps(va, vb);
    _mm_store_ps((float*)dst, result);
}

void v4f_scale(v4f *dst, const v4f *v, float s) {
    __m128 vv = _mm_load_ps((const float*)v);
    __m128 vs = _mm_set1_ps(s);  // Broadcast scalar to all lanes
    __m128 result = _mm_mul_ps(vv, vs);
    _mm_store_ps((float*)dst, result);
}

float v4f_dot(const v4f *a, const v4f *b) {
    __m128 va = _mm_load_ps((const float*)a);
    __m128 vb = _mm_load_ps((const float*)b);
    
    // Multiply components
    __m128 mul = _mm_mul_ps(va, vb);
    
    // Horizontal add: SSE3 would use _mm_hadd_ps, but for SSE2:
    // Shuffle and add to sum all components
    __m128 shuf = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 3, 0, 1));
    __m128 sums = _mm_add_ps(mul, shuf);
    shuf = _mm_movehl_ps(shuf, sums);
    sums = _mm_add_ss(sums, shuf);
    
    return _mm_cvtss_f32(sums);
}

void v4f_cross(v4f *dst, const v4f *a, const v4f *b) {
    // Cross product: (a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x)
    __m128 va = _mm_load_ps((const float*)a);
    __m128 vb = _mm_load_ps((const float*)b);
    
    // Shuffle to get (y, z, x, w) permutations
    __m128 a_yzx = _mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 b_yzx = _mm_shuffle_ps(vb, vb, _MM_SHUFFLE(3, 0, 2, 1));
    
    // First term: a_yzx * b
    __m128 term1 = _mm_mul_ps(a_yzx, vb);
    
    // Second term: a * b_yzx
    __m128 term2 = _mm_mul_ps(va, b_yzx);
    
    // Cross = term1 - term2, then shuffle back to (x, y, z, w)
    __m128 cross_yzx = _mm_sub_ps(term1, term2);
    __m128 result = _mm_shuffle_ps(cross_yzx, cross_yzx, _MM_SHUFFLE(3, 0, 2, 1));
    
    // Clear w component
    __m128 mask = _mm_setr_ps(1.0f, 1.0f, 1.0f, 0.0f);
    result = _mm_mul_ps(result, mask);
    
    _mm_store_ps((float*)dst, result);
}

float v4f_length(const v4f *v) {
    __m128 vv = _mm_load_ps((const float*)v);
    __m128 sq = _mm_mul_ps(vv, vv);
    
    // Horizontal sum (same as dot product)
    __m128 shuf = _mm_shuffle_ps(sq, sq, _MM_SHUFFLE(2, 3, 0, 1));
    __m128 sums = _mm_add_ps(sq, shuf);
    shuf = _mm_movehl_ps(shuf, sums);
    sums = _mm_add_ss(sums, shuf);
    
    // Square root
    __m128 result = _mm_sqrt_ss(sums);
    return _mm_cvtss_f32(result);
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
    __m128 va = _mm_load_ps((const float*)a);
    __m128 vb = _mm_load_ps((const float*)b);
    __m128 result = _mm_mul_ps(va, vb);
    _mm_store_ps((float*)dst, result);
}

// ========================================
// Batch Operations (AVX2 optimized if available)
// ========================================

#ifdef SIMD_AVX2

void v4f_add_batch(v4f *dst, const v4f *a, const v4f *b, uint32_t count) {
    uint32_t i = 0;
    
    // Process 8 v4f at a time (256 bits * 8 = 2048 bits = 64 floats)
    // Actually AVX2 processes 8 floats per instruction, so we do 8 v4f vectors
    for (; i + 8 <= count; i += 8) {
        // Load 8 v4f vectors (32 floats) using 4 AVX2 loads
        __m256 a0 = _mm256_load_ps((const float*)&a[i]);       // v4f[0-1]
        __m256 a1 = _mm256_load_ps((const float*)&a[i + 2]);   // v4f[2-3]
        __m256 a2 = _mm256_load_ps((const float*)&a[i + 4]);   // v4f[4-5]
        __m256 a3 = _mm256_load_ps((const float*)&a[i + 6]);   // v4f[6-7]
        
        __m256 b0 = _mm256_load_ps((const float*)&b[i]);
        __m256 b1 = _mm256_load_ps((const float*)&b[i + 2]);
        __m256 b2 = _mm256_load_ps((const float*)&b[i + 4]);
        __m256 b3 = _mm256_load_ps((const float*)&b[i + 6]);
        
        // Vectorized addition
        __m256 r0 = _mm256_add_ps(a0, b0);
        __m256 r1 = _mm256_add_ps(a1, b1);
        __m256 r2 = _mm256_add_ps(a2, b2);
        __m256 r3 = _mm256_add_ps(a3, b3);
        
        // Store results
        _mm256_store_ps((float*)&dst[i], r0);
        _mm256_store_ps((float*)&dst[i + 2], r1);
        _mm256_store_ps((float*)&dst[i + 4], r2);
        _mm256_store_ps((float*)&dst[i + 6], r3);
    }
    
    // Handle remaining with SSE2
    for (; i < count; i++) {
        v4f_add(&dst[i], &a[i], &b[i]);
    }
}

void v4f_scale_batch(v4f *dst, const v4f *v, float s, uint32_t count) {
    __m256 vs = _mm256_set1_ps(s);
    uint32_t i = 0;
    
    for (; i + 8 <= count; i += 8) {
        __m256 v0 = _mm256_load_ps((const float*)&v[i]);
        __m256 v1 = _mm256_load_ps((const float*)&v[i + 2]);
        __m256 v2 = _mm256_load_ps((const float*)&v[i + 4]);
        __m256 v3 = _mm256_load_ps((const float*)&v[i + 6]);
        
        __m256 r0 = _mm256_mul_ps(v0, vs);
        __m256 r1 = _mm256_mul_ps(v1, vs);
        __m256 r2 = _mm256_mul_ps(v2, vs);
        __m256 r3 = _mm256_mul_ps(v3, vs);
        
        _mm256_store_ps((float*)&dst[i], r0);
        _mm256_store_ps((float*)&dst[i + 2], r1);
        _mm256_store_ps((float*)&dst[i + 4], r2);
        _mm256_store_ps((float*)&dst[i + 6], r3);
    }
    
    for (; i < count; i++) {
        v4f_scale(&dst[i], &v[i], s);
    }
}

#else  // SSE2 only

void v4f_add_batch(v4f *dst, const v4f *a, const v4f *b, uint32_t count) {
    // SSE2: process 4 v4f at a time (4 * 128 bits = 512 bits)
    uint32_t i = 0;
    for (; i + 4 <= count; i += 4) {
        __m128 a0 = _mm_load_ps((const float*)&a[i]);
        __m128 a1 = _mm_load_ps((const float*)&a[i + 1]);
        __m128 a2 = _mm_load_ps((const float*)&a[i + 2]);
        __m128 a3 = _mm_load_ps((const float*)&a[i + 3]);
        
        __m128 b0 = _mm_load_ps((const float*)&b[i]);
        __m128 b1 = _mm_load_ps((const float*)&b[i + 1]);
        __m128 b2 = _mm_load_ps((const float*)&b[i + 2]);
        __m128 b3 = _mm_load_ps((const float*)&b[i + 3]);
        
        _mm_store_ps((float*)&dst[i], _mm_add_ps(a0, b0));
        _mm_store_ps((float*)&dst[i + 1], _mm_add_ps(a1, b1));
        _mm_store_ps((float*)&dst[i + 2], _mm_add_ps(a2, b2));
        _mm_store_ps((float*)&dst[i + 3], _mm_add_ps(a3, b3));
    }
    
    for (; i < count; i++) {
        v4f_add(&dst[i], &a[i], &b[i]);
    }
}

void v4f_scale_batch(v4f *dst, const v4f *v, float s, uint32_t count) {
    __m128 vs = _mm_set1_ps(s);
    uint32_t i = 0;
    
    for (; i + 4 <= count; i += 4) {
        __m128 v0 = _mm_load_ps((const float*)&v[i]);
        __m128 v1 = _mm_load_ps((const float*)&v[i + 1]);
        __m128 v2 = _mm_load_ps((const float*)&v[i + 2]);
        __m128 v3 = _mm_load_ps((const float*)&v[i + 3]);
        
        _mm_store_ps((float*)&dst[i], _mm_mul_ps(v0, vs));
        _mm_store_ps((float*)&dst[i + 1], _mm_mul_ps(v1, vs));
        _mm_store_ps((float*)&dst[i + 2], _mm_mul_ps(v2, vs));
        _mm_store_ps((float*)&dst[i + 3], _mm_mul_ps(v3, vs));
    }
    
    for (; i < count; i++) {
        v4f_scale(&dst[i], &v[i], s);
    }
}

#endif // SIMD_AVX2

// ========================================
// Matrix Operations
// ========================================

void m33f_transform(v4f *dst, const m33f *M, const v4f *v) {
    __m128 vv = _mm_load_ps((const float*)v);
    
    // Load matrix columns
    __m128 col0 = _mm_load_ps((const float*)&M->col0);
    __m128 col1 = _mm_load_ps((const float*)&M->col1);
    __m128 col2 = _mm_load_ps((const float*)&M->col2);
    
    // Broadcast each component of v
    __m128 vx = _mm_shuffle_ps(vv, vv, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 vy = _mm_shuffle_ps(vv, vv, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 vz = _mm_shuffle_ps(vv, vv, _MM_SHUFFLE(2, 2, 2, 2));
    
    // Multiply and accumulate: dst = col0*vx + col1*vy + col2*vz
    __m128 result = _mm_mul_ps(col0, vx);
    result = _mm_add_ps(result, _mm_mul_ps(col1, vy));
    result = _mm_add_ps(result, _mm_mul_ps(col2, vz));
    
    // Clear w component
    __m128 mask = _mm_setr_ps(1.0f, 1.0f, 1.0f, 0.0f);
    result = _mm_mul_ps(result, mask);
    
    _mm_store_ps((float*)dst, result);
}

void m33f_mul(m33f *dst, const m33f *A, const m33f *B) {
    // Each column of C is A * (column of B)
    m33f_transform(&dst->col0, A, &B->col0);
    m33f_transform(&dst->col1, A, &B->col1);
    m33f_transform(&dst->col2, A, &B->col2);
}

void m33f_transpose(m33f *dst, const m33f *M) {
    // Load columns
    __m128 col0 = _mm_load_ps((const float*)&M->col0);
    __m128 col1 = _mm_load_ps((const float*)&M->col1);
    __m128 col2 = _mm_load_ps((const float*)&M->col2);
    
    // Transpose 3x3 using shuffle operations
    // We only care about the first 3 components of each vector
    __m128 tmp0 = _mm_unpacklo_ps(col0, col1);  // x0,x1,y0,y1
    __m128 tmp1 = _mm_unpacklo_ps(col2, col2);  // x2,x2,y2,y2
    __m128 tmp2 = _mm_unpackhi_ps(col0, col1);  // z0,z1,w0,w1
    
    __m128 row0 = _mm_movelh_ps(tmp0, tmp1);    // x0,x1,x2,y2
    __m128 row1 = _mm_movehl_ps(tmp1, tmp0);    // y0,y1,x2,x2
    __m128 row2 = _mm_movelh_ps(tmp2, tmp2);    // z0,z1,w0,w1
    
    // Mask to keep only xyz
    __m128 mask = _mm_setr_ps(1.0f, 1.0f, 1.0f, 0.0f);
    
    _mm_store_ps((float*)&dst->col0, _mm_mul_ps(row0, mask));
    _mm_store_ps((float*)&dst->col1, _mm_mul_ps(row1, mask));
    _mm_store_ps((float*)&dst->col2, _mm_mul_ps(row2, mask));
}

void m33f_from_axis_angle(m33f *dst, const v4f *axis, float angle_rad) {
    // Use scalar implementation (Rodrigues formula is complex to vectorize efficiently)
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

#endif // SIMD_SSE2 || SIMD_AVX2
