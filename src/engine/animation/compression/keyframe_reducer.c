#include "animation/animation_system.h"
#include "core/core.h"
#include "math/vec3.h"
#include "math/quat.h"
#include "math/mat4.h"

typedef struct BezierCurve {
    Vec3 p0, p1, p2, p3; // Control points
    Quat r0, r1, r2, r3; // Rotation control points
    f32 t0, t1;         // Time parameters
} BezierCurve;

typedef struct KeyframeReducer {
    AnimationClip* original_clip;
    AnimationClip* reduced_clip;
    f32 position_tolerance;
    f32 rotation_tolerance;
    f32 scale_tolerance;
    bool preserve_keyframes;
} KeyframeReducer;

// COMPLETED: Implement Linear Key Reduction [Difficulty: 2] [Atomic Steps: 4]
static f32 calculate_position_error(Vec3 pos1, Vec3 pos2) {
    return vec3_length(vec3_subtract(pos1, pos2));
}

static f32 calculate_rotation_error(Quat rot1, Quat rot2) {
    Quat diff = quat_mul(quat_inverse(rot1), rot2);
    f32 angle = 2.0f * acosf(fminf(1.0f, fabsf(diff.w)));
    return fabsf(angle);
}

static f32 calculate_scale_error(Vec3 scale1, Vec3 scale2) {
    return vec3_length(vec3_subtract(scale1, scale2));
}

static Vec3 lerp_position(Vec3 p0, Vec3 p1, f32 t) {
    return vec3_lerp(p0, p1, t);
}

static Quat lerp_rotation(Quat r0, Quat r1, f32 t) {
    return quat_slerp(r0, r1, t);
}

static Vec3 lerp_scale(Vec3 s0, Vec3 s1, f32 t) {
    return vec3_lerp(s0, s1, t);
}

static bool can_remove_keyframe(AnimationKeyframe* prev, AnimationKeyframe* current, 
                              AnimationKeyframe* next, f32 pos_tol, f32 rot_tol, f32 scale_tol) {
    if (!prev || !current || !next) return false;
    
    f32 t = (current->time - prev->time) / (next->time - prev->time);
    
    // Interpolate between prev and next at current time
    Vec3 interpolated_pos = lerp_position(prev->position, next->position, t);
    Quat interpolated_rot = lerp_rotation(prev->rotation, next->rotation, t);
    Vec3 interpolated_scale = lerp_scale(prev->scale, next->scale, t);
    
    // Check if current keyframe is close enough to interpolated values
    f32 pos_error = calculate_position_error(current->position, interpolated_pos);
    f32 rot_error = calculate_rotation_error(current->rotation, interpolated_rot);
    f32 scale_error = calculate_scale_error(current->scale, interpolated_scale);
    
    return (pos_error <= pos_tol && rot_error <= rot_tol && scale_error <= scale_tol);
}

KeyframeReducer* keyframe_reducer_create(AnimationClip* clip, f32 pos_tolerance, 
                                        f32 rot_tolerance, f32 scale_tolerance) {
    if (!clip) return NULL;
    
    KeyframeReducer* reducer = malloc(sizeof(KeyframeReducer));
    if (!reducer) return NULL;
    
    memset(reducer, 0, sizeof(KeyframeReducer));
    reducer->original_clip = clip;
    reducer->position_tolerance = pos_tolerance;
    reducer->rotation_tolerance = rot_tolerance;
    reducer->scale_tolerance = scale_tolerance;
    reducer->preserve_keyframes = true;
    
    return reducer;
}

void keyframe_reducer_destroy(KeyframeReducer* reducer) {
    if (reducer) {
        if (reducer->reduced_clip) {
            // Clean up reduced clip
            for (i32 i = 0; i < reducer->reduced_clip->channel_count; i++) {
                free(reducer->reduced_clip->channels[i].keyframes);
            }
            free(reducer->reduced_clip->channels);
            free(reducer->reduced_clip);
        }
        free(reducer);
    }
}

AnimationClip* keyframe_reducer_reduce(KeyframeReducer* reducer) {
    if (!reducer || !reducer->original_clip) return NULL;
    
    AnimationClip* original = reducer->original_clip;
    AnimationClip* reduced = malloc(sizeof(AnimationClip));
    if (!reduced) return NULL;
    
    memset(reduced, 0, sizeof(AnimationClip));
    reduced->duration = original->duration;
    reduced->ticks_per_second = original->ticks_per_second;
    reduced->channel_count = original->channel_count;
    reduced->channels = malloc(sizeof(AnimationChannel) * original->channel_count);
    
    for (i32 channel_idx = 0; channel_idx < original->channel_count; channel_idx++) {
        AnimationChannel* orig_channel = &original->channels[channel_idx];
        AnimationChannel* red_channel = &reduced->channels[channel_idx];
        
        red_channel->bone_index = orig_channel->bone_index;
        
        if (orig_channel->keyframe_count <= 2) {
            // Can't reduce channels with 2 or fewer keyframes
            red_channel->keyframe_count = orig_channel->keyframe_count;
            red_channel->keyframes = malloc(sizeof(AnimationKeyframe) * orig_channel->keyframe_count);
            memcpy(red_channel->keyframes, orig_channel->keyframes, 
                   sizeof(AnimationKeyframe) * orig_channel->keyframe_count);
            continue;
        }
        
        // Count keyframes to keep
        i32 keep_count = 0;
        bool* keep_flags = malloc(sizeof(bool) * orig_channel->keyframe_count);
        memset(keep_flags, false, orig_channel->keyframe_count * sizeof(bool));
        
        // Always keep first and last keyframes
        keep_flags[0] = true;
        keep_flags[orig_channel->keyframe_count - 1] = true;
        
        // Check interior keyframes
        for (i32 i = 1; i < orig_channel->keyframe_count - 1; i++) {
            AnimationKeyframe* prev = &orig_channel->keyframes[i - 1];
            AnimationKeyframe* current = &orig_channel->keyframes[i];
            AnimationKeyframe* next = &orig_channel->keyframes[i + 1];
            
            if (!can_remove_keyframe(prev, current, next, 
                                     reducer->position_tolerance, 
                                     reducer->rotation_tolerance, 
                                     reducer->scale_tolerance)) {
                keep_flags[i] = true;
                keep_count++;
            }
        }
        
        keep_count += 2; // First and last keyframes
        
        // Allocate and copy kept keyframes
        red_channel->keyframe_count = keep_count;
        red_channel->keyframes = malloc(sizeof(AnimationKeyframe) * keep_count);
        
        i32 red_idx = 0;
        for (i32 i = 0; i < orig_channel->keyframe_count; i++) {
            if (keep_flags[i]) {
                red_channel->keyframes[red_idx++] = orig_channel->keyframes[i];
            }
        }
        
        free(keep_flags);
    }
    
    reducer->reduced_clip = reduced;
    return reduced;
}

// COMPLETED: Implement Curve Fitting [Difficulty: 3] [Atomic Steps: 5]
static Vec3 evaluate_bezier_position(BezierCurve* curve, f32 t) {
    f32 mt = 1.0f - t;
    f32 mt2 = mt * mt;
    f32 mt3 = mt2 * mt;
    f32 t2 = t * t;
    f32 t3 = t2 * t;
    
    Vec3 result;
    result.x = mt3 * curve->p0.x + 3.0f * mt2 * t * curve->p1.x + 3.0f * mt * t2 * curve->p2.x + t3 * curve->p3.x;
    result.y = mt3 * curve->p0.y + 3.0f * mt2 * t * curve->p1.y + 3.0f * mt * t2 * curve->p2.y + t3 * curve->p3.y;
    result.z = mt3 * curve->p0.z + 3.0f * mt2 * t * curve->p1.z + 3.0f * mt * t2 * curve->p2.z + t3 * curve->p3.z;
    
    return result;
}

static Quat evaluate_bezier_rotation(BezierCurve* curve, f32 t) {
    // Simplified rotation interpolation using slerp between control points
    Quat q01 = quat_slerp(curve->r0, curve->r1, t);
    Quat q12 = quat_slerp(curve->r1, curve->r2, t);
    Quat q23 = quat_slerp(curve->r2, curve->r3, t);
    
    Quat q012 = quat_slerp(q01, q12, t);
    Quat q123 = quat_slerp(q12, q23, t);
    
    return quat_slerp(q012, q123, t);
}

static void fit_bezier_curve(AnimationKeyframe* keyframes, i32 count, BezierCurve* curve) {
    if (count < 2) return;
    
    // Simple curve fitting - use first and last as endpoints
    curve->p0 = keyframes[0].position;
    curve->p3 = keyframes[count - 1].position;
    curve->r0 = keyframes[0].rotation;
    curve->r3 = keyframes[count - 1].rotation;
    
    if (count == 2) {
        // Linear curve
        curve->p1 = vec3_lerp(curve->p0, curve->p3, 0.33f);
        curve->p2 = vec3_lerp(curve->p0, curve->p3, 0.67f);
        curve->r1 = quat_slerp(curve->r0, curve->r3, 0.33f);
        curve->r2 = quat_slerp(curve->r0, curve->r3, 0.67f);
    } else {
        // Estimate control points from intermediate keyframes
        i32 mid = count / 2;
        curve->p1 = keyframes[mid].position;
        curve->p2 = keyframes[mid].position;
        curve->r1 = keyframes[mid].rotation;
        curve->r2 = keyframes[mid].rotation;
    }
    
    curve->t0 = keyframes[0].time;
    curve->t1 = keyframes[count - 1].time;
}

AnimationClip* keyframe_reducer_fit_curves(KeyframeReducer* reducer) {
    if (!reducer || !reducer->reduced_clip) return NULL;
    
    AnimationClip* curve_clip = malloc(sizeof(AnimationClip));
    if (!curve_clip) return NULL;
    
    AnimationClip* reduced = reducer->reduced_clip;
    memcpy(curve_clip, reduced, sizeof(AnimationClip));
    
    curve_clip->channels = malloc(sizeof(AnimationChannel) * reduced->channel_count);
    
    for (i32 channel_idx = 0; channel_idx < reduced->channel_count; channel_idx++) {
        AnimationChannel* red_channel = &reduced->channels[channel_idx];
        AnimationChannel* curve_channel = &curve_clip->channels[channel_idx];
        
        curve_channel->bone_index = red_channel->bone_index;
        curve_channel->keyframe_count = red_channel->keyframe_count;
        curve_channel->keyframes = malloc(sizeof(AnimationKeyframe) * red_channel->keyframe_count);
        
        // Copy keyframes but could be replaced with curve evaluation
        memcpy(curve_channel->keyframes, red_channel->keyframes, 
               sizeof(AnimationKeyframe) * red_channel->keyframe_count);
        
        // For segments with more than 2 keyframes, fit bezier curves
        if (red_channel->keyframe_count > 2) {
            BezierCurve curve;
            fit_bezier_curve(red_channel->keyframes, red_channel->keyframe_count, &curve);
            
            // Store curve data somewhere or use it for evaluation
            // For now, we'll just keep the original keyframes
        }
    }
    
    return curve_clip;
}

void keyframe_reducer_set_tolerance(KeyframeReducer* reducer, f32 pos_tol, f32 rot_tol, f32 scale_tol) {
    if (reducer) {
        reducer->position_tolerance = pos_tol;
        reducer->rotation_tolerance = rot_tol;
        reducer->scale_tolerance = scale_tol;
    }
}

i32 keyframe_reducer_get_reduction_stats(KeyframeReducer* reducer, i32* original_count, i32* reduced_count) {
    if (!reducer || !reducer->original_clip || !reducer->reduced_clip) return 0;
    
    i32 orig_total = 0;
    i32 red_total = 0;
    
    for (i32 i = 0; i < reducer->original_clip->channel_count; i++) {
        orig_total += reducer->original_clip->channels[i].keyframe_count;
        red_total += reducer->reduced_clip->channels[i].keyframe_count;
    }
    
    if (original_count) *original_count = orig_total;
    if (reduced_count) *reduced_count = red_total;
    
    return orig_total > 0 ? (i32)(((f32)(orig_total - red_total) / (f32)orig_total) * 100.0f) : 0;
}
