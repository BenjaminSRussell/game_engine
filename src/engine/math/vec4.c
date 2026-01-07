#include "math/vec4.h"
#include <math/math.h>

float vec4_length_sq(vec4_t a) {
    return vec4_dot(a, a);
}

float vec4_length(vec4_t a) {
    return sqrtf(vec4_length_sq(a));
}

vec4_t vec4_lerp(vec4_t a, vec4_t b, float t) {
    vec4_t v;
#if defined(RENDERING_SIMD_SSE)
    __m128 vt = _mm_set1_ps(t);
    v.simd = _mm_add_ps(a.simd, _mm_mul_ps(_mm_sub_ps(b.simd, a.simd), vt));
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vaddq_f32(a.simd, vmulq_n_f32(vsubq_f32(b.simd, a.simd), t));
#else
    v.x = a.x + (b.x - a.x) * t;
    v.y = a.y + (b.y - a.y) * t;
    v.z = a.z + (b.z - a.z) * t;
    v.w = a.w + (b.w - a.w) * t;
#endif
    return v;
}

vec4_t vec4_min(vec4_t a, vec4_t b) {
    vec4_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_min_ps(a.simd, b.simd);
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vminq_f32(a.simd, b.simd);
#else
    v.x = a.x < b.x ? a.x : b.x;
    v.y = a.y < b.y ? a.y : b.y;
    v.z = a.z < b.z ? a.z : b.z;
    v.w = a.w < b.w ? a.w : b.w;
#endif
    return v;
}

vec4_t vec4_max(vec4_t a, vec4_t b) {
    vec4_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_max_ps(a.simd, b.simd);
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vmaxq_f32(a.simd, b.simd);
#else
    v.x = a.x > b.x ? a.x : b.x;
    v.y = a.y > b.y ? a.y : b.y;
    v.z = a.z > b.z ? a.z : b.z;
    v.w = a.w > b.w ? a.w : b.w;
#endif
    return v;
}
