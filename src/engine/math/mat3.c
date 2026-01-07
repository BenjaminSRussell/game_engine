#include "math/mat3.h"
#include <math/math.h>

mat3_t mat3_identity(void) {
    mat3_t res = {0};
    res.e[0][0] = 1.0f;
    res.e[1][1] = 1.0f;
    res.e[2][2] = 1.0f;
    return res;
}

mat3_t mat3_zero(void) {
    mat3_t res = {0};
    return res;
}

mat3_t mat3_mul(mat3_t a, mat3_t b) {
    mat3_t out = {0};
    for (int c = 0; c < 3; c++) {
        for (int r = 0; r < 3; r++) {
            out.e[c][r] = a.e[0][r] * b.e[c][0] +
                         a.e[1][r] * b.e[c][1] +
                         a.e[2][r] * b.e[c][2];
        }
    }
    return out;
}

mat3_t mat3_transpose(mat3_t m) {
    mat3_t res;
    res.e[0][0] = m.e[0][0]; res.e[1][0] = m.e[0][1]; res.e[2][0] = m.e[0][2];
    res.e[0][1] = m.e[1][0]; res.e[1][1] = m.e[1][1]; res.e[2][1] = m.e[1][2];
    res.e[0][2] = m.e[2][0]; res.e[1][2] = m.e[2][1]; res.e[2][2] = m.e[2][2];
    return res;
}

mat3_t mat3_inverse(mat3_t m) {
    float det = m.e[0][0] * (m.e[1][1] * m.e[2][2] - m.e[2][1] * m.e[1][2]) -
                m.e[1][0] * (m.e[0][1] * m.e[2][2] - m.e[2][1] * m.e[0][2]) +
                m.e[2][0] * (m.e[0][1] * m.e[1][2] - m.e[1][1] * m.e[0][2]);

    if (det == 0.0f) return mat3_identity();
    float inv_det = 1.0f / det;

    mat3_t res;
    res.e[0][0] =  (m.e[1][1] * m.e[2][2] - m.e[2][1] * m.e[1][2]) * inv_det;
    res.e[1][0] = -(m.e[1][0] * m.e[2][2] - m.e[2][0] * m.e[1][2]) * inv_det;
    res.e[2][0] =  (m.e[1][0] * m.e[2][1] - m.e[2][0] * m.e[1][1]) * inv_det;
    res.e[0][1] = -(m.e[0][1] * m.e[2][2] - m.e[2][1] * m.e[0][2]) * inv_det;
    res.e[1][1] =  (m.e[0][0] * m.e[2][2] - m.e[2][0] * m.e[0][2]) * inv_det;
    res.e[2][1] = -(m.e[0][0] * m.e[2][1] - m.e[2][0] * m.e[0][1]) * inv_det;
    res.e[0][2] =  (m.e[0][1] * m.e[1][2] - m.e[1][1] * m.e[0][2]) * inv_det;
    res.e[1][2] = -(m.e[0][0] * m.e[1][2] - m.e[1][0] * m.e[0][2]) * inv_det;
    res.e[2][2] =  (m.e[0][0] * m.e[1][1] - m.e[1][0] * m.e[0][1]) * inv_det;
    return res;
}

vec3_t mat3_mul_vec3(mat3_t m, vec3_t v) {
    return vec3_set(
        m.e[0][0] * v.x + m.e[1][0] * v.y + m.e[2][0] * v.z,
        m.e[0][1] * v.x + m.e[1][1] * v.y + m.e[2][1] * v.z,
        m.e[0][2] * v.x + m.e[1][2] * v.y + m.e[2][2] * v.z
    );
}
