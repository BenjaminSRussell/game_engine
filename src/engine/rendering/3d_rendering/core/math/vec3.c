#include "vec3.h"
#include <math.h>

// Note: Most vec3 functions are inlined in headers for performance.
// These are the non-inlined or complex versions if any.
// Since we use static inline for almost everything, we'll keep this as a stub
// or move the non-critical paths here if desired.
// For now, satisfy the .c requirement by providing the core constructor/utilities.

float vec3_length(vec3_t a) {
    return sqrtf(vec3_dot(a, a));
}

vec3_t vec3_lerp(vec3_t a, vec3_t b, float t) {
    vec3_t v;
#if defined(RENDERING_SIMD_SSE)
    __m128 vt = _mm_set1_ps(t);
    v.simd = _mm_add_ps(a.simd, _mm_mul_ps(_mm_sub_ps(b.simd, a.simd), vt));
#elif defined(RENDERING_SIMD_NEON)
    v.simd = vaddq_f32(a.simd, vmulq_n_f32(vsubq_f32(b.simd, a.simd), t));
#else
    v.x = a.x + (b.x - a.x) * t;
    v.y = a.y + (b.y - a.y) * t;
    v.z = a.z + (b.z - a.z) * t;
    v._w = 0.0f;
#endif
    return v;
}
