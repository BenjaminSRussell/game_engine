/**
 * =================================================================================================
 *                          ANIMATION BLENDING
 *                          Phase 3: Animation System
 * =================================================================================================
 */

#include <animation/animation_system.h>
#include <math/quat.h>
#include <math/vec3.h>
#include <string.h> // for memcpy

void animation_sample_clip(AnimationClip *clip, f32 time, Pose *out_pose) {
  if (!clip || !out_pose)
    return;

  // Handle looping/duration
  if (clip->duration > 0.0f) {
    // Simple mod for looping
    // TODO: Use AnimationState for loop control
    time = fmodf(time, clip->duration);
  }

  out_pose->bone_count = clip->channel_count; // Assuming 1 channel per bone map

  // Default pose if channels missing?
  // Ideally we initialized out_pose to bind pose before calling this.

  for (u32 i = 0; i < clip->channel_count; i++) {
    AnimationChannel *channel = &clip->channels[i];
    i32 bone_idx = channel->bone_index;

    if (bone_idx < 0 || bone_idx >= MAX_BONES)
      continue;

    // Find keyframe based on time
    // Linear search for now (optimize with binary search)
    if (channel->keyframe_count == 0)
      continue;

    u32 frame_idx = 0;
    for (u32 k = 0; k < channel->keyframe_count - 1; k++) {
      if (time < channel->keyframes[k + 1].time) {
        frame_idx = k;
        break;
      }
    }

    // Simple interpolation between frame_idx and frame_idx+1
    AnimationKeyframe *k0 = &channel->keyframes[frame_idx];
    AnimationKeyframe *k1 =
        &channel->keyframes[(frame_idx + 1) < channel->keyframe_count
                                ? frame_idx + 1
                                : frame_idx];

    float t = 0.0f;
    float dt = k1->time - k0->time;
    if (dt > 0.0001f) {
      t = (time - k0->time) / dt;
      if (t < 0.0f)
        t = 0.0f;
      if (t > 1.0f)
        t = 1.0f;
    }

    out_pose->positions[bone_idx] = vec3_lerp(k0->position, k1->position, t);
    out_pose->rotations[bone_idx] = quat_slerp(k0->rotation, k1->rotation, t);
    out_pose->scales[bone_idx] = vec3_lerp(k0->scale, k1->scale, t);
  }
}

void animation_blend_poses(const Pose *a, const Pose *b, f32 weight,
                           Pose *out_pose) {
  if (!a || !b || !out_pose)
    return;

  u32 count = a->bone_count < b->bone_count ? a->bone_count : b->bone_count;
  out_pose->bone_count = count;

  if (weight <= 0.0f) {
    *out_pose = *a;
    return;
  }
  if (weight >= 1.0f) {
    *out_pose = *b;
    return;
  }

  for (u32 i = 0; i < count; i++) {
    out_pose->positions[i] =
        vec3_lerp(a->positions[i], b->positions[i], weight);
    out_pose->rotations[i] =
        quat_slerp(a->rotations[i], b->rotations[i], weight);
    out_pose->scales[i] = vec3_lerp(a->scales[i], b->scales[i], weight);
  }
}

void animation_apply_pose_to_skeleton(const Pose *pose, Skeleton *skeleton) {
  if (!pose || !skeleton)
    return;

  // Recompute global transforms from local pose
  // This assumes bones are topologically sorted (parent < child)
  for (u32 i = 0; i < skeleton->bone_count; i++) {
    Bone *bone = &skeleton->bones[i];

    // Local Matrix
    Mat4 T = mat4_translate(pose->positions[i]);
    Mat4 R = quat_to_mat4(pose->rotations[i]);
    Mat4 S = mat4_scale(pose->scales[i]);

    Mat4 local = mat4_mul(T, mat4_mul(R, S));

    if (bone->parent_index >= 0) {
      skeleton->global_transforms[i] =
          mat4_mul(skeleton->global_transforms[bone->parent_index], local);
    } else {
      skeleton->global_transforms[i] = local;
    }
  }
}
