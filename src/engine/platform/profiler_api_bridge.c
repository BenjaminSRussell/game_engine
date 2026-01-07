// Profiler API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/profiler_api_bridge.h"
#include <string.h>
#include "../include/tools/profiler.h"

// Global state
static bool g_enabled = true;
static bool g_gpu_profiling = true;
static uint32_t g_update_frequency = 60;

void profiler_set_enabled(bool enabled) {
  g_enabled = enabled;
  // Stubbed until engine supports it
  // profiler_enable(enabled);
  LOG_INFO("Profiler: %s", enabled ? "Enabled" : "Disabled");
}

bool profiler_is_enabled(void) { return g_enabled; }

void profiler_get_snapshot(ProfilerSnapshot *snapshot) {
  if (!snapshot)
    return;

  // Get profiler stats from engine
  ProfilerMetrics stats = profiler_get_metrics();

  // Frame timing
  snapshot->cpu_frame_time = stats.cpu_time; // mapped from cpu_time
  snapshot->gpu_frame_time = stats.gpu_frame_time;
  snapshot->total_frame_time = stats.frame_time;

  // System breakdown - these fields don't exist in ProfilerMetrics currently
  // We'll set them to 0 or use available metrics if they map
  snapshot->physics_time = 0.0f;
  snapshot->render_time = 0.0f;
  snapshot->script_time = 0.0f;
  snapshot->audio_time = 0.0f;
  snapshot->ui_time = 0.0f;

  // Rendering stats
  snapshot->draw_calls = stats.draw_calls;
  snapshot->triangles = stats.triangles;
  snapshot->vertices = 0; // Not tracked
  snapshot->shader_switches = 0; 
  snapshot->texture_switches = 0;

  // Memory
  snapshot->memory_used = stats.memory_usage;
  snapshot->memory_peak = stats.peak_memory; // Correct field name
  snapshot->gpu_memory_used = stats.gpu_memory_used;
  snapshot->gpu_memory_peak = 0; // Not tracked

  // Frame stats
  snapshot->fps = stats.fps;
  snapshot->avg_frame_time = stats.avg_frame_time;
  snapshot->min_frame_time = stats.min_frame_time;
  snapshot->max_frame_time = stats.max_frame_time;
}

void profiler_begin_sample(const char *name) {
  if (!g_enabled || !name)
    return;
  profiler_begin_zone(name);
}

void profiler_end_sample(void) {
  if (!g_enabled)
    return;
  // Name is required but ignored by implementation for popping
  profiler_end_zone("BridgedSample"); 
}

uint32_t profiler_get_samples(ProfilerSample *samples, uint32_t max_count) {
  if (!samples || max_count == 0)
    return 0;

  // TODO: Implement sample retrieval from engine
  // For now return 0
  return 0;
}

void profiler_reset_stats(void) {
  // TODO: Implement profiler_reset in engine if needed
  LOG_WARN("profiler_reset_stats stubbed");
}

void profiler_set_update_frequency(uint32_t frequency) {
  g_update_frequency = frequency;
  // TODO: Implement profiler_set_frequency in engine
}

uint32_t profiler_get_update_frequency(void) { return g_update_frequency; }

void profiler_set_gpu_profiling_enabled(bool enabled) {
  g_gpu_profiling = enabled;
  // TODO: Implement profiler_enable_gpu in engine if needed
  LOG_WARN("profiler_set_gpu_profiling_enabled stubbed");
}

bool profiler_is_gpu_profiling_enabled(void) { return g_gpu_profiling; }
