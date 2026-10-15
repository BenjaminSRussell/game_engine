// Perspective camera: yaw/pitch basis, view, projection, and shake.
// Yaw 0 looks along +X and yaw -90 looks along -Z. Pitch is degrees, clamped
// so the basis stays orthonormal. Math comes from math_all.h.

#include "math/math_all.h"
#include "rendering/camera.h"

#include <math.h>
#include <string.h>

static void camera_update_basis(Camera *camera) {
  f32 yaw_rad;
  f32 pitch_rad;
  Vec3 world_up;
  Vec3 right;

  camera->pitch = clamp(camera->pitch, -89.0f, 89.0f);
  yaw_rad = camera->yaw * DEG_TO_RAD;
  pitch_rad = camera->pitch * DEG_TO_RAD;

  camera->front = vec3(cosf(yaw_rad) * cosf(pitch_rad), sinf(pitch_rad),
                       sinf(yaw_rad) * cosf(pitch_rad));
  camera->front = vec3_normalize(camera->front);

  world_up = vec3(0.0f, 1.0f, 0.0f);
  right = vec3_cross(camera->front, world_up);
  if (vec3_length_sq(right) < 1.0e-8f) {
    world_up = vec3(0.0f, 0.0f, 1.0f);
    right = vec3_cross(camera->front, world_up);
  }
  camera->right = vec3_normalize(right);
  camera->up = vec3_normalize(vec3_cross(camera->right, camera->front));
}

void camera_init(Camera *camera, Vec3 position, f32 yaw, f32 pitch) {
  if (!camera) {
    return;
  }

  memset(camera, 0, sizeof(*camera));
  camera->position = position;
  camera->yaw = yaw;
  camera->pitch = pitch;
  camera->fov = 70.0f;
  camera->near_plane = 0.1f;
  camera->far_plane = 1000.0f;
  camera->shake_offset = vec3(0.0f, 0.0f, 0.0f);
  camera_update_basis(camera);
}

void camera_set_position(Camera *camera, Vec3 position) {
  if (!camera) {
    return;
  }
  camera->position = position;
}

Vec3 camera_get_forward(Camera *camera) {
  if (!camera) {
    return vec3(0.0f, 0.0f, -1.0f);
  }
  return camera->front;
}

Mat4 camera_get_view_matrix(const Camera *camera) {
  Vec3 eye;
  Vec3 target;

  if (!camera || vec3_length_sq(camera->front) < 1.0e-8f ||
      vec3_length_sq(camera->up) < 1.0e-8f) {
    return mat4_identity();
  }

  eye = vec3_add(camera->position, camera->shake_offset);
  target = vec3_add(eye, camera->front);
  return mat4_look_at(eye, target, camera->up);
}

Mat4 camera_get_projection_matrix(const Camera *camera, f32 aspect) {
  if (!camera || !(aspect > 0.0f) || !(camera->fov > 0.0f) ||
      !(camera->near_plane > 0.0f) ||
      !(camera->far_plane > camera->near_plane)) {
    return mat4_identity();
  }
  return mat4_perspective(camera->fov * DEG_TO_RAD, aspect, camera->near_plane,
                          camera->far_plane);
}

void camera_add_shake(Camera *camera, f32 intensity, f32 duration) {
  if (!camera || !(intensity > 0.0f) || !(duration > 0.0f)) {
    return;
  }
  if (intensity > camera->shake_intensity) {
    camera->shake_intensity = intensity;
    camera->shake_duration = duration;
    camera->shake_time = 0.0f;
  }
}

void camera_update_shake(Camera *camera, f32 delta_time) {
  f32 progress;
  f32 current;
  f32 random_x;
  f32 random_y;
  f32 random_z;

  if (!camera || delta_time < 0.0f) {
    return;
  }
  if (!(camera->shake_duration > 0.0f)) {
    camera->shake_offset = vec3(0.0f, 0.0f, 0.0f);
    return;
  }

  camera->shake_time += delta_time;
  if (camera->shake_time >= camera->shake_duration) {
    camera->shake_duration = 0.0f;
    camera->shake_intensity = 0.0f;
    camera->shake_time = 0.0f;
    camera->shake_offset = vec3(0.0f, 0.0f, 0.0f);
    return;
  }

  progress = camera->shake_time / camera->shake_duration;
  current = camera->shake_intensity * (1.0f - progress);
  random_x =
      (sinf(camera->shake_time * 15.0f) + sinf(camera->shake_time * 23.7f)) *
      0.5f;
  random_y =
      (cosf(camera->shake_time * 17.3f) + cosf(camera->shake_time * 29.1f)) *
      0.5f;
  random_z =
      (sinf(camera->shake_time * 19.7f) + cosf(camera->shake_time * 31.3f)) *
      0.5f;
  camera->shake_offset =
      vec3(random_x * current, random_y * current, random_z * current * 0.5f);
}
