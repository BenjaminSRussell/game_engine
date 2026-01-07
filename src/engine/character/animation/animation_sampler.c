#include "character/animation/animation_sampler.h"
#include <include/math/math.h>

static void get_prev_and_next_keyframes(const bone_track_t* track, float time, 
                                      const keyframe_t** prev, const keyframe_t** next, float* factor) {
    if (track->keyframe_count == 0) {
        *prev = NULL;
        *next = NULL;
        *factor = 0.0f;
        return;
    }

    if (track->keyframe_count == 1 || time <= track->keyframes[0].time) {
        *prev = &track->keyframes[0];
        *next = &track->keyframes[0];
        *factor = 0.0f;
        return;
    }

    if (time >= track->keyframes[track->keyframe_count - 1].time) {
        *prev = &track->keyframes[track->keyframe_count - 1];
        *next = &track->keyframes[track->keyframe_count - 1];
        *factor = 0.0f;
        return;
    }

    for (uint32_t i = 0; i < track->keyframe_count - 1; ++i) {
        if (time < track->keyframes[i+1].time) {
            *prev = &track->keyframes[i];
            *next = &track->keyframes[i+1];
            float duration = (*next)->time - (*prev)->time;
            if (duration > 0.0001f) {
                *factor = (time - (*prev)->time) / duration;
            } else {
                *factor = 0.0f;
            }
            return;
        }
    }
}

static simd_float4x4 form_matrix(simd_float3 curr_pos, simd_quatf curr_rot, simd_float3 curr_scale) {
    // Rotation matrix
    simd_float4x4 rot_matrix = simd_matrix4x4(curr_rot);
    
    // Scale matrix
    simd_float4x4 scale_matrix = (simd_float4x4){ .columns = {
        simd_make_float4(curr_scale.x, 0, 0, 0),
        simd_make_float4(0, curr_scale.y, 0, 0),
        simd_make_float4(0, 0, curr_scale.z, 0),
        simd_make_float4(0, 0, 0, 1)
    }};
    
    // Translation matrix
    simd_float4x4 trans_matrix = matrix_identity_float4x4;
    trans_matrix.columns[3] = simd_make_float4(curr_pos.x, curr_pos.y, curr_pos.z, 1.0f);
    
    // Combine: T * R * S
    return simd_mul(trans_matrix, simd_mul(rot_matrix, scale_matrix));
}

void animation_sample(const animation_clip_t* clip, float time,
                      simd_float4x4* bone_transforms, uint32_t bone_count) {
    if (!clip || !bone_transforms) return;

    float sample_time = time;
    if (clip->looping && clip->duration > 0.0f) {
        sample_time = fmodf(time, clip->duration);
    } else if (sample_time > clip->duration) {
        sample_time = clip->duration;
    } else if (sample_time < 0.0f) {
        sample_time = 0.0f;
    }

    for (uint32_t i = 0; i < clip->track_count; ++i) {
        const bone_track_t* track = &clip->tracks[i];
        if (track->bone_index >= bone_count) continue;

        const keyframe_t* prev = NULL;
        const keyframe_t* next = NULL;
        float factor = 0.0f;

        get_prev_and_next_keyframes(track, sample_time, &prev, &next, &factor);
        
        if (prev && next) {
            simd_float3 curr_pos = simd_mix(prev->position, next->position, factor);
            simd_quatf curr_rot = simd_slerp(prev->rotation, next->rotation, factor);
            simd_float3 curr_scale = simd_mix(prev->scale, next->scale, factor);

            bone_transforms[track->bone_index] = form_matrix(curr_pos, curr_rot, curr_scale);
        }
    }
}
