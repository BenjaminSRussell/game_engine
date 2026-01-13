#include "include/common.h"

void fabrik_init(void) {}

// Stub for fabrik_solve matching the stub signature
void fabrik_solve_stub(void *chain, float target[3], int iterations) {
  (void)chain;
  (void)target;
  (void)iterations;
}

void fabrik_add_constraint(void *chain, int joint_index, float min_angle,
                           float max_angle) {
  (void)chain;
  (void)joint_index;
  (void)min_angle;
  (void)max_angle;
}
