// performance.c - Minimal implementation to bypass corruption
#include <core/performance.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

Profiler g_profiler = {0};
FrameStats g_frame_stats = {0};

static f64 internal_get_time(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec / 1e9;
}

Timer *perf_timer_create(const char *name) {
  Timer *timer = (Timer *)malloc(sizeof(Timer));
  if (timer) {
    timer->name = name;
    timer->start_time = 0.0;
    timer->elapsed_time = 0.0;
    timer->frame_count = 0;
    timer->total_time = 0.0;
  }
  return timer;
}

void perf_timer_destroy(Timer *timer) {
  if (timer)
    free(timer);
}

void perf_timer_start(Timer *timer) {
  if (timer)
    timer->start_time = internal_get_time();
}

void perf_timer_stop(Timer *timer) {
  if (timer) {
    timer->elapsed_time = internal_get_time() - timer->start_time;
    timer->total_time += timer->elapsed_time;
    timer->frame_count++;
  }
}

f64 perf_timer_get_elapsed(Timer *timer) {
  return timer ? timer->elapsed_time : 0.0;
}

void perf_timer_reset(Timer *timer) {
  if (timer) {
    timer->elapsed_time = 0.0;
    timer->frame_count = 0;
    timer->total_time = 0.0;
  }
}

void profiler_init(void) {
  memset(&g_profiler, 0, sizeof(Profiler));
  g_profiler.enabled = true;
  LOG_GENERAL_DEBUG("Profiler initialized");
}

void profiler_shutdown(void) {
  profiler_report();
  memset(&g_profiler, 0, sizeof(Profiler));
}

static ProfilerEntry *profiler_get_entry(const char *name) {
  for (u32 i = 0; i < g_profiler.entry_count; i++) {
    if (strcmp(g_profiler.entries[i].name, name) == 0) {
      return &g_profiler.entries[i];
    }
  }

  if (g_profiler.entry_count < 64) {
    ProfilerEntry *entry = &g_profiler.entries[g_profiler.entry_count];
    entry->name = name;
    entry->call_count = 0;
    entry->total_time = 0.0;
    entry->min_time = 1e9;
    entry->max_time = 0.0;
    entry->avg_time = 0.0;
    g_profiler.entry_count++;
    return entry;
  }

  return NULL;
}

void profiler_start(const char *name) {
  if (!g_profiler.enabled)
    return;

  ProfilerEntry *entry = profiler_get_entry(name);
  if (entry) {
    entry->start_time = internal_get_time();
  }
}

void profiler_stop(const char *name) {
  if (!g_profiler.enabled)
    return;

  f64 current_time = internal_get_time();

  for (u32 i = 0; i < g_profiler.entry_count; i++) {
    ProfilerEntry *entry = &g_profiler.entries[i];
    if (entry->name && strcmp(entry->name, name) == 0) {
      f64 elapsed = current_time - entry->start_time;

      entry->call_count++;
      entry->total_time += elapsed;
      entry->min_time = (elapsed < entry->min_time) ? elapsed : entry->min_time;
      entry->max_time = (elapsed > entry->max_time) ? elapsed : entry->max_time;
      entry->avg_time = entry->total_time / entry->call_count;

      return;
    }
  }
}

void profiler_reset(void) {
  for (u32 i = 0; i < g_profiler.entry_count; i++) {
    g_profiler.entries[i].call_count = 0;
    g_profiler.entries[i].total_time = 0.0;
    g_profiler.entries[i].min_time = 1e9;
    g_profiler.entries[i].max_time = 0.0;
    g_profiler.entries[i].avg_time = 0.0;
  }
}

void profiler_report(void) {
  LOG_GENERAL_INFO("=== Profiler Report ===");
  LOG_GENERAL_INFO("%-40s %10s %12s %12s %12s", "Name", "Calls", "Total (ms)",
                   "Avg (ms)", "Max (ms)");
  LOG_GENERAL_INFO("%s", "");

  for (u32 i = 0; i < g_profiler.entry_count; i++) {
    ProfilerEntry *entry = &g_profiler.entries[i];
    if (entry->call_count > 0) {
      LOG_GENERAL_INFO("%-40s %10u %12.3f %12.3f %12.3f", entry->name,
                       entry->call_count, entry->total_time * 1000.0,
                       entry->avg_time * 1000.0, entry->max_time * 1000.0);
    }
  }
}

void profiler_enable(bool enable) { g_profiler.enabled = enable; }

void frame_stats_update(f64 delta_time) {
  g_frame_stats.frame_time = delta_time;
  g_frame_stats.frame_count++;

  if (g_frame_stats.min_frame_time == 0 ||
      delta_time < g_frame_stats.min_frame_time) {
    g_frame_stats.min_frame_time = delta_time;
  }

  if (delta_time > g_frame_stats.max_frame_time) {
    g_frame_stats.max_frame_time = delta_time;
  }

  if (delta_time > 0.0) {
    g_frame_stats.fps = 1.0f / (f32)delta_time;
  }
}

void frame_stats_reset(void) { memset(&g_frame_stats, 0, sizeof(FrameStats)); }

f32 frame_stats_get_fps(void) { return g_frame_stats.fps; }

void logger_errno_context(int err_code) {
  extern int errno;
  const char *err_str = strerror(err_code ? err_code : errno);
  LOG_GENERAL_ERROR("System error [%d]: %s", err_code ? err_code : errno,
                    err_str);
}

void performance_frame_breakdown(void) {
  LOG_GENERAL_INFO("=== FRAME TIME BREAKDOWN ===");
  LOG_GENERAL_INFO("Total frame time: %.2fms",
                   g_frame_stats.frame_time * 1000.0);
  LOG_GENERAL_INFO("FPS: %.1f", g_frame_stats.fps);
  LOG_GENERAL_INFO("Frame count: %u", g_frame_stats.frame_count);
}

typedef struct {
  u64 peak_memory;
  u64 current_memory;
  f64 spike_threshold;
} MemoryMonitor;

static MemoryMonitor g_memory_monitor = {0};

void memory_spike_detector_init(u64 threshold) {
  g_memory_monitor.spike_threshold = (f64)threshold;
  g_memory_monitor.peak_memory = 0;
}

void memory_spike_detector_check(u64 current_memory) {
  g_memory_monitor.current_memory = current_memory;

  if (current_memory > g_memory_monitor.peak_memory) {
    g_memory_monitor.peak_memory = current_memory;
  }

  if (current_memory > (g_memory_monitor.peak_memory * 0.9)) {
    LOG_GENERAL_WARN("Memory spike detected: %llu bytes (peak: %llu)",
                     (unsigned long long)current_memory,
                     (unsigned long long)g_memory_monitor.peak_memory);
  }
}

void profiler_call_count_increment(const char *function) {
  if (!function)
    return;

  for (u32 i = 0; i < g_profiler.entry_count; i++) {
    if (g_profiler.entries[i].name &&
        strcmp(g_profiler.entries[i].name, function) == 0) {
      g_profiler.entries[i].call_count++;
      return;
    }
  }
}

u32 profiler_get_call_count(const char *function) {
  if (!function)
    return 0;

  for (u32 i = 0; i < g_profiler.entry_count; i++) {
    if (g_profiler.entries[i].name &&
        strcmp(g_profiler.entries[i].name, function) == 0) {
      return g_profiler.entries[i].call_count;
    }
  }
  return 0;
}

static struct {
  const char *scope_names[64];
  u32 scope_depth;
} g_scope_tracking = {0};

int profiler_scope_enter(const char *name) {
  if (g_scope_tracking.scope_depth < 64) {
    g_scope_tracking.scope_names[g_scope_tracking.scope_depth] = name;
    g_scope_tracking.scope_depth++;
    profiler_call_count_increment(name);
  }
  return 0; // Return explicit handled scope ID (0 for stack-based default)
}

void profiler_scope_exit(int scope_id) {
  (void)scope_id; // Unused for stack-based tracking
  if (g_scope_tracking.scope_depth > 0) {
    g_scope_tracking.scope_depth--;
  }
}

void physics_profile_start(void) { profiler_scope_enter("physics_update"); }

void physics_profile_end(void) { profiler_scope_exit(0); }

void entity_lifecycle_log(const char *event, const char *entity_type) {
  LOG_GAME_DEBUG("Entity event: %s for %s", event, entity_type);
}

void world_gen_profile_start(void) { profiler_scope_enter("world_generation"); }

void world_gen_profile_end(void) { profiler_scope_exit(0); }

void chunk_lifecycle_log(int chunk_x, int chunk_z, const char *event) {
  LOG_GAME_DEBUG("Chunk [%d,%d] %s", chunk_x, chunk_z, event);
}

void npc_behavior_log(const char *npc_name, const char *behavior) {
  LOG_AI_DEBUG("NPC %s: %s", npc_name, behavior);
}

void input_event_log(const char *input_type, const char *action) {
  LOG_GAME_DEBUG("Input %s: %s", input_type, action);
}

void crafting_event_log(const char *recipe_name, bool success) {
  LOG_GAME_INFO("Crafting %s: %s", recipe_name, success ? "SUCCESS" : "FAILED");
}

void save_load_log(const char *operation, const char *target) {
  // There is no LOG_IO_INFO macro defined in unified_logger.h as per my read.
  // I should use LOG_GENERAL_INFO with category LOG_CAT_IO or verify macros.
  // Looking at unified_logger.h read earlier:
  // It has LOG_GENERAL_*, LOG_GAME_*, LOG_PHYSICS_*, LOG_GRAPHICS_*,
  // LOG_RENDERER_*, LOG_AI_*, LOG_NETWORK_*, LOG_AUDIO_*, LOG_MEMORY_* It does
  // NOT have LOG_IO_*. So I will use logger_log directly for IO.
  logger_log(LOG_LEVEL_INFO, LOG_CAT_IO, __FILE__, __LINE__, __func__,
             "Save/Load operation: %s on %s", operation, target);
}
