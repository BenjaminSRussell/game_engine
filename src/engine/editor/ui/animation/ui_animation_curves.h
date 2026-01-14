/*
 * ui_animation_curves.h
 * UI Animation Curves System
 * Keyframe-based animation curves with interpolation
 *
 * Part of UI Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_ANIMATION_CURVES_H
#define UI_ANIMATION_CURVES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "engine/include/math/math_all.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================
 */

/* Keyframe interpolation types */
typedef enum ui_curve_interpolation_type {
    UI_CURVE_INTERP_LINEAR,
    UI_CURVE_INTERP_STEP,
    UI_CURVE_INTERP_CUBIC_SPLINE,
    UI_CURVE_INTERP_BEZIER,
    UI_CURVE_INTERP_EASE_IN,
    UI_CURVE_INTERP_EASE_OUT,
    UI_CURVE_INTERP_EASE_IN_OUT,
    UI_CURVE_INTERP_EASE_CUSTOM
} ui_curve_interpolation_type_t;

/* Tangent modes for cubic spline interpolation */
typedef enum ui_curve_tangent_mode {
    UI_CURVE_TANGENT_AUTO,
    UI_CURVE_TANGENT_LINEAR,
    UI_CURVE_TANGENT_CONSTANT,
    UI_CURVE_TANGENT_CUSTOM
} ui_curve_tangent_mode_t;

/* Animation curve handle */
typedef struct ui_animation_curve_handle {
    uint32_t id;
} ui_animation_curve_handle_t;

/* Keyframe data */
typedef struct ui_curve_keyframe {
    float time;
    float value;
    float in_tangent;
    float out_tangent;
    ui_curve_tangent_mode_t tangent_mode;
    ui_curve_interpolation_type_t interpolation;
} ui_curve_keyframe_t;

/* Bezier control points */
typedef struct ui_curve_bezier_control {
    Vec2 in_control;
    Vec2 out_control;
} ui_curve_bezier_control_t;

/* Animation curve descriptor */
typedef struct ui_animation_curve_desc {
    /* Curve properties */
    bool loop;
    bool pre_infinity;
    bool post_infinity;
    float pre_infinity_time;
    float post_infinity_time;
    
    /* Default interpolation for new keyframes */
    ui_curve_interpolation_type_t default_interpolation;
    ui_curve_tangent_mode_t default_tangent_mode;
    
    /* Keyframe data */
    uint32_t keyframe_count;
    const ui_curve_keyframe_t *keyframes;
    
    /* Bezier controls (for BEZIER interpolation) */
    const ui_curve_bezier_control_t *bezier_controls;
    
    /* User data */
    void *user_data;
} ui_animation_curve_desc_t;

/* Animation curve state */
typedef struct ui_animation_curve_state {
    uint32_t id;
    
    /* Curve properties */
    bool loop;
    bool pre_infinity;
    bool post_infinity;
    float pre_infinity_time;
    float post_infinity_time;
    
    /* Keyframes */
    ui_curve_keyframe_t *keyframes;
    ui_curve_bezier_control_t *bezier_controls;
    uint32_t keyframe_count;
    uint32_t keyframe_capacity;
    
    /* Default settings */
    ui_curve_interpolation_type_t default_interpolation;
    ui_curve_tangent_mode_t default_tangent_mode;
    
    /* Cached data for optimization */
    bool dirty;
    float *cached_values;
    uint32_t cache_resolution;
    
    /* User data */
    void *user_data;
    
    /* Next in linked list */
    struct ui_animation_curve_state *next;
} ui_animation_curve_state_t;

/* Curve evaluation result */
typedef struct ui_curve_evaluation_result {
    float value;
    float derivative;
    bool valid;
} ui_curve_evaluation_result_t;

/* ============================================================================
 * API
 * ============================================================================
 */

/* Initialization */
int ui_animation_curves_init(void);
void ui_animation_curves_shutdown(void);

/* Curve Creation and Management */
int ui_animation_curve_create(
    ui_animation_curve_handle_t *out_handle,
    const ui_animation_curve_desc_t *desc
);
void ui_animation_curve_destroy(ui_animation_curve_handle_t handle);

/* Keyframe Operations */
int ui_animation_curve_add_keyframe(
    ui_animation_curve_handle_t curve,
    float time,
    float value
);
int ui_animation_curve_add_keyframe_with_tangents(
    ui_animation_curve_handle_t curve,
    float time,
    float value,
    float in_tangent,
    float out_tangent
);
int ui_animation_curve_remove_keyframe(
    ui_animation_curve_handle_t curve,
    uint32_t keyframe_index
);
int ui_animation_curve_move_keyframe(
    ui_animation_curve_handle_t curve,
    uint32_t keyframe_index,
    float new_time
);

/* Keyframe Property Access */
int ui_animation_curve_set_keyframe_value(
    ui_animation_curve_handle_t curve,
    uint32_t keyframe_index,
    float value
);
int ui_animation_curve_set_keyframe_tangents(
    ui_animation_curve_handle_t curve,
    uint32_t keyframe_index,
    float in_tangent,
    float out_tangent
);
int ui_animation_curve_set_keyframe_interpolation(
    ui_animation_curve_handle_t curve,
    uint32_t keyframe_index,
    ui_curve_interpolation_type_t interpolation
);

/* Curve Evaluation */
ui_curve_evaluation_result_t ui_animation_curve_evaluate(
    ui_animation_curve_handle_t curve,
    float time
);
float ui_animation_curve_evaluate_simple(
    ui_animation_curve_handle_t curve,
    float time
);

/* Curve Properties */
int ui_animation_curve_set_loop(ui_animation_curve_handle_t curve, bool loop);
int ui_animation_curve_set_infinity(
    ui_animation_curve_handle_t curve,
    bool pre_infinity,
    bool post_infinity,
    float pre_time,
    float post_time
);

/* Curve Optimization */
void ui_animation_curve_optimize(ui_animation_curve_handle_t curve);
void ui_animation_curve_bake(
    ui_animation_curve_handle_t curve,
    float start_time,
    float end_time,
    uint32_t resolution
);

/* Curve Queries */
uint32_t ui_animation_curve_get_keyframe_count(ui_animation_curve_handle_t curve);
ui_curve_keyframe_t ui_animation_curve_get_keyframe(
    ui_animation_curve_handle_t curve,
    uint32_t index
);
float ui_animation_curve_get_duration(ui_animation_curve_handle_t curve);
bool ui_animation_curve_is_valid(ui_animation_curve_handle_t curve);

/* Batch Operations */
void ui_animation_curves_optimize_all(void);
void ui_animation_curves_bake_all(
    float start_time,
    float end_time,
    uint32_t resolution
);

/* Statistics */
uint32_t ui_animation_curves_get_total_count(void);
uint32_t ui_animation_curves_get_total_keyframes(void);

/* ============================================================================
 * CURVE PRESETS
 * ============================================================================
 */

/* Common animation curves */
ui_animation_curve_handle_t ui_animation_curve_create_linear(void);
ui_animation_curve_handle_t ui_animation_curve_create_ease_in(void);
ui_animation_curve_handle_t ui_animation_curve_create_ease_out(void);
ui_animation_curve_handle_t ui_animation_curve_create_ease_in_out(void);
ui_animation_curve_handle_t ui_animation_curve_create_bounce(void);
ui_animation_curve_handle_t ui_animation_curve_create_elastic(void);
ui_animation_curve_handle_t ui_animation_curve_create_sine(void);
ui_animation_curve_handle_t ui_animation_curve_create_sawtooth(void);
ui_animation_curve_handle_t ui_animation_curve_create_square(void);

/* Custom curve creation from function */
ui_animation_curve_handle_t ui_animation_curve_from_function(
    float (*func)(float t),
    float start_time,
    float end_time,
    uint32_t resolution
);

#ifdef __cplusplus
}
#endif

#endif /* UI_ANIMATION_CURVES_H */
