/*
 * mtl_statistics.c
 * GPU statistics collection implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_statistics.h"
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#if defined(__OBJC__)
#import <Metal/Metal.h>
#define TO_CMD_BUF(x) ((__bridge id<MTLCommandBuffer>)(x))
#endif

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

struct mtl_statistics_collector {
    mtl_render_statistics_t render_stats;
    mtl_compute_statistics_t compute_stats;
    
    double frame_start_time;
    double frame_end_time;
};

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

static double get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

/* ============================================================================
 * IMPLEMENTATION
 * ============================================================================ */

mtl_statistics_collector_t metal_statistics_collector_create(void) {
    mtl_statistics_collector_t collector = calloc(1, sizeof(struct mtl_statistics_collector));
    return collector;
}

void metal_statistics_collector_destroy(mtl_statistics_collector_t collector) {
    if (collector) {
        free(collector);
    }
}

void metal_statistics_begin_frame(mtl_statistics_collector_t collector) {
    if (!collector) return;
    collector->frame_start_time = get_current_time_ms();
}

void metal_statistics_end_frame(mtl_statistics_collector_t collector) {
    if (!collector) return;
    collector->frame_end_time = get_current_time_ms();
    
    // Calculate CPU time
    double cpu_time = collector->frame_end_time - collector->frame_start_time;
    collector->render_stats.cpu_time_ms = cpu_time;
    collector->compute_stats.cpu_time_ms = cpu_time;
}

/* ============================================================================
 * RENDER STATISTICS
 * ============================================================================ */

void metal_statistics_record_draw(mtl_statistics_collector_t collector,
                                   uint64_t vertex_count,
                                   uint64_t instance_count) {
    if (!collector) return;
    
    collector->render_stats.draw_calls++;
    collector->render_stats.vertices_submitted += vertex_count * instance_count;
    collector->render_stats.instances_drawn += instance_count;
    
    // Estimate primitives (assuming triangles)
    collector->render_stats.primitives_submitted += (vertex_count / 3) * instance_count;
    
    // Estimate shader invocations
    collector->render_stats.vertex_shader_invocations += vertex_count * instance_count;
    collector->render_stats.fragment_shader_invocations += collector->render_stats.primitives_submitted;
}

void metal_statistics_record_indexed_draw(mtl_statistics_collector_t collector,
                                          uint64_t index_count,
                                          uint64_t instance_count) {
    if (!collector) return;
    
    collector->render_stats.draw_calls++;
    collector->render_stats.vertices_submitted += index_count * instance_count;
    collector->render_stats.instances_drawn += instance_count;
    
    // Estimate primitives
    collector->render_stats.primitives_submitted += (index_count / 3) * instance_count;
    
    // Estimate shader invocations
    collector->render_stats.vertex_shader_invocations += index_count * instance_count;
    collector->render_stats.fragment_shader_invocations += collector->render_stats.primitives_submitted;
}

void metal_statistics_get_render_stats(mtl_statistics_collector_t collector,
                                       mtl_render_statistics_t* out_stats) {
    if (!collector || !out_stats) return;
    *out_stats = collector->render_stats;
}

/* ============================================================================
 * COMPUTE STATISTICS
 * ============================================================================ */

void metal_statistics_record_dispatch(mtl_statistics_collector_t collector,
                                      uint64_t threadgroups_x,
                                      uint64_t threadgroups_y,
                                      uint64_t threadgroups_z,
                                      uint64_t threads_per_group_x,
                                      uint64_t threads_per_group_y,
                                      uint64_t threads_per_group_z) {
    if (!collector) return;
    
    collector->compute_stats.dispatch_calls++;
    
    uint64_t total_threadgroups = threadgroups_x * threadgroups_y * threadgroups_z;
    collector->compute_stats.threadgroups_dispatched += total_threadgroups;
    
    uint64_t threads_per_group = threads_per_group_x * threads_per_group_y * threads_per_group_z;
    uint64_t total_threads = total_threadgroups * threads_per_group;
    
    collector->compute_stats.threads_dispatched += total_threads;
    collector->compute_stats.compute_shader_invocations += total_threads;
}

void metal_statistics_get_compute_stats(mtl_statistics_collector_t collector,
                                        mtl_compute_statistics_t* out_stats) {
    if (!collector || !out_stats) return;
    *out_stats = collector->compute_stats;
}

/* ============================================================================
 * GPU TIMING
 * ============================================================================ */

void metal_statistics_record_gpu_time(mtl_statistics_collector_t collector,
                                      mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (!collector || !buffer) return;
    
    id<MTLCommandBuffer> cmd_buf = TO_CMD_BUF(buffer);
    
    double gpu_start = [cmd_buf GPUStartTime];
    double gpu_end = [cmd_buf GPUEndTime];
    
    if (gpu_end > gpu_start) {
        double gpu_time_ms = (gpu_end - gpu_start) * 1000.0;
        collector->render_stats.gpu_time_ms += gpu_time_ms;
        collector->compute_stats.gpu_time_ms += gpu_time_ms;
    }
#endif
}

void metal_statistics_reset(mtl_statistics_collector_t collector) {
    if (!collector) return;
    memset(&collector->render_stats, 0, sizeof(mtl_render_statistics_t));
    memset(&collector->compute_stats, 0, sizeof(mtl_compute_statistics_t));
}
