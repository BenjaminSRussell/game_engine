#ifndef PARALLEL_ANIMATION_H
#define PARALLEL_ANIMATION_H

#include "core/types.h"
#include "ecs/components/animation_components.h"

/**
 * Parallel Animation Processing
 *
 * Uses worker threads for:
 * - Pose sampling (Clip evaluation)
 * - Pose blending (Interpolation)
 * - Matrix generation (Skinning)
 */

/**
 * Initialize parallel animation
 */
void parallel_animation_init(u32 thread_count);

/**
 * Batch context for parallel evaluation
 */
typedef struct {
  AnimatorComponent *animators;
  SkeletonComponent *skeletons;
  u32 count;
} AnimationBatch;

/**
 * Evaluate poses in parallel
 *
 * @param animators Array of animators
 * @param skeletons Array of skeletons
 * @param count Number of entities
 */
void parallel_evaluate_poses(AnimatorComponent *animators,
                             SkeletonComponent *skeletons, u32 count);

#endif // PARALLEL_ANIMATION_H
