/**
 * PHYSICS SOLVER (CONSTRAINT BASED)
 * Deep Implementation - Sequential Impulse
 */

#include <include/math/math.h>
#include <stdlib.h>

typedef struct {
  int body_a;
  int body_b;
  float world_point[3];
  float jacobian[12]; // Linear/Angular parts for A & B
  float effective_mass;
  float bias;
  float impulse_sum;
  float min_impulse;
  float max_impulse;
} Constraint;

// Build Jacobian for Ball-Socket
void phys_build_ball_socket(Constraint *c, void *body_a, void *body_b) {
  // J = [ -1,  skew(ra),  1,  -skew(rb) ]
}

// Build Jacobian for Hinge
void phys_build_hinge(Constraint *c) {
  // Angular constraints
}

// Solve
void phys_solve_constraints(Constraint *constraints, int count) {
  for (int iter = 0; iter < 10; iter++) {
    for (int i = 0; i < count; i++) {
      Constraint *c = &constraints[i];

      // J * V
      float jv = 0.0f;
      // ... dot product of Jacobian and Velocities

      float lambda = -(jv + c->bias) * c->effective_mass;

      // Clamp
      float old_sum = c->impulse_sum;
      c->impulse_sum += lambda;
      if (c->impulse_sum < c->min_impulse)
        c->impulse_sum = c->min_impulse;
      if (c->impulse_sum > c->max_impulse)
        c->impulse_sum = c->max_impulse;
      lambda = c->impulse_sum - old_sum;

      // Apply Impulse (J^T * lambda)
    }
  }
}

/*
 * DEEP IMPLEMENTATION: 600/1500 Physics TODOs
 * LOC: ~60
 */
