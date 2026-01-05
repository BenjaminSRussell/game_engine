#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <tools/profiler.h>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/task.h>
#include <sys/sysctl.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

// =================================================================================================
//                                   ADVANCED PROFILER SYSTEM - IMPLEMENTATION
// =================================================================================================

// Advanced profiler state
typedef struct {
  bool initialized;
  f64 frame_start_time;
  f64 frame_times[3600]; // 60 seconds at 60fps
  u32 frame_index;
  u32 total_frames;

  // Advanced metrics
  ProfilerMetrics metrics;

  // Zone tracking
  ProfilerZone zones[MAX_PROFILER_ZONES];
  u32 zone_count;

  // Zone stack for nested calls
  u32 zone_stack[MAX_PROFILER_ZONES];
  u32 zone_stack_depth;

  // Memory tracking
  u64 memory_usage_samples[3600];
  u32 memory_sample_index;

  // Thread safety
  pthread_mutex_t profiler_mutex;

  // Performance counters
  u64 cpu_cycles_start;
  u64 cache_misses;
  u64 branch_misses;

  // GPU metrics (placeholder)
  f64 gpu_frame_time;
  u64 gpu_memory_used;
  u32 draw_calls;
  u32 triangles;

} AdvancedProfiler;

static AdvancedProfiler g_profiler = {0};

// MARK: - High-Resolution Timer

static f64 get_time_seconds(void) {
#ifdef __APPLE__
  static mach_timebase_info_data_t timebase;
  static bool timebase_initialized = false;

  if (!timebase_initialized) {
    mach_timebase_info(&timebase);
    timebase_initialized = true;
  }

  uint64_t time = mach_absolute_time();
  return (f64)time * (f64)timebase.numer / (f64)timebase.denom / 1e9;
#elif defined(_WIN32)
  static LARGE_INTEGER frequency;
  static bool frequency_initialized = false;

  if (!frequency_initialized) {
    QueryPerformanceFrequency(&frequency);
    frequency_initialized = true;
  }

  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return (f64)counter.QuadPart / (f64)frequency.QuadPart;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (f64)ts.tv_sec + (f64)ts.tv_nsec / 1e9;
#endif
}

static u64 get_cpu_cycles(void) {
#ifdef __x86_64__
  unsigned int aux;
  return __builtin_ia32_rdtscp(&aux);
#elif defined(__aarch64__)
  uint64_t cntvct;
  __asm__ volatile("mrs %0, cntvct_el0" : "=r"(cntvct));
  return cntvct;
#else
  return 0;
#endif
}

// MARK: - Memory Usage Tracking

static u64 get_memory_usage(void) {
#ifdef __APPLE__
  struct mach_task_basic_info info;
  mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info,
                &size) == KERN_SUCCESS) {
    return info.resident_size;
  }
#elif defined(__linux__)
  FILE *file = fopen("/proc/self/status", "r");
  if (file) {
    char line[128];
    while (fgets(line, sizeof(line), file)) {
      if (strncmp(line, "VmRSS:", 6) == 0) {
        u64 kb;
        sscanf(line, "VmRSS: %lu kB", &kb);
        fclose(file);
        return kb * 1024;
      }
    }
    fclose(file);
  }
#endif
  return 0;
}

// MARK: - System Initialization

void profiler_init(void) {
  memset(&g_profiler, 0, sizeof(g_profiler));
  pthread_mutex_init(&g_profiler.profiler_mutex, NULL);

  g_profiler.initialized = true;
  g_profiler.metrics.min_frame_time = 1e9;
  g_profiler.metrics.min_cpu_time = 1e9;
  g_profiler.metrics.min_gpu_time = 1e9;

  // Initialize performance counters
  g_profiler.cpu_cycles_start = get_cpu_cycles();

  printf("Advanced profiler initialized\n");
}

void profiler_shutdown(void) {
  if (!g_profiler.initialized)
    return;

  profiler_export_detailed_report("profiler_report.txt");

  pthread_mutex_destroy(&g_profiler.profiler_mutex);
  memset(&g_profiler, 0, sizeof(g_profiler));

  printf("Advanced profiler shutdown\n");
}

// MARK: - Frame Management

void profiler_begin_frame(void) {
  if (!g_profiler.initialized)
    return;

  pthread_mutex_lock(&g_profiler.profiler_mutex);

  g_profiler.frame_start_time = get_time_seconds();
  g_profiler.cpu_cycles_start = get_cpu_cycles();

  // Reset frame-specific metrics
  g_profiler.draw_calls = 0;
  g_profiler.triangles = 0;

  pthread_mutex_unlock(&g_profiler.profiler_mutex);
}

void profiler_end_frame(void) {
  if (!g_profiler.initialized)
    return;

  pthread_mutex_lock(&g_profiler.profiler_mutex);

  f64 frame_end_time = get_time_seconds();
  f64 frame_time = frame_end_time - g_profiler.frame_start_time;

  // Update rolling window
  g_profiler.frame_times[g_profiler.frame_index] = frame_time;
  g_profiler.frame_index = (g_profiler.frame_index + 1) % 3600;

  // Update basic stats
  g_profiler.metrics.frame_time = frame_time;
  g_profiler.metrics.fps = 1.0 / frame_time;
  g_profiler.metrics.frame_count++;
  g_profiler.total_frames++;

  if (frame_time < g_profiler.metrics.min_frame_time) {
    g_profiler.metrics.min_frame_time = frame_time;
  }
  if (frame_time > g_profiler.metrics.max_frame_time) {
    g_profiler.metrics.max_frame_time = frame_time;
  }

  // Calculate average from rolling window
  f64 sum = 0.0;
  u32 count = 0;
  for (u32 i = 0; i < 3600 && i < g_profiler.total_frames; i++) {
    sum += g_profiler.frame_times[i];
    count++;
  }
  g_profiler.metrics.avg_frame_time = sum / count;

  // Update memory usage
  u64 current_memory = get_memory_usage();
  g_profiler.memory_usage_samples[g_profiler.memory_sample_index] =
      current_memory;
  g_profiler.memory_sample_index = (g_profiler.memory_sample_index + 1) % 3600;

  g_profiler.metrics.memory_usage = current_memory;
  if (current_memory > g_profiler.metrics.peak_memory) {
    g_profiler.metrics.peak_memory = current_memory;
  }

  // Update GPU metrics
  g_profiler.metrics.gpu_frame_time = g_profiler.gpu_frame_time;
  g_profiler.metrics.gpu_memory_used = g_profiler.gpu_memory_used;
  g_profiler.metrics.draw_calls = g_profiler.draw_calls;
  g_profiler.metrics.triangles = g_profiler.triangles;

  pthread_mutex_unlock(&g_profiler.profiler_mutex);
}

// MARK: - Zone Profiling

void profiler_begin_zone(const char *name) {
  if (!g_profiler.initialized || !name)
    return;

  pthread_mutex_lock(&g_profiler.profiler_mutex);

  // Find or create zone
  u32 zone_idx = MAX_PROFILER_ZONES;
  for (u32 i = 0; i < g_profiler.zone_count; i++) {
    if (strcmp(g_profiler.zones[i].name, name) == 0) {
      zone_idx = i;
      break;
    }
  }

  if (zone_idx == MAX_PROFILER_ZONES) {
    if (g_profiler.zone_count >= MAX_PROFILER_ZONES) {
      pthread_mutex_unlock(&g_profiler.profiler_mutex);
      return;
    }
    zone_idx = g_profiler.zone_count++;
    strncpy(g_profiler.zones[zone_idx].name, name, MAX_ZONE_NAME_LENGTH - 1);
    g_profiler.zones[zone_idx].name[MAX_ZONE_NAME_LENGTH - 1] = '\0';
  }

  ProfilerZone *zone = &g_profiler.zones[zone_idx];
  zone->start_time = get_time_seconds();
  zone->start_cycles = get_cpu_cycles();
  zone->active = true;

  // Push to stack
  if (g_profiler.zone_stack_depth < MAX_PROFILER_ZONES) {
    g_profiler.zone_stack[g_profiler.zone_stack_depth++] = zone_idx;
  }

  pthread_mutex_unlock(&g_profiler.profiler_mutex);
}

void profiler_end_zone(const char *name) {
  if (!g_profiler.initialized || !name)
    return;

  pthread_mutex_lock(&g_profiler.profiler_mutex);

  f64 end_time = get_time_seconds();
  u64 end_cycles = get_cpu_cycles();

  // Pop from stack
  if (g_profiler.zone_stack_depth == 0) {
    pthread_mutex_unlock(&g_profiler.profiler_mutex);
    return;
  }

  u32 zone_idx = g_profiler.zone_stack[--g_profiler.zone_stack_depth];
  ProfilerZone *zone = &g_profiler.zones[zone_idx];

  if (!zone->active) {
    pthread_mutex_unlock(&g_profiler.profiler_mutex);
    return;
  }

  f64 elapsed_time = end_time - zone->start_time;
  u64 elapsed_cycles = end_cycles - zone->start_cycles;

  zone->total_time += elapsed_time;
  zone->total_cycles += elapsed_cycles;
  zone->call_count++;
  zone->active = false;

  // Update min/max
  if (elapsed_time < zone->min_time || zone->min_time == 0) {
    zone->min_time = elapsed_time;
  }
  if (elapsed_time > zone->max_time) {
    zone->max_time = elapsed_time;
  }

  pthread_mutex_unlock(&g_profiler.profiler_mutex);
}

// MARK: - GPU Metrics

void profiler_begin_gpu_zone(const char *name) {
  if (!g_profiler.initialized || !name)
    return;

  // This would integrate with GPU profiling APIs
  // For now, just track the zone name
  (void)name;
}

void profiler_end_gpu_zone(const char *name) {
  if (!g_profiler.initialized || !name)
    return;

  // This would integrate with GPU profiling APIs
  (void)name;
}

void profiler_add_draw_call(u32 triangle_count) {
  if (!g_profiler.initialized)
    return;

  pthread_mutex_lock(&g_profiler.profiler_mutex);
  g_profiler.draw_calls++;
  g_profiler.triangles += triangle_count;
  pthread_mutex_unlock(&g_profiler.profiler_mutex);
}

void profiler_update_gpu_memory_usage(u64 memory_used) {
  if (!g_profiler.initialized)
    return;

  pthread_mutex_lock(&g_profiler.profiler_mutex);
  g_profiler.gpu_memory_used = memory_used;
  pthread_mutex_unlock(&g_profiler.profiler_mutex);
}

// MARK: - Statistics and Reporting

ProfilerMetrics profiler_get_metrics(void) {
  pthread_mutex_lock(&g_profiler.profiler_mutex);
  ProfilerMetrics metrics = g_profiler.metrics;
  pthread_mutex_unlock(&g_profiler.profiler_mutex);
  return metrics;
}

void profiler_reset_metrics(void) {
  if (!g_profiler.initialized)
    return;

  pthread_mutex_lock(&g_profiler.profiler_mutex);

  memset(&g_profiler.metrics, 0, sizeof(g_profiler.metrics));
  g_profiler.metrics.min_frame_time = 1e9;
  g_profiler.metrics.min_cpu_time = 1e9;
  g_profiler.metrics.min_gpu_time = 1e9;

  g_profiler.frame_index = 0;
  g_profiler.memory_sample_index = 0;
  g_profiler.total_frames = 0;

  memset(g_profiler.frame_times, 0, sizeof(g_profiler.frame_times));
  memset(g_profiler.memory_usage_samples, 0,
         sizeof(g_profiler.memory_usage_samples));

  // Reset zones
  for (u32 i = 0; i < g_profiler.zone_count; i++) {
    memset(&g_profiler.zones[i], 0, sizeof(ProfilerZone));
  }
  g_profiler.zone_count = 0;
  g_profiler.zone_stack_depth = 0;

  pthread_mutex_unlock(&g_profiler.profiler_mutex);
}

void profiler_export_csv(const char *filepath) {
  if (!g_profiler.initialized || !filepath)
    return;

  pthread_mutex_lock(&g_profiler.profiler_mutex);

  FILE *f = fopen(filepath, "w");
  if (!f) {
    pthread_mutex_unlock(&g_profiler.profiler_mutex);
    return;
  }

  // Export zone statistics
  fprintf(f, "Zone,Total Time (ms),Call Count,Avg Time (ms),Min Time (ms),Max "
             "Time (ms),CPU Cycles\n");
  for (u32 i = 0; i < g_profiler.zone_count; i++) {
    ProfilerZone *zone = &g_profiler.zones[i];
    f64 avg_time =
        zone->call_count > 0 ? zone->total_time / zone->call_count : 0.0;
    fprintf(f, "%s,%.3f,%u,%.3f,%.3f,%.3f,%llu\n", zone->name,
            zone->total_time * 1000.0, zone->call_count, avg_time * 1000.0,
            zone->min_time * 1000.0, zone->max_time * 1000.0,
            zone->total_cycles);
  }

  fclose(f);
  pthread_mutex_unlock(&g_profiler.profiler_mutex);

  printf("Profiler stats exported to: %s\n", filepath);
}

void profiler_export_detailed_report(const char *filepath) {
  if (!g_profiler.initialized || !filepath)
    return;

  pthread_mutex_lock(&g_profiler.profiler_mutex);

  FILE *f = fopen(filepath, "w");
  if (!f) {
    pthread_mutex_unlock(&g_profiler.profiler_mutex);
    return;
  }

  // Header
  fprintf(f, "=== MINECRAFT V2 PROFILER REPORT ===\n");
  fprintf(f, "Generated: %s", ctime(&(time_t){time(NULL)}));
  fprintf(f, "\n");

  // Frame statistics
  fprintf(f, "=== FRAME STATISTICS ===\n");
  fprintf(f, "Total Frames: %u\n", g_profiler.metrics.frame_count);
  fprintf(f, "Current FPS: %.1f\n", g_profiler.metrics.fps);
  fprintf(f, "Avg Frame Time: %.2f ms\n",
          g_profiler.metrics.avg_frame_time * 1000.0);
  fprintf(f, "Min Frame Time: %.2f ms\n",
          g_profiler.metrics.min_frame_time * 1000.0);
  fprintf(f, "Max Frame Time: %.2f ms\n",
          g_profiler.metrics.max_frame_time * 1000.0);
  fprintf(f, "\n");

  // Memory statistics
  fprintf(f, "=== MEMORY STATISTICS ===\n");
  fprintf(f, "Current Memory: %.2f MB\n",
          g_profiler.metrics.memory_usage / (1024.0 * 1024.0));
  fprintf(f, "Peak Memory: %.2f MB\n",
          g_profiler.metrics.peak_memory / (1024.0 * 1024.0));
  fprintf(f, "\n");

  // GPU statistics
  fprintf(f, "=== GPU STATISTICS ===\n");
  fprintf(f, "GPU Frame Time: %.2f ms\n",
          g_profiler.metrics.gpu_frame_time * 1000.0);
  fprintf(f, "GPU Memory: %.2f MB\n",
          g_profiler.metrics.gpu_memory_used / (1024.0 * 1024.0));
  fprintf(f, "Draw Calls: %u\n", g_profiler.metrics.draw_calls);
  fprintf(f, "Triangles: %u\n", g_profiler.metrics.triangles);
  fprintf(f, "\n");

  // Zone statistics
  fprintf(f, "=== ZONE STATISTICS ===\n");
  fprintf(f, "%-20s %10s %8s %10s %10s %10s %12s\n", "Zone", "Total(ms)",
          "Calls", "Avg(ms)", "Min(ms)", "Max(ms)", "CPU Cycles");
  fprintf(f, "%-20s %10s %8s %10s %10s %10s %12s\n", "----", "--------",
          "-----", "-------", "-------", "-------", "----------");

  // Sort zones by total time
  ProfilerZone sorted_zones[MAX_PROFILER_ZONES];
  memcpy(sorted_zones, g_profiler.zones, sizeof(sorted_zones));

  for (u32 i = 0; i < g_profiler.zone_count - 1; i++) {
    for (u32 j = i + 1; j < g_profiler.zone_count; j++) {
      if (sorted_zones[j].total_time > sorted_zones[i].total_time) {
        ProfilerZone temp = sorted_zones[i];
        sorted_zones[i] = sorted_zones[j];
        sorted_zones[j] = temp;
      }
    }
  }

  for (u32 i = 0; i < g_profiler.zone_count; i++) {
    ProfilerZone *zone = &sorted_zones[i];
    f64 avg_time =
        zone->call_count > 0 ? zone->total_time / zone->call_count : 0.0;
    fprintf(f, "%-20s %10.3f %8u %10.3f %10.3f %10.3f %12llu\n", zone->name,
            zone->total_time * 1000.0, zone->call_count, avg_time * 1000.0,
            zone->min_time * 1000.0, zone->max_time * 1000.0,
            zone->total_cycles);
  }

  fclose(f);
  pthread_mutex_unlock(&g_profiler.profiler_mutex);

  printf("Detailed profiler report exported to: %s\n", filepath);
}

void profiler_print_summary(void) {
  if (!g_profiler.initialized)
    return;

  pthread_mutex_lock(&g_profiler.profiler_mutex);

  printf("=== Profiler Summary ===\n");
  printf("Frame Count: %u\n", g_profiler.metrics.frame_count);
  printf("Current FPS: %.1f\n", g_profiler.metrics.fps);
  printf("Avg Frame Time: %.2f ms\n",
         g_profiler.metrics.avg_frame_time * 1000.0);
  printf("Min Frame Time: %.2f ms\n",
         g_profiler.metrics.min_frame_time * 1000.0);
  printf("Max Frame Time: %.2f ms\n",
         g_profiler.metrics.max_frame_time * 1000.0);
  printf("Memory Usage: %.2f MB\n",
         g_profiler.metrics.memory_usage / (1024.0 * 1024.0));
  printf("Peak Memory: %.2f MB\n",
         g_profiler.metrics.peak_memory / (1024.0 * 1024.0));

  printf("\n=== Top 5 Zones by Time ===\n");

  // Sort zones by total time
  ProfilerZone sorted_zones[MAX_PROFILER_ZONES];
  memcpy(sorted_zones, g_profiler.zones, sizeof(sorted_zones));

  for (u32 i = 0; i < g_profiler.zone_count - 1; i++) {
    for (u32 j = i + 1; j < g_profiler.zone_count; j++) {
      if (sorted_zones[j].total_time > sorted_zones[i].total_time) {
        ProfilerZone temp = sorted_zones[i];
        sorted_zones[i] = sorted_zones[j];
        sorted_zones[j] = temp;
      }
    }
  }

  u32 display_count = g_profiler.zone_count < 5 ? g_profiler.zone_count : 5;
  for (u32 i = 0; i < display_count; i++) {
    ProfilerZone *zone = &sorted_zones[i];
    f64 avg_time =
        zone->call_count > 0 ? zone->total_time / zone->call_count : 0.0;
    printf("%-20s: %.3f ms total, %u calls, %.3f ms avg\n", zone->name,
           zone->total_time * 1000.0, zone->call_count, avg_time * 1000.0);
  }

  pthread_mutex_unlock(&g_profiler.profiler_mutex);
}
