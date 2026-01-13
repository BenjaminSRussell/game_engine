/*
 * ui_animation_binding.h
 * UI Property Animation Binding System
 * Binds UI element properties to animation curves
 *
 * Part of UI Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_ANIMATION_BINDING_H
#define UI_ANIMATION_BINDING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "math/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================
 */

/* Property types that can be animated */
typedef enum ui_animation_property_type {
    UI_ANIM_PROPERTY_FLOAT,
    UI_ANIM_PROPERTY_VEC2,
    UI_ANIM_PROPERTY_VEC3,
    UI_ANIM_PROPERTY_VEC4,
    UI_ANIM_PROPERTY_COLOR,
    UI_ANIM_PROPERTY_RECT,
    UI_ANIM_PROPERTY_ROTATION,
    UI_ANIM_PROPERTY_SCALE,
    UI_ANIM_PROPERTY_OPACITY
} ui_animation_property_type_t;

/* Animation binding handle */
typedef struct ui_animation_binding_handle {
    uint32_t id;
} ui_animation_binding_handle_t;

/* Property binding descriptor */
typedef struct ui_animation_binding_desc {
    /* Target element */
    uint32_t element_id;
    
    /* Property information */
    ui_animation_property_type_t property_type;
    const char *property_name;
    
    /* Animation parameters */
    float duration;
    float delay;
    bool loop;
    bool ping_pong;
    
    /* Easing function */
    float (*easing_func)(float t);
    
    /* Value ranges */
    Vec2 float_range;      // For float properties
    Vec2 vec2_range[2];    // For vec2 properties (min, max for each component)
    Vec3 vec3_range[2];    // For vec3 properties
    Vec4 vec4_range[2];    // For vec4/color properties
    
    /* Callbacks */
    void (*on_start)(uint32_t element_id);
    void (*on_complete)(uint32_t element_id);
    void (*on_update)(uint32_t element_id, float progress);
    
    /* User data */
    void *user_data;
} ui_animation_binding_desc_t;

/* Animation binding state */
typedef struct ui_animation_binding_state {
    uint32_t id;
    uint32_t element_id;
    ui_animation_property_type_t property_type;
    char property_name[64];
    
    /* Timing */
    float duration;
    float delay;
    float current_time;
    float progress;
    
    /* State */
    bool active;
    bool paused;
    bool loop;
    bool ping_pong;
    bool reverse;
    
    /* Values */
    Vec4 start_value;
    Vec4 end_value;
    Vec4 current_value;
    
    /* Easing */
    float (*easing_func)(float t);
    
    /* Callbacks */
    void (*on_start)(uint32_t element_id);
    void (*on_complete)(uint32_t element_id);
    void (*on_update)(uint32_t element_id, float progress);
    
    /* User data */
    void *user_data;
    
    /* Next in linked list */
    struct ui_animation_binding_state *next;
} ui_animation_binding_state_t;

/* Property getter/setter function types */
typedef void (*ui_property_setter_t)(uint32_t element_id, const void *value);
typedef void (*ui_property_getter_t)(uint32_t element_id, void *value);

/* Property registration */
typedef struct ui_animation_property_registry {
    ui_animation_property_type_t type;
    const char *name;
    ui_property_setter_t setter;
    ui_property_getter_t getter;
    size_t value_size;
} ui_animation_property_registry_t;

/* ============================================================================
 * API
 * ============================================================================
 */

/* Initialization */
int ui_animation_binding_init(void);
void ui_animation_binding_shutdown(void);

/* Property Registration */
int ui_animation_binding_register_property(
    ui_animation_property_type_t type,
    const char *name,
    ui_property_setter_t setter,
    ui_property_getter_t getter,
    size_t value_size
);

/* Animation Binding Lifecycle */
int ui_animation_binding_create(
    ui_animation_binding_handle_t *out_handle,
    const ui_animation_binding_desc_t *desc
);
void ui_animation_binding_destroy(ui_animation_binding_handle_t handle);

/* Animation Control */
void ui_animation_binding_play(ui_animation_binding_handle_t handle);
void ui_animation_binding_pause(ui_animation_binding_handle_t handle);
void ui_animation_binding_stop(ui_animation_binding_handle_t handle);
void ui_animation_binding_restart(ui_animation_binding_handle_t handle);

/* Animation Update */
void ui_animation_binding_update(float delta_time);

/* Property Access */
int ui_animation_binding_set_property(
    uint32_t element_id,
    const char *property_name,
    const void *value
);
int ui_animation_binding_get_property(
    uint32_t element_id,
    const char *property_name,
    void *value
);

/* State Queries */
bool ui_animation_binding_is_active(ui_animation_binding_handle_t handle);
bool ui_animation_binding_is_paused(ui_animation_binding_handle_t handle);
float ui_animation_binding_get_progress(ui_animation_binding_handle_t handle);
float ui_animation_binding_get_time(ui_animation_binding_handle_t handle);

/* Batch Operations */
void ui_animation_binding_pause_all(void);
void ui_animation_binding_resume_all(void);
void ui_animation_binding_stop_all(void);

/* Statistics */
uint32_t ui_animation_binding_get_active_count(void);
uint32_t ui_animation_binding_get_total_count(void);

/* ============================================================================
 * EASING FUNCTIONS
 * ============================================================================
 */

/* Standard easing functions */
float ui_easing_linear(float t);
float ui_easing_ease_in_quad(float t);
float ui_easing_ease_out_quad(float t);
float ui_easing_ease_in_out_quad(float t);
float ui_easing_ease_in_cubic(float t);
float ui_easing_ease_out_cubic(float t);
float ui_easing_ease_in_out_cubic(float t);
float ui_easing_ease_in_quart(float t);
float ui_easing_ease_out_quart(float t);
float ui_easing_ease_in_out_quart(float t);
float ui_easing_ease_in_back(float t);
float ui_easing_ease_out_back(float t);
float ui_easing_ease_in_out_back(float t);
float ui_easing_ease_in_elastic(float t);
float ui_easing_ease_out_elastic(float t);
float ui_easing_ease_in_out_elastic(float t);
float ui_easing_ease_in_bounce(float t);
float ui_easing_ease_out_bounce(float t);
float ui_easing_ease_in_out_bounce(float t);

#ifdef __cplusplus
}
#endif

#endif /* UI_ANIMATION_BINDING_H */
