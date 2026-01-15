#include <math/matrix.h>
#include <math/mat4.h>
#include <math/vec3.h>
#include <math.h>

/* ===== MATRIX DECOMPOSITION ===== */

Vec3 matrix_get_translation(Mat4 *m) {
    return (Vec3){m->data[3][0], m->data[3][1], m->data[3][2]};
}

Vec3 matrix_get_scale(Mat4 *m) {
    Vec3 scale;
    scale.x = sqrtf(m->data[0][0] * m->data[0][0] + m->data[0][1] * m->data[0][1] + m->data[0][2] * m->data[0][2]);
    scale.y = sqrtf(m->data[1][0] * m->data[1][0] + m->data[1][1] * m->data[1][1] + m->data[1][2] * m->data[1][2]);
    scale.z = sqrtf(m->data[2][0] * m->data[2][0] + m->data[2][1] * m->data[2][1] + m->data[2][2] * m->data[2][2]);
    return scale;
}

Vec3 matrix_get_rotation(Mat4 *m) {
    Vec3 rotation = {0};
    
    // Extract euler angles from rotation matrix
    // Assuming matrix contains only rotation and translation
    float sy = sqrtf(m->data[0][0] * m->data[0][0] + m->data[1][0] * m->data[1][0]);
    
    if (sy < EPSILON) {
        rotation.x = atan2f(-m->data[1][2], m->data[1][1]);
        rotation.y = atan2f(-m->data[2][0], m->data[0][0]);
        rotation.z = 0.0f;
    } else {
        rotation.x = atan2f(m->data[2][1], m->data[2][2]);
        rotation.y = atan2f(-m->data[2][0], sy);
        rotation.z = atan2f(m->data[1][0], m->data[0][0]);
    }
    
    return rotation;
}

/* ===== COMMON TRANSFORMATIONS ===== */

Mat4 matrix_translate(Vec3 offset) {
    Mat4 result = mat4_identity();
    result.data[3][0] = offset.x;
    result.data[3][1] = offset.y;
    result.data[3][2] = offset.z;
    return result;
}

Mat4 matrix_rotate_euler(Vec3 euler) {
    Mat4 rx = matrix_rotate_x(euler.x);
    Mat4 ry = matrix_rotate_y(euler.y);
    Mat4 rz = matrix_rotate_z(euler.z);
    
    // Combined rotation: Z * Y * X (common convention)
    Mat4 temp = mat4_mul(rz, ry);
    return mat4_mul(temp, rx);
}

Mat4 matrix_rotate_x(f32 angle) {
    Mat4 result = mat4_identity();
    f32 cos_a = cosf(angle);
    f32 sin_a = sinf(angle);
    
    result.data[1][1] = cos_a;
    result.data[1][2] = -sin_a;
    result.data[2][1] = sin_a;
    result.data[2][2] = cos_a;
    
    return result;
}

Mat4 matrix_rotate_y(f32 angle) {
    Mat4 result = mat4_identity();
    f32 cos_a = cosf(angle);
    f32 sin_a = sinf(angle);
    
    result.data[0][0] = cos_a;
    result.data[0][2] = sin_a;
    result.data[2][0] = -sin_a;
    result.data[2][2] = cos_a;
    
    return result;
}

Mat4 matrix_rotate_z(f32 angle) {
    Mat4 result = mat4_identity();
    f32 cos_a = cosf(angle);
    f32 sin_a = sinf(angle);
    
    result.data[0][0] = cos_a;
    result.data[0][1] = -sin_a;
    result.data[1][0] = sin_a;
    result.data[1][1] = cos_a;
    
    return result;
}

Mat4 matrix_scale(Vec3 scale) {
    Mat4 result = mat4_identity();
    result.data[0][0] = scale.x;
    result.data[1][1] = scale.y;
    result.data[2][2] = scale.z;
    return result;
}

Mat4 matrix_perspective(f32 fov, f32 aspect, f32 near, f32 far) {
    return mat4_perspective(fov, aspect, near, far);
}

Mat4 matrix_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    return mat4_ortho(left, right, bottom, top, near, far);
}

Mat4 matrix_look_at(Vec3 eye, Vec3 target, Vec3 up) {
    return mat4_look_at(eye, target, up);
}
