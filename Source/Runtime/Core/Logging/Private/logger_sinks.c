#include "logger_private.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

// Forward decls of format helpers if not in header
const char *get_level_string(LogLevel level);
const char *get_category_string(LogCategory category);

// ============================================================================
// COLOR UTILS
// ============================================================================

static void get_color_code(LogLevel level, char *color_code,
                           size_t buffer_size) {
#ifdef _WIN32
  // Windows console colors are handled differently (via SetConsoleTextAttribute
  // typically, but original code just did (void) and relied on separate
  // platform init for VT codes? Original code: "Initialize Windows console for
  // colors... ENABLE_VIRTUAL_TERMINAL_PROCESSING" So ANSI codes work on Windows
  // 10+. But original code snippet's get_color_code #ifdef _WIN32 block was
  // empty? Wait, let's check step 281 lines 137-142. It says: #ifdef _WIN32
  // (void)level; ...
  // #else
  // switch(level) ...
  // #endif
  // So on Windows it returned empty string? That means colors were disabled or
  // handled differently? Yet logger_platform_init enabled VT processing. Maybe
  // the original code intended to use ANSI on Windows too but the #ifdef
  // blocked it? Or maybe it relies on verify separate calls. I will enable ANSI
  // for Windows too if VT is enabled. For now I'll replicate original behavior:
  // EMPTY for Windows.
  (void)level;
  (void)color_code;
  (void)buffer_size;
#else
  switch (level) {
  case LOG_LEVEL_TRACE:
    snprintf(color_code, buffer_size, "\033[0;37m");
    break; // White
  case LOG_LEVEL_DEBUG:
    snprintf(color_code, buffer_size, "\033[1;34m");
    break; // Blue
  case LOG_LEVEL_INFO:
    snprintf(color_code, buffer_size, "\033[1;36m");
    break; // Cyan
  case LOG_LEVEL_WARN:
    snprintf(color_code, buffer_size, "\033[1;33m");
    break; // Yellow
  case LOG_LEVEL_ERROR:
    snprintf(color_code, buffer_size, "\033[1;31m");
    break; // Red
  case LOG_LEVEL_FATAL:
    snprintf(color_code, buffer_size, "\033[1;35m");
    break; // Magenta
  default:
    snprintf(color_code, buffer_size, "\033[0m");
    break; // Reset
  }
#endif
}

static void reset_color(void) {
#ifndef _WIN32
  printf("\033[0m");
#endif
}

// ============================================================================
// SINK IMPLEMENTATION
// ============================================================================

void output_to_console(LogLevel level, LogCategory category,
                       const LogEntry *entry) {
  if (!g_logger || !(g_logger->config.active_channels & LOG_CHANNEL_CONSOLE)) {
    return;
  }

  char color_code[16] = {0};
  char timestamp[32];
  format_timestamp(entry->timestamp_ms, timestamp, sizeof(timestamp));

  if (g_logger->config.use_colors) {
    get_color_code(level, color_code, sizeof(color_code));
    printf("%s", color_code);
  }

  switch (g_logger->config.format) {
  case LOG_FORMAT_MINIMAL:
    printf("%s\n", entry->message);
    break;

  case LOG_FORMAT_DEVELOPMENT:
    printf("[%s] [%s] %s\n", timestamp, get_level_string(level),
           entry->message);
    break;

  case LOG_FORMAT_PRODUCTION:
    printf("[%s] [%s] [%s] %s\n", timestamp, get_level_string(level),
           get_category_string(category), entry->message);
    break;

  case LOG_FORMAT_DEBUG:
    printf("[%s] [%s] [%s] %s:%d %s() %s\n", timestamp, get_level_string(level),
           get_category_string(category), entry->file ? entry->file : "unknown",
           entry->line, entry->function ? entry->function : "unknown",
           entry->message);
    break;

  case LOG_FORMAT_JSON:
    printf("{\"timestamp\":\"%s\",\"level\":\"%s\",\"category\":\"%s\","
           "\"message\":\"%s\"}\n",
           timestamp, get_level_string(level), get_category_string(category),
           entry->message);
    break;
  }

  if (g_logger->config.use_colors) {
    reset_color();
  }
}

void output_to_file(LogLevel level, LogCategory category,
                    const LogEntry *entry) {
  if (!g_logger || !(g_logger->config.active_channels & LOG_CHANNEL_FILE) ||
      !g_logger->log_file) {
    return;
  }

  char timestamp[32];
  format_timestamp(entry->timestamp_ms, timestamp, sizeof(timestamp));

  switch (g_logger->config.format) {
  case LOG_FORMAT_MINIMAL:
    fprintf(g_logger->log_file, "%s\n", entry->message);
    break;

  case LOG_FORMAT_DEVELOPMENT:
    fprintf(g_logger->log_file, "[%s] [%s] %s\n", timestamp,
            get_level_string(level), entry->message);
    break;

  case LOG_FORMAT_PRODUCTION:
    fprintf(g_logger->log_file, "[%s] [%s] [%s] %s\n", timestamp,
            get_level_string(level), get_category_string(category),
            entry->message);
    break;

  case LOG_FORMAT_DEBUG:
    fprintf(g_logger->log_file, "[%s] [%s] [%s] %s:%d %s() %s\n", timestamp,
            get_level_string(level), get_category_string(category),
            entry->file ? entry->file : "unknown", entry->line,
            entry->function ? entry->function : "unknown", entry->message);
    break;

  case LOG_FORMAT_JSON:
    fprintf(g_logger->log_file,
            "{\"timestamp\":\"%s\",\"level\":\"%s\",\"category\":\"%s\","
            "\"message\":\"%s\"}\n",
            timestamp, get_level_string(level), get_category_string(category),
            entry->message);
    break;
  }

  if (g_logger->config.flush_on_write) {
    fflush(g_logger->log_file);
  }

  g_logger->current_file_size += strlen(entry->message) + 64; // Rough estimate

  // Check rotation
  if (g_logger->current_file_size >
      g_logger->config.rotation.max_file_size_mb * 1024 * 1024) {
    logger_rotate_file();
  }
}

void output_to_debugger(LogLevel level, const LogEntry *entry) {
  if (!g_logger || !(g_logger->config.active_channels & LOG_CHANNEL_DEBUGGER)) {
    return;
  }

  char output[MAX_LOG_MESSAGE_SIZE];
  snprintf(output, sizeof(output), "[%s] %s", get_level_string(level),
           entry->message);

#ifdef _WIN32
  OutputDebugStringA(output);
  OutputDebugStringA("\n");
#elif defined(__APPLE__)
  // macOS - write to system log or stderr
  fprintf(stderr, "%s\n", output);
#else
  // Linux
  fprintf(stderr, "%s\n", output);
#endif
}
