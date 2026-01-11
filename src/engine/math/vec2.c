#include <math/vec2.h>
#include <math.h>

float vec2_distance(vec2_t a, vec2_t b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

float vec2_angle(vec2_t a, vec2_t b) {
    float dot = vec2_dot(a, b);
    float det = a.x * b.y - a.y * b.x;
    return atan2f(det, dot);
}

vec2_t vec2_rotate(vec2_t v, float angle) {
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    return (vec2_t){
        v.x * cos_a - v.y * sin_a,
        v.x * sin_a + v.y * cos_a
    };
}

vec2_t vec2_reflect(vec2_t v, vec2_t normal) {
    float dot = vec2_dot(v, normal);
    return vec2_sub(v, vec2_scale(normal, 2.0f * dot));
}

vec2_t vec2_project(vec2_t a, vec2_t b) {
    float dot = vec2_dot(a, b);
    float len_sq = vec2_dot(b, b);
    if (len_sq < EPSILON) return vec2_zero();
    return vec2_scale(b, dot / len_sq);
}

vec2_t vec2_perpendicular(vec2_t v) {
    return (vec2_t){ -v.y, v.x };
}

bool vec2_equals(vec2_t a, vec2_t b) {
    return fabsf(a.x - b.x) < EPSILON && fabsf(a.y - b.y) < EPSILON;
}
