#pragma once

#include <stdbool.h>

typedef enum {
    GIZMO_MODE_TRANSLATE,
    GIZMO_MODE_ROTATE,
    GIZMO_MODE_SCALE
} GizmoMode;

typedef enum {
    GIZMO_AXIS_NONE = 0,
    GIZMO_AXIS_X = 1,
    GIZMO_AXIS_Y = 2,
    GIZMO_AXIS_Z = 4,
    GIZMO_AXIS_XY = 3,
    GIZMO_AXIS_XZ = 5,
    GIZMO_AXIS_YZ = 6,
    GIZMO_AXIS_SCREEN = 7
} GizmoAxis;

typedef enum {
    GIZMO_SPACE_WORLD,
    GIZMO_SPACE_LOCAL
} GizmoSpace;

typedef struct {
    GizmoMode mode;
    GizmoSpace space;
    GizmoAxis active_axis;
    GizmoAxis hovered_axis;
    
    float position[3];
    float rotation[4]; // quaternion for local space
    float scale_factor;
    
    bool is_dragging;
    float drag_start_pos[3];
    float drag_plane_normal[3];
} TransformGizmo;

void gizmo_init(TransformGizmo *gizmo);
void gizmo_set_mode(TransformGizmo *gizmo, GizmoMode mode);
void gizmo_set_space(TransformGizmo *gizmo, GizmoSpace space);
void gizmo_set_transform(TransformGizmo *gizmo, const float *pos, const float *rot);

// Hit testing (returns which axis was hit)
GizmoAxis gizmo_raycast(TransformGizmo *gizmo, const float *ray_origin, const float *ray_dir, const float *view_mat);

// Interaction
void gizmo_begin_drag(TransformGizmo *gizmo, GizmoAxis axis, const float *ray_origin, const float *ray_dir);
void gizmo_update_drag(TransformGizmo *gizmo, const float *ray_origin, const float *ray_dir, float *out_delta);
void gizmo_end_drag(TransformGizmo *gizmo);

// Rendering (stub)
void gizmo_render(TransformGizmo *gizmo, const float *view_mat, const float *proj_mat);
