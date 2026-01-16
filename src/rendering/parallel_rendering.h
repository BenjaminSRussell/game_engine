#ifndef PARALLEL_RENDERING_H
#define PARALLEL_RENDERING_H

#include "core/types.h"

/**
 * Parallel Rendering Utilities
 *
 * Leverages Phase 3 threading system for parallel rendering operations:
 * - Parallel mesh culling
 * - Parallel command buffer generation
 * - Parallel shadow map generation
 */

/**
 * Initialize parallel rendering
 * Sets up worker threads for rendering
 */
void parallel_rendering_init(u32 thread_count);

/**
 * Shutdown parallel rendering
 */
void parallel_rendering_shutdown(void);

/**
 * Parallel frustum culling
 * Culls meshes against camera frustum in parallel
 *
 * @param mesh_count Number of meshes to cull
 * @param visible_out Output array of visibility flags
 */
void parallel_cull_meshes(u32 mesh_count, bool *visible_out);

/**
 * Parallel command buffer generation
 * Generates render commands in parallel across multiple threads
 *
 * @param mesh_count Number of meshes to process
 */
void parallel_generate_commands(u32 mesh_count);

/**
 * Get parallel rendering statistics
 */
typedef struct {
  u32 meshes_culled;
  u32 meshes_visible;
  f32 culling_time_ms;
  f32 command_gen_time_ms;
  u32 worker_threads;
} ParallelRenderStats;

ParallelRenderStats parallel_rendering_get_stats(void);
void parallel_rendering_print_stats(void);

#endif // PARALLEL_RENDERING_H
