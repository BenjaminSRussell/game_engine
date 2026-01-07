#include "character/animation/ik/fabrik_solver.h"
#include <include/math/math.h>

void fabrik_init() {}

void fabrik_solve(void *chain, float target[3], int iterations) {
    // FABRIK (Forward And Backward Reaching Inverse Kinematics)
    // Forward pass: reach toward target
    // Backward pass: reach back to root
}

void fabrik_add_constraint(void *chain, int joint_index, float min_angle, float max_angle) {}
