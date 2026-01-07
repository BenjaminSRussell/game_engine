/*
 * mtl_statistics.h
 * GPU statistics collection (workaround for lack of native queries)
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_STATISTICS_H
#define PLATFORM_MTL_STATISTICS_H

#include "mtl_command_buffer.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct mtl_statistics_collector* mtl_statistics_collector_t;

typedef struct mtl_render_statistics {
    uint64_t draw_calls;
    uint64_t vertices_submitted;
    uint64_t primitives_submitted;
    uint64_t instances_drawn;
    uint64_t vertex_shader_invocations;
    uint64_t fragment_shader_invocations;
    double gpu_time_ms;
    double cpu_time_ms;
} mtl_render_statistics_t;

typedef struct mtl_compute_statistics {
    uint64_t dispatch_calls;
    uint64_t threadgroups_dispatched;
    uint64_t threads_dispatched;
    uint64_t compute_shader_invocations;
    double gpu_time_ms;
    double cpu_time_ms;
} mtl_compute_statistics_t;

/* ============================================================================
 * STATISTICS COLLECTOR API
 * ============================================================================ */

/**
 * Creates a statistics collector for tracking rendering metrics.
 * @return The statistics collector.
 */
mtl_statistics_collector_t metal_statistics_collector_create(void);

/**
 * Destroys a statistics collector.
 * @param collector The collector to destroy.
 */
void metal_statistics_collector_destroy(mtl_statistics_collector_t collector);

/**
 * Begins a statistics collection frame.
 * @param collector The statistics collector.
 */
void metal_statistics_begin_frame(mtl_statistics_collector_t collector);

/**
 * Ends a statistics collection frame.
 * @param collector The statistics collector.
 */
void metal_statistics_end_frame(mtl_statistics_collector_t collector);

/* ============================================================================
 * RENDER STATISTICS
 * ============================================================================ */

/**
 * Records a draw call.
 * @param collector The statistics collector.
 * @param vertex_count Number of vertices in the draw call.
 * @param instance_count Number of instances.
 */
void metal_statistics_record_draw(mtl_statistics_collector_t collector,
                                   uint64_t vertex_count,
                                   uint64_t instance_count);

/**
 * Records an indexed draw call.
 * @param collector The statistics collector.
 * @param index_count Number of indices.
 * @param instance_count Number of instances.
 */
void metal_statistics_record_indexed_draw(mtl_statistics_collector_t collector,
                                          uint64_t index_count,
                                          uint64_t instance_count);

/**
 * Gets the current render statistics.
 * @param collector The statistics collector.
 * @param out_stats Pointer to structure to fill.
 */
void metal_statistics_get_render_stats(mtl_statistics_collector_t collector,
                                       mtl_render_statistics_t* out_stats);

/* ============================================================================
 * COMPUTE STATISTICS
 * ============================================================================ */

/**
 * Records a compute dispatch.
 * @param collector The statistics collector.
 * @param threadgroups_x Number of threadgroups in X.
 * @param threadgroups_y Number of threadgroups in Y.
 * @param threadgroups_z Number of threadgroups in Z.
 * @param threads_per_group_x Threads per group X.
 * @param threads_per_group_y Threads per group Y.
 * @param threads_per_group_z Threads per group Z.
 */
void metal_statistics_record_dispatch(mtl_statistics_collector_t collector,
                                      uint64_t threadgroups_x,
                                      uint64_t threadgroups_y,
                                      uint64_t threadgroups_z,
                                      uint64_t threads_per_group_x,
                                      uint64_t threads_per_group_y,
                                      uint64_t threads_per_group_z);

/**
 * Gets the current compute statistics.
 * @param collector The statistics collector.
 * @param out_stats Pointer to structure to fill.
 */
void metal_statistics_get_compute_stats(mtl_statistics_collector_t collector,
                                        mtl_compute_statistics_t* out_stats);

/* ============================================================================
 * GPU TIMING
 * ============================================================================ */

/**
 * Records GPU execution time for a command buffer.
 * @param collector The statistics collector.
 * @param buffer The completed command buffer.
 */
void metal_statistics_record_gpu_time(mtl_statistics_collector_t collector,
                                      mtl_command_buffer_t buffer);

/**
 * Resets all statistics.
 * @param collector The statistics collector.
 */
void metal_statistics_reset(mtl_statistics_collector_t collector);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_STATISTICS_H */
