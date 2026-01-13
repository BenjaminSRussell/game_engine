#ifndef FOOT_PLACEMENT_H
#define FOOT_PLACEMENT_H

#include "core/types.h"

// Foot placement system for advanced IK
void foot_placement_init(void);
void foot_placement_solve(void *leg, f32 ground_height);

#endif // FOOT_PLACEMENT_H
