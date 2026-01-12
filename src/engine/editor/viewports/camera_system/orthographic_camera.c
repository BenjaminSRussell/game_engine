/**
 * @file orthographic_camera.c
 * @brief Orthographic camera system implementation
 */

#include "orthographic_camera.h"
#include "core/logger.h"
#include "math/mat4.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Internal helper functions
static void
orthographic_camera_update_projection_matrix(OrthographicCamera *camera);
static void orthographic_camera_update_view_matrix(OrthographicCamera *camera);
static void
orthographic_camera_update_view_projection_matrix(OrthographicCamera *camera);

// Core camera functions
int orthographic_camera_create(OrthographicCamera *camera, float left,
                               float right, float bottom, float top,
                               float near_plane, float far_plane) {
  if (!camera)
    return -1;

  // Initialize camera with zero values
  memset(camera, 0, sizeof(OrthographicCamera));

  // Set projection parameters
  camera->left = left;
  camera->right = right;
  camera->bottom = bottom;
  camera->top = top;
  camera->near_plane = near_plane;
  camera->far_plane = far_plane;

  // Set default camera position and orientation
  camera->position = (Vec3){0.0f, 0.0f, 0.0f};
  camera->target = (Vec3){0.0f, 0.0f, -1.0f};
  camera->up = (Vec3){0.0f, 1.0f, 0.0f};

  // Set default viewport
  camera->viewport_origin = (Vec2){0.0f, 0.0f};
  camera->viewport_size = (Vec2){800.0f, 600.0f};

  // Set default zoom and pan
  camera->zoom_level = 1.0f;
  camera->pan_offset = (Vec2){0.0f, 0.0f};

  // Set default configuration
  camera->auto_resize = true;
  camera->maintain_aspect_ratio = true;
  camera->aspect_ratio = (float)800.0f / 600.0f;

  // Initialize matrices to identity
  camera->view_matrix = mat4_identity();
  camera->projection_matrix = mat4_identity();
  camera->view_projection_matrix = mat4_identity();

  // Update all matrices
  orthographic_camera_update_matrices(camera);

  camera->last_update_time = (float)clock() / CLOCKS_PER_SEC;
  LOG_INFO("Orthographic camera created: L=%.1f, R=%.1f, B=%.1f, T=%.1f, "
           "N=%.1f, F=%.1f",
           left, right, bottom, top, near_plane, far_plane);
  return 0;
}

int orthographic_camera_destroy(OrthographicCamera *camera) {
  if (!camera)
    return -1;

  memset(camera, 0, sizeof(OrthographicCamera));
  LOG_INFO("Orthographic camera destroyed");
  return 0;
}

// View and projection management
int orthographic_camera_set_projection(OrthographicCamera *camera, float left,
                                       float right, float bottom, float top,
                                       float near_plane, float far_plane) {
  if (!camera)
    return -1;

  camera->left = left;
  camera->right = right;
  camera->bottom = bottom;
  camera->top = top;
  camera->near_plane = near_plane;
  camera->far_plane = far_plane;

  orthographic_camera_update_projection_matrix(camera);
  orthographic_camera_update_view_projection_matrix(camera);

  LOG_DEBUG("Orthographic projection updated: L=%.1f, R=%.1f, B=%.1f, T=%.1f, "
            "N=%.1f, F=%.1f",
            left, right, bottom, top, near_plane, far_plane);
  return 0;
}

int orthographic_camera_set_view(OrthographicCamera *camera, Vec3 position,
                                 Vec3 target, Vec3 up) {
  if (!camera)
    return -1;

  camera->position = position;
  camera->target = target;
  camera->up = up;

  orthographic_camera_update_view_matrix(camera);
  orthographic_camera_update_view_projection_matrix(camera);

  LOG_DEBUG("Orthographic view updated: pos=(%.1f,%.1f,%.1f), "
            "target=(%.1f,%.1f,%.1f)",
            position.x, position.y, position.z, target.x, target.y, target.z);
  return 0;
}

int orthographic_camera_set_viewport(OrthographicCamera *camera, Vec2 origin,
                                     Vec2 size) {
  if (!camera)
    return -1;

  camera->viewport_origin = origin;
  camera->viewport_size = size;

  if (camera->auto_resize) {
    // Adjust projection to match viewport aspect ratio
    float viewport_aspect = size.x / size.y;
    if (camera->maintain_aspect_ratio) {
      if (viewport_aspect > camera->aspect_ratio) {
        // Wider than expected, adjust height
        float new_height = size.x / camera->aspect_ratio;
        camera->viewport_size.y = new_height;
      } else {
        // Taller than expected, adjust width
        float new_width = size.y * camera->aspect_ratio;
        camera->viewport_size.x = new_width;
      }
    }
  }

  orthographic_camera_update_projection_matrix(camera);
  orthographic_camera_update_view_projection_matrix(camera);

  LOG_DEBUG(
      "Orthographic viewport updated: origin=(%.1f,%.1f), size=(%.1f,%.1f)",
      origin.x, origin.y, size.x, size.y);
  return 0;
}

// Zoom and pan controls
int orthographic_camera_set_zoom(OrthographicCamera *camera, float zoom_level) {
  if (!camera)
    return -1;

  camera->zoom_level = fmaxf(0.1f, zoom_level);
  orthographic_camera_update_projection_matrix(camera);
  orthographic_camera_update_view_projection_matrix(camera);

  LOG_DEBUG("Orthographic zoom set to: %.2fx", camera->zoom_level);
  return 0;
}

int orthographic_camera_zoom_in(OrthographicCamera *camera, float zoom_factor) {
  if (!camera)
    return -1;

  float new_zoom = camera->zoom_level * zoom_factor;
  return orthographic_camera_set_zoom(camera, new_zoom);
}

int orthographic_camera_zoom_out(OrthographicCamera *camera,
                                 float zoom_factor) {
  if (!camera)
    return -1;

  float new_zoom = camera->zoom_level / zoom_factor;
  return orthographic_camera_set_zoom(camera, new_zoom);
}

int orthographic_camera_pan(OrthographicCamera *camera, Vec2 delta) {
  if (!camera)
    return -1;

  camera->pan_offset = vec2_add(camera->pan_offset, delta);
  orthographic_camera_update_view_matrix(camera);
  orthographic_camera_update_view_projection_matrix(camera);

  LOG_DEBUG("Orthographic panned by: (%.1f, %.1f)", delta.x, delta.y);
  return 0;
}

int orthographic_camera_center(OrthographicCamera *camera) {
  if (!camera)
    return -1;

  // Center the viewport
  Vec2 center = {camera->viewport_origin.x + camera->viewport_size.x * 0.5f,
                 camera->viewport_origin.y + camera->viewport_size.y * 0.5f};

  Vec2 delta = vec2_sub(center, camera->viewport_origin);

  return orthographic_camera_pan(camera, delta);
}

// Matrix computation
int orthographic_camera_update_matrices(OrthographicCamera *camera) {
  if (!camera)
    return -1;

  orthographic_camera_update_view_matrix(camera);
  orthographic_camera_update_projection_matrix(camera);
  orthographic_camera_update_view_projection_matrix(camera);

  camera->frame_count++;
  camera->last_update_time = (float)clock() / CLOCKS_PER_SEC;
  return 0;
}

int orthographic_camera_get_view_matrix(const OrthographicCamera *camera,
                                        Mat4 *out_matrix) {
  if (!camera || !out_matrix)
    return -1;
  *out_matrix = camera->view_matrix;
  return 0;
}

int orthographic_camera_get_projection_matrix(const OrthographicCamera *camera,
                                              Mat4 *out_matrix) {
  if (!camera || !out_matrix)
    return -1;
  *out_matrix = camera->projection_matrix;
  return 0;
}

int orthographic_camera_get_view_projection_matrix(
    const OrthographicCamera *camera, Mat4 *out_matrix) {
  if (!camera || !out_matrix)
    return -1;
  *out_matrix = camera->view_projection_matrix;
  return 0;
}

// Viewport and screen conversion
int orthographic_camera_world_to_screen(const OrthographicCamera *camera,
                                        Vec3 world_pos, Vec2 *out_screen_pos) {
  if (!camera || !out_screen_pos)
    return -1;

  // Transform world position to clip space
  Mat4 view_proj;
  orthographic_camera_get_view_projection_matrix(camera, &view_proj);

  // Apply view-projection transformation
  Vec4 clip_pos = mat4_mul_vec4(
      view_proj, (Vec4){world_pos.x, world_pos.y, world_pos.z, 1.0f});

  // Perspective divide
  if (clip_pos.w > 0.001f) {
    clip_pos = clip_pos / clip_pos.w;
  }

  // Apply viewport transformation
  *out_screen_pos =
      vec2_add(vec2_scale_componentwise(screen_pos_ndc, camera->viewport_size),
               camera->viewport_origin);

  return 0;
}

int orthographic_camera_screen_to_world(const OrthographicCamera *camera,
                                        Vec2 screen_pos, Vec3 *out_world_pos) {
  if (!camera || !out_world_pos)
    return -1;

  // Convert screen coordinates to NDC
  Vec2 ndc = {
      (screen_pos.x - camera->viewport_origin.x) / camera->viewport_size.x,
      (screen_pos.y - camera->viewport_origin.y) / camera->viewport_size.y};

  // Convert NDC to clip space
  Vec4 clip_pos = {(ndc.x * 2.0f - 1.0f), (ndc.y * 2.0f - 1.0f), 0.0f, 1.0f};

  // Inverse view-projection transformation
  Mat4 inv_view_proj = mat4_inverse(camera->view_projection_matrix);

  // Transform to world space
  Vec4 world_pos4 = mat4_mul_vec4(inv_view_proj, clip_pos);

  // Perspective divide
  if (world_pos4.w > 0.001f) {
    float inv_w = 1.0f / world_pos4.w;
    world_pos4.x *= inv_w;
    world_pos4.y *= inv_w;
    world_pos4.z *= inv_w;
  }

  *out_world_pos = (Vec3){world_pos4.x, world_pos4.y, world_pos4.z};
  return 0;
}

int orthographic_camera_ray_from_screen(const OrthographicCamera *camera,
                                        Vec2 screen_pos, Vec3 *out_origin,
                                        Vec3 *out_direction) {
  if (!camera || !out_origin || !out_direction)
    return -1;

  // Convert screen position to world space
  Vec3 world_pos;
  if (orthographic_camera_screen_to_world(camera, screen_pos, &world_pos) !=
      0) {
    return -1;
  }

  *out_origin = world_pos;

  // For orthographic camera, ray direction is always forward (0, 0, -1)
  *out_direction = (Vec3){0.0f, 0.0f, -1.0f};

  return 0;
}

// Utility functions
int orthographic_camera_get_bounds(const OrthographicCamera *camera,
                                   Vec2 *out_min, Vec2 *out_max) {
  if (!camera || !out_min || !out_max)
    return -1;

  *out_min = (Vec2){camera->left, camera->bottom};
  *out_max = (Vec2){camera->right, camera->top};
  return 0;
}

int orthographic_camera_get_center(const OrthographicCamera *camera,
                                   Vec2 *out_center) {
  if (!camera || !out_center)
    return -1;

  *out_center = (Vec2){(camera->left + camera->right) * 0.5f,
                       (camera->bottom + camera->top) * 0.5f};
  return 0;
}

float orthographic_camera_get_width(const OrthographicCamera *camera) {
  return camera ? camera->right - camera->left : 0.0f;
}

float orthographic_camera_get_height(const OrthographicCamera *camera) {
  return camera ? camera->top - camera->bottom : 0.0f;
}

float orthographic_get_aspect_ratio(const OrthographicCamera *camera) {
  return camera ? camera->aspect_ratio : 1.0f;
}

// Configuration functions
int orthographic_camera_set_auto_resize(OrthographicCamera *camera,
                                        bool enabled) {
  if (!camera)
    return -1;

  camera->auto_resize = enabled;
  LOG_INFO("Orthographic auto-resize %s", enabled ? "enabled" : "disabled");
  return 0;
}

int orthographic_camera_set_maintain_aspect_ratio(OrthographicCamera *camera,
                                                  bool enabled,
                                                  float aspect_ratio) {
  if (!camera)
    return -1;

  camera->maintain_aspect_ratio = enabled;
  camera->aspect_ratio = aspect_ratio;

  if (camera->auto_resize) {
    orthographic_camera_set_viewport(camera, camera->viewport_origin,
                                     camera->viewport_size);
  }

  LOG_INFO("Orthographic aspect ratio maintenance %s, ratio: %.2f",
           enabled ? "enabled" : "disabled", aspect_ratio);
  return 0;
}

int orthographic_camera_set_background_color(OrthographicCamera *camera,
                                             Vec4 color) {
  if (!camera)
    return -1;

  // This would set the background color for the camera's view
  // Implementation would depend on the rendering backend
  LOG_INFO("Orthographic background color set to RGBA(%.1f, %.1f, %.1f, %.1f)",
           color.x, color.y, color.z, color.w);
  return 0;
}

// Animation and interpolation
int orthographic_camera_animate_to(OrthographicCamera *camera,
                                   Vec3 target_position, Vec3 target_target,
                                   float duration) {
  if (!camera)
    return -1;

  // Simple linear interpolation
  float start_time = (float)clock() / CLOCKS_PER_SEC;
  float current_time = start_time;
  float end_time = start_time + duration;

  while (current_time < end_time) {
    float t = (current_time - start_time) / duration;

    // Interpolate position
    Vec3 current_pos = {
        camera->position.x + (target_position.x - camera->position.x) * t,
        camera->position.y + (target_position.y - camera->position.y) * t,
        camera->position.z + (target_position.z - camera->position.z) * t};

    // Interpolate target
    Vec3 current_target = {
        camera->target.x + (target_target.x - camera->target.x) * t,
        camera->target.y + (target_target.y - camera->target.y) * t,
        camera->target.z + (target_target.z - camera->target.z) * t};

    orthographic_camera_set_view(camera, current_pos, current_target,
                                 camera->up);

    current_time += 0.016f; // ~60 FPS update rate
  }

  LOG_INFO(
      "Orthographic camera animation completed to position (%.2f, %.2f, %.2f)",
      target_position.x, target_position.y, target_position.z);
  return 0;
}

int orthographic_camera_animate_zoom(OrthographicCamera *camera,
                                     float target_zoom, float duration) {
  if (!camera)
    return -1;

  float start_time = (float)clock() / CLOCKS_PER_SEC;
  float current_time = start_time;
  float end_time = start_time + duration;
  float start_zoom = camera->zoom_level;

  while (current_time < end_time) {
    float t = (current_time - start_time) / duration;
    float current_zoom = start_zoom + (target_zoom - start_zoom) * t;

    orthographic_camera_set_zoom(camera, current_zoom);
    current_time += 0.016f; // ~60 FPS update rate
  }

  LOG_INFO("Orthographic zoom animation completed to %.2fx", target_zoom);
  return 0;
}

int orthographic_camera_animate_pan(OrthographicCamera *camera, Vec2 target_pan,
                                    float duration) {
  if (!camera)
    return -1;

  float start_time = (float)clock() / CLOCKS_PER_SEC;
  float current_time = start_time;
  float end_time = start_time + duration;
  Vec2 start_pan = camera->pan_offset;

  while (current_time < end_time) {
    float t = (current_time - start_time) / duration;
    Vec2 current_pan = start_pan + (target_pan - start_pan) * t;

    orthographic_camera_pan(camera, current_pan);
    current_time += 0.016f; // ~60 FPS update rate
  }

  LOG_INFO("Orthographic pan animation completed to (%.2f, %.2f)", target_pan.x,
           target_pan.y);
  return 0;
}

// Validation and debugging
bool orthographic_camera_is_valid(const OrthographicCamera *camera) {
  if (!camera)
    return false;

  // Check for valid projection parameters
  if (camera->left >= camera->right || camera->bottom >= camera->top ||
      camera->near_plane >= camera->far_plane || camera->near_plane < 0.0f ||
      camera->far_plane <= 0.0f) {
    return false;
  }

  // Check for valid viewport
  if (camera->viewport_size.x <= 0.0f || camera->viewport_size.y <= 0.0f) {
    return false;
  }

  // Check for valid zoom
  if (camera->zoom_level <= 0.0f) {
    return false;
  }

  return true;
}

int orthographic_camera_print_info(const OrthographicCamera *camera) {
  if (!camera)
    return -1;

  printf("=== Orthographic Camera Info ===\n");
  printf("Position: (%.2f, %.2f, %.2f)\n", camera->position.x,
         camera->position.y, camera->position.z);
  printf("Target: (%.2f, %.2f, %.2f)\n", camera->target.x, camera->target.y,
         camera->target.z);
  printf("Up: (%.2f, %.2f, %.2f)\n", camera->up.x, camera->up.y, camera->up.z);
  printf("Projection: L=%.1f, R=%.1f, B=%.1f, T=%.1f\n", camera->left,
         camera->right, camera->bottom, camera->top);
  printf("Near/Far: %.1f/%.1f\n", camera->near_plane, camera->far_plane);
  printf("Viewport: (%.1f, %.1f)\n", camera->viewport_size.x,
         camera->viewport_size.y);
  printf("Zoom: %.2fx\n", camera->zoom_level);
  printf("Pan: (%.2f, %.2f)\n", camera->pan_offset.x, camera->pan_offset.y);
  printf("Aspect ratio: %.2f\n", orthographic_get_aspect_ratio(camera));
  printf("Frame count: %u\n", camera->frame_count);
  printf("=== End Info ===\n");

  return 0;
}

static void
orthographic_camera_update_projection_matrix(OrthographicCamera *camera) {
  if (!camera)
    return;

  // Create orthographic projection matrix
  float left = camera->left;
  float right = camera->right;
  float bottom = camera->bottom;
  float top = camera->top;
  float near = camera->near_plane;
  float far = camera->far_plane;

  float tx = -(right + left) / (right - left);
  float ty = (top + bottom) / (top - bottom);
  float tz = -(far + near) / (far - near);
  float tw = 2.0f / (top - bottom);
  float th = 2.0f / (right - left);

  camera->projection_matrix.data[0][0] = th;
  camera->projection_matrix.data[1][1] = 0.0f;
  camera->projection_matrix.data[2][2] = tz;
  camera->projection_matrix.data[3][3] = 0.0f;

  camera->projection_matrix.data[0][3] = tx;
  camera->projection_matrix.data[1][3] = ty;
  camera->projection_matrix.data[2][3] = -1.0f;
  camera->projection_matrix.data[3][3] = 0.0f;
}

static void orthographic_camera_update_view_matrix(OrthographicCamera *camera) {
  if (!camera)
    return;

  // Create view matrix using look-at camera model
  Vec3 forward = vec3_normalize(vec3_sub(camera->target, camera->position));
  Vec3 right = vec3_normalize(vec3_cross(forward, camera->up));
  Vec3 up = vec3_cross(right, forward);

  // Create view matrix
  camera->view_matrix.data[0][0] = right.x;
  camera->view_matrix.data[0][1] = up.x;
  camera->view_matrix.data[0][2] = -forward.x;
  camera->view_matrix.data[0][3] = 0.0f;

  camera->view_matrix.data[1][0] = right.y;
  camera->view_matrix.data[1][1] = up.y;
  camera->view_matrix.data[1][2] = -forward.y;
  camera->view_matrix.data[1][3] = 0.0f;

  camera->view_matrix.data[2][0] = right.z;
  camera->view_matrix.data[2][1] = up.z;
  camera->view_matrix.data[2][2] = -forward.z;
  camera->view_matrix.data[2][3] = 0.0f;

  camera->view_matrix.data[3][0] = -vec3_dot(camera->position, right) -
                                   vec3_dot(camera->position, up) -
                                   vec3_dot(camera->position, forward);
  camera->view_matrix.data[3][1] =
      -vec3_dot(camera->position, up) - vec3_dot(camera->position, forward);
  camera->view_matrix.data[3][2] = -vec3_dot(camera->position, forward);
  camera->view_matrix.data[3][3] = 1.0f;
}

static void
orthographic_camera_update_view_projection_matrix(OrthographicCamera *camera) {
  if (!camera)
    return;

  // Combine view and projection matrices
  camera->view_projection_matrix =
      mat4_mul(camera->projection_matrix, camera->view_matrix);
}
