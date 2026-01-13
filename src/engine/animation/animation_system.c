#include "../include/character/animation/animation_system.h"
#include "../include/math/mat4.h"
#include "../include/math/quat.h"
#include "../include/math/vec3.h"
#include "engine/include/core/logger.h"
#include <core/memory.h>
#include <stdlib.h>
#include <string.h>

// Global state (internal)
static struct {
  bool initialized;
} anim_sys_state;

void animation_system_init(void) {
  if (anim_sys_state.initialized)
    return;
  anim_sys_state.initialized = true;
  LOG_INFO("Animation System Initialized");
}

void animation_system_update(f32 dt) {
  if (!anim_sys_state.initialized)
    return;
  // Implementation placeholder
}

void animation_system_shutdown(void) {
  anim_sys_state.initialized = false;
  LOG_INFO("Animation System Shutdown");
}

Skeleton *animation_create_skeleton(u32 bone_count) {
  if (bone_count > MAX_BONES) {
    LOG_ERROR("Bone count %u exceeds maximum %u", bone_count, MAX_BONES);
    return NULL;
  }

  Skeleton *skeleton = (Skeleton *)calloc(1, sizeof(Skeleton));
  if (skeleton) {
    skeleton->bone_count = bone_count;
    // Initialize matrices to identity
    for (u32 i = 0; i < bone_count; i++) {
      skeleton->global_transforms[i] = mat4_identity();
      skeleton->bones[i].local_bind_pose = mat4_identity();
      skeleton->bones[i].inverse_bind_pose = mat4_identity();
      skeleton->bones[i].parent_index = -1;
    }
  }
  return skeleton;
}

void animation_destroy_skeleton(Skeleton *skeleton) {
  if (skeleton) {
    free(skeleton);
  }
}

void animation_sample_clip(AnimationClip *clip, f32 time, Pose *out_pose) {
  if (!clip || !out_pose)
    return;

  out_pose->bone_count = MAX_BONES;

  for (u32 i = 0; i < MAX_BONES; i++) {
    out_pose->positions[i] = vec3(0, 0, 0);
    out_pose->rotations[i] = quat_identity();
    out_pose->scales[i] = vec3(1, 1, 1);
  }

  for (u32 i = 0; i < clip->channel_count; i++) {
    AnimationChannel *channel = &clip->channels[i];
    if (channel->bone_index < 0 || (u32)channel->bone_index >= MAX_BONES)
      continue;

    u32 bone_idx = (u32)channel->bone_index;
    if (channel->keyframe_count == 0)
      continue;

    u32 k0 = 0;
    u32 k1 = 0;

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

    if (k0 == k1) {
      out_pose->positions[bone_idx] = channel->keyframes[k0].position;
      out_pose->rotations[bone_idx] = channel->keyframes[k0].rotation;
      out_pose->scales[bone_idx] = channel->keyframes[k0].scale;
    } else {
      AnimationKeyframe *kf0 = &channel->keyframes[k0];
      AnimationKeyframe *kf1 = &channel->keyframes[k1];
      f32 t = (time - kf0->time) / (kf1->time - kf0->time);

      out_pose->positions[bone_idx] =
          vec3_lerp(kf0->position, kf1->position, t);
      out_pose->rotations[bone_idx] =
          quat_slerp(kf0->rotation, kf1->rotation, t);
      out_pose->scales[bone_idx] = vec3_lerp(kf0->scale, kf1->scale, t);
    }
  }
}

void animation_blend_poses(const Pose *a, const Pose *b, f32 weight,
                           Pose *out_pose) {
  if (!a || !b || !out_pose)
    return;

  out_pose->bone_count = a->bone_count;
  for (u32 i = 0; i < out_pose->bone_count; i++) {
    out_pose->positions[i] =
        vec3_lerp(a->positions[i], b->positions[i], weight);
    out_pose->rotations[i] =
        quat_slerp(a->rotations[i], b->rotations[i], weight);
    out_pose->scales[i] = vec3_lerp(a->scales[i], b->scales[i], weight);
  }
}

static Mat4 mat4_trs(Vec3 t, Quat r, Vec3 s) {
  Mat4 m = mat4_identity();
  Mat4 t_mat = mat4_translate(t);
  Mat4 r_mat = quat_to_mat4(r);
  Mat4 s_mat = mat4_scale(s);

  m = mat4_mul(t_mat, r_mat);
  m = mat4_mul(m, s_mat);
  return m;
}

void animation_apply_pose_to_skeleton(const Pose *pose, Skeleton *skeleton) {
  if (!pose || !skeleton)
    return;

  Mat4 local_matrices[MAX_BONES];
  for (u32 i = 0; i < skeleton->bone_count; i++) {
    local_matrices[i] =
        mat4_trs(pose->positions[i], pose->rotations[i], pose->scales[i]);
  }

  for (u32 i = 0; i < skeleton->bone_count; i++) {
    i32 parent = skeleton->bones[i].parent_index;
    if (parent == -1) {
      skeleton->global_transforms[i] = local_matrices[i];
    } else {
      skeleton->global_transforms[i] =
          mat4_mul(skeleton->global_transforms[parent], local_matrices[i]);
    }
    skeleton->global_transforms[i] = mat4_mul(
        skeleton->global_transforms[i], skeleton->bones[i].inverse_bind_pose);
  }
}

void animation_update_skeleton(Skeleton *skeleton, AnimationState *state) {
  if (!skeleton || !state || !state->current_clip)
    return;

  if (state->is_playing) {
    if (state->current_time > state->current_clip->duration) {
      if (state->is_looping) {
        state->current_time =
            fmodf(state->current_time, state->current_clip->duration);
      } else {
        state->current_time = state->current_clip->duration;
        state->is_playing = false;
      }
    }
  }

  Pose local_pose;
  animation_sample_clip(state->current_clip, state->current_time, &local_pose);
  animation_apply_pose_to_skeleton(&local_pose, skeleton);
}
