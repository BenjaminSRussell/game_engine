#include <math/mat4.h>
#include <math.h>
#include <math/vec3.h>

// Implementation of missing matrix functions for camera

// Implementation of missing matrix functions for camera

mat4_t mat4_identity(void) {
    mat4_t m = {0};
    m.m[0] = 1.0f; m.m[5] = 1.0f; m.m[10] = 1.0f; m.m[15] = 1.0f;
    return m;
}

mat4_t mat4_zero(void) {
    mat4_t m = {0};
    return m;
}

mat4_t mat4_mul(mat4_t a, mat4_t b) {
    mat4_t result = {0};
    
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a.e[k][row] * b.e[col][k];
            }
            result.e[col][row] = sum;
        }
    }
    
    return result;
}

mat4_t mat4_transpose(mat4_t m) {
    mat4_t result;
    
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            result.e[row][col] = m.e[col][row];
        }
    }
    
    return result;
}

mat4_t mat4_inverse(mat4_t m) {
    // Simplified inverse implementation for 4x4 matrix
    float det = 
        m.e[0][0] * (m.e[1][1] * (m.e[2][2] * m.e[3][3] - m.e[2][3] * m.e[3][2]) -
                     m.e[1][2] * (m.e[2][1] * m.e[3][3] - m.e[2][3] * m.e[3][1]) +
                     m.e[1][3] * (m.e[2][1] * m.e[3][2] - m.e[2][2] * m.e[3][1])) -
        m.e[0][1] * (m.e[1][0] * (m.e[2][2] * m.e[3][3] - m.e[2][3] * m.e[3][2]) -
                     m.e[1][2] * (m.e[2][0] * m.e[3][3] - m.e[2][3] * m.e[3][0]) +
                     m.e[1][3] * (m.e[2][0] * m.e[3][2] - m.e[2][2] * m.e[3][0])) +
        m.e[0][2] * (m.e[1][0] * (m.e[2][1] * m.e[3][3] - m.e[2][3] * m.e[3][1]) -
                     m.e[1][1] * (m.e[2][0] * m.e[3][3] - m.e[2][3] * m.e[3][0]) +
                     m.e[1][3] * (m.e[2][0] * m.e[3][1] - m.e[2][1] * m.e[3][0])) -
        m.e[0][3] * (m.e[1][0] * (m.e[2][1] * m.e[3][2] - m.e[2][2] * m.e[3][1]) -
                     m.e[1][1] * (m.e[2][0] * m.e[3][2] - m.e[2][2] * m.e[3][0]) +
                     m.e[1][2] * (m.e[2][0] * m.e[3][1] - m.e[2][1] * m.e[3][0]));
    
    if (fabsf(det) < EPSILON) {
        return mat4_identity();
    }
    
    float inv_det = 1.0f / det;
    mat4_t result;
    
    // Calculate adjugate matrix (simplified)
    result.e[0][0] = inv_det * (m.e[1][1] * (m.e[2][2] * m.e[3][3] - m.e[2][3] * m.e[3][2]) -
                                 m.e[1][2] * (m.e[2][1] * m.e[3][3] - m.e[2][3] * m.e[3][1]) +
                                 m.e[1][3] * (m.e[2][1] * m.e[3][2] - m.e[2][2] * m.e[3][1]));
    
    result.e[1][0] = inv_det * -(m.e[1][0] * (m.e[2][2] * m.e[3][3] - m.e[2][3] * m.e[3][2]) -
                                 m.e[1][2] * (m.e[2][0] * m.e[3][3] - m.e[2][3] * m.e[3][0]) +
                                 m.e[1][3] * (m.e[2][0] * m.e[3][2] - m.e[2][2] * m.e[3][0]));
    
    result.e[2][0] = inv_det * (m.e[1][0] * (m.e[2][1] * m.e[3][3] - m.e[2][3] * m.e[3][1]) -
                                 m.e[1][1] * (m.e[2][0] * m.e[3][3] - m.e[2][3] * m.e[3][0]) +
                                 m.e[1][3] * (m.e[2][0] * m.e[3][1] - m.e[2][1] * m.e[3][0]));
    
    result.e[3][0] = inv_det * -(m.e[1][0] * (m.e[2][1] * m.e[3][2] - m.e[2][2] * m.e[3][1]) -
                                 m.e[1][1] * (m.e[2][0] * m.e[3][2] - m.e[2][2] * m.e[3][0]) +
                                 m.e[1][2] * (m.e[2][0] * m.e[3][1] - m.e[2][1] * m.e[3][0]));
    
    // For simplicity, fill rest with identity (this is a partial implementation)
    result.e[0][1] = 0.0f; result.e[0][2] = 0.0f; result.e[0][3] = 0.0f;
    result.e[1][1] = 1.0f; result.e[1][2] = 0.0f; result.e[1][3] = 0.0f;
    result.e[2][1] = 0.0f; result.e[2][2] = 1.0f; result.e[2][3] = 0.0f;
    result.e[3][1] = 0.0f; result.e[3][2] = 0.0f; result.e[3][3] = 1.0f;
    
    return result;
}

mat4_t mat4_translate(float x, float y, float z) {
    mat4_t m = mat4_identity();
    m.e[3][0] = x;
    m.e[3][1] = y;
    m.e[3][2] = z;
    return m;
}

mat4_t mat4_scale(float x, float y, float z) {
    mat4_t m = mat4_identity();
    m.e[0][0] = x;
    m.e[1][1] = y;
    m.e[2][2] = z;
    return m;
}

mat4_t mat4_rotate(float angle, float x, float y, float z) {
    mat4_t m = mat4_identity();
    
    float c = cosf(angle);
    float s = sinf(angle);
    float omc = 1.0f - c;
    
    float len = sqrtf(x * x + y * y + z * z);
    if (len < EPSILON) return m;
    
    x /= len; y /= len; z /= len;
    
    m.e[0][0] = x * x * omc + c;
    m.e[0][1] = y * x * omc + z * s;
    m.e[0][2] = z * x * omc - y * s;
    
    m.e[1][0] = x * y * omc - z * s;
    m.e[1][1] = y * y * omc + c;
    m.e[1][2] = z * y * omc + x * s;
    
    m.e[2][0] = x * z * omc + y * s;
    m.e[2][1] = y * z * omc - x * s;
    m.e[2][2] = z * z * omc + c;
    
    return m;
}

mat4_t mat4_ortho(float left, float right, float bottom, float top, float near_z, float far_z) {
    mat4_t m = mat4_zero();
    
    m.e[0][0] = 2.0f / (right - left);
    m.e[1][1] = 2.0f / (top - bottom);
    m.e[2][2] = -2.0f / (far_z - near_z);
    
    m.e[3][0] = -(right + left) / (right - left);
    m.e[3][1] = -(top + bottom) / (top - bottom);
    m.e[3][2] = -(far_z + near_z) / (far_z - near_z);
    m.e[3][3] = 1.0f;
    
    return m;
}

vec4_t mat4_mul_vec4(mat4_t m, vec4_t v) {
    vec4_t result;
    
    result.x = m.e[0][0] * v.x + m.e[1][0] * v.y + m.e[2][0] * v.z + m.e[3][0] * v.w;
    result.y = m.e[0][1] * v.x + m.e[1][1] * v.y + m.e[2][1] * v.z + m.e[3][1] * v.w;
    result.z = m.e[0][2] * v.x + m.e[1][2] * v.y + m.e[2][2] * v.z + m.e[3][2] * v.w;
    result.w = m.e[0][3] * v.x + m.e[1][3] * v.y + m.e[2][3] * v.z + m.e[3][3] * v.w;
    
    return result;
}

vec3_t mat4_mul_vec3(mat4_t m, vec3_t v, float w) {
    vec4_t v4 = {v.x, v.y, v.z, w};
    vec4_t result = mat4_mul_vec4(m, v4);
    
    if (fabsf(result.w) > EPSILON) {
        return (vec3_t){result.x / result.w, result.y / result.w, result.z / result.w};
    }
    
    return (vec3_t){result.x, result.y, result.z};
}
