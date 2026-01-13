#ifndef UI_ANIMATION_H
#define UI_ANIMATION_H

#include <stdbool.h>
#include <stdint.h>
#include "../core/math.h"

// Animation error codes
typedef enum {
    UI_ANIM_SUCCESS = 0,
    UI_ANIM_ERROR_INIT_FAILED,
    UI_ANIM_ERROR_OUT_OF_MEMORY,
    UI_ANIM_ERROR_INVALID_PARAMETER,
    UI_ANIM_ERROR_CAPACITY_EXCEEDED
} UIAnimationError;

// Easing types for smooth animations
typedef enum {
    EASING_LINEAR = 0,
    EASING_IN_QUAD, EASING_OUT_QUAD, EASING_IN_OUT_QUAD,
    EASING_IN_CUBIC, EASING_OUT_CUBIC, EASING_IN_OUT_CUBIC,
    EASING_IN_SINE, EASING_OUT_SINE, EASING_IN_OUT_SINE,
    EASING_IN_EXPO, EASING_OUT_EXPO, EASING_IN_OUT_EXPO,
    EASING_IN_BACK, EASING_OUT_BACK, EASING_IN_OUT_BACK,
    EASING_IN_BOUNCE, EASING_OUT_BOUNCE, EASING_IN_OUT_BOUNCE,
    EASING_IN_ELASTIC, EASING_OUT_ELASTIC, EASING_IN_OUT_ELASTIC,
    EASING_TYPE_COUNT
} EasingType;

// Forward declarations
typedef struct UITween UITween;
typedef struct UIKeyframe UIKeyframe;
typedef struct UIKeyframeAnimation UIKeyframeAnimation;

// Easing function pointer
typedef float (*EasingFunction)(float t);

// Tween callback functions
typedef void (*TweenUpdateCallback)(UITween *tween, float progress);
typedef void (*TweenCompleteCallback)(UITween *tween);

// Keyframe animation callback functions
typedef void (*KeyframeCompleteCallback)(UIKeyframeAnimation *animation);

// Tween structure for simple property animations
struct UITween {
    // Timing
    float duration;
    float current_time;
    bool is_active;
    EasingFunction easing_func;
    
    // Target properties
    vec2 *target_position;
    vec2 start_position;
    vec2 end_position;
    
    vec2 *target_scale;
    vec2 start_scale;
    vec2 end_scale;
    
    float *target_rotation;
    float start_rotation;
    float end_rotation;
    
    vec4 *target_color;
    vec4 start_color;
    vec4 end_color;
    
    float *target_alpha;
    float start_alpha;
    float end_alpha;
    
    // Callbacks
    TweenUpdateCallback on_update;
    TweenCompleteCallback on_complete;
};

// Keyframe structure for complex animations
struct UIKeyframe {
    float time;
    vec2 position;
    vec2 scale;
    float rotation;
    vec4 color;
    float alpha;
    EasingType easing_type;
};

// Keyframe animation structure
struct UIKeyframeAnimation {
    // Timing
    float duration;
    float current_time;
    bool is_active;
    bool loop;
    
    // Keyframes
    UIKeyframe *keyframes;
    uint32_t keyframe_count;
    uint32_t keyframe_capacity;
    
    // Target properties
    vec2 *target_position;
    vec2 *target_scale;
    float *target_rotation;
    vec4 *target_color;
    float *target_alpha;
    
    // Callback
    KeyframeCompleteCallback on_complete;
};

// Animation statistics
typedef struct {
    uint32_t active_tweens;
    uint32_t active_keyframe_anims;
    uint32_t animations_updated;
    uint32_t tweens_completed;
    uint32_t keyframes_processed;
    float global_time_scale;
    bool is_paused;
} UIAnimationStats;

// Core animation system functions
int ui_animation_init(void);
void ui_animation_shutdown(void);
void ui_animation_update(float delta_time);

// Tween creation and management
UITween* ui_animation_create_tween(float duration, EasingType easing_type);

// Keyframe animation creation and management
UIKeyframeAnimation* ui_animation_create_keyframe_animation(float duration, bool loop);

// Global animation control
void ui_animation_set_global_time_scale(float scale);
void ui_animation_pause(void);
void ui_animation_resume(void);

// Statistics and debugging
UIAnimationStats ui_animation_get_stats(void);

// Easing function access
EasingFunction ui_get_easing_function(EasingType type);

#endif // UI_ANIMATION_H
