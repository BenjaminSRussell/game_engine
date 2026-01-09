/**
 * @file skeleton_manager.h
 * @brief Bone hierarchy management for skeletal animation.
 */
#ifndef ANIMATION_CORE_SKELETON_MANAGER_H
#define ANIMATION_CORE_SKELETON_MANAGER_H

#include <math/mat4.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct Skeleton Skeleton;
typedef struct Bone Bone;

/**
 * Update global transforms from local transforms.
 * Must be called after animation sampling and before rendering.
 */
void skeleton_update_globals(Skeleton *skel);

/**
 * Generate skinning matrices for the GPU.
 */
void skeleton_get_skinning_matrices(Skeleton *skel, Mat4 *out_matrices);

/**
 * Find bone index by name.
 */
int skeleton_find_bone_idx(Skeleton *skel, const char *name);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_CORE_SKELETON_MANAGER_H
