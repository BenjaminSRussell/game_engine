// camera.c - Stub implementation
#include "rendering/camera.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>

void camera_init(Camera* camera, Vec3 position, Vec3 target, Vec3 up) {
    if (!camera) return;
    (void)position; (void)target; (void)up;
    LOG_INFO("Initializing camera (stub)");
}

void camera_set_position(Camera* camera, Vec3 position) {
    (void)camera; (void)position;
    // Stub
}

Vec3 camera_get_forward(const Camera* camera) {
    (void)camera;
    return (Vec3){0.0f, 0.0f, -1.0f}; // Stub
}

Mat4 camera_get_view_matrix(const Camera* camera) {
    (void)camera;
    // Return identity matrix as stub
    Mat4 m = {0};
    m.m00 = m.m11 = m.m22 = m.m33 = 1.0f;
    return m;
}

Mat4 camera_get_projection_matrix(const Camera* camera) {
    (void)camera;
    // Return identity matrix as stub
    Mat4 m = {0};
    m.m00 = m.m11 = m.m22 = m.m33 = 1.0f;
    return m;
}
