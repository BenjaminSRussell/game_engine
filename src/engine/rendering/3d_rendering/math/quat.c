#include "quat.h"
#include <math.h>

quat_t quat_identity(void) {
    return (quat_t){ 0.0f, 0.0f, 0.0f, 1.0f };
}

quat_t quat_from_axis_angle(vec3_t axis, float angle) {
    float half_angle = angle * 0.5f;
    float s = sinf(half_angle);
    vec3_t n = vec3_normalize(axis);
    return (quat_t){ n.x * s, n.y * s, n.z * s, cosf(half_angle) };
}

quat_t quat_mul(quat_t q1, quat_t q2) {
    return (quat_t){
        q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
        q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
        q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
        q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z
    };
}

quat_t quat_normalize(quat_t q) {
    float len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len < EPSILON) return quat_identity();
    float inv_len = 1.0f / len;
    return (quat_t){ q.x * inv_len, q.y * inv_len, q.z * inv_len, q.w * inv_len };
}

quat_t quat_slerp(quat_t a, quat_t b, float t) {
    float cos_half_theta = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    
    if (cos_half_theta < 0.0f) {
        b.x = -b.x; b.y = -b.y; b.z = -b.z; b.w = -b.w;
        cos_half_theta = -cos_half_theta;
    }
    
    if (fabsf(cos_half_theta) >= 1.0f) return a;
    
    float half_theta = acosf(cos_half_theta);
    float sin_half_theta = sqrtf(1.0f - cos_half_theta * cos_half_theta);
    
    if (fabsf(sin_half_theta) < 0.001f) {
        return (quat_t){
            a.x * 0.5f + b.x * 0.5f,
            a.y * 0.5f + b.y * 0.5f,
            a.z * 0.5f + b.z * 0.5f,
            a.w * 0.5f + b.w * 0.5f
        };
    }
    
    float ratio_a = sinf((1.0f - t) * half_theta) / sin_half_theta;
    float ratio_b = sinf(t * half_theta) / sin_half_theta;
    
    return (quat_t){
        a.x * ratio_a + b.x * ratio_b,
        a.y * ratio_a + b.y * ratio_b,
        a.z * ratio_a + b.z * ratio_b,
        a.w * ratio_a + b.w * ratio_b
    };
}

mat4_t quat_to_mat4(quat_t q) {
    mat4_t res = mat4_identity();
    float xx = q.x * q.x;
    float yy = q.y * q.y;
    float zz = q.z * q.z;
    float xy = q.x * q.y;
    float xz = q.x * q.z;
    float yz = q.y * q.z;
    float wx = q.w * q.x;
    float wy = q.w * q.y;
    float wz = q.w * q.z;

    res.e[0][0] = 1.0f - 2.0f * (yy + zz);
    res.e[1][0] = 2.0f * (xy - wz);
    res.e[2][0] = 2.0f * (xz + wy);

    res.e[0][1] = 2.0f * (xy + wz);
    res.e[1][1] = 1.0f - 2.0f * (xx + zz);
    res.e[2][1] = 2.0f * (yz - wx);

    res.e[0][2] = 2.0f * (xz - wy);
    res.e[1][2] = 2.0f * (yz + wx);
    res.e[2][2] = 1.0f - 2.0f * (xx + yy);

    return res;
}
