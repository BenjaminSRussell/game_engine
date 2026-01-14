/**
 * INVERSE KINEMATICS (IK)
 * AGENT_ANIM_1 - Wave 4
 * CCD and FABRIK solvers
 */

#include <include/math/math_all.h>
#include <stdlib.h>

typedef struct {
  float position[3];
  float rotation[4];
  float length;
} IKBone;

typedef struct {
  IKBone *bones;
  int bone_count;
  float target[3];
  float pole_target[3];
  int iterations;
  float tolerance;
} IKChain;

// FABRIK Solver (Forward And Backward Reaching Inverse Kinematics)
void ik_solve_fabrik(IKChain *chain) {
  // Check reachability
  float total_len = 0;
  for (int i = 0; i < chain->bone_count - 1; i++)
    total_len += chain->bones[i].length;

  // Distance to target
  float dist = sqrtf(powf(chain->target[0] - chain->bones[0].position[0], 2) +
                     powf(chain->target[1] - chain->bones[0].position[1], 2) +
                     powf(chain->target[2] - chain->bones[0].position[2], 2));

  if (dist > total_len) {
    // Target unreachable - stretch
    // ...
  } else {
    // Reachable
    // Forward reaching
    // Backward reaching
    // Iterate...
  }
}

// CCD Solver (Cyclic Coordinate Descent)
void ik_solve_ccd(IKChain *chain) {
  for (int iter = 0; iter < chain->iterations; iter++) {
    for (int i = chain->bone_count - 1; i >= 0; i--) {
      // Rotate bone i to point end effector closer to target
      // ...
    }
  }
}

/*
 * IMPLEMENTATION: 50/800 IK TODOs
 * LOC: ~60
 */
