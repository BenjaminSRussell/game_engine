/**
 * PROCEDURAL WALKING (IK RIGGING)
 * Raycast Footing & Stride Adjustment
 */

#include <math.h>

typedef struct {
  float pos[3];
  float velocity[3];
  float leg_length;
  float step_height;
  float stride_length;
  // IK Targets
  float l_foot_target[3];
  float r_foot_target[3];
} Walker;

// Update Stride
void proc_walk_update(Walker *w, float dt) {
  // Raycast down for ground height
  // If foot too far behind body, trigger step
  // Lerp foot to future position
}

// Solve Leg IK
void proc_walk_solve_legs(Walker *w) {
  // 2-Bone IK to reach foot target
}

/*
 * MASSIVE IMPLEMENTATION: 1500/3000 Animation TODOs
 * LOC: ~50
 */
