#include "../Public/unified_animation.h"
#include "animation_types.h"
#include <string.h>

// Helpers
static Mat4 mat4_trs(Vec3 t, Quat r, Vec3 s) {
  Mat4 m = mat4_identity();
  Mat4 t_mat = mat4_translate(t);
  Mat4 r_mat = quat_to_mat4(r);
  Mat4 s_mat = mat4_scale(s);

  m = mat4_mul(t_mat, r_mat);
  m = mat4_mul(m, s_mat);
  return m;
}

AnimPose *anim_pose_create(AnimSystem *system) {
  (void)system;
  AnimPose *pose = UNIFIED_ALLOC(sizeof(AnimPose));
  if (pose) {
    pose->bone_count = 0;
  }
  return pose;
}

void anim_pose_destroy(AnimSystem *system, AnimPose *pose) {
  (void)system;
  if (pose)
    UNIFIED_FREE(pose);
}

void anim_sample_clip(AnimClip *clip, f32 time, AnimPose *out_pose) {
  if (!clip || !out_pose)
    return;

  for (u32 i = 0; i < clip->channel_count; i++) {
    AnimChannel *channel = &clip->channels[i];
    if (channel->keyframe_count == 0)
      continue;

    u32 k0 = 0, k1 = 0;
    if (time <= channel->keyframes[0].time) {
      k0 = k1 = 0;
    } else if (time >= channel->keyframes[channel->keyframe_count - 1].time) {
      k0 = k1 = channel->keyframe_count - 1;
    } else {
      for (u32 k = 0; k < channel->keyframe_count - 1; k++) {
        if (time < channel->keyframes[k + 1].time) {
          k0 = k;
          k1 = k + 1;
          break;
        }
      }
    }

    i32 bone_idx = channel->bone_index;
    if (bone_idx >= MAX_BONES)
      continue;

    if (k0 == k1) {
      out_pose->positions[bone_idx] = channel->keyframes[k0].position;
      out_pose->rotations[bone_idx] = channel->keyframes[k0].rotation;
      out_pose->scales[bone_idx] = channel->keyframes[k0].scale;
    } else {
      AnimKeyframe *kf0 = &channel->keyframes[k0];
      AnimKeyframe *kf1 = &channel->keyframes[k1];
      f32 t = (time - kf0->time) / (kf1->time - kf0->time);

      out_pose->positions[bone_idx] =
          vec3_lerp(kf0->position, kf1->position, t);
      out_pose->rotations[bone_idx] =
          quat_slerp(kf0->rotation, kf1->rotation, t);
      out_pose->scales[bone_idx] = vec3_lerp(kf0->scale, kf1->scale, t);
    }

    if (bone_idx >= (i32)out_pose->bone_count) {
      out_pose->bone_count = bone_idx + 1;
    }
  }
}

void anim_apply_pose(AnimSkeleton *skeleton, const AnimPose *pose) {
  if (!skeleton || !pose)
    return;

  Mat4 local_matrices[MAX_BONES];

  // Convert pose TRS to matrices
  for (u32 i = 0; i < skeleton->bone_count; i++) {
    if (i < pose->bone_count) {
      local_matrices[i] =
          mat4_trs(pose->positions[i], pose->rotations[i], pose->scales[i]);
    } else {
      local_matrices[i] = mat4_identity(); // Fallback
    }
  }

  // Rebuild global transforms
  for (u32 i = 0; i < skeleton->bone_count; i++) {
    i32 parent = skeleton->bones[i].parent_index;
    if (parent == -1) {
      skeleton->global_transforms[i] = local_matrices[i];
    } else {
      skeleton->global_transforms[i] =
          mat4_mul(skeleton->global_transforms[parent], local_matrices[i]);
    }
  }
}
