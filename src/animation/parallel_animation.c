#include "animation/parallel_animation.h"
#include "core/thread_pool.h"
#include "core/threading/parallel_utils.h"
#include "engine/include/core/logger.h"

void parallel_animation_init(u32 thread_count) {
  if (!thread_pool_get_global()) {
    thread_pool_init(thread_count);
  }
  LOG_INFO("[Animation] Parallel processing initialized");
}

static void animation_batch_job(u32 index, void *user_data) {
  AnimationBatch *batch = (AnimationBatch *)user_data;

  // In real ECS, data might be SOA or separate arrays,
  // here we assume parallel_for provides an index into contiguous component
  // arrays (requires Archetype chunk processing in real impl)

  // Placeholder logic for single entity[index] processing:
  /*
  AnimatorComponent* anim = &batch->animators[index];
  SkeletonComponent* skel = &batch->skeletons[index];

  if (anim->is_playing) {
      // 1. Evaluate Clip(anim->time) -> LocalPose
      // 2. Compute Global Matrices -> skel->bone_matrices
  }
  */
}

void parallel_evaluate_poses(AnimatorComponent *animators,
                             SkeletonComponent *skeletons, u32 count) {
  if (count == 0 || !animators || !skeletons)
    return;

  AnimationBatch batch = {
      .animators = animators, .skeletons = skeletons, .count = count};

  // Process in batches
  parallel_for(0, count, animation_batch_job, &batch);
}
