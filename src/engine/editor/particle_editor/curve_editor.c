#include "editor/particle_editor/curve_editor.h"
#include "engine/include/core/memory.h"
#include "engine/include/core/logger.h"
#include <string.h>
#include <include/math/math.h>
#include <stdlib.h>

#define MAX_CURVES 32
#define INITIAL_KEYFRAME_CAPACITY 16

// Hermite interpolation helper
static float hermite_interp(float t, float p0, float p1, float m0, float m1) {
    float t2 = t * t;
    float t3 = t2 * t;
    
    float h00 = 2*t3 - 3*t2 + 1;
    float h10 = t3 - 2*t2 + t;
    float h01 = -2*t3 + 3*t2;
    float h11 = t3 - t2;
    
    return h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1;
}

void curve_editor_init(CurveEditor* editor) {
    if (!editor) return;
    memset(editor, 0, sizeof(CurveEditor));
    
    editor->curve_capacity = MAX_CURVES;
    editor->curves = (AnimationCurve*)calloc(MAX_CURVES, sizeof(AnimationCurve));
    
    editor->selected_curve = -1;
    editor->selected_keyframe = -1;
    
    // Default view
    editor->view_min = (Vec2){0.0f, 0.0f};
    editor->view_max = (Vec2){1.0f, 1.0f};
    editor->zoom = 1.0f;
    
    LOG_INFO("Curve editor initialized");
}

void curve_editor_shutdown(CurveEditor* editor) {
    if (!editor) return;
    
    for (u32 i = 0; i < editor->curve_count; i++) {
        if (editor->curves[i].keyframes) {
            free(editor->curves[i].keyframes);
        }
    }
    free(editor->curves);
    
    memset(editor, 0, sizeof(CurveEditor));
}

AnimationCurve* curve_editor_add_curve(CurveEditor* editor, const char* name) {
    if (!editor || editor->curve_count >= editor->curve_capacity) return NULL;
    
    AnimationCurve* curve = &editor->curves[editor->curve_count++];
    memset(curve, 0, sizeof(AnimationCurve));
    
    strncpy(curve->name, name, sizeof(curve->name) - 1);
    curve->interpolation = CURVE_INTERP_SMOOTH;
    curve->keyframe_capacity = INITIAL_KEYFRAME_CAPACITY;
    curve->keyframes = (CurveKeyframe*)calloc(INITIAL_KEYFRAME_CAPACITY, sizeof(CurveKeyframe));
    curve->min_value = 0.0f;
    curve->max_value = 1.0f;
    
    // Add default keyframes
    curve_add_keyframe(curve, 0.0f, 0.0f);
    curve_add_keyframe(curve, 1.0f, 1.0f);
    
    LOG_INFO("Added curve '%s'", name);
    return curve;
}

void curve_editor_remove_curve(CurveEditor* editor, u32 curve_index) {
    if (!editor || curve_index >= editor->curve_count) return;
    
    if (editor->curves[curve_index].keyframes) {
        free(editor->curves[curve_index].keyframes);
    }
    
    // Shift remaining curves
    for (u32 i = curve_index; i < editor->curve_count - 1; i++) {
        editor->curves[i] = editor->curves[i + 1];
    }
    editor->curve_count--;
    
    if (editor->selected_curve == (i32)curve_index) {
        curve_editor_deselect(editor);
    }
}

AnimationCurve* curve_editor_get_curve(CurveEditor* editor, u32 curve_index) {
    if (!editor || curve_index >= editor->curve_count) return NULL;
    return &editor->curves[curve_index];
}

void curve_add_keyframe(AnimationCurve* curve, float time, float value) {
    if (!curve) return;
    
    // Check capacity
    if (curve->keyframe_count >= curve->keyframe_capacity) {
        curve->keyframe_capacity *= 2;
        curve->keyframes = (CurveKeyframe*)realloc(curve->keyframes, 
                                                   curve->keyframe_capacity * sizeof(CurveKeyframe));
    }
    
    // Find insertion point (keep sorted by time)
    u32 insert_idx = curve->keyframe_count;
    for (u32 i = 0; i < curve->keyframe_count; i++) {
        if (time < curve->keyframes[i].time) {
            insert_idx = i;
            break;
        } else if (fabsf(time - curve->keyframes[i].time) < 0.001f) {
            // Update existing keyframe
            curve->keyframes[i].value = value;
            return;
        }
    }
    
    // Shift keyframes
    for (u32 i = curve->keyframe_count; i > insert_idx; i--) {
        curve->keyframes[i] = curve->keyframes[i - 1];
    }
    
    // Insert new keyframe
    curve->keyframes[insert_idx].time = time;
    curve->keyframes[insert_idx].value = value;
    curve->keyframes[insert_idx].in_tangent = 0.0f;
    curve->keyframes[insert_idx].out_tangent = 0.0f;
    curve->keyframe_count++;
    
    // Auto-calculate tangents
    curve_smooth_tangents(curve, insert_idx);
}

void curve_remove_keyframe(AnimationCurve* curve, u32 keyframe_index) {
    if (!curve || keyframe_index >= curve->keyframe_count) return;
    
    // Don't allow removing all keyframes
    if (curve->keyframe_count <= 1) return;
    
    // Shift keyframes
    for (u32 i = keyframe_index; i < curve->keyframe_count - 1; i++) {
        curve->keyframes[i] = curve->keyframes[i + 1];
    }
    curve->keyframe_count--;
}

void curve_update_keyframe(AnimationCurve* curve, u32 keyframe_index, float time, float value) {
    if (!curve || keyframe_index >= curve->keyframe_count) return;
    
    curve->keyframes[keyframe_index].time = time;
    curve->keyframes[keyframe_index].value = value;
    
    // Resort if needed
    // For simplicity, we'll just allow out-of-order for now
}

void curve_set_tangents(AnimationCurve* curve, u32 keyframe_index, float in_tangent, float out_tangent) {
    if (!curve || keyframe_index >= curve->keyframe_count) return;
    
    curve->keyframes[keyframe_index].in_tangent = in_tangent;
    curve->keyframes[keyframe_index].out_tangent = out_tangent;
}

float curve_evaluate(const AnimationCurve* curve, float time) {
    if (!curve || curve->keyframe_count == 0) return 0.0f;
    
    // Clamp time to curve range
    if (time <= curve->keyframes[0].time) {
        return curve->keyframes[0].value;
    }
    if (time >= curve->keyframes[curve->keyframe_count - 1].time) {
        return curve->keyframes[curve->keyframe_count - 1].value;
    }
    
    // Find surrounding keyframes
    for (u32 i = 0; i < curve->keyframe_count - 1; i++) {
        if (time >= curve->keyframes[i].time && time <= curve->keyframes[i + 1].time) {
            CurveKeyframe* k0 = &curve->keyframes[i];
            CurveKeyframe* k1 = &curve->keyframes[i + 1];
            
            float dt = k1->time - k0->time;
            if (dt < 0.0001f) return k0->value;
            
            float t = (time - k0->time) / dt;
            
            switch (curve->interpolation) {
                case CURVE_INTERP_LINEAR:
                    return k0->value + (k1->value - k0->value) * t;
                
                case CURVE_INTERP_SMOOTH:
                    t = t * t * (3.0f - 2.0f * t); // Smoothstep
                    return k0->value + (k1->value - k0->value) * t;
                
                case CURVE_INTERP_BEZIER:
                    return hermite_interp(t, k0->value, k1->value, 
                                        k0->out_tangent * dt, k1->in_tangent * dt);
                
                case CURVE_INTERP_STEP:
                    return k0->value;
                
                default:
                    return k0->value;
            }
        }
    }
    
    return 0.0f;
}

void curve_editor_select_curve(CurveEditor* editor, u32 curve_index) {
    if (!editor || curve_index >= editor->curve_count) return;
    editor->selected_curve = curve_index;
    editor->selected_keyframe = -1;
}

void curve_editor_select_keyframe(CurveEditor* editor, u32 curve_index, u32 keyframe_index) {
    if (!editor || curve_index >= editor->curve_count) return;
    
    AnimationCurve* curve = &editor->curves[curve_index];
    if (keyframe_index >= curve->keyframe_count) return;
    
    editor->selected_curve = curve_index;
    editor->selected_keyframe = keyframe_index;
}

void curve_editor_deselect(CurveEditor* editor) {
    if (!editor) return;
    editor->selected_curve = -1;
    editor->selected_keyframe = -1;
}

void curve_editor_zoom(CurveEditor* editor, float delta) {
    if (!editor) return;
    
    float new_zoom = editor->zoom * (1.0f + delta * 0.1f);
    new_zoom = fmaxf(0.1f, fminf(10.0f, new_zoom));
    
    editor->zoom = new_zoom;
    
    // Adjust view based on zoom
    float range_x = editor->view_max.x - editor->view_min.x;
    float range_y = editor->view_max.y - editor->view_min.y;
    float center_x = (editor->view_min.x + editor->view_max.x) * 0.5f;
    float center_y = (editor->view_min.y + editor->view_max.y) * 0.5f;
    
    range_x /= (1.0f + delta * 0.1f);
    range_y /= (1.0f + delta * 0.1f);
    
    editor->view_min.x = center_x - range_x * 0.5f;
    editor->view_max.x = center_x + range_x * 0.5f;
    editor->view_min.y = center_y - range_y * 0.5f;
    editor->view_max.y = center_y + range_y * 0.5f;
}

void curve_editor_pan(CurveEditor* editor, Vec2 delta) {
    if (!editor) return;
    
    editor->view_min.x += delta.x;
    editor->view_max.x += delta.x;
    editor->view_min.y += delta.y;
    editor->view_max.y += delta.y;
}

void curve_editor_frame_all(CurveEditor* editor) {
    if (!editor || editor->curve_count == 0) return;
    
    // Find bounds of all curves
    float min_time = 0.0f;
    float max_time = 1.0f;
    float min_value = 0.0f;
    float max_value = 1.0f;
    
    for (u32 i = 0; i < editor->curve_count; i++) {
        AnimationCurve* curve = &editor->curves[i];
        for (u32 j = 0; j < curve->keyframe_count; j++) {
            float t = curve->keyframes[j].time;
            float v = curve->keyframes[j].value;
            
            if (i == 0 && j == 0) {
                min_time = max_time = t;
                min_value = max_value = v;
            } else {
                min_time = fminf(min_time, t);
                max_time = fmaxf(max_time, t);
                min_value = fminf(min_value, v);
                max_value = fmaxf(max_value, v);
            }
        }
    }
    
    // Add padding
    float padding_x = (max_time - min_time) * 0.1f;
    float padding_y = (max_value - min_value) * 0.1f;
    
    editor->view_min.x = min_time - padding_x;
    editor->view_max.x = max_time + padding_x;
    editor->view_min.y = min_value - padding_y;
    editor->view_max.y = max_value + padding_y;
}

void curve_normalize_time(AnimationCurve* curve) {
    if (!curve || curve->keyframe_count == 0) return;
    
    float min_time = curve->keyframes[0].time;
    float max_time = curve->keyframes[curve->keyframe_count - 1].time;
    float range = max_time - min_time;
    
    if (range < 0.0001f) return;
    
    for (u32 i = 0; i < curve->keyframe_count; i++) {
        curve->keyframes[i].time = (curve->keyframes[i].time - min_time) / range;
    }
}

void curve_smooth_tangents(AnimationCurve* curve, u32 keyframe_index) {
    if (!curve || keyframe_index >= curve->keyframe_count) return;
    
    CurveKeyframe* key = &curve->keyframes[keyframe_index];
    
    // Calculate tangent based on neighbors
    if (keyframe_index > 0 && keyframe_index < curve->keyframe_count - 1) {
        CurveKeyframe* prev = &curve->keyframes[keyframe_index - 1];
        CurveKeyframe* next = &curve->keyframes[keyframe_index + 1];
        
        float slope = (next->value - prev->value) / (next->time - prev->time);
        key->in_tangent = slope;
        key->out_tangent = slope;
    } else if (keyframe_index == 0 && curve->keyframe_count > 1) {
        CurveKeyframe* next = &curve->keyframes[1];
        float slope = (next->value - key->value) / (next->time - key->time);
        key->out_tangent = slope;
        key->in_tangent = 0.0f;
    } else if (keyframe_index == curve->keyframe_count - 1 && curve->keyframe_count > 1) {
        CurveKeyframe* prev = &curve->keyframes[keyframe_index - 1];
        float slope = (key->value - prev->value) / (key->time - prev->time);
        key->in_tangent = slope;
        key->out_tangent = 0.0f;
    }
}

void curve_auto_tangents(AnimationCurve* curve) {
    if (!curve) return;
    
    for (u32 i = 0; i < curve->keyframe_count; i++) {
        curve_smooth_tangents(curve, i);
    }
}
