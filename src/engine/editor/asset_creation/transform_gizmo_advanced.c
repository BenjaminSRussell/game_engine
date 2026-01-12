/**
 * =================================================================================================
 *                          TRANSFORM GIZMO (ADVANCED)
 * =================================================================================================
 */

#include "editor/asset_creation/transform_gizmo_advanced.h"
#include <include/math/math.h>
#include <stdio.h>
#include <string.h>

void gizmo_init(TransformGizmo *gizmo) {
  memset(gizmo, 0, sizeof(TransformGizmo));
  gizmo->mode = GIZMO_MODE_TRANSLATE;
  gizmo->space = GIZMO_SPACE_WORLD;
  gizmo->scale_factor = 1.0f;
  gizmo->rotation[3] = 1.0f; // Identity quaternion
}

void gizmo_set_mode(TransformGizmo *gizmo, GizmoMode mode) {
  gizmo->mode = mode;
}

void gizmo_set_space(TransformGizmo *gizmo, GizmoSpace space) {
  gizmo->space = space;
}

void gizmo_set_transform(TransformGizmo *gizmo, const float *pos,
                         const float *rot) {
  if (pos) {
    gizmo->position[0] = pos[0];
    gizmo->position[1] = pos[1];
    gizmo->position[2] = pos[2];
  }
  if (rot) {
    gizmo->rotation[0] = rot[0];
    gizmo->rotation[1] = rot[1];
    gizmo->rotation[2] = rot[2];
    gizmo->rotation[3] = rot[3];
  }
}

static float ray_distance_to_line(const float *ray_o, const float *ray_d,
                                  const float *line_p, const float *line_d) {
  // TODO(Jules): Fix ray intersection precision issues for distant objects.
  // Distance from ray to line segment
  float w[3] = {ray_o[0] - line_p[0], ray_o[1] - line_p[1],
                ray_o[2] - line_p[2]};
  float a = ray_d[0] * ray_d[0] + ray_d[1] * ray_d[1] + ray_d[2] * ray_d[2];
  float b = ray_d[0] * line_d[0] + ray_d[1] * line_d[1] + ray_d[2] * line_d[2];
  float c =
      line_d[0] * line_d[0] + line_d[1] * line_d[1] + line_d[2] * line_d[2];
  float d = ray_d[0] * w[0] + ray_d[1] * w[1] + ray_d[2] * w[2];
  float e = line_d[0] * w[0] + line_d[1] * w[1] + line_d[2] * w[2];

  float denom = a * c - b * b;
  if (fabsf(denom) < 0.0001f)
    return 1e6f;

  float sc = (b * e - c * d) / denom;
  float tc = (a * e - b * d) / denom;

  float closest[3];
  closest[0] = w[0] + sc * ray_d[0] - tc * line_d[0];
  closest[1] = w[1] + sc * ray_d[1] - tc * line_d[1];
  closest[2] = w[2] + sc * ray_d[2] - tc * line_d[2];

  return sqrtf(closest[0] * closest[0] + closest[1] * closest[1] +
               closest[2] * closest[2]);
}

GizmoAxis gizmo_raycast(TransformGizmo *gizmo, const float *ray_origin,
                        const float *ray_dir, const float *view_mat) {
  // TODO(Jules): Add scale planes (XY, YZ, XZ) handling in gizmo_raycast.
  const float threshold = 0.2f; // Pick threshold

  float min_dist = 1e6f;
  GizmoAxis result = GIZMO_AXIS_NONE;

  if (gizmo->mode == GIZMO_MODE_TRANSLATE) {
    // Test X axis
    float x_axis[3] = {1, 0, 0};
    float dist_x =
        ray_distance_to_line(ray_origin, ray_dir, gizmo->position, x_axis);
    if (dist_x < threshold && dist_x < min_dist) {
      min_dist = dist_x;
      result = GIZMO_AXIS_X;
    }

    // Test Y axis
    float y_axis[3] = {0, 1, 0};
    float dist_y =
        ray_distance_to_line(ray_origin, ray_dir, gizmo->position, y_axis);
    if (dist_y < threshold && dist_y < min_dist) {
      min_dist = dist_y;
      result = GIZMO_AXIS_Y;
    }

    // Test Z axis
    float z_axis[3] = {0, 0, 1};
    float dist_z =
        ray_distance_to_line(ray_origin, ray_dir, gizmo->position, z_axis);
    if (dist_z < threshold && dist_z < min_dist) {
      min_dist = dist_z;
      result = GIZMO_AXIS_Z;
    }
  }

  gizmo->hovered_axis = result;
  // TODO(Jules): Add visual feedback when hovering over axes (highlighting).
  return result;
}

void gizmo_begin_drag(TransformGizmo *gizmo, GizmoAxis axis,
                      const float *ray_origin, const float *ray_dir) {
  gizmo->is_dragging = true;
  gizmo->active_axis = axis;

  // Store start position
  memcpy(gizmo->drag_start_pos, gizmo->position, sizeof(float) * 3);

  // Determine drag plane normal based on axis
  if (axis == GIZMO_AXIS_X) {
    // Use YZ plane
    gizmo->drag_plane_normal[0] = 1;
    gizmo->drag_plane_normal[1] = 0;
    gizmo->drag_plane_normal[2] = 0;
  } else if (axis == GIZMO_AXIS_Y) {
    gizmo->drag_plane_normal[0] = 0;
    gizmo->drag_plane_normal[1] = 1;
    gizmo->drag_plane_normal[2] = 0;
  } else if (axis == GIZMO_AXIS_Z) {
    gizmo->drag_plane_normal[0] = 0;
    gizmo->drag_plane_normal[1] = 0;
    gizmo->drag_plane_normal[2] = 1;
  }
}

void gizmo_update_drag(TransformGizmo *gizmo, const float *ray_origin,
                       const float *ray_dir, float *out_delta) {
  if (!gizmo->is_dragging)
    return;

  // Intersect ray with drag plane
  // Plane: dot(P - plane_point, normal) = 0
  // Ray: P = O + t*D
  // dot(O + t*D - plane_point, normal) = 0
  // t = dot(plane_point - O, normal) / dot(D, normal)

  float to_plane[3] = {gizmo->drag_start_pos[0] - ray_origin[0],
                       gizmo->drag_start_pos[1] - ray_origin[1],
                       gizmo->drag_start_pos[2] - ray_origin[2]};

  float num = to_plane[0] * gizmo->drag_plane_normal[0] +
              to_plane[1] * gizmo->drag_plane_normal[1] +
              to_plane[2] * gizmo->drag_plane_normal[2];

  float denom = ray_dir[0] * gizmo->drag_plane_normal[0] +
                ray_dir[1] * gizmo->drag_plane_normal[1] +
                ray_dir[2] * gizmo->drag_plane_normal[2];

  if (fabsf(denom) < 0.0001f) {
    out_delta[0] = out_delta[1] = out_delta[2] = 0;
    return;
  }

  float t = num / denom;
  float hit[3] = {ray_origin[0] + t * ray_dir[0],
                  ray_origin[1] + t * ray_dir[1],
                  ray_origin[2] + t * ray_dir[2]};

  // Project onto axis
  float delta[3] = {hit[0] - gizmo->drag_start_pos[0],
                    hit[1] - gizmo->drag_start_pos[1],
                    hit[2] - gizmo->drag_start_pos[2]};

  if (gizmo->active_axis == GIZMO_AXIS_X) {
    out_delta[0] = delta[0];
    out_delta[1] = 0;
    out_delta[2] = 0;
  } else if (gizmo->active_axis == GIZMO_AXIS_Y) {
    out_delta[0] = 0;
    out_delta[1] = delta[1];
    out_delta[2] = 0;
  } else if (gizmo->active_axis == GIZMO_AXIS_Z) {
    out_delta[0] = 0;
    out_delta[1] = 0;
    out_delta[2] = delta[2];
  }
}

void gizmo_end_drag(TransformGizmo *gizmo) {
  gizmo->is_dragging = false;
  gizmo->active_axis = GIZMO_AXIS_NONE;
  // TODO(Jules): Add undo/redo support for gizmo transformations.
}

void gizmo_render(TransformGizmo *gizmo, const float *view_mat,
                  const float *proj_mat) {
  // Render logic would push line/mesh draw commands to a debug renderer
  // Color coding: X=Red, Y=Green, Z=Blue
  // Highlighted axis: Brighter color

  // Example pseudo-code:
  // draw_line(gizmo->position, gizmo->position + X_axis, RED);
  // draw_line(gizmo->position, gizmo->position + Y_axis, GREEN);
  // draw_line(gizmo->position, gizmo->position + Z_axis, BLUE);

  // For rotation mode, draw circles
  // For scale mode, draw cubes at ends
  // TODO(Jules): Implement actual rendering logic in gizmo_render.
}
