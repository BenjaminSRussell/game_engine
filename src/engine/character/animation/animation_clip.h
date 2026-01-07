#ifndef ANIMATION_CLIP_H
#define ANIMATION_CLIP_H

#include <simd/simd.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct keyframe {
    float time;
    simd_float3 position;
    simd_quatf rotation;
    simd_float3 scale;
} keyframe_t;

typedef struct bone_track {
    uint32_t bone_index;
    keyframe_t* keyframes;
    uint32_t keyframe_count;
} bone_track_t;

typedef struct animation_clip {
    char name[64];
    float duration;
    float ticks_per_second;
    bone_track_t* tracks;
    uint32_t track_count;
    bool looping;
} animation_clip_t;

animation_clip_t* animation_clip_create(const char* name, float duration, float tps, uint32_t track_count);
void animation_clip_destroy(animation_clip_t* clip);
void animation_clip_add_track(animation_clip_t* clip, uint32_t track_index, const bone_track_t* track);

#endif // ANIMATION_CLIP_H
