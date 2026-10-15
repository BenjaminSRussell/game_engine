// camera.c - Camera state. camera_init and camera_get_forward are implemented.
// camera_set_position, camera_get_view_matrix and camera_get_projection_matrix
// are still stubs (see #188).
#include "rendering/camera.h"
#include <math.h>
#include <stdlib.h>

// Derives front, right and up from yaw and pitch (degrees). Yaw 0 looks along
// +X and yaw -90 looks along -Z, the same convention as src/core/services/camera.c.
// Right depends only on yaw, so the basis stays orthonormal at pitch +-90.
static void camera_update_basis(Camera* camera) {
    f32 yaw_rad = camera->yaw * DEG_TO_RAD;
    f32 pitch_rad = camera->pitch * DEG_TO_RAD;
    f32 cy = cosf(yaw_rad);
    f32 sy = sinf(yaw_rad);
    f32 cp = cosf(pitch_rad);
    f32 sp = sinf(pitch_rad);

    camera->front = vec3(cy * cp, sp, sy * cp);
    camera->right = vec3(-sy, 0.0f, cy);
    camera->up = vec3(-cy * sp, cp, -sy * sp);
}

void camera_init(Camera* camera, Vec3 position, f32 yaw, f32 pitch) {
    if (!camera) return;
    camera->position = position;
    camera->yaw = yaw;
    camera->pitch = pitch;
    camera->fov = 70.0f;
    camera->near_plane = 0.1f;
    camera->far_plane = 1000.0f;
    camera->shake_duration = 0.0f;
    camera->shake_intensity = 0.0f;
    camera->shake_time = 0.0f;
    camera->shake_offset = vec3(0.0f, 0.0f, 0.0f);
    camera_update_basis(camera);
}

void camera_set_position(Camera* camera, Vec3 position) {
    (void)camera; (void)position;
    // Stub
}

Vec3 camera_get_forward(const Camera* camera) {
    if (!camera) return vec3(0.0f, 0.0f, -1.0f);
    return camera->front;
}

Mat4 camera_get_view_matrix(const Camera* camera) {
    (void)camera;
    // Return identity matrix as stub
    Mat4 m = {0};
    m.m00 = m.m11 = m.m22 = m.m33 = 1.0f;
    return m;
}

Mat4 camera_get_projection_matrix(const Camera* camera, f32 aspect) {
    (void)camera; (void)aspect;
    // Return identity matrix as stub
    Mat4 m = {0};
    m.m00 = m.m11 = m.m22 = m.m33 = 1.0f;
    return m;
}
