#ifndef RENDERING_VEC3_H
#define RENDERING_VEC3_H

#include <math.h>
#include "../../../include/common.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define RENDERING_SIMD_SSE
    #include <immintrin.h>
    typedef __m128 simd_vec3_t;
#elif defined(__arm64__) || defined(__aarch64__) || defined(_M_ARM64)
    #define RENDERING_SIMD_NEON
    #include <arm_neon.h>
    typedef float32x4_t simd_vec3_t;
#endif

// vec3_t is padded to 4 floats for SIMD efficiency
typedef union ALIGN(16) vec3 {
    struct { float x, y, z, _w; };
    float e[4];
    simd_vec3_t simd;
} vec3_t;

static inline vec3_t vec3_set(float x, float y, float z) {
    vec3_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_setr_ps(x, y, z, 0.0f);
#elif defined(RENDERING_SIMD_NEON)
    float data[4] = {x, y, z, 0.0f};
    v.simd = vld1q_f32(data);
#else
    v.x = x; v.y = y; v.z = z; v._w = 0.0f;
#endif
    return v;
}

static inline vec3_t vec3_zero(void) {
    vec3_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_setzero_ps();
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vdupq_n_f32(0.0f);
#else
    v.x = v.y = v.z = v._w = 0.0f;
#endif
    return v;
}

static inline vec3_t vec3_add(vec3_t a, vec3_t b) {
    vec3_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_add_ps(a.simd, b.simd);
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vaddq_f32(a.simd, b.simd);
#else
    v.x = a.x + b.x; v.y = a.y + b.y; v.z = a.z + b.z;
#endif
    return v;
}

static inline vec3_t vec3_sub(vec3_t a, vec3_t b) {
    vec3_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_sub_ps(a.simd, b.simd);
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vsubq_f32(a.simd, b.simd);
#else
    v.x = a.x - b.x; v.y = a.y - b.y; v.z = a.z - b.z;
#endif
    return v;
}

static inline float vec3_dot(vec3_t a, vec3_t b) {
#if defined(RENDERING_SIMD_SSE)
    __m128 res = _mm_dp_ps(a.simd, b.simd, 0x7F);
    return _mm_cvtss_f32(res);
#elif defined(RENDERING_SIMD_NEON)
    float32x4_t mul = vmulq_f32(a.simd, b.simd);
    // Zero out the 4th component before summing if necessary, 
    // but usually it's already 0 for vec3_t
    return vaddvq_f32(mul); 
#else
    return a.x * b.x + a.y * b.y + a.z * b.z;
#endif
}

static inline vec3_t vec3_cross(vec3_t a, vec3_t b) {
    vec3_t v;
#if defined(RENDERING_SIMD_SSE)
    __m128 a_yzx = _mm_shuffle_ps(a.simd, a.simd, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 b_zxy = _mm_shuffle_ps(b.simd, b.simd, _MM_SHUFFLE(3, 1, 0, 2));
    __m128 a_zxy = _mm_shuffle_ps(a.simd, a.simd, _MM_SHUFFLE(3, 1, 0, 2));
    __m128 b_yzx = _mm_shuffle_ps(b.simd, b.simd, _MM_SHUFFLE(3, 0, 2, 1));
    v.simd = _mm_sub_ps(_mm_mul_ps(a_yzx, b_zxy), _mm_mul_ps(a_zxy, b_yzx));
#elif defined(RENDERING_SIMD_NEON)
    // a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x
    float32x4_t a_yzx = vcombine_f32(vrev64_f32(vget_low_f32(a.simd)), vget_high_f32(a.simd)); // Not quite shuffle
    // For NEON, easier to just use standard formula or vextq_f32
    v.x = a.y * b.z - a.z * b.y;
    v.y = a.z * b.x - a.x * b.z;
    v.z = a.x * b.y - a.y * b.x;
    v._w = 0.0f;
    v.simd = vld1q_f32(v.e);
#else
    v.x = a.y * b.z - a.z * b.y;
    v.y = a.z * b.x - a.x * b.z;
    v.z = a.x * b.y - a.y * b.x;
    v._w = 0.0f;
#endif
    return v;
    return v;
}

static inline vec3_t vec3_scale(vec3_t a, float s) {
    vec3_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_mul_ps(a.simd, _mm_set1_ps(s));
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vmulq_n_f32(a.simd, s);
#else
    v.x = a.x * s; v.y = a.y * s; v.z = a.z * s;
    v._w = 0.0f;
#endif
    return v;
}

static inline float vec3_length(vec3_t a) {
    return sqrtf(vec3_dot(a, a));
}

static inline vec3_t vec3_min(vec3_t a, vec3_t b) {
    vec3_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_min_ps(a.simd, b.simd);
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vminq_f32(a.simd, b.simd);
#else
    v.x = a.x < b.x ? a.x : b.x;
    v.y = a.y < b.y ? a.y : b.y;
    v.z = a.z < b.z ? a.z : b.z;
    v._w = 0.0f;
#endif
    return v;
}

static inline vec3_t vec3_max(vec3_t a, vec3_t b) {
    vec3_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_max_ps(a.simd, b.simd);
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vmaxq_f32(a.simd, b.simd);
#else
    v.x = a.x > b.x ? a.x : b.x;
    v.y = a.y > b.y ? a.y : b.y;
    v.z = a.z > b.z ? a.z : b.z;
    v._w = 0.0f;
#endif
    return v;
}

static inline vec3_t vec3_normalize(vec3_t a) {
    float len = sqrtf(vec3_dot(a, a));
    if (len < EPSILON) return vec3_zero();
    float inv_len = 1.0f / len;
    vec3_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_mul_ps(a.simd, _mm_set1_ps(inv_len));
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vmulq_n_f32(a.simd, inv_len);
#else
    v.x = a.x * inv_len; v.y = a.y * inv_len; v.z = a.z * inv_len;
#endif
    return v;
}

#endif // RENDERING_VEC3_H
