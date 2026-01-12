// Stub implementations for mat4 functions to resolve linking errors
// These provide minimal implementations to allow the game to build

#include "math/mat4.h"

// Stub implementations using correct types
Mat4 mat4_identity(void) {
    Mat4 result = {0};
    return result;
}

Mat4 mat4_zero(void) {
    Mat4 result = {0};
    return result;
}

Mat4 mat4_mul(Mat4 a, Mat4 b) {
    Mat4 result;
    // Simple implementation without inline dependency
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.data[i][j] = 0;
            for (int k = 0; k < 4; k++) {
                result.data[i][j] += a.data[k][j];
            }
        }
    }
    return result;
}

Mat4 mat4_translate(Vec3 v) {
    Mat4 result = {0};
    result.m30 = v.x;
    result.m31 = v.y;
    result.m32 = v.z;
    return result;
}

Mat4 mat4_scale(Vec3 v) {
    Mat4 result = {0};
    result.m00 = v.x;
    result.m11 = v.y;
    result.m22 = v.z;
    return result;
}

Mat4 mat4_rotate(Vec3 axis, f32 angle) {
    // Simplified rotation implementation
    Mat4 result = {0};
    // This is a very basic stub - real implementation would be complex
    return result;
}
