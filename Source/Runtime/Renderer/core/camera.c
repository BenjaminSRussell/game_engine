// camera.c - Stub implementation
#include "rendering/camera.h"
#include "core/logger/unified_logger.h"
#include <stdlib.h>

void camera_init(Camera *camera, Vec3 position, f32 yaw, f32 pitch) {
  if (!camera)
    return;
  camera->position = position;
  camera->yaw = yaw;
  camera->pitch = pitch;
  // Update vectors based on yaw/pitch
  // Stub: just set defaults
  camera->front = (Vec3){0.0f, 0.0f, -1.0f};
  camera->up = (Vec3){0.0f, 1.0f, 0.0f};
  camera->right = (Vec3){1.0f, 0.0f, 0.0f};

  LOG_INFO("Initializing camera");
}

void camera_set_position(Camera *camera, Vec3 position) {
  if (!camera)
    return;
  camera->position = position;
}

Vec3 camera_get_forward(const Camera *camera) {
  if (!camera)
    return (Vec3){0};
  return camera->front;
}

Mat4 camera_get_view_matrix(const Camera *camera) {
  if (!camera) {
    Mat4 m = {0};
    m.m00 = m.m11 = m.m22 = m.m33 = 1.0f;
    return m;
  }
  // Stub: return identity
  Mat4 m = {0};
  m.m00 = m.m11 = m.m22 = m.m33 = 1.0f;
  return m;
}

Mat4 camera_get_projection_matrix(const Camera *camera, f32 aspect) {
  (void)aspect;
  if (!camera) {
    Mat4 m = {0};
    m.m00 = m.m11 = m.m22 = m.m33 = 1.0f;
    return m;
  }
  // Stub: return identity
  Mat4 m = {0};
  m.m00 = m.m11 = m.m22 = m.m33 = 1.0f;
  return m;
}

void camera_add_shake(Camera *camera, f32 intensity, f32 duration) {
  if (!camera)
    return;
  camera->shake_intensity = intensity;
  camera->shake_duration = duration;
}

void camera_update_shake(Camera *camera, f32 delta_time) {
  (void)camera;
  (void)delta_time;
}
