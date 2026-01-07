// include/editor/viewport/gizmos.h
//
// Purpose: 3D transform gizmo system for editor viewport
// Provides translate, rotate, and scale manipulation tools
//
#ifndef EDITOR_VIEWPORT_GIZMOS_H
#define EDITOR_VIEWPORT_GIZMOS_H

#include "include/common.h"
#include "include/math/math.h"

// Forward declarations
typedef struct Camera Camera;
typedef struct IRenderer IRenderer;

// Gizmo types
typedef enum {
    GIZMO_TYPE_TRANSLATE,
    GIZMO_TYPE_ROTATE,
    GIZMO_TYPE_SCALE,
    GIZMO_TYPE_COUNT
} GizmoType;

// Coordinate space modes
typedef enum {
    GIZMO_SPACE_WORLD,
    GIZMO_SPACE_LOCAL,
    GIZMO_SPACE_VIEW,
    GIZMO_SPACE_COUNT
} GizmoSpace;

// Gizmo axis selection
typedef enum {
    GIZMO_AXIS_NONE,
    GIZMO_AXIS_X,
    GIZMO_AXIS_Y,
    GIZMO_AXIS_Z,
    GIZMO_AXIS_XY,
    GIZMO_AXIS_YZ,
    GIZMO_AXIS_ZX,
    GIZMO_AXIS_SCREEN,
    GIZMO_AXIS_CENTER,
    GIZMO_AXIS_COUNT
} GizmoAxis;

// Pivot point modes
typedef enum {
    GIZMO_PIVOT_CENTER,
    GIZMO_PIVOT_PIVOT,
    GIZMO_PIVOT_COUNT
} GizmoPivot;

// Gizmo configuration
typedef struct {
    GizmoType type;
    GizmoSpace space;
    GizmoPivot pivot;
    f32 size;                    // Gizmo visual size
    f32 snap_translate;          // Translation snap units (0 = disabled)
    f32 snap_rotate;             // Rotation snap degrees (0 = disabled)
    f32 snap_scale;              // Scale snap units (0 = disabled)
    bool enable_snapping;
    Vec3 position;               // Gizmo world position
    Mat4 transform;              // Object transform for local space
} GizmoConfig;

// Gizmo interaction state
typedef struct {
    bool is_hovering;
    bool is_dragging;
    GizmoAxis hovered_axis;
    GizmoAxis selected_axis;
    Vec3 drag_start_position;
    Vec3 drag_start_mouse;
    Vec3 drag_delta;
    f32 drag_start_angle;
    f32 drag_angle_delta;
    Vec3 drag_start_scale;
    Vec3 drag_scale_delta;
} GizmoInteraction;

// Gizmo system
typedef struct {
    GizmoConfig config;
    GizmoInteraction interaction;
    bool enabled;
    bool visible;
    u32 selection_count;
    Vec3 selection_center;
} GizmoSystem;

// Public API
GizmoSystem* gizmo_system_create(void);
void gizmo_system_destroy(GizmoSystem *gizmo);

// Configuration
void gizmo_set_type(GizmoSystem *gizmo, GizmoType type);
void gizmo_set_space(GizmoSystem *gizmo, GizmoSpace space);
void gizmo_set_pivot(GizmoSystem *gizmo, GizmoPivot pivot);
void gizmo_set_position(GizmoSystem *gizmo, Vec3 position);
void gizmo_set_transform(GizmoSystem *gizmo, Mat4 transform);
void gizmo_set_selection(GizmoSystem *gizmo, const Vec3 *positions, u32 count);

// Interaction
bool gizmo_handle_mouse_down(GizmoSystem *gizmo, Vec2 mouse_pos, Vec3 ray_origin, Vec3 ray_dir);
bool gizmo_handle_mouse_move(GizmoSystem *gizmo, Vec2 mouse_pos, Vec3 ray_origin, Vec3 ray_dir);
bool gizmo_handle_mouse_up(GizmoSystem *gizmo);
GizmoAxis gizmo_get_hovered_axis(const GizmoSystem *gizmo);

// Transform retrieval
Vec3 gizmo_get_translation_delta(const GizmoSystem *gizmo);
f32 gizmo_get_rotation_delta(const GizmoSystem *gizmo);
Vec3 gizmo_get_scale_delta(const GizmoSystem *gizmo);

// Rendering
void gizmo_render(const GizmoSystem *gizmo, IRenderer *renderer, const Camera *camera);

// Utility
bool gizmo_is_enabled(const GizmoSystem *gizmo);
void gizmo_set_enabled(GizmoSystem *gizmo, bool enabled);
bool gizmo_is_visible(const GizmoSystem *gizmo);
void gizmo_set_visible(GizmoSystem *gizmo, bool visible);

#endif // EDITOR_VIEWPORT_GIZMOS_H
