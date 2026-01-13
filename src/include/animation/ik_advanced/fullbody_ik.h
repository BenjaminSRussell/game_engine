#ifndef FULLBODY_IK_H
#define FULLBODY_IK_H

#include "core/types.h"

// Full body IK system
void fullbody_ik_init(void);
void fullbody_ik_solve(void *skeleton, float *target_positions, int target_count);

#endif // FULLBODY_IK_H
