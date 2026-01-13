// include/core/performance.h
//
// Purpose: Defines a comprehensive performance monitoring and profiling system
// for the game engine. This header provides tools for measuring execution times
// of code segments, profiling function call counts and durations, and tracking
// overall frame statistics (e.g., FPS). It integrates with the logging system
// to provide detailed performance reports and debugging information.
//
// Public APIs:
// - `Timer`: Structure for basic timing of code sections (start, stop, elapsed
// time).
// - `ProfilerEntry`: Structure to store aggregated statistics for a profiled
// function (call count, min/max/avg time).
// - `Profiler`: The main structure managing a collection of `ProfilerEntry`
// instances,
//   with global control over profiling enablement.
// - `g_profiler`: An external global instance of the `Profiler`.
// - `perf_timer_create`, `perf_timer_destroy`, `perf_timer_start`,
// `perf_timer_stop`, `perf_timer_get_elapsed`, `perf_timer_reset`: API for
// `Timer` management.
// - `profiler_init`, `profiler_shutdown`, `profiler_start`, `profiler_stop`,
// `profiler_reset`, `profiler_report`, `profiler_enable`: API for `Profiler`
// management and reporting.
// - `FrameStats`: Structure to track per-frame performance metrics (frame time,
// FPS, frame count).
// - `g_frame_stats`: An external global instance of `FrameStats`.
// - `frame_stats_update`, `frame_stats_reset`, `frame_stats_get_fps`: API for
// updating and querying frame statistics.
// - `profiler_call_count_increment`, `profiler_get_call_count`: For counting
// specific function calls.
// - `profiler_scope_enter`, `profiler_scope_exit`: For convenient scope-based
// profiling.
// - Numerous other functions for specific profiling contexts (physics, world
// gen, entity lifecycle, etc.)
//   and integration with the logger for extended diagnostics.
//
// Ownership: The `Profiler` and `FrameStats` structures manage their internal
// data. These are typically global instances (`g_profiler`, `g_frame_stats`).
//
// Invariants:
// - `profiler_init` and `frame_stats_update` should be called once per
// application/frame respectively.
// - `profiler_start` and `profiler_stop` calls must be correctly paired to
// ensure accurate timing.
// - Profiling incurs some overhead and can be enabled/disabled as needed.
// - The `common.h` header should be included for basic types.
//
#ifndef PERFORMANCE_H
#define PERFORMANCE_H

#include "engine/include/common.h"

typedef struct {
  const char *name;
  f64 start_time;
  f64 elapsed_time;
  u32 frame_count;
  f64 total_time;
} Timer;

typedef struct {
  const char *name;
  u32 call_count;
  f64 total_time;
  f64 min_time;
  f64 max_time;
  f64 avg_time;
  f64 start_time;
} ProfilerEntry;

typedef struct {
  ProfilerEntry entries[64];
  u32 entry_count;
  bool enabled;
} Profiler;

extern Profiler g_profiler;

// Timer functions
Timer *perf_timer_create(const char *name);
void perf_timer_destroy(Timer *timer);
void perf_timer_start(Timer *timer);
void perf_timer_stop(Timer *timer);
f64 perf_timer_get_elapsed(Timer *timer);
void perf_timer_reset(Timer *timer);

// Profiler functions
void profiler_init(void);
void profiler_shutdown(void);
void profiler_start(const char *name);
void profiler_stop(const char *name);
void profiler_reset(void);
void profiler_report(void);
void profiler_enable(bool enable);

// Frame time tracking
typedef struct {
  f64 frame_time;
  f64 min_frame_time;
  f64 max_frame_time;
  f32 fps;
  u32 frame_count;
} FrameStats;

extern FrameStats g_frame_stats;

void frame_stats_update(f64 delta_time);
void frame_stats_reset(void);
f32 frame_stats_get_fps(void);

void logger_errno_context(int err_code);
void performance_frame_breakdown(void);
void memory_spike_detector_init(u64 threshold);
void memory_spike_detector_check(u64 current_memory);
void profiler_call_count_increment(const char *function);
u32 profiler_get_call_count(const char *function);

void physics_profile_start(void);
void physics_profile_end(void);
void entity_lifecycle_log(const char *event, const char *entity_type);
void world_gen_profile_start(void);
void world_gen_profile_end(void);
void chunk_lifecycle_log(int chunk_x, int chunk_z, const char *event);
void npc_behavior_log(const char *npc_name, const char *behavior);
void input_event_log(const char *input_type, const char *action);
void crafting_event_log(const char *recipe_name, bool success);
void save_load_log(const char *operation, const char *target);

typedef struct {
  const char *name;
  f64 start_time;
  f64 elapsed_time;
} ScopeEntry;

int profiler_scope_enter(const char *name);
void profiler_scope_exit(int scope_id);

#endif
