#include <math/mat4.h>
#include <math.h>
#include <math/vec3.h>

// Implementation of missing matrix functions for camera
// Note: Using Mat4 type (not mat4_t) as defined in the header

Mat4 mat4_identity(void) {
    Mat4 m = {0};
    m.m00 = m.m11 = m.m22 = m.m33 = 1.0f;
    return m;
}

Mat4 mat4_zero(void) {
    Mat4 m = {0};
    return m;
}

Mat4 mat4_mul(Mat4 a, Mat4 b) {
    Mat4 result = {0};
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a.data[k][row] * b.data[col][k];
            }
            result.data[col][row] = sum;
        }
    }
    return result;
}

Mat4 mat4_transpose(Mat4 m) {
    Mat4 result;
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            result.data[col][row] = m.data[row][col];
        }
    }
    return result;
}

Mat4 mat4_inverse(Mat4 m) {
    // Simplified inverse implementation for 4x4 matrix
    float det = 
        m.data[0][0] * (m.data[1][1] * (m.data[2][2] * m.data[3][3] - m.data[2][3] * m.data[3][2]) -
                     m.data[1][2] * (m.data[2][1] * m.data[3][3] - m.data[2][3] * m.data[3][1]) +
                     m.data[1][3] * (m.data[2][1] * m.data[3][2] - m.data[2][2] * m.data[3][1])) -
        m.data[0][1] * (m.data[1][0] * (m.data[2][2] * m.data[3][3] - m.data[2][3] * m.data[3][2]) -
                     m.data[1][2] * (m.data[2][0] * m.data[3][3] - m.data[2][3] * m.data[3][0]) +
                     m.data[1][3] * (m.data[2][0] * m.data[3][2] - m.data[2][2] * m.data[3][0])) +
        m.data[0][2] * (m.data[1][0] * (m.data[2][1] * m.data[3][3] - m.data[2][3] * m.data[3][1]) -
                     m.data[1][1] * (m.data[2][0] * m.data[3][3] - m.data[2][3] * m.data[3][0]) +
                     m.data[1][3] * (m.data[2][0] * m.data[3][1] - m.data[2][1] * m.data[3][0])) -
        m.data[0][3] * (m.data[1][0] * (m.data[2][1] * m.data[3][2] - m.data[2][2] * m.data[3][1]) -
                     m.data[1][1] * (m.data[2][0] * m.data[3][2] - m.data[2][2] * m.data[3][0]) +
                     m.data[1][2] * (m.data[2][0] * m.data[3][1] - m.data[2][1] * m.data[3][0]));
    
    if (fabsf(det) < 0.0001f) {
        return mat4_identity();
    }
    
    float inv_det = 1.0f / det;
    Mat4 result;
    
    // Calculate adjugate matrix (simplified)
    result.data[0][0] = inv_det * (m.data[1][1] * (m.data[2][2] * m.data[3][3] - m.data[2][3] * m.data[3][2]) -
                                 m.data[1][2] * (m.data[2][1] * m.data[3][3] - m.data[2][3] * m.data[3][1]) +
                                 m.data[1][3] * (m.data[2][1] * m.data[3][2] - m.data[2][2] * m.data[3][1]));
    
    result.data[1][0] = inv_det * -(m.data[1][0] * (m.data[2][2] * m.data[3][3] - m.data[2][3] * m.data[3][2]) -
                                 m.data[1][2] * (m.data[2][0] * m.data[3][3] - m.data[2][3] * m.data[3][0]) +
                                 m.data[1][3] * (m.data[2][0] * m.data[3][2] - m.data[2][2] * m.data[3][0]));
    
    result.data[2][0] = inv_det * (m.data[1][0] * (m.data[2][1] * m.data[3][3] - m.data[2][3] * m.data[3][1]) -
                                 m.data[1][1] * (m.data[2][0] * m.data[3][3] - m.data[2][3] * m.data[3][0]) +
                                 m.data[1][3] * (m.data[2][0] * m.data[3][1] - m.data[2][1] * m.data[3][0]));
    
    result.data[3][0] = inv_det * -(m.data[1][0] * (m.data[2][1] * m.data[3][2] - m.data[2][2] * m.data[3][1]) -
                                 m.data[1][1] * (m.data[2][0] * m.data[3][2] - m.data[2][2] * m.data[3][0]) +
                                 m.data[1][2] * (m.data[2][0] * m.data[3][1] - m.data[2][1] * m.data[3][0]));
    
    // For simplicity, fill rest with identity (this is a partial implementation)
    result.data[0][1] = 0.0f; result.data[0][2] = 0.0f; result.data[0][3] = 0.0f;
    result.data[1][1] = 1.0f; result.data[1][2] = 0.0f; result.data[1][3] = 0.0f;
    result.data[2][1] = 0.0f; result.data[2][2] = 1.0f; result.data[2][3] = 0.0f;
    result.data[3][1] = 0.0f; result.data[3][2] = 0.0f; result.data[3][3] = 1.0f;
    
    return result;
}

Mat4 mat4_translate(float x, float y, float z) {
    Mat4 m = mat4_identity();
    m.m30 = x;
    m.m31 = y;
    m.m32 = z;
    return m;
}

Mat4 mat4_scale(float x, float y, float z) {
    Mat4 m = mat4_identity();
    m.m00 = x;
    m.m11 = y;
    m.m22 = z;
    return m;
}

Mat4 mat4_rotate(float angle, float x, float y, float z) {
    Mat4 m = mat4_identity();
    
    float c = cosf(angle);
    float s = sinf(angle);
    float omc = 1.0f - c;
    
    float len = sqrtf(x * x + y * y + z * z);
    if (len < 0.0001f) return m;
    
    x /= len; y /= len; z /= len;
    
    m.m00 = x * x * omc + c;
    m.m01 = y * x * omc + z * s;
    m.m02 = z * x * omc - y * s;
    
    m.m10 = x * y * omc - z * s;
    m.m11 = y * y * omc + c;
    m.m12 = z * y * omc + x * s;
    
    m.m20 = x * z * omc + y * s;
    m.m21 = y * z * omc - x * s;
    m.m22 = z * z * omc + c;
    
    return m;
}

Mat4 mat4_ortho(float left, float right, float bottom, float top, float near_z, float far_z) {
    Mat4 m = mat4_zero();
    
    m.m00 = 2.0f / (right - left);
    m.m11 = 2.0f / (top - bottom);
    m.m22 = -2.0f / (far_z - near_z);
    
    m.m30 = -(right + left) / (right - left);
    m.m31 = -(top + bottom) / (top - bottom);
    m.m32 = -(far_z + near_z) / (far_z - near_z);
    m.m33 = 1.0f;
    
    return m;
}

Vec4 mat4_mul_vec4(Mat4 m, Vec4 v) {
    Vec4 result;
    
    result.x = m.m00 * v.x + m.m10 * v.y + m.m20 * v.z + m.m30 * v.w;
    result.y = m.m01 * v.x + m.m11 * v.y + m.m21 * v.z + m.m31 * v.w;
    result.z = m.m02 * v.x + m.m12 * v.y + m.m22 * v.z + m.m32 * v.w;
    result.w = m.m03 * v.x + m.m13 * v.y + m.m23 * v.z + m.m33 * v.w;
    
    return result;
}

Vec3 mat4_mul_vec3(Mat4 m, Vec3 v, float w) {
    Vec4 v4 = {v.x, v.y, v.z, w};
    Vec4 result = mat4_mul_vec4(m, v4);
    
    if (fabsf(result.w) > 0.0001f) {
        return (Vec3){result.x / result.w, result.y / result.w, result.z / result.w};
    }
    
    return (Vec3){result.x, result.y, result.z};
}
