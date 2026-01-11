#include <math/mat3.h>
#include <math/vec3.h>
#include <math.h>
#include <string.h>

mat3_t mat3_identity(void) {
    mat3_t m = {0};
    m.m[0] = 1.0f; m.m[4] = 1.0f; m.m[8] = 1.0f;
    return m;
}

mat3_t mat3_zero(void) {
    mat3_t m = {0};
    return m;
}

mat3_t mat3_mul(mat3_t a, mat3_t b) {
    mat3_t result = {0};
    
    for (int col = 0; col < 3; col++) {
        for (int row = 0; row < 3; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 3; k++) {
                sum += a.e[k][row] * b.e[col][k];
            }
            result.e[col][row] = sum;
        }
    }
    
    return result;
}

mat3_t mat3_transpose(mat3_t m) {
    mat3_t result;
    
    for (int col = 0; col < 3; col++) {
        for (int row = 0; row < 3; row++) {
            result.e[row][col] = m.e[col][row];
        }
    }
    
    return result;
}

mat3_t mat3_inverse(mat3_t m) {
    // Calculate determinant
    float det = 
        m.e[0][0] * (m.e[1][1] * m.e[2][2] - m.e[2][1] * m.e[1][2]) -
        m.e[1][0] * (m.e[0][1] * m.e[2][2] - m.e[2][1] * m.e[0][2]) +
        m.e[2][0] * (m.e[0][1] * m.e[1][2] - m.e[1][1] * m.e[0][2]);
    
    if (fabsf(det) < EPSILON) {
        return mat3_identity(); // Return identity if not invertible
    }
    
    float inv_det = 1.0f / det;
    
    mat3_t result;
    
    // Calculate inverse using adjugate matrix
    result.e[0][0] = inv_det * (m.e[1][1] * m.e[2][2] - m.e[2][1] * m.e[1][2]);
    result.e[0][1] = inv_det * (m.e[2][1] * m.e[0][2] - m.e[0][1] * m.e[2][2]);
    result.e[0][2] = inv_det * (m.e[0][1] * m.e[1][2] - m.e[1][1] * m.e[0][2]);
    
    result.e[1][0] = inv_det * (m.e[2][0] * m.e[1][2] - m.e[1][0] * m.e[2][2]);
    result.e[1][1] = inv_det * (m.e[0][0] * m.e[2][2] - m.e[2][0] * m.e[0][2]);
    result.e[1][2] = inv_det * (m.e[1][0] * m.e[0][2] - m.e[0][0] * m.e[1][2]);
    
    result.e[2][0] = inv_det * (m.e[1][0] * m.e[2][1] - m.e[2][0] * m.e[1][1]);
    result.e[2][1] = inv_det * (m.e[2][0] * m.e[0][1] - m.e[0][0] * m.e[2][1]);
    result.e[2][2] = inv_det * (m.e[0][0] * m.e[1][1] - m.e[1][0] * m.e[0][1]);
    
    return result;
}

vec3_t mat3_mul_vec3(mat3_t m, vec3_t v) {
    vec3_t result;
    
    result.x = m.e[0][0] * v.x + m.e[1][0] * v.y + m.e[2][0] * v.z;
    result.y = m.e[0][1] * v.x + m.e[1][1] * v.y + m.e[2][1] * v.z;
    result.z = m.e[0][2] * v.x + m.e[1][2] * v.y + m.e[2][2] * v.z;
    
    return result;
}

mat3_t mat3_from_mat4(mat4_t m) {
    mat3_t result;
    
    // Extract the upper-left 3x3 from the 4x4 matrix
    result.e[0][0] = m.e[0][0]; result.e[1][0] = m.e[1][0]; result.e[2][0] = m.e[2][0];
    result.e[0][1] = m.e[0][1]; result.e[1][1] = m.e[1][1]; result.e[2][1] = m.e[2][1];
    result.e[0][2] = m.e[0][2]; result.e[1][2] = m.e[1][2]; result.e[2][2] = m.e[2][2];
    
    return result;
}

mat3_t mat3_scale(float x, float y, float z) {
    mat3_t m = mat3_identity();
    m.e[0][0] = x;
    m.e[1][1] = y;
    m.e[2][2] = z;
    return m;
}

mat3_t mat3_rotate_x(float angle) {
    mat3_t m = mat3_identity();
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    
    m.e[1][1] = cos_a;
    m.e[2][1] = -sin_a;
    m.e[1][2] = sin_a;
    m.e[2][2] = cos_a;
    
    return m;
}

mat3_t mat3_rotate_y(float angle) {
    mat3_t m = mat3_identity();
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    
    m.e[0][0] = cos_a;
    m.e[2][0] = sin_a;
    m.e[0][2] = -sin_a;
    m.e[2][2] = cos_a;
    
    return m;
}

mat3_t mat3_rotate_z(float angle) {
    mat3_t m = mat3_identity();
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    
    m.e[0][0] = cos_a;
    m.e[1][0] = -sin_a;
    m.e[0][1] = sin_a;
    m.e[1][1] = cos_a;
    
    return m;
}

float mat3_determinant(mat3_t m) {
    return 
        m.e[0][0] * (m.e[1][1] * m.e[2][2] - m.e[2][1] * m.e[1][2]) -
        m.e[1][0] * (m.e[0][1] * m.e[2][2] - m.e[2][1] * m.e[0][2]) +
        m.e[2][0] * (m.e[0][1] * m.e[1][2] - m.e[1][1] * m.e[0][2]);
}
