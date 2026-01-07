#ifndef RENDERING_VEC4_H
#define RENDERING_VEC4_H

#include "../../../include/common.h"

// SIMD architecture detection
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define RENDERING_SIMD_SSE
    #include <immintrin.h>
    typedef __m128 simd_vec4_t;
#elif defined(__arm64__) || defined(__aarch64__) || defined(_M_ARM64)
    #define RENDERING_SIMD_NEON
    #include <arm_neon.h>
    typedef float32x4_t simd_vec4_t;
#endif

typedef union ALIGN(16) vec4 {
    struct { float x, y, z, w; };
    float e[4];
    simd_vec4_t simd;
} vec4_t;

static inline vec4_t vec4_set(float x, float y, float z, float w) {
    vec4_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_setr_ps(x, y, z, w);
#elif defined(RENDERING_SIMD_NEON)
    float data[4] = {x, y, z, w};
    v.simd = vld1q_f32(data);
#else
    v.x = x; v.y = y; v.z = z; v.w = w;
#endif
    return v;
}

static inline vec4_t vec4_set1(float s) {
    vec4_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_set1_ps(s);
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vdupq_n_f32(s);
#else
    v.x = v.y = v.z = v.w = s;
#endif
    return v;
}

static inline vec4_t vec4_zero(void) {
    vec4_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_setzero_ps();
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vdupq_n_f32(0.0f);
#else
    v.x = v.y = v.z = v.w = 0.0f;
#endif
    return v;
}

static inline vec4_t vec4_add(vec4_t a, vec4_t b) {
    vec4_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_add_ps(a.simd, b.simd);
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vaddq_f32(a.simd, b.simd);
#else
    v.x = a.x + b.x; v.y = a.y + b.y; v.z = a.z + b.z; v.w = a.w + b.w;
#endif
    return v;
}

static inline vec4_t vec4_sub(vec4_t a, vec4_t b) {
    vec4_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_sub_ps(a.simd, b.simd);
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vsubq_f32(a.simd, b.simd);
#else
    v.x = a.x - b.x; v.y = a.y - b.y; v.z = a.z - b.z; v.w = a.w - b.w;
#endif
    return v;
}

static inline vec4_t vec4_mul(vec4_t a, vec4_t b) {
    vec4_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_mul_ps(a.simd, b.simd);
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vmulq_f32(a.simd, b.simd);
#else
    v.x = a.x * b.x; v.y = a.y * b.y; v.z = a.z * b.z; v.w = a.w * b.w;
#endif
    return v;
}

static inline vec4_t vec4_div(vec4_t a, vec4_t b) {
    vec4_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_div_ps(a.simd, b.simd);
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vdivq_f32(a.simd, b.simd);
#else
    v.x = a.x / b.x; v.y = a.y / b.y; v.z = a.z / b.z; v.w = a.w / b.w;
#endif
    return v;
}

static inline float vec4_dot(vec4_t a, vec4_t b) {
#if defined(RENDERING_SIMD_SSE)
    __m128 res = _mm_dp_ps(a.simd, b.simd, 0xFF);
    return _mm_cvtss_f32(res);
#elif defined(RENDERING_SIMD_NEON)
    return vaddvq_f32(vmulq_f32(a.simd, b.simd));
#else
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
#endif
}

static inline vec4_t vec4_normalize(vec4_t a) {
#if defined(RENDERING_SIMD_SSE)
    __m128 res = _mm_dp_ps(a.simd, a.simd, 0xFF);
    __m128 inv_len = _mm_rsqrt_ps(res);
    vec4_t v;
    v.simd = _mm_mul_ps(a.simd, inv_len);
    return v;
#elif defined(RENDERING_SIMD_NEON)
    float32x4_t res = vmulq_f32(a.simd, a.simd);
    float len = sqrtf(vaddvq_f32(res));
    if (len < EPSILON) return (vec4_t){0};
    vec4_t v;
    v.simd = vmulq_n_f32(a.simd, 1.0f / len);
    return v;
#else
    float len = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
    if (len < EPSILON) return (vec4_t){0};
    float inv_len = 1.0f / len;
    return (vec4_t){a.x * inv_len, a.y * inv_len, a.z * inv_len, a.w * inv_len};
#endif
}

#endif // RENDERING_VEC4_H
