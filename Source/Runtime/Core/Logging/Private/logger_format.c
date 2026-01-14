#include "logger_private.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

// ============================================================================
// UTILITY FUNCTIONS implementation
// ============================================================================

u32 hash_string(const char *str) {
  u32 hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

u64 get_time_ms(void) {
#ifdef _WIN32
  LARGE_INTEGER frequency, counter;
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&counter);
  return (counter.QuadPart * 1000) / frequency.QuadPart;
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (u64)tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

void format_timestamp(u64 timestamp_ms, char *buffer, size_t buffer_size) {
  time_t seconds = timestamp_ms / 1000;
  int milliseconds = timestamp_ms % 1000;

  struct tm *tm_info = localtime(&seconds);
  snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, milliseconds);
}

// Helper to get string representations (could be in core logic, but fits
// formatting utils) But wait, the original file had static helpers. We need
// them accessible. They are not exposed in private header yet, but if only
// sinks use them, sinks can implement or we expose them. Let's expose them in
// private header or duplicate small helpers? Actually, `logger_level_to_string`
// is public API, but `get_level_string` was static. We can implement
// `get_level_string` here and expose it in private header if needed by sinks.

const char *get_level_string(LogLevel level) {
  switch (level) {
  case LOG_LEVEL_TRACE:
    return "TRACE";
  case LOG_LEVEL_DEBUG:
    return "DEBUG";
  case LOG_LEVEL_INFO:
    return "INFO";
  case LOG_LEVEL_WARN:
    return "WARN";
  case LOG_LEVEL_ERROR:
    return "ERROR";
  case LOG_LEVEL_FATAL:
    return "FATAL";
  default:
    return "UNKNOWN";
  }
}

const char *get_category_string(LogCategory category) {
  switch (category) {
  case LOG_CAT_GENERAL:
    return "GENERAL";
  case LOG_CAT_GAME:
    return "GAME";
  case LOG_CAT_PHYSICS:
    return "PHYSICS";
  case LOG_CAT_GRAPHICS:
    return "GRAPHICS";
  case LOG_CAT_RENDERER:
    return "RENDERER";
  case LOG_CAT_AI:
    return "AI";
  case LOG_CAT_NETWORK:
    return "NETWORK";
  case LOG_CAT_AUDIO:
    return "AUDIO";
  case LOG_CAT_MEMORY:
    return "MEMORY";
  case LOG_CAT_IO:
    return "IO";
  case LOG_CAT_SCRIPT:
    return "SCRIPT";
  case LOG_CAT_PLATFORM:
    return "PLATFORM";
  case LOG_CAT_EDITOR:
    return "EDITOR";
  case LOG_CAT_ASSET:
    return "ASSET";
  case LOG_CAT_ANIMATION:
    return "ANIMATION";
  case LOG_CAT_INPUT:
    return "INPUT";
  case LOG_CAT_UI:
    return "UI";
  default:
    return "UNKNOWN";
  }
}
