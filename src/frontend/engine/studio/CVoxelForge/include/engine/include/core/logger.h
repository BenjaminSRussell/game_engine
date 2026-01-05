// include/core/logger.h
//
// Purpose: Defines a comprehensive and configurable logging system for the game
// engine. This header provides various log levels (DEBUG, INFO, WARN, ERROR,
// FATAL), multiple output targets (console, file), and the ability to
// categorize log messages. It supports advanced features such as log buffering,
// file rotation, context filtering, runtime configuration, performance logging,
// and assertion handling.
//
// Public APIs:
// - `LogLevel`, `LogTarget`, `LogCategory`: Enumerations for controlling log
// verbosity,
//   output destinations, and thematic categorization of messages.
// - `LogCategoryFilter`: Structure for filtering logs by category and level.
// - `Logger`: The main structure encapsulating the state and configuration of
// the logging system.
// - `g_logger`: An external global instance of the `Logger`.
// - `logger_init`, `logger_shutdown`: Lifecycle functions for the logging
// system.
// - `logger_set_level`, `logger_set_target`: Functions to dynamically adjust
// global log settings.
// - `logger_log`, `logger_log_hex`: Core functions for logging messages and hex
// dumps.
// - `logger_set_buffering`, `logger_flush_buffer`: Control log message
// buffering for performance.
// - `logger_set_category_filter`, `logger_set_category_name`: Manage
// category-specific logging.
// - `logger_set_file_rotation`, `logger_rotate_file`: Configure and perform log
// file rotation.
// - `logger_log_with_category`: Allows logging with explicit category
// specification.
// - `logger_set_session_id`, `logger_log_session_info`: Manage and log
// session-specific information.
// - `LOG_DEBUG`, `LOG_INFO`, `LOG_WARN`, `LOG_ERROR`, `LOG_FATAL`: Convenience
// macros for common logging.
// - `LOG_CAT_DEBUG`, `LOG_CAT_INFO`, etc.: Convenience macros for
// category-specific logging.
// - `ASSERT`, `ASSERTF`: Assertion macros that leverage the logging system for
// critical error reporting.
// - Numerous other functions for advanced logging control, including
// breakpoints, memory usage,
//   performance stats, JSON output, and message throttling.
//
// Ownership: The `Logger` structure manages internal state, including file
// handles and buffers. The global `g_logger` instance is the central point of
// control for the logging system.
//
// Invariants:
// - `logger_init` must be called before any logging operations, and
// `logger_shutdown` when done.
// - Log levels and targets can be dynamically changed at runtime.
// - Assertion macros (`ASSERT`, `ASSERTF`) will terminate the application on
// failure in debug builds.
// - Category filters allow fine-grained control over which messages are
// processed.
//
#ifndef LOGGER_H
#define LOGGER_H

#include "../common.h"
#include <stdio.h>
#include <time.h>
#include <pthread.h>

typedef enum {
  LOG_LEVEL_DEBUG = 0,
  LOG_LEVEL_INFO = 1,
  LOG_LEVEL_WARN = 2,
  LOG_LEVEL_ERROR = 3,
  LOG_LEVEL_FATAL = 4
} LogLevel;

typedef enum {
  LOG_TARGET_CONSOLE = 1,
  LOG_TARGET_FILE = 2,
  LOG_TARGET_BOTH = 3
} LogTarget;

typedef enum {
  LOG_CAT_GAME = 0,
  LOG_CAT_PHYSICS = 1,
  LOG_CAT_GRAPHICS = 2,
  LOG_CAT_AI = 3,
  LOG_CAT_NETWORK = 4,
  LOG_CAT_AUDIO = 5,
  LOG_CAT_MEMORY = 6,
  LOG_CAT_IO = 7,
  LOG_CAT_SCRIPT = 8,
  LOG_CAT_GENERAL = 9,
  LOG_CAT_COUNT = 10
} LogCategory;

typedef struct {
  char name[64];
  LogLevel level;
  bool enabled;
} LogCategoryFilter;

typedef struct {
  LogLevel level;
  LogTarget target;
  FILE *file;
  char filename[256];
  bool use_colors;
  bool show_timestamp;
  bool show_level;
  char log_buffer[16384];
  u32 buffer_size;
  u32 buffer_pos;
  bool use_buffering;
  u32 max_file_size;
  u32 max_backups;
  char session_id[32];
  time_t start_time;
  LogCategoryFilter category_filters[LOG_CAT_COUNT];
  bool console_only;
  pthread_mutex_t mutex;
} Logger;

extern Logger g_logger;

void logger_init(LogLevel level, LogTarget target, const char *filename);
void logger_shutdown(void);
void logger_set_level(LogLevel level);
void logger_set_target(LogTarget target);
void logger_log(LogLevel level, const char *category, const char *format, ...);
void logger_log_hex(const char *data, u32 length);

void logger_set_buffering(bool enabled, u32 buffer_size);
void logger_flush_buffer(void);

void logger_set_category_filter(LogCategory category, LogLevel level,
                                bool enabled);
void logger_set_category_name(LogCategory category, const char *name);

void logger_set_file_rotation(u32 max_size, u32 max_backups);
void logger_rotate_file(void);

void logger_log_with_category(LogCategory category, LogLevel level,
                              const char *function, const char *format, ...);

void logger_set_session_id(const char *session_id);
void logger_log_session_info(void);

void logger_set_use_colors(bool enabled);
void logger_set_show_timestamp(bool enabled);

typedef struct {
  const char *category;
  LogLevel level;
  const char *function;
  const char *message;
  u64 timestamp_ms;
  int thread_id;
} LogEntry;

#define LOG_ENTRY_RINGBUFFER_SIZE 256

void logger_breadcrumb_add(LogEntry entry);
const LogEntry *logger_breadcrumb_get_last(u32 count, u32 *out_count);

#undef LOG_DEBUG
#undef LOG_INFO
#undef LOG_WARN
#undef LOG_ERROR
#undef LOG_FATAL
#undef ASSERT

#define LOG_TRACE(fmt, ...)                                                    \
  logger_log(LOG_LEVEL_DEBUG, __func__, fmt, ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...)                                                    \
  logger_log(LOG_LEVEL_DEBUG, __func__, fmt, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...)                                                     \
  logger_log(LOG_LEVEL_INFO, __func__, fmt, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...)                                                     \
  logger_log(LOG_LEVEL_WARN, __func__, fmt, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...)                                                    \
  logger_log(LOG_LEVEL_ERROR, __func__, fmt, ##__VA_ARGS__)

#define LOG_FATAL(fmt, ...)                                                    \
  logger_log(LOG_LEVEL_FATAL, __func__, fmt, ##__VA_ARGS__)

#define LOG_CAT_DEBUG(cat, fmt, ...)                                           \
  logger_log_with_category(cat, LOG_LEVEL_DEBUG, __func__, fmt, ##__VA_ARGS__)

#define LOG_CAT_INFO(cat, fmt, ...)                                            \
  logger_log_with_category(cat, LOG_LEVEL_INFO, __func__, fmt, ##__VA_ARGS__)

#define LOG_CAT_WARN(cat, fmt, ...)                                            \
  logger_log_with_category(cat, LOG_LEVEL_WARN, __func__, fmt, ##__VA_ARGS__)

#define LOG_CAT_ERROR(cat, fmt, ...)                                           \
  logger_log_with_category(cat, LOG_LEVEL_ERROR, __func__, fmt, ##__VA_ARGS__)

#define LOG_CAT_FATAL(cat, fmt, ...)                                           \
  logger_log_with_category(cat, LOG_LEVEL_FATAL, __func__, fmt, ##__VA_ARGS__)

typedef struct {
  const char *pattern;
  bool break_on_match;
  u32 match_count;
} LogBreakpoint;

#define ASSERT(condition, message)                                             \
  if (!(condition)) {                                                          \
    LOG_FATAL("Assertion failed: %s", message);                                \
    exit(1);                                                                   \
  }

#define ASSERTF(condition, format, ...)                                        \
  if (!(condition)) {                                                          \
    LOG_FATAL("Assertion failed: " format, ##__VA_ARGS__);                     \
    exit(1);                                                                   \
  }

void logger_hex_dump(const char *data, u32 length, const char *label);
void logger_memory_usage_report(void);
void logger_set_breakpoint(const char *pattern, bool enabled);
void logger_set_millisecond_timestamps(bool enabled);
void logger_enable_function(const char *function, bool enabled);
void logger_filter_pattern(const char *pattern, bool enabled);
void logger_config_file(const char *filename);
void logger_runtime_config(const char *key, const char *value);
void logger_add_context_tag(const char *tag, const char *value);
void logger_log_performance_stats(void);
void logger_enable_json_output(bool enabled);
void logger_performance_frame_stats(u64 frame_time_ms, u32 fps);
void logger_throttle_message(const char *key, u32 max_count);
void logger_set_semantic_version(const char *version);

#endif
