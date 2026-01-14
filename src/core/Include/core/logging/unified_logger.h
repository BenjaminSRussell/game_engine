#pragma once

#include "../math/types.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

// ============================================================================
// UNIFIED LOGGING SYSTEM - CONSOLIDATED
// ============================================================================

// Log levels
typedef enum {
  LOG_LEVEL_TRACE = 0,
  LOG_LEVEL_DEBUG = 1,
  LOG_LEVEL_INFO = 2,
  LOG_LEVEL_WARN = 3,
  LOG_LEVEL_ERROR = 4,
  LOG_LEVEL_FATAL = 5,
  LOG_LEVEL_COUNT
} LogLevel;

// Log output channels
typedef enum {
  LOG_CHANNEL_CONSOLE = 1 << 0,
  LOG_CHANNEL_FILE = 1 << 1,
  LOG_CHANNEL_DEBUGGER = 1 << 2,
  LOG_CHANNEL_NETWORK = 1 << 3,
  LOG_CHANNEL_MEMORY = 1 << 4,
  LOG_CHANNEL_ALL = 0xFF
} LogChannel;

// Log categories
typedef enum {
  LOG_CAT_GENERAL = 0,
  LOG_CAT_GAME = 1,
  LOG_CAT_PHYSICS = 2,
  LOG_CAT_GRAPHICS = 3,
  LOG_CAT_RENDERER = 4,
  LOG_CAT_AI = 5,
  LOG_CAT_NETWORK = 6,
  LOG_CAT_AUDIO = 7,
  LOG_CAT_MEMORY = 8,
  LOG_CAT_IO = 9,
  LOG_CAT_SCRIPT = 10,
  LOG_CAT_PLATFORM = 11,
  LOG_CAT_EDITOR = 12,
  LOG_CAT_ASSET = 13,
  LOG_CAT_ANIMATION = 14,
  LOG_CAT_INPUT = 15,
  LOG_CAT_UI = 16,
  LOG_CAT_COUNT
} LogCategory;

// Log entry structure
typedef struct {
  LogLevel level;
  LogCategory category;
  LogChannel channels;
  time_t timestamp;
  const char *file;
  int line;
  const char *function;
  char message[4096];
  char context_json[1024];
} LogEntry;

// Logger configuration
typedef struct {
  LogLevel min_level;
  LogChannel enabled_channels;
  bool enabled_categories[LOG_CAT_COUNT];
  bool use_colors;
  bool show_timestamp;
  bool show_file_line;
  bool show_function;
  bool use_buffering;
  bool enable_rotation;
  bool enable_compression;
  bool enable_remote;
  bool enable_encryption;
  char log_file_path[512];
  size_t max_file_size;
  int max_backup_files;
} LoggerConfig;

// ============================================================================
// UNIFIED LOGGER API
// ============================================================================

bool unified_logger_init(const LoggerConfig *config);
void unified_logger_shutdown(void);
void unified_logger_flush(void);
void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...);

// Category-based logging macros
#define LOG_TRACE_CAT(cat, format, ...)                                        \
  unified_logger_log(LOG_LEVEL_TRACE, cat, __FILE__, __LINE__, __func__,       \
                     format, ##__VA_ARGS__)
#define LOG_DEBUG_CAT(cat, format, ...)                                        \
  unified_logger_log(LOG_LEVEL_DEBUG, cat, __FILE__, __LINE__, __func__,       \
                     format, ##__VA_ARGS__)
#define LOG_INFO_CAT(cat, format, ...)                                         \
  unified_logger_log(LOG_LEVEL_INFO, cat, __FILE__, __LINE__, __func__,        \
                     format, ##__VA_ARGS__)
#define LOG_WARN_CAT(cat, format, ...)                                         \
  unified_logger_log(LOG_LEVEL_WARN, cat, __FILE__, __LINE__, __func__,        \
                     format, ##__VA_ARGS__)
#define LOG_ERROR_CAT(cat, format, ...)                                        \
  unified_logger_log(LOG_LEVEL_ERROR, cat, __FILE__, __LINE__, __func__,       \
                     format, ##__VA_ARGS__)
#define LOG_FATAL_CAT(cat, format, ...)                                        \
  unified_logger_log(LOG_LEVEL_FATAL, cat, __FILE__, __LINE__, __func__,       \
                     format, ##__VA_ARGS__)

// Legacy/General-purpose logging macros (Defaults to LOG_CAT_GENERAL)
#define LOG_TRACE(format, ...)                                                 \
  LOG_TRACE_CAT(LOG_CAT_GENERAL, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)                                                 \
  LOG_DEBUG_CAT(LOG_CAT_GENERAL, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)                                                  \
  LOG_INFO_CAT(LOG_CAT_GENERAL, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)                                                  \
  LOG_WARN_CAT(LOG_CAT_GENERAL, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...)                                                 \
  LOG_ERROR_CAT(LOG_CAT_GENERAL, format, ##__VA_ARGS__)
#define LOG_FATAL(format, ...)                                                 \
  LOG_FATAL_CAT(LOG_CAT_GENERAL, format, ##__VA_ARGS__)

// Shorthand/Legacy aliases
#define log_trace LOG_TRACE
#define log_debug LOG_DEBUG
#define log_info LOG_INFO
#define log_warn LOG_WARN
#define log_error LOG_ERROR
#define log_fatal LOG_FATAL

#define LOGD LOG_DEBUG
#define LOGI LOG_INFO
#define LOGW LOG_WARN
#define LOGE LOG_ERROR

// Compatibility constants
#define LOG_CAT_IO_INFO LOG_CAT_IO
#define LOG_CAT_IO_ERROR LOG_CAT_IO
#define LOG_CAT_NET_INFO LOG_CAT_NETWORK
#define LOG_CAT_NET_ERROR LOG_CAT_NETWORK

// Compatibility aliases
#define logger_init unified_logger_init
#define logger_shutdown unified_logger_shutdown
#define logger_log unified_logger_log
#define logger_flush unified_logger_flush
#define logger_get_stats unified_logger_get_stats
#define logger_benchmark unified_logger_benchmark
