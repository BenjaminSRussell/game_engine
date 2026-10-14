#include "ui_animation.h"
#include "widgets/widget.h"
#include "core/memory/unified_memory_allocator.h"
#include "core/logger.h"
#include <math.h>
#include <string.h>

static UIAnimation* g_active_animations = NULL;

// Easing function implementations
static float ease_linear(float t) { return t; }

static float ease_in_quad(float t) { return t * t; }

static float ease_out_quad(float t) { return t * (2 - t); }

static float ease_in_out_quad(float t) {
    return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t;
}

static float ease_in_cubic(float t) { return t * t * t; }

static float ease_out_cubic(float t) {
    float f = t - 1;
    return f * f * f + 1;
}

static float ease_in_out_cubic(float t) {
    return t < 0.5f ? 4 * t * t * t : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1;
}

static float ease_in_elastic(float t) {
    float c4 = (2 * M_PI) / 3;
    return t == 0 ? 0 : t == 1 ? 1 : -powf(2, 10 * t - 10) * sinf((t * 10 - 10.75f) * c4);
}

static float ease_out_elastic(float t) {
    float c4 = (2 * M_PI) / 3;
    return t == 0 ? 0 : t == 1 ? 1 : powf(2, -10 * t) * sinf((t * 10 - 0.75f) * c4) + 1;
}

static float ease_in_out_elastic(float t) {
    float c5 = (2 * M_PI) / 4.5f;
    return t == 0 ? 0 : t == 1 ? 1 : t < 0.5f
        ? -(powf(2, 20 * t - 10) * sinf((20 * t - 11.125f) * c5)) / 2
        : (powf(2, -20 * t + 10) * sinf((20 * t - 11.125f) * c5)) / 2 + 1;
}

static float ease_out_bounce(float t) {
    float n1 = 7.5625f;
    float d1 = 2.75f;
    if (t < 1 / d1) {
        return n1 * t * t;
    } else if (t < 2 / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    } else if (t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    } else {
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}

static float ease_in_bounce(float t) {
    return 1 - ease_out_bounce(1 - t);
}

static float ease_in_out_bounce(float t) {
    return t < 0.5f ? (1 - ease_out_bounce(1 - 2 * t)) / 2
                    : (1 + ease_out_bounce(2 * t - 1)) / 2;
}

static float cubic_bezier_component(float t, float p0, float p1, float p2, float p3) {
    float u = 1.0f - t;
    return u * u * u * p0 + 3.0f * u * u * t * p1 + 3.0f * u * t * t * p2 + t * t * t * p3;
}

static float solve_cubic_bezier_x(float x, float x1, float x2) {
    float t = x;
    for (int i = 0; i < 8; i++) {
        float x_est = cubic_bezier_component(t, 0.0f, x1, x2, 1.0f);
        float slope = (3.0f * (1.0f - t) * (1.0f - t) * x1)
            + (6.0f * (1.0f - t) * t * (x2 - x1))
            + (3.0f * t * t * (1.0f - x2));
        if (fabsf(slope) < 1e-6f) break;
        t -= (x_est - x) / slope;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
    }
    return t;
}

static float evaluate_cubic_bezier(float time, float x1, float y1, float x2, float y2) {
    float t = solve_cubic_bezier_x(time, x1, x2);
    return cubic_bezier_component(t, 0.0f, y1, y2, 1.0f);
}

static float apply_easing(float t, UIEasingType type, const UIAnimation* anim) {
    switch (type) {
        case UI_EASE_LINEAR: return ease_linear(t);
        case UI_EASE_IN_QUAD: return ease_in_quad(t);
        case UI_EASE_OUT_QUAD: return ease_out_quad(t);
        case UI_EASE_IN_OUT_QUAD: return ease_in_out_quad(t);
        case UI_EASE_IN_CUBIC: return ease_in_cubic(t);
        case UI_EASE_OUT_CUBIC: return ease_out_cubic(t);
        case UI_EASE_IN_OUT_CUBIC: return ease_in_out_cubic(t);
        case UI_EASE_IN_ELASTIC: return ease_in_elastic(t);
        case UI_EASE_OUT_ELASTIC: return ease_out_elastic(t);
        case UI_EASE_IN_OUT_ELASTIC: return ease_in_out_elastic(t);
        case UI_EASE_IN_BOUNCE: return ease_in_bounce(t);
        case UI_EASE_OUT_BOUNCE: return ease_out_bounce(t);
        case UI_EASE_IN_OUT_BOUNCE: return ease_in_out_bounce(t);
        case UI_EASE_CUSTOM_BEZIER:
            if (anim) {
                return evaluate_cubic_bezier(t, anim->bezier_p1_x, anim->bezier_p1_y,
                                             anim->bezier_p2_x, anim->bezier_p2_y);
            }
            return t;
        default: return t;
    }
}

// Interpolation helpers
static float ui_lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

static Vec2 ui_vec2_lerp_custom(Vec2 a, Vec2 b, float t) {
    Vec2 r;
    r.x = ui_lerp(a.x, b.x, t);
    r.y = ui_lerp(a.y, b.y, t);
    return r;
}

static Vec4 ui_vec4_lerp_custom(Vec4 a, Vec4 b, float t) {
    Vec4 r;
    r.x = ui_lerp(a.x, b.x, t);
    r.y = ui_lerp(a.y, b.y, t);
    r.z = ui_lerp(a.z, b.z, t);
    r.w = ui_lerp(a.w, b.w, t);
    return r;
}

void ui_animation_init(void) {
    g_active_animations = NULL;
}

void ui_animation_shutdown(void) {
    ui_animation_destroy_all();
}

UIAnimation* ui_animation_create(Widget* target, UIAnimationProperty property, float duration) {
    UIAnimation* anim = MALLOC(sizeof(UIAnimation));
    if (!anim) {
        LOG_ERROR_CAT(LOG_CAT_ANIMATION, "Failed to allocate UIAnimation");
        return NULL;
    }
    memset(anim, 0, sizeof(UIAnimation));

    anim->target = target;
    anim->property = property;
    anim->duration = duration;
    anim->easing = UI_EASE_LINEAR;
    anim->mode = UI_ANIM_MODE_TWEEN;
    anim->is_playing = false;
    anim->is_finished = false;
    anim->auto_destroy = false;

    // Auto-add to global list
    anim->next = g_active_animations;
    g_active_animations = anim;

    return anim;
}

void ui_animation_destroy(UIAnimation* anim) {
    if (!anim) return;

    if (anim->keyframes) {
        FREE(anim->keyframes);
        anim->keyframes = NULL;
    }

    // Remove from list
    if (g_active_animations == anim) {
        g_active_animations = anim->next;
    } else {
        UIAnimation* curr = g_active_animations;
        while (curr && curr->next != anim) {
            curr = curr->next;
        }
        if (curr) {
            curr->next = anim->next;
        }
    }

    memory_free(anim);
}

void ui_animation_destroy_all(void) {
    while (g_active_animations) {
        ui_animation_destroy(g_active_animations);
    }
}

void ui_animation_set_float(UIAnimation* anim, float start, float end) {
    if (anim) {
        anim->mode = UI_ANIM_MODE_TWEEN;
        anim->values.f.start = start;
        anim->values.f.end = end;
    }
}

void ui_animation_set_vec2(UIAnimation* anim, Vec2 start, Vec2 end) {
    if (anim) {
        anim->mode = UI_ANIM_MODE_TWEEN;
        anim->values.v2.start = start;
        anim->values.v2.end = end;
    }
}

void ui_animation_set_vec4(UIAnimation* anim, Vec4 start, Vec4 end) {
    if (anim) {
        anim->mode = UI_ANIM_MODE_TWEEN;
        anim->values.v4.start = start;
        anim->values.v4.end = end;
    }
}

void ui_animation_set_easing(UIAnimation* anim, UIEasingType easing) {
    if (anim) anim->easing = easing;
}

void ui_animation_set_bezier_curve(UIAnimation* anim, float p1x, float p1y, float p2x, float p2y) {
    if (!anim) return;
    anim->easing = UI_EASE_CUSTOM_BEZIER;
    anim->bezier_p1_x = p1x;
    anim->bezier_p1_y = p1y;
    anim->bezier_p2_x = p2x;
    anim->bezier_p2_y = p2y;
}

void ui_animation_bind_float(UIAnimation* anim, float* target) {
    if (!anim) return;
    anim->property = UI_ANIM_CUSTOM_FLOAT;
    anim->custom_target = target;
}

void ui_animation_bind_vec2(UIAnimation* anim, Vec2* target) {
    if (!anim) return;
    anim->property = UI_ANIM_CUSTOM_VEC2;
    anim->custom_target = target;
}

void ui_animation_bind_vec4(UIAnimation* anim, Vec4* target) {
    if (!anim) return;
    anim->property = UI_ANIM_CUSTOM_VEC4;
    anim->custom_target = target;
}

static bool ensure_keyframes_capacity(UIAnimation* anim, uint32_t needed) {
    if (anim->keyframe_capacity >= needed && (anim->keyframes || needed == 0)) {
        return true;
    }
    uint32_t new_cap = anim->keyframe_capacity == 0 ? 4 : anim->keyframe_capacity * 2;
    if (new_cap < needed) new_cap = needed;
    UIKeyframe* grown = (UIKeyframe*)REALLOC(anim->keyframes, new_cap * sizeof(UIKeyframe));
    if (!grown) {
        LOG_ERROR_CAT(LOG_CAT_ANIMATION, "Failed to resize keyframes buffer");
        return false;
    }
    anim->keyframes = grown;
    anim->keyframe_capacity = new_cap;
    return true;
}

static void insert_keyframe(UIAnimation* anim, UIKeyframe frame) {
    if (!ensure_keyframes_capacity(anim, anim->keyframe_count + 1)) return;
    uint32_t i = anim->keyframe_count;
    while (i > 0 && anim->keyframes[i - 1].time > frame.time) {
        anim->keyframes[i] = anim->keyframes[i - 1];
        i--;
    }
    anim->keyframes[i] = frame;
    anim->keyframe_count++;
    anim->mode = UI_ANIM_MODE_KEYFRAME;
}

void ui_animation_add_keyframe_float(UIAnimation* anim, float time, float value, UIEasingType easing) {
    if (!anim) return;
    UIKeyframe frame;
    memset(&frame, 0, sizeof(frame));
    frame.time = time;
    frame.value.f = value;
    frame.easing = easing;
    insert_keyframe(anim, frame);
}

void ui_animation_add_keyframe_vec2(UIAnimation* anim, float time, Vec2 value, UIEasingType easing) {
    if (!anim) return;
    UIKeyframe frame;
    memset(&frame, 0, sizeof(frame));
    frame.time = time;
    frame.value.v2 = value;
    frame.easing = easing;
    insert_keyframe(anim, frame);
}

void ui_animation_add_keyframe_vec4(UIAnimation* anim, float time, Vec4 value, UIEasingType easing) {
    if (!anim) return;
    UIKeyframe frame;
    memset(&frame, 0, sizeof(frame));
    frame.time = time;
    frame.value.v4 = value;
    frame.easing = easing;
    insert_keyframe(anim, frame);
}

void ui_animation_set_delay(UIAnimation* anim, float delay) {
    if (anim) anim->delay = delay;
}

void ui_animation_set_loop(UIAnimation* anim, bool loop, bool ping_pong) {
    if (anim) {
        anim->loop = loop;
        anim->ping_pong = ping_pong;
    }
}

void ui_animation_set_callback(UIAnimation* anim, UIAnimationCallback callback, void* user_data) {
    if (anim) {
        anim->on_complete = callback;
        anim->user_data = user_data;
    }
}

void ui_animation_play(UIAnimation* anim) {
    if (anim) {
        anim->is_playing = true;
        anim->is_finished = false;
        anim->elapsed = 0;
        anim->reverse = false;
    }
}

void ui_animation_stop(UIAnimation* anim) {
    if (anim) {
        anim->is_playing = false;
        anim->elapsed = 0;
        anim->reverse = false;
        // Optionally reset value to start?
        // For now, just stop updating.
    }
}

void ui_animation_pause(UIAnimation* anim) {
    if (anim) anim->is_playing = false;
}

void ui_animation_resume(UIAnimation* anim) {
    if (anim && !anim->is_finished) anim->is_playing = true;
}

void ui_animation_cancel(UIAnimation* anim) {
    ui_animation_destroy(anim);
}


static int property_rank(UIAnimationProperty property) {
    switch (property) {
        case UI_ANIM_POSITION:
        case UI_ANIM_SIZE:
        case UI_ANIM_CUSTOM_VEC2:
            return 2;
        case UI_ANIM_BACKGROUND_COLOR:
        case UI_ANIM_TEXT_COLOR:
        case UI_ANIM_BORDER_COLOR:
        case UI_ANIM_CUSTOM_VEC4:
            return 4;
        default:
            return 1;
    }
}

static void apply_float_value(UIAnimation* anim, float val) {
    if (anim->property == UI_ANIM_CUSTOM_FLOAT) {
        if (anim->custom_target) *(float*)anim->custom_target = val;
        return;
    }
    if (!anim->target) return;
    switch (anim->property) {
        case UI_ANIM_OPACITY:
            widget_set_opacity(anim->target, val);
            break;
        case UI_ANIM_POSITION_X: {
            Vec2 pos = widget_get_position(anim->target);
            pos.x = val;
            widget_set_position(anim->target, pos);
            break;
        }
        case UI_ANIM_POSITION_Y: {
            Vec2 pos = widget_get_position(anim->target);
            pos.y = val;
            widget_set_position(anim->target, pos);
            break;
        }
        case UI_ANIM_WIDTH: {
            Vec2 size = widget_get_size(anim->target);
            size.x = val;
            widget_set_size(anim->target, size);
            break;
        }
        case UI_ANIM_HEIGHT: {
            Vec2 size = widget_get_size(anim->target);
            size.y = val;
            widget_set_size(anim->target, size);
            break;
        }
        default:
            break;
    }
}

static void apply_vec2_value(UIAnimation* anim, Vec2 val) {
    if (anim->property == UI_ANIM_CUSTOM_VEC2) {
        if (anim->custom_target) *(Vec2*)anim->custom_target = val;
        return;
    }
    if (!anim->target) return;
    switch (anim->property) {
        case UI_ANIM_POSITION:
            widget_set_position(anim->target, val);
            break;
        case UI_ANIM_SIZE:
            widget_set_size(anim->target, val);
            break;
        default:
            break;
    }
}

static void apply_vec4_value(UIAnimation* anim, Vec4 val) {
    if (anim->property == UI_ANIM_CUSTOM_VEC4) {
        if (anim->custom_target) *(Vec4*)anim->custom_target = val;
        return;
    }
    if (!anim->target) return;
    switch (anim->property) {
        case UI_ANIM_BACKGROUND_COLOR:
            widget_set_background_color(anim->target, val);
            break;
        case UI_ANIM_TEXT_COLOR:
            widget_set_text_color(anim->target, val);
            break;
        case UI_ANIM_BORDER_COLOR:
            widget_set_border_color(anim->target, val);
            break;
        default:
            break;
    }
}

static void apply_ui_tween(UIAnimation* anim, float eased_t) {
    int rank = property_rank(anim->property);
    if (rank == 2) {
        apply_vec2_value(anim, ui_vec2_lerp_custom(anim->values.v2.start, anim->values.v2.end, eased_t));
    } else if (rank == 4) {
        apply_vec4_value(anim, ui_vec4_lerp_custom(anim->values.v4.start, anim->values.v4.end, eased_t));
    } else {
        apply_float_value(anim, ui_lerp(anim->values.f.start, anim->values.f.end, eased_t));
    }
}

static void apply_keyframe_sample(UIAnimation* anim, const UIKeyframe* sample) {
    int rank = property_rank(anim->property);
    if (rank == 2) apply_vec2_value(anim, sample->value.v2);
    else if (rank == 4) apply_vec4_value(anim, sample->value.v4);
    else apply_float_value(anim, sample->value.f);
}

static void apply_ui_keyframes(UIAnimation* anim, float final_t) {
    if (!anim->keyframes || anim->keyframe_count == 0) return;
    UIKeyframe* frames = anim->keyframes;
    uint32_t count = anim->keyframe_count;
    if (count == 1 || final_t <= frames[0].time) {
        apply_keyframe_sample(anim, &frames[0]);
        return;
    }
    if (final_t >= frames[count - 1].time) {
        apply_keyframe_sample(anim, &frames[count - 1]);
        return;
    }

    uint32_t idx = 0;
    for (uint32_t i = 0; i + 1 < count; i++) {
        if (final_t >= frames[i].time && final_t <= frames[i + 1].time) {
            idx = i;
            break;
        }
    }
    UIKeyframe* k1 = &frames[idx];
    UIKeyframe* k2 = &frames[idx + 1];
    float span = k2->time - k1->time;
    if (span <= 0.0001f) span = 0.0001f;
    float segment_t = (final_t - k1->time) / span;
    if (segment_t < 0.0f) segment_t = 0.0f;
    if (segment_t > 1.0f) segment_t = 1.0f;
    float eased = apply_easing(segment_t, k2->easing, anim);
    int rank = property_rank(anim->property);
    if (rank == 2) {
        apply_vec2_value(anim, ui_vec2_lerp_custom(k1->value.v2, k2->value.v2, eased));
    } else if (rank == 4) {
        apply_vec4_value(anim, ui_vec4_lerp_custom(k1->value.v4, k2->value.v4, eased));
    } else {
        apply_float_value(anim, ui_lerp(k1->value.f, k2->value.f, eased));
    }
}

void ui_animation_update(float delta_time) {
    UIAnimation* curr = g_active_animations;
    UIAnimation* prev = NULL;

    while (curr) {
        UIAnimation* next_backup = curr->next;
        bool exists = true;

        if (curr->is_playing && !curr->is_finished) {
            if (curr->delay > 0) {
                curr->delay -= delta_time;
            } else {
                curr->elapsed += delta_time;
                float duration = curr->duration > 0.0001f ? curr->duration : 0.0001f;
                float t = curr->elapsed / duration;
                bool cycle_complete = false;

                if (t >= 1.0f) {
                    if (curr->loop) {
                        if (curr->ping_pong) {
                            curr->reverse = !curr->reverse;
                            curr->elapsed = 0; // Reset time but keep direction
                            t = curr->reverse ? 1.0f : 0.0f;
                        } else {
                            curr->elapsed = 0;
                            t = 0.0f;
                        }
                    } else {
                        t = 1.0f;
                        cycle_complete = true;
                        curr->is_finished = true;
                        curr->is_playing = false;
                    }
                }

                // Adjust t for ping-pong reverse
                float final_t = curr->reverse ? (1.0f - t) : t;
                // Clamp final_t [0,1]
                if (final_t < 0.0f) final_t = 0.0f;
                if (final_t > 1.0f) final_t = 1.0f;

                if (curr->mode == UI_ANIM_MODE_KEYFRAME) {
                    apply_ui_keyframes(curr, final_t);
                } else {
                    apply_ui_tween(curr, apply_easing(final_t, curr->easing, curr));
                }

                if (cycle_complete && curr->on_complete) {
                    curr->on_complete(curr, curr->user_data);
                    // Check if destroyed by callback
                    if (prev) {
                         if (prev->next != curr) exists = false;
                    } else {
                         if (g_active_animations != curr) exists = false;
                    }
                }
            }
        }

        if (exists) {
            if (curr->auto_destroy && curr->is_finished) {
                ui_animation_destroy(curr);
                // curr is dead. prev stays same.
                curr = next_backup;
            } else {
                prev = curr;
                curr = curr->next;
            }
        } else {
            // curr destroyed externally
            curr = prev ? prev->next : g_active_animations;
        }
    }
}

// Helpers
UIAnimation* ui_animate_fade(Widget* widget, float target_opacity, float duration, UIEasingType easing) {
    UIAnimation* anim = ui_animation_create(widget, UI_ANIM_OPACITY, duration);
    if (anim) {
        ui_animation_set_float(anim, widget->opacity, target_opacity);
        ui_animation_set_easing(anim, easing);
        ui_animation_play(anim);
    }
    return anim;
}

UIAnimation* ui_animate_move(Widget* widget, Vec2 target_pos, float duration, UIEasingType easing) {
    UIAnimation* anim = ui_animation_create(widget, UI_ANIM_POSITION, duration);
    if (anim) {
        ui_animation_set_vec2(anim, widget_get_position(widget), target_pos);
        ui_animation_set_easing(anim, easing);
        ui_animation_play(anim);
    }
    return anim;
}

UIAnimation* ui_animate_resize(Widget* widget, Vec2 target_size, float duration, UIEasingType easing) {
    UIAnimation* anim = ui_animation_create(widget, UI_ANIM_SIZE, duration);
    if (anim) {
        ui_animation_set_vec2(anim, widget_get_size(widget), target_size);
        ui_animation_set_easing(anim, easing);
        ui_animation_play(anim);
    }
    return anim;
}

// Transition callback helpers
typedef struct {
    Widget* current;
    Widget* next;
} TransitionData;

static void on_fade_out_complete(UIAnimation* anim, void* user_data) {
    TransitionData* data = (TransitionData*)user_data;
    if (data->current) {
        widget_set_visible(data->current, false);
        // Reset opacity for next usage
        widget_set_opacity(data->current, 1.0f);
    }
    FREE(data);
    // Animation auto-destroyed
}

static void on_slide_out_complete(UIAnimation* anim, void* user_data) {
    TransitionData* data = (TransitionData*)user_data;
    if (data->current) {
        widget_set_visible(data->current, false);
        // Optionally reset position?
    }
    FREE(data);
    // Animation auto-destroyed
}

// Transition implementation
void ui_transition_screen(Widget* current, Widget* next, UITransitionType type, float duration) {
    if (!next) return;

    // Ensure next is visible but prepared for animation
    widget_set_visible(next, true);
    // Bring next to front?
    // widget_bring_to_front(next); // Assuming we had this API

    TransitionData* data = MALLOC(sizeof(TransitionData));
    data->current = current;
    data->next = next;

    switch (type) {
        case UI_TRANSITION_FADE: {
            if (current) {
                // Fade out current
                UIAnimation* anim = ui_animate_fade(current, 0.0f, duration, UI_EASE_IN_QUAD);
                if (anim) {
                    anim->auto_destroy = true;
                    ui_animation_set_callback(anim, on_fade_out_complete, data);
                } else {
                    FREE(data);
                }
            } else {
                FREE(data);
            }

            // Fade in next
            widget_set_opacity(next, 0.0f);
            UIAnimation* anim2 = ui_animate_fade(next, 1.0f, duration, UI_EASE_OUT_QUAD);
            if (anim2) anim2->auto_destroy = true;
            break;
        }

        case UI_TRANSITION_SLIDE_LEFT: {
            // Current moves Left (-width)
            // Next moves from Right (+width to 0)
            float width = next->size.x; // Assume same size
            if (current) width = current->size.x;

            if (current) {
                Vec2 target = widget_get_position(current);
                target.x -= width;
                UIAnimation* anim = ui_animate_move(current, target, duration, UI_EASE_IN_QUAD);
                if (anim) {
                    anim->auto_destroy = true;
                    ui_animation_set_callback(anim, on_slide_out_complete, data);
                } else {
                    FREE(data);
                }
            } else {
                FREE(data);
            }

            Vec2 start = widget_get_position(next);
            start.x = width; // Start off-screen right
            Vec2 end = start;
            end.x = 0; // End at 0 (assuming origin)

            widget_set_position(next, start);
            UIAnimation* anim2 = ui_animate_move(next, end, duration, UI_EASE_OUT_QUAD);
            if (anim2) anim2->auto_destroy = true;
            break;
        }

        case UI_TRANSITION_SLIDE_RIGHT: {
            // Current moves Right (+width)
            // Next moves from Left (-width to 0)
            float width = next->size.x;
            if (current) width = current->size.x;

            if (current) {
                Vec2 target = widget_get_position(current);
                target.x += width;
                UIAnimation* anim = ui_animate_move(current, target, duration, UI_EASE_IN_QUAD);
                if (anim) {
                    anim->auto_destroy = true;
                    ui_animation_set_callback(anim, on_slide_out_complete, data);
                } else {
                    FREE(data);
                }
            } else {
                FREE(data);
            }

            Vec2 start = widget_get_position(next);
            start.x = -width;
            Vec2 end = start;
            end.x = 0;

            widget_set_position(next, start);
            UIAnimation* anim2 = ui_animate_move(next, end, duration, UI_EASE_OUT_QUAD);
            if (anim2) anim2->auto_destroy = true;
            break;
        }

        case UI_TRANSITION_SLIDE_UP: {
            // Current moves Up (-height)
            // Next moves from Down (+height to 0) ? No, "Slide Up" usually means appearing from bottom moving up.
            // Or moving whole view up?
            // Let's assume Next comes from Bottom (+height) moving to 0. Current moves to Top (-height).
            float height = next->size.y;
            if (current) height = current->size.y;

            if (current) {
                Vec2 target = widget_get_position(current);
                target.y -= height;
                UIAnimation* anim = ui_animate_move(current, target, duration, UI_EASE_IN_QUAD);
                if (anim) {
                    anim->auto_destroy = true;
                    ui_animation_set_callback(anim, on_slide_out_complete, data);
                } else {
                    FREE(data);
                }
            } else {
                FREE(data);
            }

            Vec2 start = widget_get_position(next);
            start.y = height;
            Vec2 end = start;
            end.y = 0;

            widget_set_position(next, start);
            UIAnimation* anim2 = ui_animate_move(next, end, duration, UI_EASE_OUT_QUAD);
            if (anim2) anim2->auto_destroy = true;
            break;
        }

        case UI_TRANSITION_SLIDE_DOWN: {
            // Next comes from Top (-height) moving to 0. Current moves to Bottom (+height).
            float height = next->size.y;
            if (current) height = current->size.y;

            if (current) {
                Vec2 target = widget_get_position(current);
                target.y += height;
                UIAnimation* anim = ui_animate_move(current, target, duration, UI_EASE_IN_QUAD);
                if (anim) {
                    anim->auto_destroy = true;
                    ui_animation_set_callback(anim, on_slide_out_complete, data);
                } else {
                    FREE(data);
                }
            } else {
                FREE(data);
            }

            Vec2 start = widget_get_position(next);
            start.y = -height;
            Vec2 end = start;
            end.y = 0;

            widget_set_position(next, start);
            UIAnimation* anim2 = ui_animate_move(next, end, duration, UI_EASE_OUT_QUAD);
            if (anim2) anim2->auto_destroy = true;
            break;
        }

        case UI_TRANSITION_ZOOM: {
             if (current) {
                UIAnimation* anim = ui_animate_fade(current, 0.0f, duration, UI_EASE_IN_QUAD);
                if (anim) {
                    anim->auto_destroy = true;
                    ui_animation_set_callback(anim, on_fade_out_complete, data);
                } else {
                    FREE(data);
                }
            } else {
                FREE(data);
            }

            widget_set_opacity(next, 0.0f);
            UIAnimation* anim2 = ui_animate_fade(next, 1.0f, duration, UI_EASE_OUT_QUAD);
            if (anim2) anim2->auto_destroy = true;
            break;
        }

        case UI_TRANSITION_NONE:
        default:
            if (current) widget_set_visible(current, false);
            widget_set_visible(next, true);
            FREE(data);
            break;
    }
}
