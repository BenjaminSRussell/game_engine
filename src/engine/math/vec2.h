#ifndef RENDERING_VEC2_H
#define RENDERING_VEC2_H

#include "include/common.h"
#include <math/math.h>

typedef struct vec2 {
    float x, y;
} vec2_t;

static inline vec2_t vec2_set(float x, float y) {
    return (vec2_t){ x, y };
}

static inline vec2_t vec2_zero(void) {
    return (vec2_t){ 0.0f, 0.0f };
}

static inline vec2_t vec2_add(vec2_t a, vec2_t b) {
    return (vec2_t){ a.x + b.x, a.y + b.y };
}

static inline vec2_t vec2_sub(vec2_t a, vec2_t b) {
    return (vec2_t){ a.x - b.x, a.y - b.y };
}

static inline vec2_t vec2_mul(vec2_t a, vec2_t b) {
    return (vec2_t){ a.x * b.x, a.y * b.y };
}

static inline vec2_t vec2_scale(vec2_t a, float s) {
    return (vec2_t){ a.x * s, a.y * s };
}

static inline float vec2_dot(vec2_t a, vec2_t b) {
    return a.x * b.x + a.y * b.y;
}

static inline float vec2_length(vec2_t a) {
    return sqrtf(vec2_dot(a, a));
}

static inline vec2_t vec2_normalize(vec2_t a) {
    float len = vec2_length(a);
    if (len < EPSILON) return vec2_zero();
    return vec2_scale(a, 1.0f / len);
}

static inline vec2_t vec2_lerp(vec2_t a, vec2_t b, float t) {
    return (vec2_t){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t
    };
}

#endif // RENDERING_VEC2_H
