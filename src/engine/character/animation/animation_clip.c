#include "character/animation/animation_clip.h"
#include <stdlib.h>
#include <string.h>

animation_clip_t* animation_clip_create(const char* name, float duration, float tps, uint32_t track_count) {
    animation_clip_t* clip = (animation_clip_t*)malloc(sizeof(animation_clip_t));
    if (!clip) return NULL;

    strncpy(clip->name, name, sizeof(clip->name) - 1);
    clip->name[sizeof(clip->name) - 1] = '\0';
    clip->duration = duration;
    clip->ticks_per_second = tps;
    clip->looping = true;
    clip->track_count = track_count;

    if (track_count > 0) {
        clip->tracks = (bone_track_t*)calloc(track_count, sizeof(bone_track_t));
        if (!clip->tracks) {
            free(clip);
            return NULL;
        }
    } else {
        clip->tracks = NULL;
    }

    return clip;
}

void animation_clip_destroy(animation_clip_t* clip) {
    if (clip) {
        if (clip->tracks) {
            for (uint32_t i = 0; i < clip->track_count; ++i) {
                if (clip->tracks[i].keyframes) {
                    free(clip->tracks[i].keyframes);
                }
            }
            free(clip->tracks);
        }
        free(clip);
    }
}

void animation_clip_add_track(animation_clip_t* clip, uint32_t track_index, const bone_track_t* track) {
    if (!clip || !track || track_index >= clip->track_count) return;

    bone_track_t* dst = &clip->tracks[track_index];
    dst->bone_index = track->bone_index;
    dst->keyframe_count = track->keyframe_count;

    if (track->keyframe_count > 0) {
        dst->keyframes = (keyframe_t*)malloc(sizeof(keyframe_t) * track->keyframe_count);
        if (dst->keyframes) {
            memcpy(dst->keyframes, track->keyframes, sizeof(keyframe_t) * track->keyframe_count);
        }
    }
}
