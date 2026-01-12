#ifndef ENGINE_ANIMATION_SYSTEM_H
#define ENGINE_ANIMATION_SYSTEM_H

#include "math/mat4.h"
#include "math/quat.h"
#include "math/vec3.h"
#include <core/engine.h>

#define MAX_BONES 100

typedef struct Bone {
  char name[32];
  i32 parent_index;
  Mat4 local_bind_pose;
  Mat4 inverse_bind_pose;
} Bone;

typedef struct Skeleton {
  Bone bones[MAX_BONES];
  u32 bone_count;
  Mat4 global_transforms[MAX_BONES]; // Computed per frame
} Skeleton;

typedef struct AnimationKeyframe {
  f32 time;
  Vec3 position;
  Quat rotation;
  Vec3 scale;
} AnimationKeyframe;

typedef struct AnimationChannel {
  i32 bone_index;
  AnimationKeyframe *keyframes;
  u32 keyframe_count;
} AnimationChannel;

typedef struct AnimationClip {
  char name[64];
  f32 duration;
  f32 ticks_per_second;
  AnimationChannel *channels;
  u32 channel_count;
} AnimationClip;

typedef struct Pose {
  Vec3 positions[MAX_BONES];
  Quat rotations[MAX_BONES];
  Vec3 scales[MAX_BONES];
  u32 bone_count;
} Pose;

typedef struct AnimationState {
  AnimationClip *current_clip;
  f32 current_time;
  bool is_playing;
  bool is_looping;
  f32 playback_speed;
  f32 blend_factor; // For blending between two clips (future)
} AnimationState;

typedef struct IKChain {
  i32 root_bone_index;
  i32 end_effector_bone_index;
  Vec3 target_position;
  u32 iteration_count;
  f32 tolerance;
  bool active;
} IKChain;

// System API
void animation_system_init(void);
void animation_system_update(f32 dt);
void animation_system_shutdown(void);

// Skeleton Management
Skeleton *animation_create_skeleton(u32 bone_count);
void animation_destroy_skeleton(Skeleton *skeleton);
void animation_update_skeleton(Skeleton *skeleton, AnimationState *state);

// IK
void animation_solve_ik(Skeleton *skeleton, IKChain *chain);

// Blending
void animation_sample_clip(AnimationClip *clip, f32 time, Pose *out_pose);
void animation_blend_poses(const Pose *a, const Pose *b, f32 weight,
                           Pose *out_pose);
void animation_apply_pose_to_skeleton(const Pose *pose, Skeleton *skeleton);

#endif // ENGINE_ANIMATION_SYSTEM_H
