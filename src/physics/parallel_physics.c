#include "physics/parallel_physics.h"
#include "core/thread_pool.h"
#include "core/threading/parallel_utils.h"
#include "engine/include/core/logger.h"

void parallel_physics_init(u32 thread_count) {
  if (!thread_pool_get_global()) {
    thread_pool_init(thread_count);
  }
  LOG_INFO("[Physics] Parallel execution initialized");
}

// Broadphase Batch Worker
static void broad_phase_batch(u32 index, void *user_data) {
  // Placeholder logic:
  // 1. Update AABB for body[index]
  // 2. Insert into spatial structure (grid/tree)
  // 3. (Optional) Simple O(N) sweep vs others if small N
}

void parallel_broad_phase(void *bodies, u32 count) {
  if (count == 0)
    return;

  // Execute update/broadphase in parallel
  parallel_for(0, count, broad_phase_batch, bodies);
}

// Narrowphase Worker
static void narrow_phase_batch(u32 index, void *user_data) {
  // Placeholder logic:
  // 1. Test collision pair[index]
  // 2. Generate contact manifold if colliding
  // 3. Store in thread-local or atomic global list
}

void parallel_narrow_phase(void *pairs, u32 count) {
  if (count == 0)
    return;
  parallel_for(0, count, narrow_phase_batch, pairs);
}

// Solver Worker
static void solver_batch(u32 index, void *user_data) {
  // Placeholder logic:
  // Solve constraints for Island[index]
}

void parallel_constraint_solve(void *islands, u32 count) {
  if (count == 0)
    return;
  parallel_for(0, count, solver_batch, islands);
}
