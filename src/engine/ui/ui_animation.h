#ifndef UI_ANIMATION_H
#define UI_ANIMATION_H

#include "include/math/vec2.h"
#include "include/math/vec4.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Easing types
typedef enum {
    UI_EASE_LINEAR,
    UI_EASE_IN_QUAD,
    UI_EASE_OUT_QUAD,
    UI_EASE_IN_OUT_QUAD,
    UI_EASE_IN_CUBIC,
    UI_EASE_OUT_CUBIC,
    UI_EASE_IN_OUT_CUBIC,
    UI_EASE_IN_ELASTIC,
    UI_EASE_OUT_ELASTIC,
    UI_EASE_IN_OUT_ELASTIC,
    UI_EASE_IN_BOUNCE,
    UI_EASE_OUT_BOUNCE,
    UI_EASE_IN_OUT_BOUNCE,
    UI_EASE_CUSTOM_BEZIER // New custom bezier easing
} UIEasingType;

// Properties that can be animated
typedef enum {
    UI_ANIM_OPACITY,
    UI_ANIM_POSITION_X,
    UI_ANIM_POSITION_Y,
    UI_ANIM_POSITION,
    UI_ANIM_WIDTH,
    UI_ANIM_HEIGHT,
    UI_ANIM_SIZE,
    UI_ANIM_BACKGROUND_COLOR,
    UI_ANIM_TEXT_COLOR,
    UI_ANIM_BORDER_COLOR,
    UI_ANIM_CUSTOM_FLOAT,   // Custom property binding (float)
    UI_ANIM_CUSTOM_VEC2,    // Custom property binding (Vec2)
    UI_ANIM_CUSTOM_VEC4     // Custom property binding (Vec4)
} UIAnimationProperty;

// Transition types for screens
typedef enum {
    UI_TRANSITION_FADE,
    UI_TRANSITION_SLIDE_LEFT,
    UI_TRANSITION_SLIDE_RIGHT,
    UI_TRANSITION_SLIDE_UP,
    UI_TRANSITION_SLIDE_DOWN,
    UI_TRANSITION_ZOOM,
    UI_TRANSITION_NONE
} UITransitionType;

// Animation modes
typedef enum {
    UI_ANIM_MODE_TWEEN,     // Standard start -> end tween
    UI_ANIM_MODE_KEYFRAME   // Keyframe animation
} UIAnimationMode;

typedef struct Widget Widget;
typedef struct UIAnimation UIAnimation;
typedef void (*UIAnimationCallback)(UIAnimation* anim, void* user_data);

// Keyframe structure
typedef struct {
    float time; // Normalized 0.0 to 1.0
    union {
        float f;
        Vec2 v2;
        Vec4 v4;
    } value;
    UIEasingType easing; // Easing to get TO this keyframe
} UIKeyframe;

struct UIAnimation {
    Widget* target;
    UIAnimationProperty property;
    UIAnimationMode mode;

    // Custom binding target (if property is CUSTOM_*)
    void* custom_target;

    // Easing for TWEEN mode
    UIEasingType easing;
    // Custom Bezier control points for UI_EASE_CUSTOM_BEZIER
    // p0=(0,0), p3=(1,1) implied. These are p1 and p2.
    float bezier_p1_x, bezier_p1_y;
    float bezier_p2_x, bezier_p2_y;

    // Storage for values
    union {
        // TWEEN mode
        struct {
            union {
                struct { float start; float end; } f;
                struct { Vec2 start; Vec2 end; } v2;
                struct { Vec4 start; Vec4 end; } v4;
            } tween;
        };

        // KEYFRAME mode
        struct {
            UIKeyframe* frames;
            uint32_t count;
            uint32_t capacity;
        } keyframes;
    } data;

    float duration;
    float elapsed;
    float delay;

    bool is_playing;
    bool is_finished;
    bool loop;
    bool ping_pong; // If true, reverses direction on loop
    bool reverse;   // Internal state for ping-pong
    bool auto_destroy; // If true, automatically destroyed when finished

    UIAnimationCallback on_complete;
    void* user_data;

    UIAnimation* next; // Linked list
};

// --- System ---
void ui_animation_init(void);
void ui_animation_shutdown(void);
void ui_animation_update(float delta_time);

// --- Creation & Management ---
UIAnimation* ui_animation_create(Widget* target, UIAnimationProperty property, float duration);
void ui_animation_destroy(UIAnimation* anim);
void ui_animation_destroy_all(void);

// --- Configuration ---
// Tween setters
void ui_animation_set_float(UIAnimation* anim, float start, float end);
void ui_animation_set_vec2(UIAnimation* anim, Vec2 start, Vec2 end);
void ui_animation_set_vec4(UIAnimation* anim, Vec4 start, Vec4 end);
void ui_animation_set_easing(UIAnimation* anim, UIEasingType easing);

// Custom Binding
void ui_animation_bind_float(UIAnimation* anim, float* target);
void ui_animation_bind_vec2(UIAnimation* anim, Vec2* target);
void ui_animation_bind_vec4(UIAnimation* anim, Vec4* target);

// Bezier
void ui_animation_set_bezier_curve(UIAnimation* anim, float p1x, float p1y, float p2x, float p2y);

// Keyframes
void ui_animation_add_keyframe_float(UIAnimation* anim, float time, float value, UIEasingType easing);
void ui_animation_add_keyframe_vec2(UIAnimation* anim, float time, Vec2 value, UIEasingType easing);
void ui_animation_add_keyframe_vec4(UIAnimation* anim, float time, Vec4 value, UIEasingType easing);

void ui_animation_set_delay(UIAnimation* anim, float delay);
void ui_animation_set_loop(UIAnimation* anim, bool loop, bool ping_pong);
void ui_animation_set_callback(UIAnimation* anim, UIAnimationCallback callback, void* user_data);

// --- Control ---
void ui_animation_play(UIAnimation* anim);
void ui_animation_stop(UIAnimation* anim); // Stops and resets
void ui_animation_pause(UIAnimation* anim);
void ui_animation_resume(UIAnimation* anim);
void ui_animation_cancel(UIAnimation* anim); // Stops and destroys

// --- Helpers ---
// Create and play an opacity fade animation
UIAnimation* ui_animate_fade(Widget* widget, float target_opacity, float duration, UIEasingType easing);
// Create and play a move animation
UIAnimation* ui_animate_move(Widget* widget, Vec2 target_pos, float duration, UIEasingType easing);
// Create and play a resize animation
UIAnimation* ui_animate_resize(Widget* widget, Vec2 target_size, float duration, UIEasingType easing);

// --- Screen Transitions ---
// Animate transition between two screen widgets
void ui_transition_screen(Widget* current_screen, Widget* next_screen, UITransitionType type, float duration);

#ifdef __cplusplus
}
#endif

#endif // UI_ANIMATION_H
