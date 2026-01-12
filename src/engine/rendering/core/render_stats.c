// src/engine/rendering/core/render_stats.c
// Render Statistics Tracking - Performance monitoring and metrics collection

#include <core/logger.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#include "../render_pipeline.h"
#include <core/time_system.h>

// ============================================================================
// Render Statistics Types
// ============================================================================

typedef struct {
  uint64_t start_time;
  uint64_t end_time;
  float duration_ms;
} Timer;

typedef struct {
  // Frame statistics
  uint32_t frame_count;
  float frame_time_ms;
  float cpu_time_ms;
  float gpu_time_ms;

  // Drawing statistics
  uint32_t draw_calls;
  uint32_t triangles_drawn;
  uint32_t vertices_processed;
  uint32_t pixels_shaded;

  // Memory statistics
  uint64_t texture_memory_used;
  uint64_t buffer_memory_used;
  uint64_t total_memory_used;

  // Culling statistics
  uint32_t total_objects;
  uint32_t visible_objects;
  uint32_t culled_objects;
  uint32_t frustum_culled;
  uint32_t distance_culled;
  uint32_t occlusion_culled;

  // Shader statistics
  uint32_t shader_switches;
  uint32_t shader_compilations;
  uint32_t shader_reloads;

  // Texture statistics
  uint32_t texture_bindings;
  uint32_t texture_uploads;
  uint32_t texture_streaming_loads;

  // Pipeline statistics
  uint32_t pipeline_switches;
  uint32_t render_passes;
  uint32_t compute_dispatches;

  // Performance metrics
  float fps;
  float frame_time_avg;
  float frame_time_min;
  float frame_time_max;
  float cpu_utilization;
  float gpu_utilization;

  // Error tracking
  uint32_t shader_errors;
  uint32_t buffer_errors;
  uint32_t texture_errors;
  uint32_t pipeline_errors;
} FrameStats;

typedef struct {
  FrameStats *frames;
  uint32_t frame_count;
  uint32_t frame_capacity;
  uint32_t current_frame;

  // Aggregated statistics (over all frames)
  FrameStats total;
  FrameStats average;
  FrameStats min;
  FrameStats max;

  // Performance history
  float fps_history[60]; // Last 60 frames
  float frame_time_history[60];
  uint32_t history_index;

  // Timing
  uint64_t session_start_time;
  uint64_t last_frame_time;

  bool enabled;
  bool detailed_tracking;
} RenderStatsSystem;

static RenderStatsSystem g_stats_system = {0};

// ============================================================================
// Timing Utilities
// ============================================================================

// Use get_time_nanos and nanos_to_ms from core/time_system.h

static void timer_start(Timer *timer) { timer->start_time = get_time_nanos(); }

static void timer_end(Timer *timer) {
  timer->end_time = get_time_nanos();
  timer->duration_ms = nanos_to_ms(timer->end_time - timer->start_time);
}

// ============================================================================
// Statistics Collection API
// ============================================================================

bool render_stats_init(uint32_t frame_history_size) {
  if (g_stats_system.enabled) {
    LOG_WARN("Render statistics system already initialized");
    return true;
  }

  memset(&g_stats_system, 0, sizeof(RenderStatsSystem));

  g_stats_system.frame_capacity =
      frame_history_size > 0 ? frame_history_size : 300;
  g_stats_system.frames =
      calloc(g_stats_system.frame_capacity, sizeof(FrameStats));

  if (!g_stats_system.frames) {
    LOG_ERROR("Failed to allocate render statistics frames");
    return false;
  }

  g_stats_system.session_start_time = get_time_nanos();
  g_stats_system.last_frame_time = g_stats_system.session_start_time;
  g_stats_system.enabled = true;
  g_stats_system.detailed_tracking = false;

  // Initialize min/max with reasonable values
  g_stats_system.min.frame_time_ms = FLT_MAX;
  g_stats_system.min.fps = FLT_MAX;
  g_stats_system.max.frame_time_ms = 0.0f;
  g_stats_system.max.fps = 0.0f;

  LOG_INFO("Render statistics system initialized (history: %u frames)",
           g_stats_system.frame_capacity);
  return true;
}

void render_stats_shutdown(void) {
  if (!g_stats_system.enabled)
    return;

  free(g_stats_system.frames);
  memset(&g_stats_system, 0, sizeof(RenderStatsSystem));

  LOG_INFO("Render statistics system shutdown");
}

void render_stats_begin_frame(void) {
  if (!g_stats_system.enabled)
    return;

  FrameStats *current = &g_stats_system.frames[g_stats_system.current_frame];
  memset(current, 0, sizeof(FrameStats));

  g_stats_system.last_frame_time = get_time_nanos();
  timer_start((Timer *)&current->frame_time_ms);
}

void render_stats_end_frame(void) {
  if (!g_stats_system.enabled)
    return;

  FrameStats *current = &g_stats_system.frames[g_stats_system.current_frame];

  // Calculate frame time
  uint64_t current_time = get_time_nanos();
  current->frame_time_ms =
      nanos_to_ms(current_time - g_stats_system.last_frame_time);
  current->fps =
      current->frame_time_ms > 0.0f ? 1000.0f / current->frame_time_ms : 0.0f;

  // Update history
  g_stats_system.fps_history[g_stats_system.history_index] = current->fps;
  g_stats_system.frame_time_history[g_stats_system.history_index] =
      current->frame_time_ms;
  g_stats_system.history_index = (g_stats_system.history_index + 1) % 60;

  // Update aggregated statistics
  g_stats_system.total.frame_count++;
  g_stats_system.total.frame_time_ms += current->frame_time_ms;
  g_stats_system.total.draw_calls += current->draw_calls;
  g_stats_system.total.triangles_drawn += current->triangles_drawn;

  // Update min/max
  if (current->frame_time_ms < g_stats_system.min.frame_time_ms) {
    g_stats_system.min.frame_time_ms = current->frame_time_ms;
  }
  if (current->frame_time_ms > g_stats_system.max.frame_time_ms) {
    g_stats_system.max.frame_time_ms = current->frame_time_ms;
  }
  if (current->fps < g_stats_system.min.fps) {
    g_stats_system.min.fps = current->fps;
  }
  if (current->fps > g_stats_system.max.fps) {
    g_stats_system.max.fps = current->fps;
  }

  // Calculate averages
  if (g_stats_system.total.frame_count > 0) {
    g_stats_system.average.frame_time_ms =
        g_stats_system.total.frame_time_ms / g_stats_system.total.frame_count;
    g_stats_system.average.fps = 1000.0f / g_stats_system.average.frame_time_ms;
  }

  // Move to next frame
  g_stats_system.current_frame =
      (g_stats_system.current_frame + 1) % g_stats_system.frame_capacity;
  g_stats_system.frame_count =
      (g_stats_system.frame_count < g_stats_system.frame_capacity)
          ? g_stats_system.frame_count + 1
          : g_stats_system.frame_capacity;
}

void render_stats_add_draw_call(uint32_t triangles) {
  if (!g_stats_system.enabled || !g_stats_system.detailed_tracking)
    return;

  FrameStats *current = &g_stats_system.frames[g_stats_system.current_frame];
  current->draw_calls++;
  current->triangles_drawn += triangles;
}

void render_stats_add_culling_stats(uint32_t total, uint32_t visible,
                                    uint32_t frustum, uint32_t distance,
                                    uint32_t occlusion) {
  if (!g_stats_system.enabled || !g_stats_system.detailed_tracking)
    return;

  FrameStats *current = &g_stats_system.frames[g_stats_system.current_frame];
  current->total_objects = total;
  current->visible_objects = visible;
  current->culled_objects = total - visible;
  current->frustum_culled = frustum;
  current->distance_culled = distance;
  current->occlusion_culled = occlusion;
}

void render_stats_add_memory_usage(uint64_t texture_mem, uint64_t buffer_mem) {
  if (!g_stats_system.enabled || !g_stats_system.detailed_tracking)
    return;

  FrameStats *current = &g_stats_system.frames[g_stats_system.current_frame];
  current->texture_memory_used = texture_mem;
  current->buffer_memory_used = buffer_mem;
  current->total_memory_used = texture_mem + buffer_mem;
}

void render_stats_add_shader_switch(void) {
  if (!g_stats_system.enabled || !g_stats_system.detailed_tracking)
    return;

  FrameStats *current = &g_stats_system.frames[g_stats_system.current_frame];
  current->shader_switches++;
}

void render_stats_add_texture_binding(void) {
  if (!g_stats_system.enabled || !g_stats_system.detailed_tracking)
    return;

  FrameStats *current = &g_stats_system.frames[g_stats_system.current_frame];
  current->texture_bindings++;
}

void render_stats_set_detailed_tracking(bool enabled) {
  g_stats_system.detailed_tracking = enabled;
  LOG_INFO("Detailed render tracking %s", enabled ? "enabled" : "disabled");
}

void render_stats_get_current_frame(FrameStats *stats) {
  if (!stats || !g_stats_system.enabled)
    return;

  *stats = g_stats_system.frames[g_stats_system.current_frame];
}

void render_stats_get_average(FrameStats *stats) {
  if (!stats || !g_stats_system.enabled)
    return;

  *stats = g_stats_system.average;
}

void render_stats_get_session_time(float *seconds) {
  if (!seconds || !g_stats_system.enabled)
    return;

  uint64_t current_time = get_time_nanos();
  *seconds =
      nanos_to_ms(current_time - g_stats_system.session_start_time) / 1000.0f;
}

void render_stats_print_summary(void) {
  if (!g_stats_system.enabled) {
    LOG_INFO("Render statistics tracking is disabled");
    return;
  }

  float session_time;
  render_stats_get_session_time(&session_time);

  LOG_INFO("=== Render Statistics Summary ===");
  LOG_INFO("Session Time: %.2f seconds", session_time);
  LOG_INFO("Total Frames: %u", g_stats_system.total.frame_count);
  LOG_INFO("Average FPS: %.2f", g_stats_system.average.fps);
  LOG_INFO("Average Frame Time: %.2f ms", g_stats_system.average.frame_time_ms);
  LOG_INFO("Min/Max Frame Time: %.2f/%.2f ms", g_stats_system.min.frame_time_ms,
           g_stats_system.max.frame_time_ms);
  LOG_INFO("Min/Max FPS: %.2f/%.2f", g_stats_system.min.fps,
           g_stats_system.max.fps);

  if (g_stats_system.detailed_tracking) {
    LOG_INFO("Total Draw Calls: %u", g_stats_system.total.draw_calls);
    LOG_INFO("Total Triangles: %u", g_stats_system.total.triangles_drawn);
    LOG_INFO("Average Draw Calls/Frame: %.2f",
             (float)g_stats_system.total.draw_calls /
                 g_stats_system.total.frame_count);
    LOG_INFO("Average Triangles/Frame: %.2f",
             (float)g_stats_system.total.triangles_drawn /
                 g_stats_system.total.frame_count);
  }

  LOG_INFO("=== End Summary ===");
}

void render_stats_print_current_frame(void) {
  if (!g_stats_system.enabled) {
    return;
  }

  FrameStats current;
  render_stats_get_current_frame(&current);

  LOG_DEBUG("Frame: %u, FPS: %.2f, Time: %.2f ms, Draws: %u, Triangles: %u",
            g_stats_system.total.frame_count, current.fps,
            current.frame_time_ms, current.draw_calls, current.triangles_drawn);

  if (g_stats_system.detailed_tracking && current.total_objects > 0) {
    LOG_DEBUG("Culling: %u total, %u visible (%.1f%%), %u culled",
              current.total_objects, current.visible_objects,
              (float)current.visible_objects / current.total_objects * 100.0f,
              current.culled_objects);
  }
}

bool render_stats_is_enabled(void) { return g_stats_system.enabled; }

bool render_stats_is_detailed_tracking_enabled(void) {
  return g_stats_system.detailed_tracking;
}
