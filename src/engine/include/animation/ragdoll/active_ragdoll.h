#ifndef ACTIVE_RAGDOLL_H
#define ACTIVE_RAGDOLL_H

#include "core/types.h"

void active_ragdoll_init(void);
void active_ragdoll_create(void *skeleton, void *physics_world);
void active_ragdoll_drive_to_pose(void *ragdoll, void *target_pose,
                                  float strength);
void active_ragdoll_update(void *ragdoll, float dt);

#endif // ACTIVE_RAGDOLL_H
