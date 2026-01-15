/**
 * @file logger.c
 * @brief Implementation of unified logging system
 */

#include "core/logging/logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

// Maximum log message size
#define MAX_LOG_SIZE 4096
#define MAX_MODULE_NAME 32

typedef struct {
  char log_file_path[256];
  FILE *log_file;
  LogLevel global_level;
  bool enable_file;
  bool enable_console;

  // Filtering
  char module_filter[256];
  bool has_filter;

  // Statistics
  uint32_t message_counts[LOG_LEVEL_COUNT];

#ifdef _WIN32
  HANDLE mutex;
  HANDLE console_handle;
#else
  pthread_mutex_t mutex;
#endif

  bool initialized;
} LoggerState;

static LoggerState g_logger = {0};

// Internal helpers
static void lock(void) {
#ifdef _WIN32
  if (g_logger.mutex)
    WaitForSingleObject(g_logger.mutex, INFINITE);
#else
  if (g_logger.initialized)
    pthread_mutex_lock(&g_logger.mutex);
#endif
}

static void unlock(void) {
#ifdef _WIN32
  if (g_logger.mutex)
    ReleaseMutex(g_logger.mutex);
#else
  if (g_logger.initialized)
    pthread_mutex_unlock(&g_logger.mutex);
#endif
}

static void get_timestamp(char *buffer, size_t size) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

// Colors for console
static const char *level_colors[] = {
    "\x1b[90m", // TRACE (Gray)
    "\x1b[36m", // DEBUG (Cyan)
    "\x1b[32m", // INFO  (Green)
    "\x1b[33m", // WARN  (Yellow)
    "\x1b[31m", // ERROR (Red)
    "\x1b[35m"  // FATAL (Magenta)
};

static const char *level_names[] = {"TRACE", "DEBUG", "INFO",
                                    "WARN",  "ERROR", "FATAL"};

bool logger_init(const char *log_file) {
  if (g_logger.initialized)
    return true;

  memset(&g_logger, 0, sizeof(LoggerState));
  g_logger.global_level = LOG_INFO;
  g_logger.enable_console = true;
  g_logger.enable_file = (log_file != NULL);

  if (g_logger.enable_file) {
    strncpy(g_logger.log_file_path, log_file,
            sizeof(g_logger.log_file_path) - 1);
    g_logger.log_file = fopen(log_file, "a"); // Append mode
    if (!g_logger.log_file) {
      fprintf(stderr, "Failed to open log file: %s\n", log_file);
      g_logger.enable_file = false;
    }
  }

#ifdef _WIN32
  g_logger.mutex = CreateMutex(NULL, FALSE, NULL);
  g_logger.console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  GetConsoleMode(g_logger.console_handle, &mode);
  SetConsoleMode(g_logger.console_handle,
                 mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
  pthread_mutex_init(&g_logger.mutex, NULL);
#endif

  g_logger.initialized = true;
  return true;
}

void logger_shutdown(void) {
  if (!g_logger.initialized)
    return;

  lock();
  if (g_logger.log_file) {
    fclose(g_logger.log_file);
    g_logger.log_file = NULL;
  }
  unlock();

#ifdef _WIN32
  CloseHandle(g_logger.mutex);
#else
  pthread_mutex_destroy(&g_logger.mutex);
#endif

  g_logger.initialized = false;
}

void logger_set_level(LogLevel level) { g_logger.global_level = level; }

LogLevel logger_get_level(void) { return g_logger.global_level; }

void logger_set_file_logging(bool enabled) {
  lock();
  if (enabled && !g_logger.log_file && strlen(g_logger.log_file_path) > 0) {
    g_logger.log_file = fopen(g_logger.log_file_path, "a");
  }
  g_logger.enable_file = enabled;
  unlock();
}

void logger_set_console_logging(bool enabled) {
  g_logger.enable_console = enabled;
}

void logger_set_module_filter(const char *module_filter) {
  lock();
  if (module_filter) {
    strncpy(g_logger.module_filter, module_filter,
            sizeof(g_logger.module_filter) - 1);
    g_logger.has_filter = true;
  } else {
    g_logger.has_filter = false;
  }
  unlock();
}

void logger_clear_module_filter(void) {
  lock();
  g_logger.has_filter = false;
  memset(g_logger.module_filter, 0, sizeof(g_logger.module_filter));
  unlock();
}

void logger_log(LogLevel level, const char *module, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logger_logv(level, module, fmt, args);
  va_end(args);
}

void logger_logv(LogLevel level, const char *module, const char *fmt,
                 va_list args) {
  if (!g_logger.initialized)
    return;
  if (level < g_logger.global_level)
    return;

  lock();

  // Check module filter
  if (g_logger.has_filter && module) {
    if (strstr(g_logger.module_filter, module) == NULL) {
      unlock();
      return;
    }
  }

  g_logger.message_counts[level]++;

  char timestamp[32];
  get_timestamp(timestamp, sizeof(timestamp));

  char message[MAX_LOG_SIZE];
  vsnprintf(message, sizeof(message), fmt, args);

  // Console Output
  if (g_logger.enable_console) {
    const char *color = level < LOG_LEVEL_COUNT ? level_colors[level] : "";
    const char *reset = "\x1b[0m";
    const char *level_name =
        level < LOG_LEVEL_COUNT ? level_names[level] : "UNKNOWN";

    // [TIME] [LEVEL] [MODULE] Message
    printf("%s[%s] %s[%s]%s [%s] %s\n", "\x1b[90m", timestamp,
           reset,                    // Gray timestamp
           color, level_name, reset, // Colored level
           module ? module : "GENERAL", message);
  }

  // File Output
  if (g_logger.enable_file && g_logger.log_file) {
    const char *level_name =
        level < LOG_LEVEL_COUNT ? level_names[level] : "UNKNOWN";
    fprintf(g_logger.log_file, "[%s] [%s] [%s] %s\n", timestamp, level_name,
            module ? module : "GENERAL", message);
    fflush(g_logger.log_file); // Ensure it's written (can be slow, maybe
                               // optimize later)
  }

  unlock();
}

void logger_group_begin(const char *name) {
  logger_log(LOG_INFO, "GROUP", ">>> %s", name);
}

void logger_group_end(void) { logger_log(LOG_INFO, "GROUP", "<<<"); }

void logger_flush(void) {
  lock();
  if (g_logger.log_file)
    fflush(g_logger.log_file);
  fflush(stdout);
  unlock();
}

void logger_dump_to_file(const char *filepath) {
  // Current implementation writes to file continuously if enabled.
  // If we implemented internal buffering, this would dump the buffer.
  // For now, this is a no-op as we flush on write.
  (void)filepath;
}

uint32_t logger_get_message_count(LogLevel level) {
  if (level >= LOG_LEVEL_COUNT)
    return 0;
  return g_logger.message_counts[level];
}

void logger_clear_stats(void) {
  lock();
  memset(g_logger.message_counts, 0, sizeof(g_logger.message_counts));
  unlock();
}

void logger_print_stats(void) {
  lock();
  printf("=== Logger Statistics ===\n");
  for (int i = 0; i < LOG_LEVEL_COUNT; i++) {
    printf("  %s: %u\n", level_names[i], g_logger.message_counts[i]);
  }
  printf("=========================\n");
  unlock();
}
