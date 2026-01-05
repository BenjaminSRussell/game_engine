#pragma once

#include <stdbool.h>

typedef enum {
    GIZMO_MODE_TRANSLATE,
    GIZMO_MODE_ROTATE,
    GIZMO_MODE_SCALE
} GizmoMode;

typedef enum {
    GIZMO_SPACE_WORLD,
    GIZMO_SPACE_LOCAL
} GizmoSpace;

typedef enum {
    GIZMO_PIVOT_SELECTION_CENTER,
    GIZMO_PIVOT_INDIVIDUAL,
    GIZMO_PIVOT_ACTIVE
} GizmoPivotMode;

typedef struct {
    GizmoMode mode;
    GizmoSpace space;
    GizmoPivotMode pivot_mode;
    
    // Transform
    float position[3];
    float rotation[4]; // quat
    float scale;
    
    // Interaction
    int selected_axis; // 0=none, 1=X, 2=Y, 3=Z, 4=XY, 5=YZ, 6=XZ
    bool is_dragging;
    
    // Settings
    bool enable_grid_snap;
    float grid_size;
    float angle_snap; // degrees
    bool show_hotkeys;
} ViewportGizmo;

void gizmo_viewport_init(ViewportGizmo *gizmo);
void gizmo_viewport_set_mode(ViewportGizmo *gizmo, GizmoMode mode);
void gizmo_viewport_set_space(ViewportGizmo *gizmo, GizmoSpace space);
void gizmo_viewport_set_transform(ViewportGizmo *gizmo, const float *pos, const float *rot);

// Raycasting
int gizmo_viewport_raycast(ViewportGizmo *gizmo, const float *ray_origin, const float *ray_dir);

// Dragging
void gizmo_viewport_begin_drag(ViewportGizmo *gizmo);
void gizmo_viewport_update_drag(ViewportGizmo *gizmo, const float *delta);
void gizmo_viewport_end_drag(ViewportGizmo *gizmo);

// Rendering (stub)
void gizmo_viewport_render(ViewportGizmo *gizmo, const float *view, const float *proj);
