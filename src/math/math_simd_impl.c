#include "engine/include/math/math.h"
#include <math/vec3.h>
#include <math/vec4.h>
#include <immintrin.h>
#include <string.h>

// SIMD-optimized implementations for common math operations

#if defined(RENDERING_SIMD_SSE)

void simd_vec3_add_array(vec3_t* result, const vec3_t* a, const vec3_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i].simd = _mm_add_ps(a[i].simd, b[i].simd);
    }
}

void simd_vec3_sub_array(vec3_t* result, const vec3_t* a, const vec3_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i].simd = _mm_sub_ps(a[i].simd, b[i].simd);
    }
}

void simd_vec3_scale_array(vec3_t* result, const vec3_t* a, float s, int count) {
    __m128 scale_vec = _mm_set1_ps(s);
    for (int i = 0; i < count; i++) {
        result[i].simd = _mm_mul_ps(a[i].simd, scale_vec);
    }
}

void simd_vec4_add_array(vec4_t* result, const vec4_t* a, const vec4_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i].simd = _mm_add_ps(a[i].simd, b[i].simd);
    }
}

void simd_vec4_mul_array(vec4_t* result, const vec4_t* a, const vec4_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i].simd = _mm_mul_ps(a[i].simd, b[i].simd);
    }
}

float simd_vec3_dot_array(const vec3_t* a, const vec3_t* b, int count) {
    __m128 sum = _mm_setzero_ps();
    
    for (int i = 0; i < count; i++) {
        __m128 dot = _mm_dp_ps(a[i].simd, b[i].simd, 0x7F);
        sum = _mm_add_ss(sum, dot);
    }
    
    return _mm_cvtss_f32(sum);
}

void simd_vec3_normalize_array(vec3_t* vecs, int count) {
    for (int i = 0; i < count; i++) {
        __m128 len_sq = _mm_dp_ps(vecs[i].simd, vecs[i].simd, 0x7F);
        __m128 inv_len = _mm_rsqrt_ps(len_sq);
        vecs[i].simd = _mm_mul_ps(vecs[i].simd, inv_len);
    }
}

void simd_vec3_cross_array(vec3_t* result, const vec3_t* a, const vec3_t* b, int count) {
    for (int i = 0; i < count; i++) {
        __m128 a_yzx = _mm_shuffle_ps(a[i].simd, a[i].simd, _MM_SHUFFLE(3, 0, 2, 1));
        __m128 b_zxy = _mm_shuffle_ps(b[i].simd, b[i].simd, _MM_SHUFFLE(3, 1, 0, 2));
        __m128 a_zxy = _mm_shuffle_ps(a[i].simd, a[i].simd, _MM_SHUFFLE(3, 1, 0, 2));
        __m128 b_yzx = _mm_shuffle_ps(b[i].simd, b[i].simd, _MM_SHUFFLE(3, 0, 2, 1));
        result[i].simd = _mm_sub_ps(_mm_mul_ps(a_yzx, b_zxy), _mm_mul_ps(a_zxy, b_yzx));
    }
}

void simd_vec3_lerp_array(vec3_t* result, const vec3_t* a, const vec3_t* b, float t, int count) {
    __m128 t_vec = _mm_set1_ps(t);
    __m128 one_minus_t = _mm_set1_ps(1.0f - t);
    
    for (int i = 0; i < count; i++) {
        __m128 a_scaled = _mm_mul_ps(a[i].simd, one_minus_t);
        __m128 b_scaled = _mm_mul_ps(b[i].simd, t_vec);
        result[i].simd = _mm_add_ps(a_scaled, b_scaled);
    }
}

void simd_vec4_min_array(vec4_t* result, const vec4_t* a, const vec4_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i].simd = _mm_min_ps(a[i].simd, b[i].simd);
    }
}

void simd_vec4_max_array(vec4_t* result, const vec4_t* a, const vec4_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i].simd = _mm_max_ps(a[i].simd, b[i].simd);
    }
}

void simd_vec4_clamp_array(vec4_t* result, const vec4_t* a, float min_val, float max_val, int count) {
    __m128 min_vec = _mm_set1_ps(min_val);
    __m128 max_vec = _mm_set1_ps(max_val);
    
    for (int i = 0; i < count; i++) {
        __m128 clamped = _mm_max_ps(a[i].simd, min_vec);
        result[i].simd = _mm_min_ps(clamped, max_vec);
    }
}

void simd_vec4_abs_array(vec4_t* result, const vec4_t* a, int count) {
    __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
    
    for (int i = 0; i < count; i++) {
        result[i].simd = _mm_and_ps(a[i].simd, mask);
    }
}

#elif defined(RENDERING_SIMD_NEON)

void simd_vec3_add_array(vec3_t* result, const vec3_t* a, const vec3_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i].simd = vaddq_f32(a[i].simd, b[i].simd);
    }
}

void simd_vec3_sub_array(vec3_t* result, const vec3_t* a, const vec3_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i].simd = vsubq_f32(a[i].simd, b[i].simd);
    }
}

void simd_vec3_scale_array(vec3_t* result, const vec3_t* a, float s, int count) {
    float32x4_t scale_vec = vdupq_n_f32(s);
    for (int i = 0; i < count; i++) {
        result[i].simd = vmulq_f32(a[i].simd, scale_vec);
    }
}

void simd_vec4_add_array(vec4_t* result, const vec4_t* a, const vec4_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i].simd = vaddq_f32(a[i].simd, b[i].simd);
    }
}

void simd_vec4_mul_array(vec4_t* result, const vec4_t* a, const vec4_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i].simd = vmulq_f32(a[i].simd, b[i].simd);
    }
}

float simd_vec3_dot_array(const vec3_t* a, const vec3_t* b, int count) {
    float sum = 0.0f;
    
    for (int i = 0; i < count; i++) {
        float32x4_t mul = vmulq_f32(a[i].simd, b[i].simd);
        sum += vaddvq_f32(mul);
    }
    
    return sum;
}

void simd_vec3_normalize_array(vec3_t* vecs, int count) {
    for (int i = 0; i < count; i++) {
        float32x4_t len_sq = vmulq_f32(vecs[i].simd, vecs[i].simd);
        float len = sqrtf(vaddvq_f32(len_sq));
        if (len > EPSILON) {
            vecs[i].simd = vmulq_n_f32(vecs[i].simd, 1.0f / len);
        }
    }
}

void simd_vec3_cross_array(vec3_t* result, const vec3_t* a, const vec3_t* b, int count) {
    for (int i = 0; i < count; i++) {
        // For NEON, fall back to scalar implementation for cross product
        result[i].x = a[i].y * b[i].z - a[i].z * b[i].y;
        result[i].y = a[i].z * b[i].x - a[i].x * b[i].z;
        result[i].z = a[i].x * b[i].y - a[i].y * b[i].x;
        result[i]._w = 0.0f;
        result[i].simd = vld1q_f32(result[i].e);
    }
}

void simd_vec3_lerp_array(vec3_t* result, const vec3_t* a, const vec3_t* b, float t, int count) {
    float32x4_t t_vec = vdupq_n_f32(t);
    float32x4_t one_minus_t = vdupq_n_f32(1.0f - t);
    
    for (int i = 0; i < count; i++) {
        float32x4_t a_scaled = vmulq_f32(a[i].simd, one_minus_t);
        float32x4_t b_scaled = vmulq_f32(b[i].simd, t_vec);
        result[i].simd = vaddq_f32(a_scaled, b_scaled);
    }
}

void simd_vec4_min_array(vec4_t* result, const vec4_t* a, const vec4_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i].simd = vminq_f32(a[i].simd, b[i].simd);
    }
}

void simd_vec4_max_array(vec4_t* result, const vec4_t* a, const vec4_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i].simd = vmaxq_f32(a[i].simd, b[i].simd);
    }
}

void simd_vec4_clamp_array(vec4_t* result, const vec4_t* a, float min_val, float max_val, int count) {
    float32x4_t min_vec = vdupq_n_f32(min_val);
    float32x4_t max_vec = vdupq_n_f32(max_val);
    
    for (int i = 0; i < count; i++) {
        float32x4_t clamped = vmaxq_f32(a[i].simd, min_vec);
        result[i].simd = vminq_f32(clamped, max_vec);
    }
}

void simd_vec4_abs_array(vec4_t* result, const vec4_t* a, int count) {
    for (int i = 0; i < count; i++) {
        result[i].simd = vabsq_f32(a[i].simd);
    }
}

#else

// Scalar fallback implementations
void simd_vec3_add_array(vec3_t* result, const vec3_t* a, const vec3_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i] = vec3_add(a[i], b[i]);
    }
}

void simd_vec3_sub_array(vec3_t* result, const vec3_t* a, const vec3_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i] = vec3_sub(a[i], b[i]);
    }
}

void simd_vec3_scale_array(vec3_t* result, const vec3_t* a, float s, int count) {
    for (int i = 0; i < count; i++) {
        result[i] = vec3_scale(a[i], s);
    }
}

void simd_vec4_add_array(vec4_t* result, const vec4_t* a, const vec4_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i] = vec4_add(a[i], b[i]);
    }
}

void simd_vec4_mul_array(vec4_t* result, const vec4_t* a, const vec4_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i] = vec4_mul(a[i], b[i]);
    }
}

float simd_vec3_dot_array(const vec3_t* a, const vec3_t* b, int count) {
    float sum = 0.0f;
    for (int i = 0; i < count; i++) {
        sum += vec3_dot(a[i], b[i]);
    }
    return sum;
}

void simd_vec3_normalize_array(vec3_t* vecs, int count) {
    for (int i = 0; i < count; i++) {
        vecs[i] = vec3_normalize(vecs[i]);
    }
}

void simd_vec3_cross_array(vec3_t* result, const vec3_t* a, const vec3_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i] = vec3_cross(a[i], b[i]);
    }
}

void simd_vec3_lerp_array(vec3_t* result, const vec3_t* a, const vec3_t* b, float t, int count) {
    for (int i = 0; i < count; i++) {
        result[i] = vec3_lerp(a[i], b[i], t);
    }
}

void simd_vec4_min_array(vec4_t* result, const vec4_t* a, const vec4_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i] = vec4_min(a[i], b[i]);
    }
}

void simd_vec4_max_array(vec4_t* result, const vec4_t* a, const vec4_t* b, int count) {
    for (int i = 0; i < count; i++) {
        result[i] = vec4_max(a[i], b[i]);
    }
}

void simd_vec4_clamp_array(vec4_t* result, const vec4_t* a, float min_val, float max_val, int count) {
    for (int i = 0; i < count; i++) {
        result[i] = vec4_clamp(a[i], min_val, max_val);
    }
}

void simd_vec4_abs_array(vec4_t* result, const vec4_t* a, int count) {
    for (int i = 0; i < count; i++) {
        result[i] = vec4_abs(a[i]);
    }
}

#endif
