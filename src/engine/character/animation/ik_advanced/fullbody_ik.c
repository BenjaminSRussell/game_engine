#include "include/common.h"

void fullbody_ik_init(void) {}

void fullbody_ik_solve(void *skeleton, void *targets, int target_count) {
  // Full-body IK solving multiple end effectors
  (void)skeleton;
  (void)targets;
  (void)target_count;
}

void fullbody_ik_add_target(void *solver, int bone_index, float position[3]) {
  (void)solver;
  (void)bone_index;
  (void)position;
}
