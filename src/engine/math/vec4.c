#include <math/vec4.h>
#include <math.h>

float vec4_length(vec4_t a) {
    return sqrtf(vec4_dot(a, a));
}

float vec4_distance(vec4_t a, vec4_t b) {
    vec4_t diff = vec4_sub(a, b);
    return vec4_length(diff);
}

vec4_t vec4_scale(vec4_t a, float s) {
    vec4_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_mul_ps(a.simd, _mm_set1_ps(s));
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vmulq_n_f32(a.simd, s);
#else
    v.x = a.x * s; v.y = a.y * s; v.z = a.z * s; v.w = a.w * s;
#endif
    return v;
}

vec4_t vec4_lerp(vec4_t a, vec4_t b, float t) {
    return vec4_add(a, vec4_scale(vec4_sub(b, a), t));
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

bool vec4_equals(vec4_t a, vec4_t b) {
    return fabsf(a.x - b.x) < EPSILON && 
           fabsf(a.y - b.y) < EPSILON && 
           fabsf(a.z - b.z) < EPSILON && 
           fabsf(a.w - b.w) < EPSILON;
}

vec4_t vec4_abs(vec4_t a) {
    vec4_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_and_ps(a.simd, _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF)));
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vabsq_f32(a.simd);
#else
    v.x = fabsf(a.x); v.y = fabsf(a.y); v.z = fabsf(a.z); v.w = fabsf(a.w);
#endif
    return v;
}

vec4_t vec4_clamp(vec4_t a, float min_val, float max_val) {
    vec4_t v;
#if defined(RENDERING_SIMD_SSE)
    v.simd = _mm_min_ps(_mm_max_ps(a.simd, _mm_set1_ps(min_val)), _mm_set1_ps(max_val));
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vminq_f32(vmaxq_f32(a.simd, vdupq_n_f32(min_val)), vdupq_n_f32(max_val));
#else
    v.x = a.x < min_val ? min_val : (a.x > max_val ? max_val : a.x);
    v.y = a.y < min_val ? min_val : (a.y > max_val ? max_val : a.y);
    v.z = a.z < min_val ? min_val : (a.z > max_val ? max_val : a.z);
    v.w = a.w < min_val ? min_val : (a.w > max_val ? max_val : a.w);
#endif
    return v;
}
