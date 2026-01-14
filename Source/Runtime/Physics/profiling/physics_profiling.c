#include "physics_profiling.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

struct physics_profiler_t {
    bool initialized;
    bool is_running;
    bool in_frame;
    
    // Timing
    uint64_t frame_start_time;
    uint64_t broadphase_start_time;
    uint64_t narrowphase_start_time;
    uint64_t solver_start_time;
    uint64_t integration_start_time;
    
    // Frame statistics
    physics_frame_stats_t current_frame;
    physics_frame_stats_t* frame_history;
    uint32_t frame_count;
    uint32_t frame_capacity;
    uint32_t frame_index;
    
    // Performance tracking
    double min_frame_time;
    double max_frame_time;
    double total_frame_time;
    uint64_t total_frames_recorded;
    uint32_t peak_body_count;
    uint32_t peak_collision_pairs;
    
    // Configuration
    uint32_t max_bodies;
    uint64_t profiling_start_time;
};

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

physics_profiler_error_t physics_profiler_init(physics_profiler_t** profiler, uint32_t max_bodies) {
    if (!profiler) return PHYSICS_PROFILER_ERROR_INVALID_PARAM;
    
    physics_profiler_t* p = (physics_profiler_t*)calloc(1, sizeof(physics_profiler_t));
    if (!p) return PHYSICS_PROFILER_ERROR_OUT_OF_MEMORY;
    
    p->frame_capacity = 10000; // Store last 10k frames
    p->frame_history = (physics_frame_stats_t*)calloc(p->frame_capacity, sizeof(physics_frame_stats_t));
    if (!p->frame_history) {
        free(p);
        return PHYSICS_PROFILER_ERROR_OUT_OF_MEMORY;
    }
    
    p->max_bodies = max_bodies;
    p->min_frame_time = 1e9; // Initialize to very large value
    p->initialized = true;
    
    *profiler = p;
    return PHYSICS_PROFILER_SUCCESS;
}

void physics_profiler_shutdown(physics_profiler_t* profiler) {
    if (!profiler) return;
    
    free(profiler->frame_history);
    free(profiler);
}

physics_profiler_error_t physics_profiler_start(physics_profiler_t* profiler) {
    if (!profiler || !profiler->initialized) return PHYSICS_PROFILER_ERROR_NOT_INITIALIZED;
    if (profiler->is_running) return PHYSICS_PROFILER_ERROR_ALREADY_RUNNING;
    
    profiler->is_running = true;
    profiler->profiling_start_time = get_timestamp_ns();
    profiler->total_frames_recorded = 0;
    profiler->total_frame_time = 0.0;
    profiler->min_frame_time = 1e9;
    profiler->max_frame_time = 0.0;
    profiler->peak_body_count = 0;
    profiler->peak_collision_pairs = 0;
    
    return PHYSICS_PROFILER_SUCCESS;
}

physics_profiler_error_t physics_profiler_stop(physics_profiler_t* profiler) {
    if (!profiler || !profiler->initialized) return PHYSICS_PROFILER_ERROR_NOT_INITIALIZED;
    if (!profiler->is_running) return PHYSICS_PROFILER_ERROR_NOT_RUNNING;
    
    profiler->is_running = false;
    return PHYSICS_PROFILER_SUCCESS;
}

physics_profiler_error_t physics_profiler_begin_frame(physics_profiler_t* profiler) {
    if (!profiler || !profiler->initialized) return PHYSICS_PROFILER_ERROR_NOT_INITIALIZED;
    if (!profiler->is_running) return PHYSICS_PROFILER_ERROR_NOT_RUNNING;
    if (profiler->in_frame) return PHYSICS_PROFILER_ERROR_INVALID_PARAM;
    
    profiler->in_frame = true;
    profiler->frame_start_time = get_timestamp_ns();
    
    // Reset current frame stats
    memset(&profiler->current_frame, 0, sizeof(physics_frame_stats_t));
    
    return PHYSICS_PROFILER_SUCCESS;
}

physics_profiler_error_t physics_profiler_end_frame(physics_profiler_t* profiler, uint32_t active_bodies, uint32_t sleeping_bodies) {
    if (!profiler || !profiler->initialized) return PHYSICS_PROFILER_ERROR_NOT_INITIALIZED;
    if (!profiler->is_running) return PHYSICS_PROFILER_ERROR_NOT_RUNNING;
    if (!profiler->in_frame) return PHYSICS_PROFILER_ERROR_INVALID_PARAM;
    
    uint64_t frame_end_time = get_timestamp_ns();
    profiler->current_frame.total_time_ns = frame_end_time - profiler->frame_start_time;
    profiler->current_frame.active_bodies = active_bodies;
    profiler->current_frame.sleeping_bodies = sleeping_bodies;
    
    // Calculate frame rate
    if (profiler->current_frame.total_time_ns > 0) {
        profiler->current_frame.frame_rate = 1e9 / (double)profiler->current_frame.total_time_ns;
    }
    
    // Update performance tracking
    double frame_time_ms = profiler->current_frame.total_time_ns / 1e6;
    profiler->total_frame_time += frame_time_ms;
    profiler->total_frames_recorded++;
    
    if (frame_time_ms < profiler->min_frame_time) {
        profiler->min_frame_time = frame_time_ms;
    }
    if (frame_time_ms > profiler->max_frame_time) {
        profiler->max_frame_time = frame_time_ms;
    }
    
    if (active_bodies > profiler->peak_body_count) {
        profiler->peak_body_count = active_bodies;
    }
    if (profiler->current_frame.collision_pairs > profiler->peak_collision_pairs) {
        profiler->peak_collision_pairs = profiler->current_frame.collision_pairs;
    }
    
    // Store in history
    uint32_t index = profiler->frame_index % profiler->frame_capacity;
    profiler->frame_history[index] = profiler->current_frame;
    profiler->frame_index++;
    profiler->frame_count = (profiler->frame_count < profiler->frame_capacity) ? 
                          profiler->frame_count + 1 : profiler->frame_capacity;
    
    profiler->in_frame = false;
    return PHYSICS_PROFILER_SUCCESS;
}

physics_profiler_error_t physics_profiler_begin_broadphase(physics_profiler_t* profiler) {
    if (!profiler || !profiler->initialized || !profiler->in_frame) return PHYSICS_PROFILER_ERROR_INVALID_PARAM;
    profiler->broadphase_start_time = get_timestamp_ns();
    return PHYSICS_PROFILER_SUCCESS;
}

physics_profiler_error_t physics_profiler_end_broadphase(physics_profiler_t* profiler, uint32_t collision_pairs) {
    if (!profiler || !profiler->initialized || !profiler->in_frame) return PHYSICS_PROFILER_ERROR_INVALID_PARAM;
    profiler->current_frame.broadphase_time_ns = get_timestamp_ns() - profiler->broadphase_start_time;
    profiler->current_frame.collision_pairs = collision_pairs;
    return PHYSICS_PROFILER_SUCCESS;
}

physics_profiler_error_t physics_profiler_begin_narrowphase(physics_profiler_t* profiler) {
    if (!profiler || !profiler->initialized || !profiler->in_frame) return PHYSICS_PROFILER_ERROR_INVALID_PARAM;
    profiler->narrowphase_start_time = get_timestamp_ns();
    return PHYSICS_PROFILER_SUCCESS;
}

physics_profiler_error_t physics_profiler_end_narrowphase(physics_profiler_t* profiler) {
    if (!profiler || !profiler->initialized || !profiler->in_frame) return PHYSICS_PROFILER_ERROR_INVALID_PARAM;
    profiler->current_frame.narrowphase_time_ns = get_timestamp_ns() - profiler->narrowphase_start_time;
    return PHYSICS_PROFILER_SUCCESS;
}

physics_profiler_error_t physics_profiler_begin_solver(physics_profiler_t* profiler) {
    if (!profiler || !profiler->initialized || !profiler->in_frame) return PHYSICS_PROFILER_ERROR_INVALID_PARAM;
    profiler->solver_start_time = get_timestamp_ns();
    return PHYSICS_PROFILER_SUCCESS;
}

physics_profiler_error_t physics_profiler_end_solver(physics_profiler_t* profiler, uint32_t constraint_count) {
    if (!profiler || !profiler->initialized || !profiler->in_frame) return PHYSICS_PROFILER_ERROR_INVALID_PARAM;
    profiler->current_frame.solver_time_ns = get_timestamp_ns() - profiler->solver_start_time;
    profiler->current_frame.constraint_count = constraint_count;
    return PHYSICS_PROFILER_SUCCESS;
}

physics_profiler_error_t physics_profiler_begin_integration(physics_profiler_t* profiler) {
    if (!profiler || !profiler->initialized || !profiler->in_frame) return PHYSICS_PROFILER_ERROR_INVALID_PARAM;
    profiler->integration_start_time = get_timestamp_ns();
    return PHYSICS_PROFILER_SUCCESS;
}

physics_profiler_error_t physics_profiler_end_integration(physics_profiler_t* profiler) {
    if (!profiler || !profiler->initialized || !profiler->in_frame) return PHYSICS_PROFILER_ERROR_INVALID_PARAM;
    profiler->current_frame.integration_time_ns = get_timestamp_ns() - profiler->integration_start_time;
    return PHYSICS_PROFILER_SUCCESS;
}

physics_profiler_error_t physics_profiler_get_performance_report(physics_profiler_t* profiler, physics_performance_report_t* report) {
    if (!profiler || !profiler->initialized || !report) return PHYSICS_PROFILER_ERROR_INVALID_PARAM;
    
    memset(report, 0, sizeof(physics_performance_report_t));
    
    if (profiler->total_frames_recorded > 0) {
        report->avg_frame_time_ms = profiler->total_frame_time / profiler->total_frames_recorded;
        report->min_frame_time_ms = profiler->min_frame_time;
        report->max_frame_time_ms = profiler->max_frame_time;
        report->avg_frame_rate = 1000.0 / report->avg_frame_time_ms;
    }
    
    report->total_frames = profiler->total_frames_recorded;
    report->peak_body_count = profiler->peak_body_count;
    report->peak_collision_pairs = profiler->peak_collision_pairs;
    
    if (profiler->is_running) {
        uint64_t current_time = get_timestamp_ns();
        report->profiling_time_ms = (current_time - profiler->profiling_start_time) / 1e6;
    }
    
    // Estimate memory usage (simplified)
    report->memory_usage_mb = (profiler->frame_count * sizeof(physics_frame_stats_t)) / (1024.0 * 1024.0);
    
    return PHYSICS_PROFILER_SUCCESS;
}

physics_profiler_error_t physics_profiler_print_summary(physics_profiler_t* profiler) {
    if (!profiler || !profiler->initialized) return PHYSICS_PROFILER_ERROR_INVALID_PARAM;
    
    physics_performance_report_t report;
    physics_profiler_get_performance_report(profiler, &report);
    
    printf("=== Physics Performance Summary ===\n");
    printf("Total Frames: %llu\n", (unsigned long long)report.total_frames);
    printf("Average Frame Time: %.2f ms\n", report.avg_frame_time_ms);
    printf("Min/Max Frame Time: %.2f / %.2f ms\n", report.min_frame_time_ms, report.max_frame_time_ms);
    printf("Average Frame Rate: %.1f FPS\n", report.avg_frame_rate);
    printf("Peak Body Count: %u\n", report.peak_body_count);
    printf("Peak Collision Pairs: %u\n", report.peak_collision_pairs);
    printf("Memory Usage: %.2f MB\n", report.memory_usage_mb);
    printf("Profiling Time: %.2f seconds\n", report.profiling_time_ms / 1000.0);
    
    if (report.total_frames > 0) {
        physics_frame_stats_t* last_frame = &profiler->frame_history[(profiler->frame_index - 1) % profiler->frame_capacity];
        printf("\n=== Last Frame Breakdown ===\n");
        printf("Broadphase: %.2f ms\n", last_frame->broadphase_time_ns / 1e6);
        printf("Narrowphase: %.2f ms\n", last_frame->narrowphase_time_ns / 1e6);
        printf("Solver: %.2f ms\n", last_frame->solver_time_ns / 1e6);
        printf("Integration: %.2f ms\n", last_frame->integration_time_ns / 1e6);
        printf("Total: %.2f ms\n", last_frame->total_time_ns / 1e6);
    }
    
    return PHYSICS_PROFILER_SUCCESS;
}
