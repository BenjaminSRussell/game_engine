#ifndef PARALLEL_PHYSICS_H
#define PARALLEL_PHYSICS_H

#include "core/types.h"

/**
 * Parallel Physics
 *
 * Uses Phase 3 threading system to accelerate:
 * - Broad-phase collision detection
 * - Narrow-phase collision testing
 * - Island-based constraint solving
 */

/**
 * Parallel Collision Context
 * Passed to parallel jobs
 */
typedef struct {
  u32 body_count;
  void *bodies;
  void *results; // Output contacts
} CollisionContext;

/**
 * Initialize parallel physics
 */
void parallel_physics_init(u32 thread_count);

/**
 * Run parallel broad-phase collision detection
 */
void parallel_broad_phase(void *bodies, u32 count);

/**
 * Run parallel narrow-phase collision detection on pairs
 */
void parallel_narrow_phase(void *pairs, u32 count);

/**
 * Run parallel constraint solver
 */
void parallel_constraint_solve(void *islands, u32 count);

#endif // PARALLEL_PHYSICS_H
