/**
 * @file animation_api.h
 * @brief Animation subsystem API
 */

#ifndef VOXELFORGE_ANIMATION_API_H
#define VOXELFORGE_ANIMATION_API_H

#include "Core/Public/Math/Matrix/mat4.h"
#include "Core/Public/Math/Quaternion/quat.h"
#include "Core/Public/Math/Vector/vec3.h"
#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Forward Declarations
// ============================================================================

typedef struct Skeleton Skeleton;
typedef struct AnimationClip AnimationClip;
typedef struct AnimationState AnimationState;
typedef struct AnimationController AnimationController;
typedef struct AnimationPose AnimationPose;
typedef struct BlendSpace1D BlendSpace1D;
typedef struct BlendSpace2D BlendSpace2D;

// ============================================================================
// Skeleton
// ============================================================================

typedef struct BoneInfo {
  char name[64];
  i32 parent_index; // -1 for root
  Mat4 local_bind_pose;
  Mat4 inverse_bind_pose;
} BoneInfo;

typedef struct SkeletonDesc {
  BoneInfo *bones;
  u32 bone_count;
} SkeletonDesc;

VF_API Skeleton *skeleton_create(const SkeletonDesc *desc);
VF_API void skeleton_destroy(Skeleton *skeleton);
VF_API u32 skeleton_get_bone_count(Skeleton *skeleton);
VF_API i32 skeleton_find_bone(Skeleton *skeleton, const char *name);
VF_API void skeleton_get_bind_pose(Skeleton *skeleton, AnimationPose *out_pose);

// ============================================================================
// Animation Clip
// ============================================================================

typedef struct BoneKeyframe {
  f32 time;
  Vec3 translation;
  Quat rotation;
  Vec3 scale;
} BoneKeyframe;

typedef struct BoneTrack {
  u32 bone_index;
  BoneKeyframe *keyframes;
  u32 keyframe_count;
} BoneTrack;

typedef struct AnimationClipDesc {
  const char *name;
  f32 duration;
  f32 ticks_per_second;
  BoneTrack *tracks;
  u32 track_count;
  b8 looping;
} AnimationClipDesc;

VF_API AnimationClip *animation_clip_create(const AnimationClipDesc *desc);
VF_API void animation_clip_destroy(AnimationClip *clip);
VF_API f32 animation_clip_get_duration(AnimationClip *clip);
VF_API void animation_clip_sample(AnimationClip *clip, f32 time,
                                  AnimationPose *out_pose);

// ============================================================================
// Animation Pose
// ============================================================================

VF_API AnimationPose *pose_create(Skeleton *skeleton);
VF_API void pose_destroy(AnimationPose *pose);
VF_API void pose_copy(AnimationPose *dst, const AnimationPose *src);
VF_API void pose_blend(AnimationPose *out, const AnimationPose *a,
                       const AnimationPose *b, f32 t);
VF_API void pose_add(AnimationPose *out, const AnimationPose *base,
                     const AnimationPose *additive, f32 weight);
VF_API void pose_compute_global(AnimationPose *pose, Skeleton *skeleton);
VF_API void pose_get_bone_transform(AnimationPose *pose, u32 bone_index,
                                    Vec3 *pos, Quat *rot, Vec3 *scale);
VF_API void pose_set_bone_transform(AnimationPose *pose, u32 bone_index,
                                    Vec3 pos, Quat rot, Vec3 scale);

// ============================================================================
// Animation Controller
// ============================================================================

VF_API AnimationController *anim_controller_create(Skeleton *skeleton);
VF_API void anim_controller_destroy(AnimationController *controller);
VF_API void anim_controller_update(AnimationController *controller,
                                   f32 delta_time);
VF_API void anim_controller_play(AnimationController *controller,
                                 AnimationClip *clip, f32 blend_time);
VF_API void anim_controller_crossfade(AnimationController *controller,
                                      AnimationClip *clip, f32 fade_time);
VF_API void anim_controller_set_parameter(AnimationController *controller,
                                          const char *name, f32 value);
VF_API f32 anim_controller_get_parameter(AnimationController *controller,
                                         const char *name);
VF_API AnimationPose *anim_controller_get_pose(AnimationController *controller);
VF_API void anim_controller_get_bone_matrices(AnimationController *controller,
                                              Mat4 *out_matrices,
                                              u32 max_bones);

// ============================================================================
// Blend Spaces
// ============================================================================

VF_API BlendSpace1D *blendspace_1d_create(void);
VF_API void blendspace_1d_destroy(BlendSpace1D *bs);
VF_API void blendspace_1d_add_clip(BlendSpace1D *bs, AnimationClip *clip,
                                   f32 position);
VF_API void blendspace_1d_sample(BlendSpace1D *bs, f32 value, f32 time,
                                 AnimationPose *out_pose);

VF_API BlendSpace2D *blendspace_2d_create(void);
VF_API void blendspace_2d_destroy(BlendSpace2D *bs);
VF_API void blendspace_2d_add_clip(BlendSpace2D *bs, AnimationClip *clip, f32 x,
                                   f32 y);
VF_API void blendspace_2d_sample(BlendSpace2D *bs, f32 x, f32 y, f32 time,
                                 AnimationPose *out_pose);

// ============================================================================
// IK (Inverse Kinematics)
// ============================================================================

typedef struct IKTarget {
  u32 bone_index;
  Vec3 target_position;
  Quat target_rotation;
  f32 weight;
} IKTarget;

VF_API void ik_solve_two_bone(AnimationPose *pose, Skeleton *skeleton,
                              u32 root_bone, u32 mid_bone, u32 end_bone,
                              Vec3 target, Vec3 pole);
VF_API void ik_solve_fabrik(AnimationPose *pose, Skeleton *skeleton, u32 *chain,
                            u32 chain_length, Vec3 target, u32 iterations);
VF_API void ik_look_at(AnimationPose *pose, Skeleton *skeleton, u32 bone_index,
                       Vec3 target);

// ============================================================================
// Animation System
// ============================================================================

VF_API VF_Result animation_init(void);
VF_API void animation_shutdown(void);
VF_API void animation_update(f32 delta_time);

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_ANIMATION_API_H
