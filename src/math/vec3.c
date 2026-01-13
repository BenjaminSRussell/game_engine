#include "include/math/vec3.h"
#include <math.h>

float vec3_angle_between(Vec3 a, Vec3 b) {
    float dot = vec3_dot(a, b);
    float len_a = vec3_length(a);
    float len_b = vec3_length(b);
    if (len_a < EPSILON || len_b < EPSILON) return 0.0f;
    return acosf(dot / (len_a * len_b));
}

Vec3 vec3_slerp(Vec3 a, Vec3 b, float t) {
    float angle = vec3_angle_between(a, b);
    if (angle < EPSILON) return vec3_lerp(a, b, t);
    
    float sin_angle = sinf(angle);
    float inv_sin_angle = 1.0f / sin_angle;
    
    float coeff1 = sinf((1.0f - t) * angle) * inv_sin_angle;
    float coeff2 = sinf(t * angle) * inv_sin_angle;
    
    return vec3_add(vec3_mul(a, coeff1), vec3_mul(b, coeff2));
}

Vec3 vec3_reflect(Vec3 v, Vec3 normal) {
    float dot = vec3_dot(v, normal);
    return vec3_sub(v, vec3_mul(normal, 2.0f * dot));
}

Vec3 vec3_project(Vec3 v, Vec3 onto) {
    float dot = vec3_dot(v, onto);
    float len_sq = vec3_dot(onto, onto);
    if (len_sq < EPSILON) return vec3_zero();
    return vec3_mul(onto, dot / len_sq);
}

Vec3 vec3_project_plane(Vec3 v, Vec3 normal) {
    return vec3_sub(v, vec3_project(v, normal));
}

bool vec3_is_equal(Vec3 a, Vec3 b, float tolerance) {
    return fabsf(a.x - b.x) < tolerance && 
           fabsf(a.y - b.y) < tolerance && 
           fabsf(a.z - b.z) < tolerance;
}

Vec3 vec3_face_forward(Vec3 n, Vec3 i, Vec3 n_ref) {
    return vec3_dot(n_ref, i) < 0.0f ? n : vec3_mul(n, -1.0f);
}
