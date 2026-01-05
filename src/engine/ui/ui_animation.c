#include <math/vec2.h>
#include <stdlib.h>
#include <ui/ui_system.h>

typedef enum {
  ANIM_TWEEN_LINEAR,
  ANIM_TWEEN_EASE_IN,
  ANIM_TWEEN_EASE_OUT,
  ANIM_TWEEN_EASE_IN_OUT
} UIAnimationCurve;

typedef struct UIAnimation {
  UIElement *target;
  f32 duration;
  f32 current_time;
  UIAnimationCurve curve;
  bool is_playing;

  // Properties to animate (mask)
  bool anim_pos;
  bool anim_opacity;

  Vec2 start_pos, end_pos;
  f32 start_opacity, end_opacity;
} UIAnimation;

static inline f32 interpolate(f32 t, UIAnimationCurve curve) {
  switch (curve) {
  case ANIM_TWEEN_LINEAR:
    return t;
  case ANIM_TWEEN_EASE_IN:
    return t * t;
  case ANIM_TWEEN_EASE_OUT:
    return t * (2 - t);
  case ANIM_TWEEN_EASE_IN_OUT:
    return t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t;
  default:
    return t;
  }
}

void ui_update_animations(f32 dt) {
  // In a real system we'd manage a list of active animations
}

void ui_animate_position(UIElement *element, Vec2 target_pos, f32 duration) {
  // Create and register animation
}
