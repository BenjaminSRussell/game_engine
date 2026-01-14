#ifndef UNIFIED_ANIMATION_H
#define UNIFIED_ANIMATION_H

#include <core/types.h>
#include <math/mat4.h>
#include <math/quat.h>
#include <math/vec3.h>

// Opaque types
typedef struct AnimSystem AnimSystem;
typedef struct AnimSkeleton AnimSkeleton;
typedef struct AnimClip AnimClip;
typedef struct AnimPose AnimPose;

// Configuration
typedef struct {
  u32 max_skeletons;
  u32 max_bones_per_skeleton;
  u32 max_clips;
} AnimConfig;

// Bone definition for creation
typedef struct {
  char name[32];
  i32 parent_index; // -1 for root
  Vec3 local_pos;
  Quat local_rot;
  Vec3 local_scale;
} AnimBoneDef;

// System Lifecycle
AnimSystem *anim_system_create(AnimConfig config);
void anim_system_destroy(AnimSystem *system);
void anim_system_update(AnimSystem *system, f32 delta_time);

// Skeleton Management
AnimSkeleton *anim_skeleton_create(AnimSystem *system, const AnimBoneDef *bones,
                                   u32 bone_count);
void anim_skeleton_destroy(AnimSystem *system, AnimSkeleton *skeleton);
void anim_skeleton_reset(AnimSkeleton *skeleton);
void anim_skeleton_get_matrix_palette(AnimSkeleton *skeleton,
                                      Mat4 *out_matrices, u32 max_matrices);

// Clip Management
AnimClip *anim_clip_create(AnimSystem *system, const char *name, f32 duration);
void anim_clip_destroy(AnimSystem *system, AnimClip *clip);

// Playback & Sampling
void anim_skeleton_play(AnimSkeleton *skeleton, AnimClip *clip, bool loop);
void anim_skeleton_stop(AnimSkeleton *skeleton);
bool anim_skeleton_is_playing(AnimSkeleton *skeleton);

// Pose & Blending
AnimPose *anim_pose_create(AnimSystem *system);
void anim_pose_destroy(AnimSystem *system, AnimPose *pose);
void anim_sample_clip(AnimClip *clip, f32 time, AnimPose *out_pose);
void anim_blend_poses(const AnimPose *a, const AnimPose *b, f32 weight,
                      AnimPose *out_pose);
void anim_apply_pose(AnimSkeleton *skeleton, const AnimPose *pose);

// IK
void anim_solve_two_bone_ik(AnimSkeleton *skeleton, const char *root_bone,
                            const char *mid_bone, const char *end_bone,
                            Vec3 target, Vec3 pole_vector);

#endif // UNIFIED_ANIMATION_H
