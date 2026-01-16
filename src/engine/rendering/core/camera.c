// camera.c - Stub implementation matching include/rendering/camera.h
#include "rendering/camera.h"
#include "engine/include/core/logger.h"
#include <math.h>
#include <stdlib.h>

void camera_init(Camera *camera, Vec3 position, f32 yaw, f32 pitch) {
  if (!camera)
    return;
  camera->position = position;
  camera->yaw = yaw;
  camera->pitch = pitch;
  camera->front = (Vec3){0.0f, 0.0f, -1.0f};
  camera->up = (Vec3){0.0f, 1.0f, 0.0f};
  camera->right = (Vec3){1.0f, 0.0f, 0.0f};
  camera->fov = 45.0f;
  camera->near_plane = 0.1f;
  camera->far_plane = 100.0f;

  // Initialize shake
  camera->shake_duration = 0.0f;
  camera->shake_intensity = 0.0f;
  camera->shake_time = 0.0f;
  camera->shake_offset = (Vec3){0};

  LOG_INFO("Initializing camera");
}

void camera_set_position(Camera *camera, Vec3 position) {
  if (camera) {
    camera->position = position;
  }
}

Vec3 camera_get_forward(Camera *camera) {
  if (!camera)
    return (Vec3){0, 0, -1};
  // In a real implementation we would calculate this from yaw/pitch
  return camera->front;
}

Mat4 camera_get_view_matrix(const Camera *camera) {
  (void)camera;
  // Return identity matrix as stub
  // In real impl: return mat4_look_at(camera->position, camera->position +
  // camera->front, camera->up);
  return mat4_identity();
}

Mat4 camera_get_projection_matrix(const Camera *camera, f32 aspect) {
  (void)camera;
  (void)aspect;
  // Return identity matrix as stub
  // In real impl: return mat4_perspective(camera->fov, aspect, camera->near,
  // camera->far);
  return mat4_identity();
}

void camera_add_shake(Camera *camera, f32 intensity, f32 duration) {
  if (camera) {
    camera->shake_intensity = intensity;
    camera->shake_duration = duration;
    camera->shake_time = duration;
  }
}

void camera_update_shake(Camera *camera, f32 delta_time) {
  if (camera && camera->shake_time > 0) {
    camera->shake_time -= delta_time;
    if (camera->shake_time < 0)
      camera->shake_time = 0;
    // Logic would update shake_offset here
  }
}
