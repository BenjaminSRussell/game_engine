#ifndef ANIMATION_TYPES_H
#define ANIMATION_TYPES_H

#include <core/types.h>

// Animation type definitions
typedef struct {
    float duration;
    int bone_count;
    void *keyframes;
} AnimationClip;

typedef struct {
    int animation_count;
    AnimationClip *animations;
} AnimationSet;

#endif // ANIMATION_TYPES_H
