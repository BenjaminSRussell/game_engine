/*
 * performance_profiling.h
 * Performance profiling and statistics for GPU-driven rendering
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_PERFORMANCE_PROFILING_H
#define GEOMETRY_PERFORMANCE_PROFILING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES & STRUCTURES
 * ============================================================================ */

// Performance metrics for a single frame
typedef struct frame_metrics {
    // Timing (in milliseconds)
    double cpu_time_total;
    double cpu_time_culling;
    double cpu_time_batch_generation;
    double gpu_time_total;
    double gpu_time_culling;
    double gpu_time_rendering;
    
    // Counts
    uint32_t total_instances;
    uint32_t visible_instances;
    uint32_t draw_calls_before;      // Without instancing
    uint32_t draw_calls_after;       // With instancing
    uint32_t batches_created;
    
    // Memory
    size_t instance_buffer_bytes;
    size_t visible_buffer_bytes;
    size_t command_buffer_bytes;
    
    // Culling efficiency
    float frustum_cull_rate;         // % culled by frustum
    float distance_cull_rate;        // % culled by distance
    float occlusion_cull_rate;       // % culled by occlusion
    float overall_cull_rate;         // Total % culled
    
    // Performance gains
    float draw_call_reduction;       // % reduction in draw calls
    float cpu_time_saved;            // ms saved on CPU
    float frame_time_improvement;    // % improvement in frame time
} frame_metrics_t;

// Rolling average statistics over N frames
typedef struct performance_stats {
    uint32_t frame_count;
    uint32_t window_size;            // Number of frames to average
    
    // Averages
    double avg_cpu_time;
    double avg_gpu_time;
    double avg_frame_time;
    float avg_draw_call_reduction;
    float avg_cull_rate;
    
    // Peaks
    double peak_cpu_time;
    double peak_gpu_time;
    uint32_t peak_instance_count;
    uint32_t peak_draw_calls;
    
    // Totals
    uint64_t total_instances_rendered;
    uint64_t total_draw_calls_saved;
} performance_stats_t;

// Profiler context
typedef struct performance_profiler {
    frame_metrics_t* frame_history;
    uint32_t history_capacity;
    uint32_t history_count;
    uint32_t current_frame_index;
    
    performance_stats_t stats;
    
    // Timing helpers
    double frame_start_time;
    double culling_start_time;
    double batch_start_time;
    
    bool enabled;
    bool gpu_timing_enabled;
    
    char label[64];
} performance_profiler_t;

/* ============================================================================
 * API - PROFILER MANAGEMENT
 * ============================================================================ */

/**
 * Create a performance profiler
 * @param history_size Number of frames to keep in history
 * @param window_size Number of frames for rolling average
 * @return Profiler or NULL on failure
 */
performance_profiler_t* performance_profiler_create(
    uint32_t history_size,
    uint32_t window_size);

/**
 * Destroy a performance profiler
 */
void performance_profiler_destroy(performance_profiler_t* profiler);

/**
 * Enable/disable profiler
 */
void performance_profiler_set_enabled(performance_profiler_t* profiler, bool enabled);

/**
 * Reset profiler statistics
 */
void performance_profiler_reset(performance_profiler_t* profiler);

/* ============================================================================
 * API -FRAME TIMING
 * ============================================================================ */

/**
 * Begin frame profiling
 */
void performance_profiler_begin_frame(performance_profiler_t* profiler);

/**
 * End frame profiling and compute statistics
 */
void performance_profiler_end_frame(performance_profiler_t* profiler);

/**
 * Begin culling pass timing
 */
void performance_profiler_begin_culling(performance_profiler_t* profiler);

/**
 * End culling pass timing
 */
void performance_profiler_end_culling(performance_profiler_t* profiler);

/**
 * Begin batch generation timing
 */
void performance_profiler_begin_batch_generation(performance_profiler_t* profiler);

/**
 * End batch generation timing
 */
void performance_profiler_end_batch_generation(performance_profiler_t* profiler);

/* ============================================================================
 * API - METRICS RECORDING
 * ============================================================================ */

/**
 * Record instance counts
 */
void performance_profiler_record_instances(
    performance_profiler_t* profiler,
    uint32_t total_instances,
    uint32_t visible_instances);

/**
 * Record draw call counts
 */
void performance_profiler_record_draw_calls(
    performance_profiler_t* profiler,
    uint32_t before_instancing,
    uint32_t after_instancing);

/**
 * Record culling statistics
 */
void performance_profiler_record_culling(
    performance_profiler_t* profiler,
    uint32_t frustum_culled,
    uint32_t distance_culled,
    uint32_t occlusion_culled,
    uint32_t total_tested);

/**
 * Record memory usage
 */
void performance_profiler_record_memory(
    performance_profiler_t* profiler,
    size_t instance_buffer_bytes,
    size_t visible_buffer_bytes,
    size_t command_buffer_bytes);

/**
 * Record GPU timing (if available)
 */
void performance_profiler_record_gpu_time(
    performance_profiler_t* profiler,
    double culling_time_ms,
    double rendering_time_ms);

/* ============================================================================
 * API - STATISTICS & REPORTING
 * ============================================================================ */

/**
 * Get current frame metrics
 */
frame_metrics_t performance_profiler_get_current_frame(
    const performance_profiler_t* profiler);

/**
 * Get overall statistics
 */
performance_stats_t performance_profiler_get_stats(
    const performance_profiler_t* profiler);

/**
 * Get frame from history
 * @param frame_offset 0 = current frame, 1 = previous, etc.
 */
frame_metrics_t performance_profiler_get_frame(
    const performance_profiler_t* profiler,
    uint32_t frame_offset);

/**
 * Print performance report to console
 */
void performance_profiler_print_report(const performance_profiler_t* profiler);

/**
 * Generate CSV export of frame history
 * @param filename Output CSV file path
 */
int performance_profiler_export_csv(
    const performance_profiler_t* profiler,
    const char* filename);

/* ============================================================================
 * API - UTILITIES
 * ============================================================================ */

/**
 * Get current high-resolution time in milliseconds
 */
double performance_get_time_ms(void);

/**
 * Calculate percentage change
 */
float performance_calculate_percentage(float before, float after);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_PERFORMANCE_PROFILING_H */
