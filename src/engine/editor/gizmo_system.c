/**
 * =================================================================================================
 *                          TRANSFORM GIZMO SYSTEM
 *                          Phase 7: Editor & Tools
 * =================================================================================================
 *
 * PURPOSE: 3D manipulation gizmos (translation, rotation, scaling) for scene
 * editor
 * =================================================================================================
 */

#include <include/math/math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  float x, y, z;
} GizmoVec3;
typedef struct {
  float x, y, z, w;
} GizmoQuat;
typedef struct {
  float m[16];
} GizmoMat4;

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
  GIZMO_AXIS_XYZ // Center/Screen
} GizmoAxis;

// Gizmo Space
typedef enum { GIZMO_SPACE_LOCAL, GIZMO_SPACE_WORLD } GizmoSpace;

// Interaction State
typedef struct {
  GizmoMode mode;
  GizmoAxis active_axis;
  GizmoSpace space;

  GizmoVec3 initial_position;
  GizmoQuat initial_rotation;
  GizmoVec3 initial_scale;

  GizmoVec3 drag_start_point;
  GizmoVec3 drag_current_point;
  float drag_start_offset;

  bool is_dragging;
  float snap_increment; // 0 = disabled
} GizmoState;

// Simple Ray structure
typedef struct {
  GizmoVec3 origin;
  GizmoVec3 direction;
} GizmoRay;

// -----------------------------------------------------------------------------
// Vector Math Helpers
// -----------------------------------------------------------------------------

static inline GizmoVec3 gizmo_vec3_add(GizmoVec3 a, GizmoVec3 b) {
  return (GizmoVec3){a.x + b.x, a.y + b.y, a.z + b.z};
}
static inline GizmoVec3 gizmo_vec3_sub(GizmoVec3 a, GizmoVec3 b) {
  return (GizmoVec3){a.x - b.x, a.y - b.y, a.z - b.z};
}
static inline GizmoVec3 gizmo_vec3_scale(GizmoVec3 v, float s) {
  return (GizmoVec3){v.x * s, v.y * s, v.z * s};
}
static inline float gizmo_vec3_dot(GizmoVec3 a, GizmoVec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}
static inline GizmoVec3 gizmo_vec3_cross(GizmoVec3 a, GizmoVec3 b) {
  return (GizmoVec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                     a.x * b.y - a.y * b.x};
}

static inline float gizmo_vec3_dist_sq_segment(GizmoVec3 p, GizmoVec3 a,
                                               GizmoVec3 b) {
  GizmoVec3 ab = gizmo_vec3_sub(b, a);
  GizmoVec3 ap = gizmo_vec3_sub(p, a);
  float t = gizmo_vec3_dot(ap, ab) / gizmo_vec3_dot(ab, ab);
  if (t < 0.0f)
    t = 0.0f;
  if (t > 1.0f)
    t = 1.0f;
  GizmoVec3 closest = gizmo_vec3_add(a, gizmo_vec3_scale(ab, t));
  GizmoVec3 d = gizmo_vec3_sub(p, closest);
  return gizmo_vec3_dot(d, d);
}

// -----------------------------------------------------------------------------
// Gizmo Context
// -----------------------------------------------------------------------------

GizmoState g_gizmo_state = {0};

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

// -----------------------------------------------------------------------------
// Intersection Tests
// -----------------------------------------------------------------------------

// Ray-Cylinder intersection approximation for axes
static bool gizmo_intersect_axis(GizmoRay ray, GizmoVec3 origin,
                                 GizmoVec3 axis_dir, float length, float radius,
                                 float *out_t) {
  GizmoVec3 end = gizmo_vec3_add(origin, gizmo_vec3_scale(axis_dir, length));

  // Closest point on rays approach
  GizmoVec3 u = axis_dir;
  GizmoVec3 v = ray.direction;
  GizmoVec3 w = gizmo_vec3_sub(origin, ray.origin);
  float a = gizmo_vec3_dot(u, u); // always 1 if normalized
  float b = gizmo_vec3_dot(u, v);
  float c = gizmo_vec3_dot(v, v); // always 1 if normalized
  float d = gizmo_vec3_dot(u, w);
  float e = gizmo_vec3_dot(v, w);
  float D = a * c - b * b;

  float sc, tc;
  if (D < 1e-6f) {
    sc = 0.0f;
    tc = (b > c ? d / b : e / c);
  } else {
    sc = (b * e - c * d) / D;
    tc = (a * e - b * d) / D;
  }

  // Clamp sc to cylinder segment
  if (sc < 0.0f)
    sc = 0.0f;
  if (sc > length)
    sc = length;

  GizmoVec3 p_axis = gizmo_vec3_add(origin, gizmo_vec3_scale(u, sc));
  GizmoVec3 p_ray = gizmo_vec3_add(ray.origin, gizmo_vec3_scale(v, tc));

  GizmoVec3 diff = gizmo_vec3_sub(p_ray, p_axis);
  float dist_sq = gizmo_vec3_dot(diff, diff);

  if (dist_sq <= radius * radius) {
    *out_t = tc;
    return true;
  }
  return false;
}

// -----------------------------------------------------------------------------
// Gizmo Update Logic
// -----------------------------------------------------------------------------

void gizmo_update(GizmoRay ray, bool mouse_down, GizmoVec3 *position,
                  GizmoQuat *rotation, GizmoVec3 *scale) {

  // Calculate axes based on rotation/space
  GizmoVec3 axis_x = {1, 0, 0};
  GizmoVec3 axis_y = {0, 1, 0};
  GizmoVec3 axis_z = {0, 0, 1};

  if (g_gizmo_state.space == GIZMO_SPACE_LOCAL && rotation) {
    // Rotate axes by object rotation (stub math)
    // q * v * q_conj
  }

  float axis_length = 1.0f; // Could be scaled by distance to camera
  float axis_radius = 0.05f * axis_length;

  // Hover Check (if not dragging)
  if (!g_gizmo_state.is_dragging) {
    g_gizmo_state.active_axis = GIZMO_AXIS_NONE;
    float min_t = 1e9f;
    float t;

    // Check X axis
    if (gizmo_intersect_axis(ray, *position, axis_x, axis_length, axis_radius,
                             &t)) {
      if (t < min_t) {
        min_t = t;
        g_gizmo_state.active_axis = GIZMO_AXIS_X;
      }
    }
    // Check Y axis
    if (gizmo_intersect_axis(ray, *position, axis_y, axis_length, axis_radius,
                             &t)) {
      if (t < min_t) {
        min_t = t;
        g_gizmo_state.active_axis = GIZMO_AXIS_Y;
      }
    }
    // Check Z axis
    if (gizmo_intersect_axis(ray, *position, axis_z, axis_length, axis_radius,
                             &t)) {
      if (t < min_t) {
        min_t = t;
        g_gizmo_state.active_axis = GIZMO_AXIS_Z;
      }
    }

    // Handle Mouse Down
    if (mouse_down && g_gizmo_state.active_axis != GIZMO_AXIS_NONE) {
      g_gizmo_state.is_dragging = true;
      g_gizmo_state.initial_position = *position;
      if (rotation)
        g_gizmo_state.initial_rotation = *rotation;
      if (scale)
        g_gizmo_state.initial_scale = *scale;

      // Calculate drag start point logic (project ray to axis/plane)
    }
  } else {
    // Dragging Logic
    if (!mouse_down) {
      g_gizmo_state.is_dragging = false;
    } else {
      // Calculate delta based on active axis
      GizmoVec3 axis_dir = {0, 0, 0};
      if (g_gizmo_state.active_axis == GIZMO_AXIS_X)
        axis_dir = axis_x;
      if (g_gizmo_state.active_axis == GIZMO_AXIS_Y)
        axis_dir = axis_y;
      if (g_gizmo_state.active_axis == GIZMO_AXIS_Z)
        axis_dir = axis_z;

      // Project ray to closest point on axis line
      // For now, simplified logic

      // Apply transform delta
      if (g_gizmo_state.mode == GIZMO_MODE_TRANSLATE) {
        // Compute new position
      }
      // Rotate/Scale logic...
    }
  }
}
