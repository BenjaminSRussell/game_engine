#include "ui_animation.h"
#include <widgets/widget.h>
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

static float apply_easing(float t, UIEasingType type) {
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
        LOG_ERROR(LOG_CAT_ANIMATION, "Failed to allocate UIAnimation");
        return NULL;
    }
    memset(anim, 0, sizeof(UIAnimation));

    anim->target = target;
    anim->property = property;
    anim->duration = duration;
    anim->easing = UI_EASE_LINEAR;
    anim->is_playing = false;
    anim->is_finished = false;
    anim->auto_destroy = false;

    // Auto-add to global list
    anim->next = g_active_animations;
    g_active_animations = anim;

    return anim;
}

void ui_animation_chain(UIAnimation* first, UIAnimation* next) {
    if (!first || !next || first == next) return;

    first->next_in_sequence = next;

    // Remove 'next' from active list if present
    if (g_active_animations == next) {
        g_active_animations = next->next;
    } else {
        UIAnimation* curr = g_active_animations;
        while (curr && curr->next != next) {
            curr = curr->next;
        }
        if (curr) {
            curr->next = next->next;
        }
    }

    // Ensure next is not playing and detached from the list
    next->is_playing = false;
    next->elapsed = 0;
    next->next = NULL;
}

void ui_animation_destroy(UIAnimation* anim) {
    if (!anim) return;

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

    // Recursively destroy sequenced animations
    if (anim->next_in_sequence) {
        ui_animation_destroy(anim->next_in_sequence);
        anim->next_in_sequence = NULL;
    }

    memory_free(anim);
}

void ui_animation_destroy_all(void) {
    while (g_active_animations) {
        UIAnimation* next = g_active_animations->next;
        memory_free(g_active_animations);
        g_active_animations = next;
    }
}

void ui_animation_set_float(UIAnimation* anim, float start, float end) {
    if (anim) {
        anim->values.f.start = start;
        anim->values.f.end = end;
    }
}

void ui_animation_set_vec2(UIAnimation* anim, Vec2 start, Vec2 end) {
    if (anim) {
        anim->values.v2.start = start;
        anim->values.v2.end = end;
    }
}

void ui_animation_set_vec4(UIAnimation* anim, Vec4 start, Vec4 end) {
    if (anim) {
        anim->values.v4.start = start;
        anim->values.v4.end = end;
    }
}

void ui_animation_set_easing(UIAnimation* anim, UIEasingType easing) {
    if (anim) anim->easing = easing;
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

                float eased_t = apply_easing(final_t, curr->easing);

                // Apply value to widget
                if (curr->target) {
                    switch (curr->property) {
                        case UI_ANIM_OPACITY: {
                            float val = ui_lerp(curr->values.f.start, curr->values.f.end, eased_t);
                            widget_set_opacity(curr->target, val);
                            break;
                        }
                        case UI_ANIM_POSITION_X: {
                            Vec2 pos = widget_get_position(curr->target);
                            pos.x = ui_lerp(curr->values.f.start, curr->values.f.end, eased_t);
                            widget_set_position(curr->target, pos);
                            break;
                        }
                        case UI_ANIM_POSITION_Y: {
                            Vec2 pos = widget_get_position(curr->target);
                            pos.y = ui_lerp(curr->values.f.start, curr->values.f.end, eased_t);
                            widget_set_position(curr->target, pos);
                            break;
                        }
                        case UI_ANIM_POSITION: {
                            Vec2 val = ui_vec2_lerp_custom(curr->values.v2.start, curr->values.v2.end, eased_t);
                            widget_set_position(curr->target, val);
                            break;
                        }
                        case UI_ANIM_WIDTH: {
                            Vec2 size = widget_get_size(curr->target);
                            size.x = ui_lerp(curr->values.f.start, curr->values.f.end, eased_t);
                            widget_set_size(curr->target, size);
                            break;
                        }
                        case UI_ANIM_HEIGHT: {
                            Vec2 size = widget_get_size(curr->target);
                            size.y = ui_lerp(curr->values.f.start, curr->values.f.end, eased_t);
                            widget_set_size(curr->target, size);
                            break;
                        }
                        case UI_ANIM_SIZE: {
                            Vec2 val = ui_vec2_lerp_custom(curr->values.v2.start, curr->values.v2.end, eased_t);
                            widget_set_size(curr->target, val);
                            break;
                        }
                        case UI_ANIM_BACKGROUND_COLOR: {
                            Vec4 val = ui_vec4_lerp_custom(curr->values.v4.start, curr->values.v4.end, eased_t);
                            widget_set_background_color(curr->target, val);
                            break;
                        }
                        case UI_ANIM_TEXT_COLOR: {
                            Vec4 val = ui_vec4_lerp_custom(curr->values.v4.start, curr->values.v4.end, eased_t);
                            widget_set_text_color(curr->target, val);
                            break;
                        }
                        case UI_ANIM_BORDER_COLOR: {
                            Vec4 val = ui_vec4_lerp_custom(curr->values.v4.start, curr->values.v4.end, eased_t);
                            widget_set_border_color(curr->target, val);
                            break;
                        }
                    }
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
            // Check for sequencing
            if (curr->is_finished && !curr->is_playing && curr->next_in_sequence) {
                // Activate next animation
                UIAnimation* next_seq = curr->next_in_sequence;
                curr->next_in_sequence = NULL; // Detach ownership

                // Add to active list
                next_seq->next = g_active_animations;
                g_active_animations = next_seq;

                ui_animation_play(next_seq);
            }

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

            // Prepare next widget (Zoom In)
            widget_set_opacity(next, 0.0f);
            Vec2 final_size = widget_get_size(next);
            Vec2 final_pos = widget_get_position(next);

            float start_scale = 0.5f;
            Vec2 start_size = { final_size.x * start_scale, final_size.y * start_scale };
            Vec2 start_pos = {
                final_pos.x + (final_size.x - start_size.x) * 0.5f,
                final_pos.y + (final_size.y - start_size.y) * 0.5f
            };

            widget_set_size(next, start_size);
            widget_set_position(next, start_pos);

            // Parallel animations for Zoom In effect
            UIAnimation* anim_fade = ui_animate_fade(next, 1.0f, duration, UI_EASE_OUT_CUBIC);
            if (anim_fade) anim_fade->auto_destroy = true;

            UIAnimation* anim_size = ui_animate_resize(next, final_size, duration, UI_EASE_OUT_CUBIC);
            if (anim_size) anim_size->auto_destroy = true;

            UIAnimation* anim_pos = ui_animate_move(next, final_pos, duration, UI_EASE_OUT_CUBIC);
            if (anim_pos) anim_pos->auto_destroy = true;

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
