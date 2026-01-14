#ifndef PHYSICS_PROFILING_H
#define PHYSICS_PROFILING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct physics_profiler_t physics_profiler_t;

typedef struct {
    uint64_t broadphase_time_ns;
    uint64_t narrowphase_time_ns;
    uint64_t solver_time_ns;
    uint64_t integration_time_ns;
    uint64_t total_time_ns;
    uint32_t active_bodies;
    uint32_t sleeping_bodies;
    uint32_t collision_pairs;
    uint32_t constraint_count;
    double frame_rate;
} physics_frame_stats_t;

typedef struct {
    double avg_frame_time_ms;
    double min_frame_time_ms;
    double max_frame_time_ms;
    double avg_frame_rate;
    uint64_t total_frames;
    uint64_t profiling_time_ms;
    uint32_t peak_body_count;
    uint32_t peak_collision_pairs;
    double memory_usage_mb;
    double cpu_usage_percent;
} physics_performance_report_t;

typedef enum {
    PHYSICS_PROFILER_SUCCESS = 0,
    PHYSICS_PROFILER_ERROR_INVALID_PARAM = -1,
    PHYSICS_PROFILER_ERROR_OUT_OF_MEMORY = -2,
    PHYSICS_PROFILER_ERROR_NOT_INITIALIZED = -3,
    PHYSICS_PROFILER_ERROR_ALREADY_RUNNING = -4,
    PHYSICS_PROFILER_ERROR_NOT_RUNNING = -5
} physics_profiler_error_t;

physics_profiler_error_t physics_profiler_init(physics_profiler_t** profiler, uint32_t max_bodies);
void physics_profiler_shutdown(physics_profiler_t* profiler);

physics_profiler_error_t physics_profiler_start(physics_profiler_t* profiler);
physics_profiler_error_t physics_profiler_stop(physics_profiler_t* profiler);
physics_profiler_error_t physics_profiler_reset(physics_profiler_t* profiler);

physics_profiler_error_t physics_profiler_begin_frame(physics_profiler_t* profiler);
physics_profiler_error_t physics_profiler_end_frame(physics_profiler_t* profiler, uint32_t active_bodies, uint32_t sleeping_bodies);

physics_profiler_error_t physics_profiler_begin_broadphase(physics_profiler_t* profiler);
physics_profiler_error_t physics_profiler_end_broadphase(physics_profiler_t* profiler, uint32_t collision_pairs);

physics_profiler_error_t physics_profiler_begin_narrowphase(physics_profiler_t* profiler);
physics_profiler_error_t physics_profiler_end_narrowphase(physics_profiler_t* profiler);

physics_profiler_error_t physics_profiler_begin_solver(physics_profiler_t* profiler);
physics_profiler_error_t physics_profiler_end_solver(physics_profiler_t* profiler, uint32_t constraint_count);

physics_profiler_error_t physics_profiler_begin_integration(physics_profiler_t* profiler);
physics_profiler_error_t physics_profiler_end_integration(physics_profiler_t* profiler);

physics_profiler_error_t physics_profiler_get_frame_stats(physics_profiler_t* profiler, physics_frame_stats_t* stats);
physics_profiler_error_t physics_profiler_get_performance_report(physics_profiler_t* profiler, physics_performance_report_t* report);

physics_profiler_error_t physics_profiler_dump_to_file(physics_profiler_t* profiler, const char* filename);
physics_profiler_error_t physics_profiler_print_summary(physics_profiler_t* profiler);

#ifdef __cplusplus
}
#endif

#endif
