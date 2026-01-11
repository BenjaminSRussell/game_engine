#include "include/common.h"

void ragdoll_init(void) {}

void ragdoll_update(void *ragdoll, float dt) {
  // Simulate physics
  (void)ragdoll;
  (void)dt;
}

void ragdoll_apply_force(void *ragdoll, int bone_index, float force[3]) {
  (void)ragdoll;
  (void)bone_index;
  (void)force;
}

void ragdoll_blend_to_anim(void *ragdoll, void *anim_pose, float blend_factor) {
  (void)ragdoll;
  (void)anim_pose;
  (void)blend_factor;
}
