#include <math/mat4.h>
#include <math.h>
#include <math/vec3.h>

// Implementation of missing matrix functions for camera

// Implementation of missing matrix functions for camera

Mat4 mat4_look_at(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 f = vec3_normalize(vec3_sub(target, eye));
    Vec3 r = vec3_normalize(vec3_cross(f, up));
    Vec3 u = vec3_cross(r, f);

    Mat4 result = mat4_identity();
    result.data[0][0] = r.x;
    result.data[1][0] = r.y;
    result.data[2][0] = r.z;
    result.data[3][0] = -vec3_dot(r, eye);

    result.data[0][1] = u.x;
    result.data[1][1] = u.y;
    result.data[2][1] = u.z;
    result.data[3][1] = -vec3_dot(u, eye);

    result.data[0][2] = -f.x;
    result.data[1][2] = -f.y;
    result.data[2][2] = -f.z;
    result.data[3][2] = vec3_dot(f, eye);

    return result;
}

Mat4 mat4_perspective(f32 fov_y, f32 aspect, f32 near_z, f32 far_z) {
    f32 tan_half_fov = tanf(fov_y / 2.0f);

    Mat4 result = mat4_zero();
    result.data[0][0] = 1.0f / (aspect * tan_half_fov);
    result.data[1][1] = 1.0f / tan_half_fov;
    result.data[2][2] = -(far_z + near_z) / (far_z - near_z);
    result.data[2][3] = -1.0f;
    result.data[3][2] = -(2.0f * far_z * near_z) / (far_z - near_z);

    return result;
}
