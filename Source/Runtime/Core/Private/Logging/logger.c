/**
 * @file logger.c
 * @brief Logging system implementation
 */

#include "Core/Public/Logging/logger.h"
#include "Core/Public/Memory/memory_api.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// ============================================================================
// Internal State
// ============================================================================

static struct {
  b8 initialized;
  VF_LogLevel global_level;
  VF_LogLevel category_levels[VF_LOG_CATEGORY_COUNT];
  FILE *file_output;
  b8 console_output;
} g_logger = {0};

static const char *level_strings[] = {"TRACE", "DEBUG", "INFO",
                                      "WARN",  "ERROR", "FATAL"};

static const char *category_strings[] = {
    "Core", "Renderer", "Physics", "Audio", "Animation", "AI",    "Networking",
    "UI",   "Scene",    "Input",   "Asset", "Game",      "Editor"};

// ANSI color codes
static const char *level_colors[] = {
    "\033[90m", // TRACE - gray
    "\033[36m", // DEBUG - cyan
    "\033[32m", // INFO  - green
    "\033[33m", // WARN  - yellow
    "\033[31m", // ERROR - red
    "\033[35m", // FATAL - magenta
};
static const char *color_reset = "\033[0m";

// ============================================================================
// Initialization
// ============================================================================

VF_Result vf_logger_init(void) {
  if (g_logger.initialized) {
    return VF_ERROR_ALREADY_EXISTS;
  }

  g_logger.global_level = VF_LOG_LEVEL_INFO;
  g_logger.console_output = true;
  g_logger.file_output = NULL;

  for (i32 i = 0; i < VF_LOG_CATEGORY_COUNT; i++) {
    g_logger.category_levels[i] = VF_LOG_LEVEL_TRACE;
  }

  g_logger.initialized = true;
  return VF_SUCCESS;
}

void vf_logger_shutdown(void) {
  if (!g_logger.initialized) {
    return;
  }

  if (g_logger.file_output) {
    fclose(g_logger.file_output);
    g_logger.file_output = NULL;
  }

  g_logger.initialized = false;
}

// ============================================================================
// Configuration
// ============================================================================

void vf_logger_set_level(VF_LogLevel level) { g_logger.global_level = level; }

void vf_logger_set_category_level(VF_LogCategory category, VF_LogLevel level) {
  if (category < VF_LOG_CATEGORY_COUNT) {
    g_logger.category_levels[category] = level;
  }
}

void vf_logger_enable_file_output(const char *filepath) {
  if (g_logger.file_output) {
    fclose(g_logger.file_output);
  }

  g_logger.file_output = fopen(filepath, "w");
}

void vf_logger_enable_console_output(b8 enable) {
  g_logger.console_output = enable;
}

// ============================================================================
// Logging
// ============================================================================

void vf_log(VF_LogLevel level, VF_LogCategory category, const char *file,
            i32 line, const char *fmt, ...) {

  // Check if should log
  if (level < g_logger.global_level)
    return;
  if (category < VF_LOG_CATEGORY_COUNT &&
      level < g_logger.category_levels[category])
    return;

  // Get timestamp
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  char timestamp[32];
  strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm_info);

  // Extract filename from path
  const char *filename = strrchr(file, '/');
  filename = filename ? filename + 1 : file;

  // Format message
  char message[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);

  // Console output (with colors)
  if (g_logger.console_output) {
    fprintf(stderr, "%s[%s] [%s] [%s:%d] %s%s\n", level_colors[level],
            timestamp, level_strings[level], filename, line, message,
            color_reset);
  }

  // File output (no colors)
  if (g_logger.file_output) {
    fprintf(g_logger.file_output, "[%s] [%s] [%s] [%s:%d] %s\n", timestamp,
            level_strings[level], category_strings[category], filename, line,
            message);
    fflush(g_logger.file_output);
  }

  // Fatal: force flush and potentially abort
  if (level == VF_LOG_LEVEL_FATAL) {
    fflush(stderr);
    if (g_logger.file_output)
      fflush(g_logger.file_output);
  }
}
