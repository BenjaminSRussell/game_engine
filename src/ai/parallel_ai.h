#ifndef PARALLEL_AI_H
#define PARALLEL_AI_H

#include "core/types.h"
#include "engine/include/math/math_all.h"

/**
 * Parallel AI Processing
 *
 * - Async Pathfinding: Offload A* to worker threads
 * - Parallel Perception: Batch raycasts
 */

typedef void (*PathCompleteCallback)(void *user_data, bool success,
                                     Vec3 *waypoints, u32 count);

/**
 * Initialize parallel AI
 */
void parallel_ai_init(u32 thread_count);

/**
 * Request async pathfinding
 *
 * @param start Start position
 * @param end   Target position
 * @param cb    Completion callback (called on main thread)
 * @param data  User data for callback
 */
void ai_request_path(Vec3 start, Vec3 end, PathCompleteCallback cb, void *data);

/**
 * Update parallel AI (process results)
 * Call every frame
 */
void parallel_ai_update(void);

#endif // PARALLEL_AI_H
