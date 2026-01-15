/**
 * @file profiler.c
 * @brief Implementation of core profiling system
 */

#include "core/profiling/profiler.h"
#include "core/logging/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#define MAX_SCOPES 256
#define MAX_DEPTH 32

typedef struct {
  char name[64];
  uint64_t total_time;
  uint32_t count;
  uint64_t min_time;
  uint64_t max_time;
} ScopeStats;

typedef struct {
  ScopeStats scopes[MAX_SCOPES];
  uint32_t scope_count;

  // Active scope stack
  uint32_t stack[MAX_DEPTH];
  uint64_t start_times[MAX_DEPTH];
  uint32_t depth;

  bool initialized;
} ProfilerState;

static ProfilerState g_profiler = {0};

static uint64_t get_time_us(void) {
#ifdef _WIN32
  LARGE_INTEGER frequency, counter;
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&counter);
  return (counter.QuadPart * 1000000) / frequency.QuadPart;
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
#endif
}

static uint32_t get_scope_index(const char *name) {
  for (uint32_t i = 0; i < g_profiler.scope_count; i++) {
    if (strcmp(g_profiler.scopes[i].name, name) == 0) {
      return i;
    }
  }

  // Create new scope
  if (g_profiler.scope_count < MAX_SCOPES) {
    uint32_t index = g_profiler.scope_count++;
    strncpy(g_profiler.scopes[index].name, name,
            sizeof(g_profiler.scopes[index].name) - 1);
    g_profiler.scopes[index].min_time = UINT64_MAX;
    g_profiler.scopes[index].max_time = 0;
    return index;
  }

  return UINT32_MAX;
}

bool profiler_init(void) {
  memset(&g_profiler, 0, sizeof(ProfilerState));
  g_profiler.initialized = true;
  LOG_INFO("PROFILER", "Profiler initialized");
  return true;
}

void profiler_shutdown(void) { g_profiler.initialized = false; }

void profiler_start_scope(const char *name) {
  if (!g_profiler.initialized)
    return;
  if (g_profiler.depth >= MAX_DEPTH)
    return;

  uint32_t index = get_scope_index(name);
  if (index == UINT32_MAX)
    return;

  g_profiler.stack[g_profiler.depth] = index;
  g_profiler.start_times[g_profiler.depth] = get_time_us();
  g_profiler.depth++;
}

void profiler_end_scope(void) {
  if (!g_profiler.initialized)
    return;
  if (g_profiler.depth == 0)
    return;

  g_profiler.depth--;
  uint32_t index = g_profiler.stack[g_profiler.depth];
  uint64_t duration = get_time_us() - g_profiler.start_times[g_profiler.depth];

  ScopeStats *stats = &g_profiler.scopes[index];
  stats->total_time += duration;
  stats->count++;

  if (duration < stats->min_time)
    stats->min_time = duration;
  if (duration > stats->max_time)
    stats->max_time = duration;
}

void profiler_dump_stats(void) {
  if (!g_profiler.initialized)
    return;

  LOG_INFO("PROFILER", "=== Frame Stats ===");
  for (uint32_t i = 0; i < g_profiler.scope_count; i++) {
    ScopeStats *stats = &g_profiler.scopes[i];
    if (stats->count > 0) {
      LOG_INFO("PROFILER", "  %s: %llu us (avg: %llu, count: %u)", stats->name,
               stats->total_time, stats->total_time / stats->count,
               stats->count);
    }
  }
}

void profiler_reset(void) {
  if (!g_profiler.initialized)
    return;

  for (uint32_t i = 0; i < g_profiler.scope_count; i++) {
    g_profiler.scopes[i].total_time = 0;
    g_profiler.scopes[i].count = 0;
    g_profiler.scopes[i].min_time = UINT64_MAX;
    g_profiler.scopes[i].max_time = 0;
  }
}
