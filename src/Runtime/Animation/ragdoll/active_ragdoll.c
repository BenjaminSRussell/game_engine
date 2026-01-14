#include <animation/ragdoll/active_ragdoll.h>

void active_ragdoll_init() {}

void active_ragdoll_create(void *skeleton, void *physics_world) {}

void active_ragdoll_drive_to_pose(void *ragdoll, void *target_pose,
                                  float strength) {
  // Use motors to drive ragdoll towards animated pose
}

void active_ragdoll_update(void *ragdoll, float dt) {}
