/**
 * @file profiler.h
 * @brief Core profiling system
 */

#ifndef PROFILER_H
#define PROFILER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const char *name;
  uint64_t start_time;
  uint64_t elapsed_us;
  uint32_t call_count;
  uint64_t total_time_us;
} ProfileScope;

/**
 * Initialize profiling system
 */
bool profiler_init(void);

/**
 * Shutdown profiling system
 */
void profiler_shutdown(void);

/**
 * Start a named profiling scope (thread-local)
 * @param name Scope name (must be string literal or persistent)
 */
void profiler_start_scope(const char *name);

/**
 * End current profiling scope
 */
void profiler_end_scope(void);

/**
 * Dump current frame stats to logger
 */
void profiler_dump_stats(void);

/**
 * Reset stats (usually per frame)
 */
void profiler_reset(void);

#ifdef __cplusplus
}
#endif

#endif // PROFILER_H
