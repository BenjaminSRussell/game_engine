#include "vec2.h"
#include <math.h>

vec2_t vec2_set(float x, float y) {
    return (vec2_t){x, y};
}

vec2_t vec2_zero(void) {
    return (vec2_t){0, 0};
}

vec2_t vec2_add(vec2_t a, vec2_t b) {
    return (vec2_t){a.x + b.x, a.y + b.y};
}

vec2_t vec2_sub(vec2_t a, vec2_t b) {
    return (vec2_t){a.x - b.x, a.y - b.y};
}

vec2_t vec2_mul(vec2_t a, vec2_t b) {
    return (vec2_t){a.x * b.x, a.y * b.y};
}

vec2_t vec2_scale(vec2_t a, float s) {
    return (vec2_t){a.x * s, a.y * s};
}

float vec2_dot(vec2_t a, vec2_t b) {
    return a.x * b.x + a.y * b.y;
}

float vec2_length(vec2_t a) {
    return sqrtf(a.x * a.x + a.y * a.y);
}

vec2_t vec2_normalize(vec2_t a) {
    float len = vec2_length(a);
    if (len < EPSILON) return vec2_zero();
    return vec2_scale(a, 1.0f / len);
}

vec2_t vec2_lerp(vec2_t a, vec2_t b, float t) {
    return (vec2_t){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t
    };
}
