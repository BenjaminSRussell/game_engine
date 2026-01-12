// include/tools/profiler.h
//
// Purpose: Advanced performance profiling system for engine subsystems
//
#ifndef TOOLS_PROFILER_H
#define TOOLS_PROFILER_H

#include <core/types.h>
#include <stdbool.h>

#define MAX_PROFILER_ZONES 64
#define MAX_ZONE_NAME_LENGTH 64

// Advanced zone tracking with CPU cycles
typedef struct {
    char name[MAX_ZONE_NAME_LENGTH];
    f64 start_time;
    f64 total_time;
    u64 start_cycles;
    u64 total_cycles;
    u32 call_count;
    f64 min_time;
    f64 max_time;
    bool active;
} ProfilerZone;

// Comprehensive metrics structure
typedef struct {
    // Frame timing
    f64 frame_time;
    f64 fps;
    f64 min_frame_time;
    f64 max_frame_time;
    f64 avg_frame_time;
    u32 frame_count;
    
    // CPU timing
    f64 cpu_time;
    f64 min_cpu_time;
    f64 max_cpu_time;
    f64 avg_cpu_time;
    
    // GPU timing
    f64 gpu_frame_time;
    f64 min_gpu_time;
    f64 max_gpu_time;
    f64 avg_gpu_time;
    
    // Memory statistics
    u64 memory_usage;
    u64 peak_memory;
    u64 gpu_memory_used;
    
    // Rendering statistics
    u32 draw_calls;
    u32 triangles;
    
    // Zone stats (legacy compatibility)
    ProfilerZone zones[MAX_PROFILER_ZONES];
    u32 zone_count;
} ProfilerMetrics;

// Legacy compatibility
typedef ProfilerMetrics ProfileStats;

// MARK: - System Management

void profiler_init(void);
void profiler_shutdown(void);

// MARK: - Frame Tracking

void profiler_begin_frame(void);
void profiler_end_frame(void);

// MARK: - Zone Profiling

void profiler_begin_zone(const char* name);
void profiler_end_zone(const char* name);

// MARK: - GPU Profiling

void profiler_begin_gpu_zone(const char* name);
void profiler_end_gpu_zone(const char* name);
void profiler_add_draw_call(u32 triangle_count);
void profiler_update_gpu_memory_usage(u64 memory_used);

// MARK: - Statistics and Reporting

ProfilerMetrics profiler_get_metrics(void);
void profiler_reset_metrics(void);
void profiler_export_csv(const char* filepath);
void profiler_export_detailed_report(const char* filepath);
void profiler_print_summary(void);

// MARK: - Convenience Macros

#define PROFILE_SCOPE(name)                                                    \
  profiler_begin_zone(name);                                                   \
  for (int _i = 0; _i < 1; _i++, profiler_end_zone(name))

#define PROFILE_GPU_SCOPE(name)                                                 \
  profiler_begin_gpu_zone(name);                                                \
  for (int _i = 0; _i < 1; _i++, profiler_end_gpu_zone(name))

#endif // TOOLS_PROFILER_H
