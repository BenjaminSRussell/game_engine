#pragma once

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct Profiler Profiler;
typedef struct PerformanceStats PerformanceStats;

// Profiling
Profiler *profiler_create(void);
void profiler_begin_frame(Profiler *profiler);
void profiler_end_frame(Profiler *profiler);

void profiler_begin_section(Profiler *profiler, const char *name);
void profiler_end_section(Profiler *profiler);

void profiler_destroy(Profiler *profiler);

// Performance statistics
struct PerformanceStats {
  float fps;
  float frame_time_ms;
  float cpu_time_ms;
  float gpu_time_ms;
  uint64_t memory_used;
  uint32_t draw_calls;
  uint32_t triangles;
};

PerformanceStats *performance_stats_create(void);
void performance_stats_update(PerformanceStats *stats);
void performance_stats_get(PerformanceStats *stats,
                           PerformanceStats *out_stats);
void performance_stats_destroy(PerformanceStats *stats);

// Memory profiling
uint64_t memory_get_current_usage(void);
uint64_t memory_get_peak_usage(void);
uint32_t memory_get_allocation_count(void);
