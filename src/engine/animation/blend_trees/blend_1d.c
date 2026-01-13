#include "animation/blend_trees/blend_1d.h"
#include "animation/animation_system.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define INITIAL_CAPACITY 4

typedef struct {
    AnimationClip *clip;
    float threshold;
} BlendSample1DParam;

struct BlendSpace1D {
    BlendSample1DParam *samples;
    int sample_count;
    int sample_capacity;
};

static int compare_samples(const void *a, const void *b) {
    const BlendSample1DParam *sa = (const BlendSample1DParam *)a;
    const BlendSample1DParam *sb = (const BlendSample1DParam *)b;
    if (sa->threshold < sb->threshold) return -1;
    if (sa->threshold > sb->threshold) return 1;
    return 0;
}

BlendSpace1D* blend_1d_create(void) {
    BlendSpace1D *blend = (BlendSpace1D*)malloc(sizeof(BlendSpace1D));
    if (blend) {
        blend->sample_count = 0;
        blend->sample_capacity = INITIAL_CAPACITY;
        blend->samples = (BlendSample1DParam*)malloc(sizeof(BlendSample1DParam) * INITIAL_CAPACITY);
        if (!blend->samples) {
            free(blend);
            return NULL;
        }
    }
    return blend;
}

void blend_1d_destroy(BlendSpace1D* blend) {
    if (blend) {
        if (blend->samples) {
            free(blend->samples);
        }
        free(blend);
    }
}

void blend_1d_add_clip(BlendSpace1D *blend, AnimationClip *clip, float threshold) {
    if (!blend || !clip) return;

    if (blend->sample_count >= blend->sample_capacity) {
        int new_capacity = blend->sample_capacity * 2;
        BlendSample1DParam *new_samples = (BlendSample1DParam*)realloc(blend->samples, sizeof(BlendSample1DParam) * new_capacity);
        if (!new_samples) return; // Allocation failed
        blend->samples = new_samples;
        blend->sample_capacity = new_capacity;
    }

    blend->samples[blend->sample_count].clip = clip;
    blend->samples[blend->sample_count].threshold = threshold;
    blend->sample_count++;

    // Sort samples immediately to keep them ordered
    qsort(blend->samples, blend->sample_count, sizeof(BlendSample1DParam), compare_samples);
}

void blend_1d_evaluate(BlendSpace1D *blend, float time, float parameter, Pose *output_pose) {
    if (!blend || !output_pose || blend->sample_count == 0) return;

    // 1. Handle single sample case
    if (blend->sample_count == 1) {
        float duration = blend->samples[0].clip->duration;
        float sample_time = duration > 0 ? fmodf(time, duration) : 0.0f;
        animation_sample_clip(blend->samples[0].clip, sample_time, output_pose);
        return;
    }

    // 2. Find samples to blend
    int index_a = 0;
    int index_b = 0;
    float weight = 0.0f;

    if (parameter <= blend->samples[0].threshold) {
        index_a = index_b = 0;
    } else if (parameter >= blend->samples[blend->sample_count - 1].threshold) {
        index_a = index_b = blend->sample_count - 1;
    } else {
        // Binary search or linear scan (linear is fine for small count)
        for (int i = 0; i < blend->sample_count - 1; i++) {
            if (parameter >= blend->samples[i].threshold && parameter <= blend->samples[i+1].threshold) {
                index_a = i;
                index_b = i + 1;
                float range = blend->samples[i+1].threshold - blend->samples[i].threshold;
                if (range > 0.0001f) {
                    weight = (parameter - blend->samples[i].threshold) / range;
                }
                break;
            }
        }
    }

    // 3. Sync Logic (Phase Matching)
    // Calculate effective duration based on weight
    float duration_a = blend->samples[index_a].clip->duration;
    float duration_b = blend->samples[index_b].clip->duration;

    // Avoid division by zero
    if (duration_a <= 0.0001f) duration_a = 1.0f;
    if (duration_b <= 0.0001f) duration_b = 1.0f;

    float blended_duration = duration_a * (1.0f - weight) + duration_b * weight;

    // Normalized phase (0.0 to 1.0)
    float phase = fmodf(time, blended_duration) / blended_duration;

    // 4. Sample clips
    // Need temporary pose for second clip
    Pose pose_a, pose_b;
    // We assume Pose struct is large enough to be on stack or we should use allocator?
    // Pose has MAX_BONES=256, so 256 * (Vec3+Quat+Vec3) ~ 256 * (12+16+12) = 256*40 = 10KB.
    // 10KB on stack is risky but might be okay for game engine thread stack (usually 1MB+).
    // Safer to just use output_pose as one buffer and another stack buffer.

    animation_sample_clip(blend->samples[index_a].clip, phase * duration_a, &pose_a);

    if (index_a == index_b) {
        *output_pose = pose_a;
    } else {
        animation_sample_clip(blend->samples[index_b].clip, phase * duration_b, &pose_b);
        animation_blend_poses(&pose_a, &pose_b, weight, output_pose);
    }
}
