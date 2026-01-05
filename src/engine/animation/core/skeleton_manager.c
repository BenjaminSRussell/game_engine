/**
 * @file skeleton_manager.c
 * @brief Bone Hierarchy Management.
 *
 * Handles Skeleton definition, Bind Poses, and Global Transform computation.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <animation/core/skeleton_manager.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

#define MAX_BONES 256

typedef struct Bone {
  char name[32];
  int parent_idx;           // -1 if root
  mat4 inverse_bind_matrix; // Used for skinning
  mat4 local_transform;     // Current local pose
  mat4 global_transform;    // Current model-space pose
} Bone;

typedef struct Skeleton {
  Bone bones[MAX_BONES];
  int bone_count;
} Skeleton;

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

/**
 * @brief Updates global transforms from local transforms.
 * Must be called after animation sampling and before rendering.
 */
void skeleton_update_globals(Skeleton *skel) {
  for (int i = 0; i < skel->bone_count; i++) {
    Bone *b = &skel->bones[i];

    if (b->parent_idx == -1) {
      b->global_transform = b->local_transform;
    } else {
      Bone *parent = &skel->bones[b->parent_idx];
      // Global = ParentGlobal * Local
      b->global_transform =
          mat4_mul(parent->global_transform, b->local_transform);
    }
  }
}

/**
 * @brief Generates skinning matrices for the GPU.
 */
void skeleton_get_skinning_matrices(Skeleton *skel, mat4 *out_matrices) {
  for (int i = 0; i < skel->bone_count; i++) {
    Bone *b = &skel->bones[i];
    // FinalMatrix = GlobalTransform * InverseBindMatrix
    // Transforms vertex from Bind Pose -> Local Bone Space -> Current Model
    // Space
    out_matrices[i] = mat4_mul(b->global_transform, b->inverse_bind_matrix);
  }
}

int skeleton_find_bone_idx(Skeleton *skel, const char *name) {
  // Basic linear search (optimize with hashmap if needed)
  for (int i = 0; i < skel->bone_count; i++) {
    // if (strcmp(...) == 0) return i;
  }
  return -1;
}
