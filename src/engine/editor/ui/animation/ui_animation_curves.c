/*
 * ui_animation_curves.c
 * UI Animation Curves Implementation
 */

#include "editor/ui/animation/ui_animation_curves.h"
#include <core/logger.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_CURVES 512
#define DEFAULT_CACHE_RESOLUTION 256

static struct {
    ui_animation_curve_state_t *curves;
    uint32_t next_id;
    bool initialized;
} g_curve_system = {0};

int ui_animation_curves_init(void) {
    if (g_curve_system.initialized) return 0;
    
    g_curve_system.curves = calloc(MAX_CURVES, sizeof(ui_animation_curve_state_t));
    if (!g_curve_system.curves) return -1;
    
    g_curve_system.next_id = 1;
    g_curve_system.initialized = true;
    LOG_INFO("UI Animation Curves System initialized");
    return 0;
}

void ui_animation_curves_shutdown(void) {
    if (!g_curve_system.initialized) return;
    
    for (uint32_t i = 1; i < MAX_CURVES; i++) {
        if (g_curve_system.curves[i].keyframes) {
            free(g_curve_system.curves[i].keyframes);
            free(g_curve_system.curves[i].bezier_controls);
            free(g_curve_system.curves[i].cached_values);
        }
    }
    
    free(g_curve_system.curves);
    memset(&g_curve_system, 0, sizeof(g_curve_system));
}

ui_curve_evaluation_result_t ui_animation_curve_evaluate(ui_animation_curve_handle_t curve, float time) {
    ui_curve_evaluation_result_t result = {0};
    if (!g_curve_system.initialized || curve.id == 0 || curve.id >= MAX_CURVES) {
        result.valid = false;
        return result;
    }
    
    ui_animation_curve_state_t *state = &g_curve_system.curves[curve.id];
    if (!state->keyframes || state->keyframe_count == 0) {
        result.valid = false;
        return result;
    }
    
    // Handle infinity
    if (time < state->keyframes[0].time) {
        if (state->pre_infinity) {
            time = state->pre_infinity_time + fmod(time - state->keyframes[0].time, 
                   state->keyframes[state->keyframe_count - 1].time - state->keyframes[0].time);
        } else {
            result.value = state->keyframes[0].value;
            result.valid = true;
            return result;
        }
    }
    
    if (time > state->keyframes[state->keyframe_count - 1].time) {
        if (state->post_infinity) {
            time = state->keyframes[0].time + fmod(time - state->keyframes[state->keyframe_count - 1].time,
                   state->keyframes[state->keyframe_count - 1].time - state->keyframes[0].time);
        } else {
            result.value = state->keyframes[state->keyframe_count - 1].value;
            result.valid = true;
            return result;
        }
    }
    
    // Find keyframe interval
    uint32_t start_idx = 0;
    for (uint32_t i = 0; i < state->keyframe_count - 1; i++) {
        if (time >= state->keyframes[i].time && time <= state->keyframes[i + 1].time) {
            start_idx = i;
            break;
        }
    }
    
    ui_curve_keyframe_t *k1 = &state->keyframes[start_idx];
    ui_curve_keyframe_t *k2 = &state->keyframes[start_idx + 1];
    
    float t = (time - k1->time) / (k2->time - k1->time);
    
    switch (k1->interpolation) {
        case UI_CURVE_INTERP_LINEAR:
            result.value = k1->value + (k2->value - k1->value) * t;
            break;
            
        case UI_CURVE_INTERP_STEP:
            result.value = t < 1.0f ? k1->value : k2->value;
            break;
            
        case UI_CURVE_INTERP_CUBIC_SPLINE:
            // Simplified cubic spline
            float t2 = t * t;
            float t3 = t2 * t;
            result.value = k1->value + (k2->value - k1->value) * t;
            break;
            
        default:
            result.value = k1->value + (k2->value - k1->value) * t;
            break;
    }
    
    result.valid = true;
    return result;
}
