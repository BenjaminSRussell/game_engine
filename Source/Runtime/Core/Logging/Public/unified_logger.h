#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

// Basic type definitions
#ifndef u32
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t i32;
#endif

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

// Typedefs removed to avoid conflicts with core/types.h

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
  LOG_CAT_SYSTEM = 17,
  LOG_CAT_COUNT
} LogCategory;

typedef enum {
  LOG_FORMAT_MINIMAL,
  LOG_FORMAT_DEVELOPMENT,
  LOG_FORMAT_PRODUCTION,
  LOG_FORMAT_DEBUG,
  LOG_FORMAT_JSON
} LogFormat;

// Log entry structure
typedef struct {
  LogLevel level;
  LogCategory category;
  // LogChannel channels; // Not used in entry creation typically
  u64 timestamp_ms; // Millisecond precision
  i32 thread_id;
  const char *file;
  int line;
  const char *function;
  char message[4096];
  // char context_json[1024]; // Removed if not used in refactor
} LogEntry;

// Configuration Structs
typedef struct {
  LogLevel min_level;
  bool enabled;
  LogChannel channels;
} LogCategoryFilter;

typedef struct {
  char log_filename[512];
  struct {
    size_t max_file_size_mb;
    int max_backup_files;
  } rotation;

  LogLevel global_level;
  LogChannel active_channels; // bitmask
  LogFormat format;

  bool use_colors;
  bool show_timestamp;
  bool show_level;
  bool show_category;
  bool flush_on_write;
  bool enable_breadcrumbs;
  bool thread_safe;
  bool enable_buffering;
  bool enable_message_throttling;
  u32 buffer_size;

  LogCategoryFilter category_filters[LOG_CAT_COUNT];
} LogConfig;

// Alias LoggerConfig for backward compatibility if needed, but we used
// LogConfig in .c
typedef LogConfig LoggerConfig;

// ============================================================================
// UNIFIED LOGGER API
// ============================================================================

bool logger_init(const LogConfig *config);
void logger_shutdown(void);
bool logger_is_initialized(void);

void logger_log(LogLevel level, LogCategory category, const char *file,
                int line, const char *function, const char *format, ...);

void logger_log_hex(LogLevel level, LogCategory category, const void *data,
                    size_t length, const char *label);

// Config API
void logger_set_level(LogLevel level);
void logger_set_channels(LogChannel channels);
void logger_set_format(LogFormat format);
void logger_set_category_filter(LogCategory category, LogLevel min_level,
                                bool enabled);
void logger_get_config(LogConfig *out_config);
void logger_apply_config(const LogConfig *config);

// Stats/Utils
void logger_flush(void);
void logger_get_stats(u64 *total_messages, u64 *start_time,
                      u32 *breadcrumb_count);
void logger_print_stats(void);

// Breadcrumbs
void logger_breadcrumb_clear(void);
const LogEntry *logger_breadcrumb_get_last(u32 count, u32 *out_count);
void logger_breadcrumb_set_enabled(bool enabled);

// Session
void logger_set_session_id(const char *session_id);
const char *logger_get_session_id(void);
void logger_set_engine_version(const char *version);
void logger_set_build_info(const char *build_info);

// Formatting helpers (public if needed, else private)
const char *logger_level_to_string(LogLevel level);
const char *logger_category_to_string(LogCategory category);
u64 logger_get_timestamp_ms(void);

// Compatibility mappings
#define unified_logger_init logger_init
#define unified_logger_shutdown logger_shutdown
#define unified_logger_log logger_log
// ... add others as needed

// Macros
#define LOG_TRACE(cat, fmt, ...)                                               \
  logger_log(LOG_LEVEL_TRACE, cat, __FILE__, __LINE__, __func__, fmt,          \
             ##__VA_ARGS__)
#define LOG_DEBUG(cat, fmt, ...)                                               \
  logger_log(LOG_LEVEL_DEBUG, cat, __FILE__, __LINE__, __func__, fmt,          \
             ##__VA_ARGS__)
#define LOG_INFO(cat, fmt, ...)                                                \
  logger_log(LOG_LEVEL_INFO, cat, __FILE__, __LINE__, __func__, fmt,           \
             ##__VA_ARGS__)
#define LOG_WARN(cat, fmt, ...)                                                \
  logger_log(LOG_LEVEL_WARN, cat, __FILE__, __LINE__, __func__, fmt,           \
             ##__VA_ARGS__)
#define LOG_ERROR(cat, fmt, ...)                                               \
  logger_log(LOG_LEVEL_ERROR, cat, __FILE__, __LINE__, __func__, fmt,          \
             ##__VA_ARGS__)
#define LOG_FATAL(cat, fmt, ...)                                               \
  logger_log(LOG_LEVEL_FATAL, cat, __FILE__, __LINE__, __func__, fmt,          \
             ##__VA_ARGS__)

// Generic Macros
#define LOG_TRACE_GEN(fmt, ...) LOG_TRACE(LOG_CAT_GENERAL, fmt, ##__VA_ARGS__)
// etc...
