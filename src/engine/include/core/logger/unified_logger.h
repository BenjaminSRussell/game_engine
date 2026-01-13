#pragma once

#include "../math/types.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

// ============================================================================
// UNIFIED LOGGING SYSTEM - CONSOLIDATED BACKENDS
// ============================================================================

// Log levels (unified from all existing systems)
typedef enum {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_WARN = 3,
    LOG_LEVEL_ERROR = 4,
    LOG_LEVEL_FATAL = 5,
    LOG_LEVEL_COUNT
} LogLevel;

// Log output channels (consolidated from all systems)
typedef enum {
    LOG_CHANNEL_CONSOLE = 1 << 0,
    LOG_CHANNEL_FILE = 1 << 1,
    LOG_CHANNEL_DEBUGGER = 1 << 2,
    LOG_CHANNEL_NETWORK = 1 << 3,
    LOG_CHANNEL_MEMORY = 1 << 4,
    LOG_CHANNEL_ALL = 0xFF
} LogChannel;

// Log categories for better organization (unified from structured logger)
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
    LOG_CAT_COUNT
} LogCategory;

// Log entry structure (consolidated from all systems)
typedef struct {
    LogLevel level;
    LogCategory category;
    LogChannel channels;
    time_t timestamp;
    const char* file;
    int line;
    const char* function;
    char message[4096];
    char context_json[1024]; // For structured logging
} LogEntry;

// Logger configuration (consolidated from all backends)
typedef struct {
    LogLevel min_level;
    LogChannel enabled_channels;
    LogCategory enabled_categories[LOG_CAT_COUNT];
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
    char remote_endpoint[256];
    char encryption_key[64];
} LoggerConfig;

// Logger statistics (unified from all systems)
typedef struct {
    u64 total_entries;
    u64 entries_per_level[LOG_LEVEL_COUNT];
    u64 entries_per_category[LOG_CAT_COUNT];
    u64 bytes_written;
    u64 files_rotated;
    u64 network_transmissions;
    u64 compression_operations;
    time_t start_time;
    time_t last_entry_time;
} LoggerStats;

// ============================================================================
// UNIFIED LOGGER API
// ============================================================================

// Logger lifecycle
bool unified_logger_init(const LoggerConfig* config);
void unified_logger_shutdown(void);
void unified_logger_flush(void);

// Configuration
void unified_logger_set_level(LogLevel level);
void unified_logger_set_channels(LogChannel channels);
void unified_logger_enable_category(LogCategory category, bool enabled);
void unified_logger_set_config(const LoggerConfig* config);
LoggerConfig unified_logger_get_config(void);

// Core logging functions
void unified_logger_log(LogLevel level, LogCategory category, const char* file, int line, 
                     const char* function, const char* format, ...);
void unified_logger_log_structured(LogLevel level, LogCategory category, const char* file, 
                              int line, const char* function, const char* message, 
                              const char* context_json);

// Convenience functions for different log levels
#define LOG_TRACE(cat, format, ...) unified_logger_log(LOG_LEVEL_TRACE, cat, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOG_DEBUG(cat, format, ...) unified_logger_log(LOG_LEVEL_DEBUG, cat, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOG_INFO(cat, format, ...)  unified_logger_log(LOG_LEVEL_INFO, cat, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOG_WARN(cat, format, ...)  unified_logger_log(LOG_LEVEL_WARN, cat, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOG_ERROR(cat, format, ...) unified_logger_log(LOG_LEVEL_ERROR, cat, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOG_FATAL(cat, format, ...) unified_logger_log(LOG_LEVEL_FATAL, cat, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

// Structured logging macros
#define LOG_TRACE_STRUCT(cat, msg, ctx) unified_logger_log_structured(LOG_LEVEL_TRACE, cat, __FILE__, __LINE__, __func__, msg, ctx)
#define LOG_DEBUG_STRUCT(cat, msg, ctx) unified_logger_log_structured(LOG_LEVEL_DEBUG, cat, __FILE__, __LINE__, __func__, msg, ctx)
#define LOG_INFO_STRUCT(cat, msg, ctx)  unified_logger_log_structured(LOG_LEVEL_INFO, cat, __FILE__, __LINE__, __func__, msg, ctx)
#define LOG_WARN_STRUCT(cat, msg, ctx)  unified_logger_log_structured(LOG_LEVEL_WARN, cat, __FILE__, __LINE__, __func__, msg, ctx)
#define LOG_ERROR_STRUCT(cat, msg, ctx) unified_logger_log_structured(LOG_LEVEL_ERROR, cat, __FILE__, __LINE__, __func__, msg, ctx)
#define LOG_FATAL_STRUCT(cat, msg, ctx) unified_logger_log_structured(LOG_LEVEL_FATAL, cat, __FILE__, __LINE__, __func__, msg, ctx)

// Advanced features
void unified_logger_rotate(void);
void unified_logger_compress_old_logs(void);
void unified_logger_send_to_remote(const LogEntry* entry);
void unified_logger_encrypt_entry(const LogEntry* entry, char* encrypted, size_t* size);
void unified_logger_benchmark(void);
LoggerStats unified_logger_get_stats(void);
void unified_logger_reset_stats(void);

// Search and query functionality
typedef struct {
    LogEntry* entries;
    u32 count;
    u32 capacity;
} LogQueryResult;

LogQueryResult unified_logger_search_by_level(LogLevel level);
LogQueryResult unified_logger_search_by_category(LogCategory category);
LogQueryResult unified_logger_search_by_time_range(time_t start, time_t end);
LogQueryResult unified_logger_search_by_text(const char* pattern);
void unified_logger_free_query_result(LogQueryResult* result);

// Backwards compatibility with existing logger systems
#define logger_init unified_logger_init
#define logger_shutdown unified_logger_shutdown
#define logger_log unified_logger_log
#define logger_flush unified_logger_flush
#define logger_get_stats unified_logger_get_stats

// Legacy compatibility macros
#define LOG(level, format, ...) LOG_DEBUG(LOG_CAT_GENERAL, format, ##__VA_ARGS__)
#define LOGE(format, ...) LOG_ERROR(LOG_CAT_GENERAL, format, ##__VA_ARGS__)
#define LOGW(format, ...) LOG_WARN(LOG_CAT_GENERAL, format, ##__VA_ARGS__)
#define LOGI(format, ...) LOG_INFO(LOG_CAT_GENERAL, format, ##__VA_ARGS__)
#define LOGD(format, ...) LOG_DEBUG(LOG_CAT_GENERAL, format, ##__VA_ARGS__)

