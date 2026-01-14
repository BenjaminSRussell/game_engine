#ifndef ANIMATION_TYPES_H
#define ANIMATION_TYPES_H

#include "../Public/unified_animation.h"
#include <unified_logger.h>
#include <unified_memory.h>

#define MAX_BONES 128
#define MAX_NAME_LEN 32
#define MAX_KEYFRAMES 512

typedef struct {
  f32 time;
  Vec3 position;
  Quat rotation;
  Vec3 scale;
} AnimKeyframe;

typedef struct {
  i32 bone_index;
  AnimKeyframe *keyframes;
  u32 keyframe_count;
  u32 capacity;
} AnimChannel;

struct AnimClip {
  char name[MAX_NAME_LEN];
  f32 duration;
  AnimChannel *channels;
  u32 channel_count;
  u32 channel_capacity;
};

typedef struct {
  char name[MAX_NAME_LEN];
  i32 parent_index;
  Mat4 local_bind_pose;
  Mat4 inverse_bind_pose;
} AnimBone;

struct AnimSkeleton {
  u32 id;
  AnimBone *bones;
  u32 bone_count;

  // Runtime state
  Mat4 *global_transforms; // Cached world transforms

  // Playback state
  AnimClip *current_clip;
  f32 current_time;
  bool is_playing;
  bool is_looping;
};

struct AnimPose {
  Vec3 positions[MAX_BONES];
  Quat rotations[MAX_BONES];
  Vec3 scales[MAX_BONES];
  u32 bone_count;
};

struct AnimSystem {
  AnimConfig config;
  // Pools or lists could go here
};

#endif // ANIMATION_TYPES_H
