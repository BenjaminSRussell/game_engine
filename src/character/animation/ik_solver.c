#include <character/animation/animation_system.h>
#include "math/math.h"

// Simple CCD (Cyclic Coordinate Descent) IK Solver
void animation_solve_ik(Skeleton *skeleton, IKChain *chain) {
  if (!skeleton || !chain || !chain->active)
    return;

  // Iterate
  for (u32 iter = 0; iter < chain->iteration_count; iter++) {
    // Forward reach (simplified)
    // Usually we iterate from end-effector up to root
    i32 bone_idx = chain->end_effector_bone_index;

    while (bone_idx != chain->root_bone_index && bone_idx >= 0) {
      // Calculate vector from bone to end effector
      // Calculate vector from bone to target
      // Rotate bone to align

      // For stubs: just logging or simple logic
      bone_idx = skeleton->bones[bone_idx].parent_index;
    }

    // Check distance to target
    // if close enough, break
  }
}
