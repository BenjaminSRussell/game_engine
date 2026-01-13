#ifndef UI_ANIMATION_H
#define UI_ANIMATION_H

#include "include/math/math.h"
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
    UI_EASE_IN_OUT_BOUNCE
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
    UI_ANIM_BORDER_COLOR
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

typedef struct Widget Widget;
typedef struct UIAnimation UIAnimation;
typedef void (*UIAnimationCallback)(UIAnimation* anim, void* user_data);

struct UIAnimation {
    Widget* target;
    UIAnimationProperty property;
    UIEasingType easing;

    // Storage for start/end values
    union {
        struct { float start; float end; } f;
        struct { Vec2 start; Vec2 end; } v2;
        struct { Vec4 start; Vec4 end; } v4;
    } values;

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

    struct UIAnimation* next_in_sequence; // Next animation to play automatically
    UIAnimation* next; // Linked list (active animations)
};

// --- System ---
void ui_animation_init(void);
void ui_animation_shutdown(void);
void ui_animation_update(float delta_time);

// --- Creation & Management ---
UIAnimation* ui_animation_create(Widget* target, UIAnimationProperty property, float duration);
void ui_animation_destroy(UIAnimation* anim);
void ui_animation_destroy_all(void);

// Chains 'next' to play after 'first' finishes.
// 'next' should NOT be playing and should not be in the active list yet.
void ui_animation_chain(UIAnimation* first, UIAnimation* next);

// --- Configuration ---
void ui_animation_set_float(UIAnimation* anim, float start, float end);
void ui_animation_set_vec2(UIAnimation* anim, Vec2 start, Vec2 end);
void ui_animation_set_vec4(UIAnimation* anim, Vec4 start, Vec4 end);
void ui_animation_set_easing(UIAnimation* anim, UIEasingType easing);
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
