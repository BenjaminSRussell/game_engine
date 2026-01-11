/**
 * =================================================================================================
 *                          TRANSFORM GIZMO SYSTEM
 *                          Phase 7: Editor & Tools
 * =================================================================================================
 */

#include "../include/math/mat4.h"
#include "../include/math/quat.h"
#include "../include/math/vec3.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Gizmo Modes
typedef enum {
  GIZMO_MODE_NONE,
  GIZMO_MODE_TRANSLATE,
  GIZMO_MODE_ROTATE,
  GIZMO_MODE_SCALE
} GizmoMode;

// Gizmo Axis
typedef enum {
  GIZMO_AXIS_NONE,
  GIZMO_AXIS_X,
  GIZMO_AXIS_Y,
  GIZMO_AXIS_Z,
  GIZMO_AXIS_XY,
  GIZMO_AXIS_XZ,
  GIZMO_AXIS_YZ,
  GIZMO_AXIS_XYZ
} GizmoAxis;

// Gizmo Space
typedef enum { GIZMO_SPACE_LOCAL, GIZMO_SPACE_WORLD } GizmoSpace;

// Interaction State
typedef struct {
  GizmoMode mode;
  GizmoAxis active_axis;
  GizmoSpace space;

  Vec3 initial_position;
  Quat initial_rotation;
  Vec3 initial_scale;

  Vec3 drag_start_point;
  float drag_start_factor; // t-value or similar

  bool is_dragging;
  float snap_increment;
} GizmoState;

// Simple Ray structure
typedef struct {
  Vec3 origin;
  Vec3 direction;
} GizmoRay;

static GizmoState g_gizmo_state = {0};

// -----------------------------------------------------------------------------
// Math Helpers
// -----------------------------------------------------------------------------

// Ray-Cylinder Intersection
static bool gizmo_intersect_axis(GizmoRay ray, Vec3 origin, Vec3 axis_dir,
                                 float length, float radius, float *out_t) {
  Vec3 q = vec3_sub(origin, ray.origin);
  float dot_ua = vec3_dot(axis_dir, ray.direction);
  float dot_uq = vec3_dot(axis_dir, q);
  float dot_aq = vec3_dot(ray.direction, q);

  float denom = 1.0f - dot_ua * dot_ua;
  if (denom < 1e-6f)
    return false; // Parallel

  // Closest point on ray (t) and axis (s)
  float t = (dot_ua * dot_uq - dot_aq) / denom;
  float s = (dot_uq - dot_ua * dot_aq) / denom;

  // Clamp s to cylinder segment
  if (s < 0.0f)
    s = 0.0f;
  if (s > length)
    s = length;

  Vec3 p_ray = vec3_add(ray.origin, vec3_mul(ray.direction, t));
  Vec3 p_axis = vec3_add(origin, vec3_mul(axis_dir, s));

  if (vec3_length_sq(vec3_sub(p_ray, p_axis)) <= radius * radius) {
    *out_t = t;
    return true;
  }
  return false;
}

// Project ray closest point onto line
static float closest_t_on_line(GizmoRay ray, Vec3 line_origin, Vec3 line_dir) {
  Vec3 w0 = vec3_sub(ray.origin, line_origin);
  float a = vec3_dot(ray.direction, ray.direction);
  float b = vec3_dot(ray.direction, line_dir);
  float c = vec3_dot(line_dir, line_dir);
  float d = vec3_dot(ray.direction, w0);
  float e = vec3_dot(line_dir, w0);

  float denom = a * c - b * b;
  if (denom < 1e-6f)
    return 0.0f;

  return (a * e - b * d) / denom;
}

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

void gizmo_init(void) {
  g_gizmo_state.mode = GIZMO_MODE_TRANSLATE;
  g_gizmo_state.space = GIZMO_SPACE_WORLD;
  g_gizmo_state.snap_increment = 0.0f;
}

void gizmo_set_mode(GizmoMode mode) { g_gizmo_state.mode = mode; }
void gizmo_set_space(GizmoSpace space) { g_gizmo_state.space = space; }
void gizmo_set_snap(float increment) {
  g_gizmo_state.snap_increment = increment;
}

void gizmo_update(GizmoRay ray, bool mouse_down, Vec3 *position, Quat *rotation,
                  Vec3 *scale) {
  Vec3 axis_x = {1, 0, 0};
  Vec3 axis_y = {0, 1, 0};
  Vec3 axis_z = {0, 0, 1};

  float axis_length = 2.0f;
  float axis_radius = 0.1f;

  // 1. Mouse Up -> Stop Dragging
  if (!mouse_down) {
    g_gizmo_state.is_dragging = false;

    // Hover Check
    g_gizmo_state.active_axis = GIZMO_AXIS_NONE;
    float min_t = 1e9f;
    float t;

    if (gizmo_intersect_axis(ray, *position, axis_x, axis_length, axis_radius,
                             &t) &&
        t < min_t) {
      min_t = t;
      g_gizmo_state.active_axis = GIZMO_AXIS_X;
    }
    if (gizmo_intersect_axis(ray, *position, axis_y, axis_length, axis_radius,
                             &t) &&
        t < min_t) {
      min_t = t;
      g_gizmo_state.active_axis = GIZMO_AXIS_Y;
    }
    if (gizmo_intersect_axis(ray, *position, axis_z, axis_length, axis_radius,
                             &t) &&
        t < min_t) {
      min_t = t;
      g_gizmo_state.active_axis = GIZMO_AXIS_Z;
    }
    return;
  }

  // 2. Mouse Down (Initial Click)
  if (mouse_down && !g_gizmo_state.is_dragging &&
      g_gizmo_state.active_axis != GIZMO_AXIS_NONE) {
    g_gizmo_state.is_dragging = true;
    g_gizmo_state.initial_position = *position;

    Vec3 axis_dir = (g_gizmo_state.active_axis == GIZMO_AXIS_X)   ? axis_x
                    : (g_gizmo_state.active_axis == GIZMO_AXIS_Y) ? axis_y
                                                                  : axis_z;

    g_gizmo_state.drag_start_factor =
        closest_t_on_line(ray, *position, axis_dir);
    return;
  }

  // 3. Dragging
  if (g_gizmo_state.is_dragging) {
    Vec3 axis_dir = (g_gizmo_state.active_axis == GIZMO_AXIS_X)   ? axis_x
                    : (g_gizmo_state.active_axis == GIZMO_AXIS_Y) ? axis_y
                                                                  : axis_z;

    float current_factor =
        closest_t_on_line(ray, g_gizmo_state.initial_position, axis_dir);
    float delta = current_factor - g_gizmo_state.drag_start_factor;

    // Snapping
    if (g_gizmo_state.snap_increment > 0.0f) {
      // Logic for snapping would go here
    }

    if (g_gizmo_state.mode == GIZMO_MODE_TRANSLATE) {
      *position =
          vec3_add(g_gizmo_state.initial_position, vec3_mul(axis_dir, delta));
    }
  }
}
