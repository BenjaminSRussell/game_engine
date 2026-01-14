#ifndef CURVE_EDITOR_H
#define CURVE_EDITOR_H

#include "core/types.h"
#include "math/vec2.h"

#ifdef __cplusplus
extern "C" {
#endif

// Curve interpolation types
typedef enum {
    CURVE_INTERP_LINEAR,
    CURVE_INTERP_SMOOTH,
    CURVE_INTERP_BEZIER,
    CURVE_INTERP_STEP
} CurveInterpolation;

// Keyframe for curve
typedef struct {
    float time;      // X-axis (0-1 for particle lifetime)
    float value;     // Y-axis (property value)
    float in_tangent;  // For Bezier curves
    float out_tangent; // For Bezier curves
} CurveKeyframe;

// Animation curve
typedef struct {
    char name[64];
    CurveKeyframe* keyframes;
    u32 keyframe_count;
    u32 keyframe_capacity;
    CurveInterpolation interpolation;
    
    // Value range (for UI display)
    float min_value;
    float max_value;
} AnimationCurve;

// Curve editor state
typedef struct {
    AnimationCurve* curves;
    u32 curve_count;
    u32 curve_capacity;
    
    // Selection
    i32 selected_curve;
    i32 selected_keyframe;
    
    // Viewport
    Vec2 view_min;
    Vec2 view_max;
    float zoom;
    
    // Interaction
    bool is_dragging_keyframe;
    bool is_panning;
    Vec2 drag_start;
} CurveEditor;

// Core API
void curve_editor_init(CurveEditor* editor);
void curve_editor_shutdown(CurveEditor* editor);

// Curve management
AnimationCurve* curve_editor_add_curve(CurveEditor* editor, const char* name);
void curve_editor_remove_curve(CurveEditor* editor, u32 curve_index);
AnimationCurve* curve_editor_get_curve(CurveEditor* editor, u32 curve_index);

// Keyframe editing
void curve_add_keyframe(AnimationCurve* curve, float time, float value);
void curve_remove_keyframe(AnimationCurve* curve, u32 keyframe_index);
void curve_update_keyframe(AnimationCurve* curve, u32 keyframe_index, float time, float value);
void curve_set_tangents(AnimationCurve* curve, u32 keyframe_index, float in_tangent, float out_tangent);

// Evaluation
float curve_evaluate(const AnimationCurve* curve, float time);

// Selection
void curve_editor_select_curve(CurveEditor* editor, u32 curve_index);
void curve_editor_select_keyframe(CurveEditor* editor, u32 curve_index, u32 keyframe_index);
void curve_editor_deselect(CurveEditor* editor);

// View manipulation
void curve_editor_zoom(CurveEditor* editor, float delta);
void curve_editor_pan(CurveEditor* editor, Vec2 delta);
void curve_editor_frame_all(CurveEditor* editor);

// Utilities
void curve_normalize_time(AnimationCurve* curve);
void curve_smooth_tangents(AnimationCurve* curve, u32 keyframe_index);
void curve_auto_tangents(AnimationCurve* curve);

#ifdef __cplusplus
}
#endif

#endif // CURVE_EDITOR_H
