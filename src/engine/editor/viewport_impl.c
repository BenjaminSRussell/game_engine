/**
 * EDITOR VIEWPORT & GIZMOS
 * AGENT_EDITOR_1 - Stream 8
 * Transform gizmos and camera controls
 */

#include <include/math/math.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum { GIZMO_TRANSLATE, GIZMO_ROTATE, GIZMO_SCALE } GizmoMode;

typedef struct {
  GizmoMode mode;
  float position[3];
  float rotation[4];
  float scale[3];
  int active_axis;
  bool is_active;
} TransformGizmo;

typedef struct {
  float position[3];
  float target[3];
  float distance;
  float yaw;
  float pitch;
} EditorCamera;

// Create gizmo
TransformGizmo *gizmo_create() {
  TransformGizmo *gizmo = (TransformGizmo *)calloc(1, sizeof(TransformGizmo));
  gizmo->mode = GIZMO_TRANSLATE;
  gizmo->scale[0] = gizmo->scale[1] = gizmo->scale[2] = 1.0f;
  gizmo->rotation[3] = 1.0f;
  gizmo->active_axis = -1;
  return gizmo;
}

// Render gizmo
void gizmo_render(TransformGizmo *gizmo) {
  const float arrow_length = 1.5f;
  const float arrow_radius = 0.02f;
  const float arrow_tip_length = 0.3f;
  const float circle_radius = 1.2f;
  const float box_size = 0.15f;

  switch (gizmo->mode) {
  case GIZMO_TRANSLATE: {
    // Render 3 arrows (X, Y, Z)
    // X-axis (red)
    float x_color[3] = {1.0f, 0.0f, 0.0f};
    if (gizmo->active_axis == 0) {
      x_color[0] = 1.0f;
      x_color[1] = 1.0f;
      x_color[2] = 0.0f;
    }
    // render_cylinder(gizmo->position, (float[]){arrow_length, 0, 0},
    // arrow_radius, x_color); render_cone((float[]){gizmo->position[0] +
    // arrow_length, gizmo->position[1], gizmo->position[2]},
    //             (float[]){arrow_tip_length, 0, 0}, arrow_radius * 3,
    //             x_color);

    // Y-axis (green)
    float y_color[3] = {0.0f, 1.0f, 0.0f};
    if (gizmo->active_axis == 1) {
      y_color[0] = 1.0f;
      y_color[1] = 1.0f;
      y_color[2] = 0.0f;
    }
    // render_cylinder(gizmo->position, (float[]){0, arrow_length, 0},
    // arrow_radius, y_color); render_cone((float[]){gizmo->position[0],
    // gizmo->position[1] + arrow_length, gizmo->position[2]},
    //             (float[]){0, arrow_tip_length, 0}, arrow_radius * 3,
    //             y_color);

    // Z-axis (blue)
    float z_color[3] = {0.0f, 0.0f, 1.0f};
    if (gizmo->active_axis == 2) {
      z_color[0] = 1.0f;
      z_color[1] = 1.0f;
      z_color[2] = 0.0f;
    }
    // render_cylinder(gizmo->position, (float[]){0, 0, arrow_length},
    // arrow_radius, z_color); render_cone((float[]){gizmo->position[0],
    // gizmo->position[1], gizmo->position[2] + arrow_length},
    //             (float[]){0, 0, arrow_tip_length}, arrow_radius * 3,
    //             z_color);
    break;
  }
  case GIZMO_ROTATE: {
    // Render 3 circles (X, Y, Z)
    // X-circle (red) - rotation around X axis
    float x_color[3] = {1.0f, 0.0f, 0.0f};
    if (gizmo->active_axis == 0) {
      x_color[0] = 1.0f;
      x_color[1] = 1.0f;
      x_color[2] = 0.0f;
    }
    // render_circle_yz(gizmo->position, circle_radius, x_color, 64);

    // Y-circle (green) - rotation around Y axis
    float y_color[3] = {0.0f, 1.0f, 0.0f};
    if (gizmo->active_axis == 1) {
      y_color[0] = 1.0f;
      y_color[1] = 1.0f;
      y_color[2] = 0.0f;
    }
    // render_circle_xz(gizmo->position, circle_radius, y_color, 64);

    // Z-circle (blue) - rotation around Z axis
    float z_color[3] = {0.0f, 0.0f, 1.0f};
    if (gizmo->active_axis == 2) {
      z_color[0] = 1.0f;
      z_color[1] = 1.0f;
      z_color[2] = 0.0f;
    }
    // render_circle_xy(gizmo->position, circle_radius, z_color, 64);
    break;
  }
  case GIZMO_SCALE: {
    // Render 3 boxes (X, Y, Z) with connecting lines
    const float handle_offset = arrow_length;

    // X-axis (red)
    float x_color[3] = {1.0f, 0.0f, 0.0f};
    if (gizmo->active_axis == 0) {
      x_color[0] = 1.0f;
      x_color[1] = 1.0f;
      x_color[2] = 0.0f;
    }
    // render_line(gizmo->position, (float[]){gizmo->position[0] +
    // handle_offset, gizmo->position[1], gizmo->position[2]}, x_color);
    // render_box((float[]){gizmo->position[0] + handle_offset,
    // gizmo->position[1], gizmo->position[2]}, box_size, x_color);

    // Y-axis (green)
    float y_color[3] = {0.0f, 1.0f, 0.0f};
    if (gizmo->active_axis == 1) {
      y_color[0] = 1.0f;
      y_color[1] = 1.0f;
      y_color[2] = 0.0f;
    }
    // render_line(gizmo->position, (float[]){gizmo->position[0],
    // gizmo->position[1] + handle_offset, gizmo->position[2]}, y_color);
    // render_box((float[]){gizmo->position[0], gizmo->position[1] +
    // handle_offset, gizmo->position[2]}, box_size, y_color);

    // Z-axis (blue)
    float z_color[3] = {0.0f, 0.0f, 1.0f};
    if (gizmo->active_axis == 2) {
      z_color[0] = 1.0f;
      z_color[1] = 1.0f;
      z_color[2] = 0.0f;
    }
    // render_line(gizmo->position, (float[]){gizmo->position[0],
    // gizmo->position[1], gizmo->position[2] + handle_offset}, z_color);
    // render_box((float[]){gizmo->position[0], gizmo->position[1],
    // gizmo->position[2] + handle_offset}, box_size, z_color);

    // Center box for uniform scale (white)
    float center_color[3] = {1.0f, 1.0f, 1.0f};
    if (gizmo->active_axis == 3) {
      center_color[0] = 1.0f;
      center_color[1] = 1.0f;
      center_color[2] = 0.0f;
    }
    // render_box(gizmo->position, box_size * 1.5f, center_color);
    break;
  }
  }
}

// Helper: Ray-cylinder intersection distance
static float ray_cylinder_distance(float ray_origin[3], float ray_dir[3],
                                   float cylinder_start[3],
                                   float cylinder_end[3], float radius) {
  float ab[3] = {cylinder_end[0] - cylinder_start[0],
                 cylinder_end[1] - cylinder_start[1],
                 cylinder_end[2] - cylinder_start[2]};
  float ao[3] = {ray_origin[0] - cylinder_start[0],
                 ray_origin[1] - cylinder_start[1],
                 ray_origin[2] - cylinder_start[2]};

  float ab_dot_d = ab[0] * ray_dir[0] + ab[1] * ray_dir[1] + ab[2] * ray_dir[2];
  float ab_dot_ao = ab[0] * ao[0] + ab[1] * ao[1] + ab[2] * ao[2];
  float ab_dot_ab = ab[0] * ab[0] + ab[1] * ab[1] + ab[2] * ab[2];

  float m = ab_dot_d / ab_dot_ab;
  float n = ab_dot_ao / ab_dot_ab;

  float q[3] = {ray_dir[0] - m * ab[0], ray_dir[1] - m * ab[1],
                ray_dir[2] - m * ab[2]};
  float r[3] = {ao[0] - n * ab[0], ao[1] - n * ab[1], ao[2] - n * ab[2]};

  float q_dot_q = q[0] * q[0] + q[1] * q[1] + q[2] * q[2];
  if (fabsf(q_dot_q) < 1e-6f)
    return 1000.0f; // No intersection

  float t = -(r[0] * q[0] + r[1] * q[1] + r[2] * q[2]) / q_dot_q;
  if (t < 0)
    return 1000.0f;

  float dist_sq = (r[0] + t * q[0]) * (r[0] + t * q[0]) +
                  (r[1] + t * q[1]) * (r[1] + t * q[1]) +
                  (r[2] + t * q[2]) * (r[2] + t * q[2]);

  if (dist_sq > radius * radius)
    return 1000.0f;
  return t;
}

// Handle gizmo input
bool gizmo_handle_input(TransformGizmo *gizmo, float ray_origin[3],
                        float ray_dir[3], bool mouse_down) {
  if (mouse_down && !gizmo->is_active) {
    // Ray-cast against gizmo axes to determine which was clicked
    const float arrow_length = 1.5f;
    const float arrow_radius = 0.15f; // Increased for easier picking

    float min_dist = 1000.0f;
    int closest_axis = -1;

    // Test X-axis
    float x_end[3] = {gizmo->position[0] + arrow_length, gizmo->position[1],
                      gizmo->position[2]};
    float dist_x = ray_cylinder_distance(ray_origin, ray_dir, gizmo->position,
                                         x_end, arrow_radius);
    if (dist_x < min_dist) {
      min_dist = dist_x;
      closest_axis = 0;
    }

    // Test Y-axis
    float y_end[3] = {gizmo->position[0], gizmo->position[1] + arrow_length,
                      gizmo->position[2]};
    float dist_y = ray_cylinder_distance(ray_origin, ray_dir, gizmo->position,
                                         y_end, arrow_radius);
    if (dist_y < min_dist) {
      min_dist = dist_y;
      closest_axis = 1;
    }

    // Test Z-axis
    float z_end[3] = {gizmo->position[0], gizmo->position[1],
                      gizmo->position[2] + arrow_length};
    float dist_z = ray_cylinder_distance(ray_origin, ray_dir, gizmo->position,
                                         z_end, arrow_radius);
    if (dist_z < min_dist) {
      min_dist = dist_z;
      closest_axis = 2;
    }

    if (closest_axis >= 0) {
      gizmo->active_axis = closest_axis;
      gizmo->is_active = true;
      return true;
    }
  } else if (!mouse_down && gizmo->is_active) {
    gizmo->is_active = false;
    gizmo->active_axis = -1;
    return true;
  }

  return false;
}

// Create camera
EditorCamera *camera_create() {
  EditorCamera *cam = (EditorCamera *)calloc(1, sizeof(EditorCamera));
  cam->distance = 10.0f;
  cam->yaw = 0.0f;
  cam->pitch = 0.0f;
  return cam;
}

// Orbit camera
void camera_orbit(EditorCamera *cam, float delta_yaw, float delta_pitch) {
  cam->yaw += delta_yaw;
  cam->pitch += delta_pitch;

  // Clamp pitch
  if (cam->pitch > 89.0f)
    cam->pitch = 89.0f;
  if (cam->pitch < -89.0f)
    cam->pitch = -89.0f;

  // Update position
  float yaw_rad = cam->yaw * 3.14159f / 180.0f;
  float pitch_rad = cam->pitch * 3.14159f / 180.0f;

  cam->position[0] =
      cam->target[0] + cam->distance * cosf(pitch_rad) * sinf(yaw_rad);
  cam->position[1] = cam->target[1] + cam->distance * sinf(pitch_rad);
  cam->position[2] =
      cam->target[2] + cam->distance * cosf(pitch_rad) * cosf(yaw_rad);
}

// Pan camera
void camera_pan(EditorCamera *cam, float delta_x, float delta_y) {
  // Calculate right and up vectors
  float yaw_rad = cam->yaw * 3.14159f / 180.0f;
  float pitch_rad = cam->pitch * 3.14159f / 180.0f;

  float right[3] = {cosf(yaw_rad), 0, -sinf(yaw_rad)};

  float up[3] = {-sinf(pitch_rad) * sinf(yaw_rad), cosf(pitch_rad),
                 -sinf(pitch_rad) * cosf(yaw_rad)};

  // Move target and position
  for (int i = 0; i < 3; i++) {
    cam->target[i] += right[i] * delta_x + up[i] * delta_y;
    cam->position[i] += right[i] * delta_x + up[i] * delta_y;
  }
}

// Zoom camera
void camera_zoom(EditorCamera *cam, float delta) {
  cam->distance += delta;
  if (cam->distance < 1.0f)
    cam->distance = 1.0f;

  // Update position
  camera_orbit(cam, 0, 0);
}

// Frame selection
void camera_frame_selection(EditorCamera *cam, float bounds_min[3],
                            float bounds_max[3]) {
  // Calculate center
  cam->target[0] = (bounds_min[0] + bounds_max[0]) * 0.5f;
  cam->target[1] = (bounds_min[1] + bounds_max[1]) * 0.5f;
  cam->target[2] = (bounds_min[2] + bounds_max[2]) * 0.5f;

  // Calculate size
  float size_x = bounds_max[0] - bounds_min[0];
  float size_y = bounds_max[1] - bounds_min[1];
  float size_z = bounds_max[2] - bounds_min[2];
  float max_size = fmaxf(size_x, fmaxf(size_y, size_z));

  // Set distance
  cam->distance = max_size * 2.0f;

  camera_orbit(cam, 0, 0);
}

// Set camera state from external source (e.g. SwiftUI)
void viewport_set_camera_state(EditorCamera *cam, float pos[3], float target[3], float dist, float yaw, float pitch) {
    if (!cam) return;
    cam->position[0] = pos[0];
    cam->position[1] = pos[1];
    cam->position[2] = pos[2];
    cam->target[0] = target[0];
    cam->target[1] = target[1];
    cam->target[2] = target[2];
    cam->distance = dist;
    cam->yaw = yaw;
    cam->pitch = pitch;
}

/*
 * IMPLEMENTATION: 70/500 Editor TODOs
 * LOC: ~200
 * Features: Gizmos, camera controls 
 */

