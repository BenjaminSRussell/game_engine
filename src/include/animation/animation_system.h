/**
 * @file animation_system.h
 * @brief Core animation system types and functions.
 */
#ifndef ANIMATION_ANIMATION_SYSTEM_H
#define ANIMATION_ANIMATION_SYSTEM_H

#include "common.h"
#include "math/vec3.h"
#include "math/quat.h"
#include "math/mat4.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BONES 256
#define MAX_KEYFRAMES 1024
#define MAX_CHANNELS 256

// Animation keyframe structure
typedef struct AnimationKeyframe {
    f32 time;
    Vec3 position;
    Quat rotation;
    Vec3 scale;
} AnimationKeyframe;

// Animation channel for a single bone
typedef struct AnimationChannel {
    i32 bone_index;
    AnimationKeyframe *keyframes;
    u32 keyframe_count;
} AnimationChannel;

// Animation clip
typedef struct AnimationClip {
    char name[64];
    f32 duration;
    f32 ticks_per_second;
    AnimationChannel *channels;
    u32 channel_count;
    bool looping;
} AnimationClip;

// Skeleton bone
typedef struct Bone {
    char name[64];
    i32 parent_index;
    Mat4 inverse_bind_pose;
    Mat4 local_transform;
} Bone;

// Skeleton structure
typedef struct Skeleton {
    Bone *bones;
    u32 bone_count;
    Mat4 *global_transforms;
} Skeleton;

// Pose for skeletal animation
typedef struct Pose {
    Vec3 positions[MAX_BONES];
    Quat rotations[MAX_BONES];
    Vec3 scales[MAX_BONES];
    u32 bone_count;
} Pose;

// Animation playback state
typedef struct AnimationState {
    AnimationClip *clip;
    f32 current_time;
    f32 playback_speed;
    bool playing;
    bool looping;
} AnimationState;

// Sample animation clip at given time
void animation_sample_clip(AnimationClip *clip, f32 time, Pose *out_pose);

// Blend two poses together
void animation_blend_poses(const Pose *a, const Pose *b, f32 weight, Pose *out_pose);

// Apply pose to skeleton (updates global transforms)
void animation_apply_pose_to_skeleton(const Pose *pose, Skeleton *skeleton);

// Pose blending helper (same as animation_blend_poses)
static inline void pose_blend(const Pose *a, const Pose *b, f32 weight, Pose *out) {
    animation_blend_poses(a, b, weight, out);
}

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_ANIMATION_SYSTEM_H
