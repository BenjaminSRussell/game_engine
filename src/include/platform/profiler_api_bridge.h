// Profiler API Bridge
// Exposes performance profiling to VoxelForgeStudio

#ifndef PROFILER_API_BRIDGE_H
#define PROFILER_API_BRIDGE_H

#include "include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Performance Profiler API
// ============================================================================

/// Profiler snapshot with comprehensive metrics
typedef struct {
  // Frame timing (milliseconds)
  float cpu_frame_time;
  float gpu_frame_time;
  float total_frame_time;

  // System timing breakdown
  float physics_time;
  float render_time;
  float script_time;
  float audio_time;
  float ui_time;

  // Rendering stats
  uint32_t draw_calls;
  uint32_t triangles;
  uint32_t vertices;
  uint32_t shader_switches;
  uint32_t texture_switches;

  // Memory (bytes)
  uint64_t memory_used;
  uint64_t memory_peak;
  uint64_t gpu_memory_used;
  uint64_t gpu_memory_peak;

  // Frame stats
  float fps;
  float avg_frame_time;
  float min_frame_time;
  float max_frame_time;
} ProfilerSnapshot;

/// Enable/disable profiler
void profiler_set_enabled(bool enabled);

/// Check if profiler is enabled
bool profiler_is_enabled(void);

/// Get current profiler snapshot
void profiler_get_snapshot(ProfilerSnapshot *snapshot);

/// Begin profiling sample
void profiler_begin_sample(const char *name);

/// End current profiling sample
void profiler_end_sample(void);

/// Profiler sample info
typedef struct {
  char name[128];
  float time_ms;
  float percentage;
  uint32_t call_count;
  uint32_t depth;
} ProfilerSample;

/// Get profiler samples for current frame
uint32_t profiler_get_samples(ProfilerSample *samples, uint32_t max_count);

/// Reset profiler statistics
void profiler_reset_stats(void);

/// Set profiler update frequency (samples per second)
void profiler_set_update_frequency(uint32_t frequency);

/// Get profiler update frequency
uint32_t profiler_get_update_frequency(void);

/// Enable/disable GPU profiling
void profiler_set_gpu_profiling_enabled(bool enabled);

/// Check if GPU profiling is enabled
bool profiler_is_gpu_profiling_enabled(void);

#ifdef __cplusplus
}
#endif

#endif // PROFILER_API_BRIDGE_H
