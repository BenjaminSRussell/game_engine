#include "../Public/unified_animation.h"
#include "animation_types.h"
#include <math/mat4.h>
#include <string.h>

AnimSkeleton *anim_skeleton_create(AnimSystem *system,
                                   const AnimBoneDef *bone_defs,
                                   u32 bone_count) {
  if (!system || !bone_defs || bone_count == 0 || bone_count > MAX_BONES)
    return NULL;

  AnimSkeleton *skeleton = UNIFIED_ALLOC(sizeof(AnimSkeleton));
  if (!skeleton)
    return NULL;

  skeleton->bone_count = bone_count;
  skeleton->bones = UNIFIED_ALLOC(sizeof(AnimBone) * bone_count);
  skeleton->global_transforms = UNIFIED_ALLOC(sizeof(Mat4) * bone_count);

  if (!skeleton->bones || !skeleton->global_transforms) {
    if (skeleton->bones)
      UNIFIED_FREE(skeleton->bones);
    if (skeleton->global_transforms)
      UNIFIED_FREE(skeleton->global_transforms);
    UNIFIED_FREE(skeleton);
    return NULL;
  }

  for (u32 i = 0; i < bone_count; i++) {
    const AnimBoneDef *def = &bone_defs[i];

    // Copy name
    strncpy(skeleton->bones[i].name, def->name, MAX_NAME_LEN - 1);
    skeleton->bones[i].name[MAX_NAME_LEN - 1] = '\0';

    skeleton->bones[i].parent_index = def->parent_index;

    // Calculate bind pose
    Mat4 t = mat4_translate(def->local_pos);
    Mat4 r = quat_to_mat4(def->local_rot);
    Mat4 s = mat4_scale(def->local_scale);

    // TRS
    Mat4 local = mat4_mul(mat4_mul(t, r), s);
    skeleton->bones[i].local_bind_pose = local;

    // Invert for inverse bind pose
    skeleton->bones[i].inverse_bind_pose = mat4_inverse(local);

    // Initialize globals to identity for safety
    skeleton->global_transforms[i] = mat4_identity();
  }

  // Correctly calculate inverse bind pose (global)
  for (u32 i = 0; i < bone_count; i++) {
    i32 parent = skeleton->bones[i].parent_index;
    if (parent == -1) {
      skeleton->global_transforms[i] = skeleton->bones[i].local_bind_pose;
    } else {
      skeleton->global_transforms[i] =
          mat4_mul(skeleton->global_transforms[parent],
                   skeleton->bones[i].local_bind_pose);
    }
  }
  for (u32 i = 0; i < bone_count; i++) {
    skeleton->bones[i].inverse_bind_pose =
        mat4_inverse(skeleton->global_transforms[i]);
  }

  anim_skeleton_reset(skeleton);
  return skeleton;
}

void anim_skeleton_destroy(AnimSystem *system, AnimSkeleton *skeleton) {
  (void)system;
  if (!skeleton)
    return;
  UNIFIED_FREE(skeleton->bones);
  UNIFIED_FREE(skeleton->global_transforms);
  UNIFIED_FREE(skeleton);
}

void anim_skeleton_reset(AnimSkeleton *skeleton) {
  if (!skeleton)
    return;
  skeleton->current_clip = NULL;
  skeleton->current_time = 0.0f;
  skeleton->is_playing = false;
  skeleton->is_looping = false;

  for (u32 i = 0; i < skeleton->bone_count; i++) {
    i32 parent = skeleton->bones[i].parent_index;
    if (parent == -1) {
      skeleton->global_transforms[i] = skeleton->bones[i].local_bind_pose;
    } else {
      skeleton->global_transforms[i] =
          mat4_mul(skeleton->global_transforms[parent],
                   skeleton->bones[i].local_bind_pose);
    }
  }
}

void anim_skeleton_play(AnimSkeleton *skeleton, AnimClip *clip, bool loop) {
  if (!skeleton || !clip)
    return;
  skeleton->current_clip = clip;
  skeleton->current_time = 0.0f;
  skeleton->is_playing = true;
  skeleton->is_looping = loop;
}

void anim_skeleton_stop(AnimSkeleton *skeleton) {
  if (!skeleton)
    return;
  skeleton->is_playing = false;
}

void anim_skeleton_get_matrix_palette(AnimSkeleton *skeleton,
                                      Mat4 *out_matrices, u32 max_matrices) {
  if (!skeleton || !out_matrices)
    return;

  u32 count =
      skeleton->bone_count < max_matrices ? skeleton->bone_count : max_matrices;

  for (u32 i = 0; i < count; i++) {
    out_matrices[i] = mat4_mul(skeleton->global_transforms[i],
                               skeleton->bones[i].inverse_bind_pose);
  }
}
