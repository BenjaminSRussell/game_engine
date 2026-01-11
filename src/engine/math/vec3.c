#include <math/vec3.h>
#include <math.h>

float vec3_distance(vec3_t a, vec3_t b) {
    vec3_t diff = vec3_sub(a, b);
    return vec3_length(diff);
}

float vec3_angle(vec3_t a, vec3_t b) {
    float dot = vec3_dot(a, b);
    float len_a = vec3_length(a);
    float len_b = vec3_length(b);
    if (len_a < EPSILON || len_b < EPSILON) return 0.0f;
    return acosf(dot / (len_a * len_b));
}

vec3_t vec3_lerp(vec3_t a, vec3_t b, float t) {
    return vec3_add(a, vec3_scale(vec3_sub(b, a), t));
}

vec3_t vec3_slerp(vec3_t a, vec3_t b, float t) {
    float angle = vec3_angle(a, b);
    if (angle < EPSILON) return vec3_lerp(a, b, t);
    
    float sin_angle = sinf(angle);
    float inv_sin_angle = 1.0f / sin_angle;
    
    float coeff1 = sinf((1.0f - t) * angle) * inv_sin_angle;
    float coeff2 = sinf(t * angle) * inv_sin_angle;
    
    return vec3_add(vec3_scale(a, coeff1), vec3_scale(b, coeff2));
}

vec3_t vec3_reflect(vec3_t v, vec3_t normal) {
    float dot = vec3_dot(v, normal);
    return vec3_sub(v, vec3_scale(normal, 2.0f * dot));
}

vec3_t vec3_project(vec3_t a, vec3_t b) {
    float dot = vec3_dot(a, b);
    float len_sq = vec3_dot(b, b);
    if (len_sq < EPSILON) return vec3_zero();
    return vec3_scale(b, dot / len_sq);
}

vec3_t vec3_project_plane(vec3_t v, vec3_t normal) {
    return vec3_sub(v, vec3_project(v, normal));
}

bool vec3_equals(vec3_t a, vec3_t b) {
    return fabsf(a.x - b.x) < EPSILON && 
           fabsf(a.y - b.y) < EPSILON && 
           fabsf(a.z - b.z) < EPSILON;
}

vec3_t vec3_face_forward(vec3_t n, vec3_t i, vec3_t n_ref) {
    return vec3_dot(n_ref, i) < 0.0f ? n : vec3_scale(n, -1.0f);
}
