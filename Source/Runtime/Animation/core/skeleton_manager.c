/**
 * @file skeleton_manager.c
 * @brief Bone Hierarchy Management.
 *
 * Handles Skeleton definition, Bind Poses, and Global Transform computation.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <animation/core/skeleton_manager.h>
#include <string.h>

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

/**
 * @brief Updates global transforms from local transforms.
 * Must be called after animation sampling and before rendering.
 */
void skeleton_update_globals(Skeleton *skel) {
  if (!skel || !skel->bones || !skel->global_transforms) return;

  for (u32 i = 0; i < skel->bone_count; i++) {
    Bone *b = &skel->bones[i];

    if (b->parent_index == -1) {
      if (i < skel->bone_count) {
          skel->global_transforms[i] = b->local_transform;
      }
    } else {
      if (b->parent_index < (i32)skel->bone_count && b->parent_index >= 0) {
          // Global = ParentGlobal * Local
          skel->global_transforms[i] =
              mat4_mul(skel->global_transforms[b->parent_index], b->local_transform);
      }
    }
  }
}

/**
 * @brief Generates skinning matrices for the GPU.
 */
void skeleton_get_skinning_matrices(Skeleton *skel, Mat4 *out_matrices) {
  if (!skel || !out_matrices) return;

  for (u32 i = 0; i < skel->bone_count; i++) {
    Bone *b = &skel->bones[i];
    // FinalMatrix = GlobalTransform * InverseBindMatrix
    // Transforms vertex from Bind Pose -> Local Bone Space -> Current Model Space
    out_matrices[i] = mat4_mul(skel->global_transforms[i], b->inverse_bind_pose);
  }
}

int skeleton_find_bone_idx(Skeleton *skel, const char *name) {
  if (!skel || !name) return -1;
  // Basic linear search (optimize with hashmap if needed)
  for (u32 i = 0; i < skel->bone_count; i++) {
    if (strcmp(skel->bones[i].name, name) == 0) return (int)i;
  }
  return -1;
}
