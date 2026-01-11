#include "include/common.h"
#include "include/math/math.h"

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
void proc_walk_update_impl(Walker *w, float dt) {
  // Raycast down for ground height
  // If foot too far behind body, trigger step
  // Lerp foot to future position
  (void)w;
  (void)dt;
}

// Solve Leg IK
void proc_walk_solve_legs(Walker *w) {
  // 2-Bone IK to reach foot target
  (void)w;
}
