#pragma once

#include "../math/types.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

// ============================================================================
// UNIFIED LOGGING SYSTEM
// ============================================================================

// Log levels (unified from both systems)
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
    LOG_CHANNEL_MEMORY = 1 << 4
} LogChannel;

// Log categories for better organization
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

// Log output formats
typedef enum {
    LOG_FORMAT_MINIMAL,      // Just message
    LOG_FORMAT_DEVELOPMENT,   // Timestamp + Level + Message
    LOG_FORMAT_PRODUCTION,   // Timestamp + Level + Category + Message
    LOG_FORMAT_DEBUG,        // Full: Timestamp + Level + Category + File:Line + Function + Message
    LOG_FORMAT_JSON          // JSON structured logging
} LogFormat;

// Log entry structure for advanced features
typedef struct {
    LogLevel level;
    LogCategory category;
    u64 timestamp_ms;
    i32 thread_id;
    const char* file;
    i32 line;
    const char* function;
    char message[4096];
    char context_tags[8][64]; // Additional context information
    u32 context_tag_count;
} LogEntry;

// Category filter configuration
typedef struct {
    char name[64];
    LogLevel min_level;
    bool enabled;
    LogChannel channels;
} LogCategoryFilter;

// File rotation configuration
typedef struct {
    u32 max_file_size_mb;
    u32 max_backup_files;
    bool compress_backups;
    char backup_pattern[256]; // e.g., "voxelforge_%Y%m%d_%H%M%S.log"
} LogRotationConfig;

// Performance logging
typedef struct {
    u64 frame_time_ns;
    u64 cpu_time_ns;
    u64 gpu_time_ns;
    u32 draw_calls;
    u32 triangles;
    f32 memory_usage_mb;
    f32 fps;
} LogPerformanceStats;

// Breadcrumb trail for debugging
#define LOG_BREADCRUMB_MAX 64
typedef struct {
    LogEntry entries[LOG_BREADCRUMB_MAX];
    u32 head;
    u32 count;
    bool enabled;
} LogBreadcrumbTrail;

// Log configuration structure
typedef struct {
    // Basic settings
    LogLevel global_level;
    LogChannel active_channels;
    LogFormat format;
    
    // File settings
    char log_filename[256];
    LogRotationConfig rotation;
    bool flush_on_write;
    
    // Console settings
    bool use_colors;
    bool show_timestamp;
    bool show_level;
    bool show_category;
    bool show_file_line;
    bool show_function;
    
    // Advanced features
    bool enable_breadcrumbs;
    bool enable_performance_logging;
    bool enable_json_output;
    bool enable_context_tags;
    bool enable_message_throttling;
    u32 max_throttled_messages_per_second;
    
    // Filtering
    LogCategoryFilter category_filters[LOG_CAT_COUNT];
    
    // Thread safety
    bool thread_safe;
    
    // Buffering
    bool enable_buffering;
    u32 buffer_size;
    u32 flush_interval_ms;
} LogConfig;

// Logger state
typedef struct Logger {
    LogConfig config;
    FILE* log_file;
    u64 current_file_size;
    u64 total_messages_logged;
    u64 start_time_ms;
    
    // Buffering
    char* log_buffer;
    u32 buffer_pos;
    u64 last_flush_time;
    
    // Breadcrumb trail
    LogBreadcrumbTrail breadcrumbs;
    
    // Performance tracking
    LogPerformanceStats current_frame;
    LogPerformanceStats average_frame;
    u64 frame_count;
    
    // Thread safety
    void* mutex; // Platform-specific mutex
    
    // State
    bool initialized;
    bool shutdown_requested;
} Logger;

// Global logger instance
extern Logger* g_logger;

// ============================================================================
// CORE LOGGING API
// ============================================================================

// Lifecycle management
bool logger_init(const LogConfig* config);
void logger_shutdown(void);
bool logger_is_initialized(void);

// Configuration
void logger_set_level(LogLevel level);
void logger_set_channels(LogChannel channels);
void logger_set_format(LogFormat format);
void logger_set_category_filter(LogCategory category, LogLevel min_level, bool enabled);
void logger_get_config(LogConfig* out_config);
void logger_apply_config(const LogConfig* config);

// Core logging functions
void logger_log(LogLevel level, LogCategory category, const char* file, int line, 
               const char* function, const char* format, ...);
void logger_log_entry(const LogEntry* entry);
void logger_log_hex(LogLevel level, LogCategory category, const void* data, size_t length, 
                   const char* label);

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

// Basic logging macros
#define LOG_TRACE(category, format, ...) \
    logger_log(LOG_LEVEL_TRACE, category, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define LOG_DEBUG(category, format, ...) \
    logger_log(LOG_LEVEL_DEBUG, category, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define LOG_INFO(category, format, ...) \
    logger_log(LOG_LEVEL_INFO, category, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define LOG_WARN(category, format, ...) \
    logger_log(LOG_LEVEL_WARN, category, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define LOG_ERROR(category, format, ...) \
    logger_log(LOG_LEVEL_ERROR, category, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define LOG_FATAL(category, format, ...) \
    logger_log(LOG_LEVEL_FATAL, category, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

// Category-specific convenience macros
#define LOG_GENERAL_TRACE(format, ...) LOG_TRACE(LOG_CAT_GENERAL, format, ##__VA_ARGS__)
#define LOG_GENERAL_DEBUG(format, ...) LOG_DEBUG(LOG_CAT_GENERAL, format, ##__VA_ARGS__)
#define LOG_GENERAL_INFO(format, ...)  LOG_INFO(LOG_CAT_GENERAL, format, ##__VA_ARGS__)
#define LOG_GENERAL_WARN(format, ...)  LOG_WARN(LOG_CAT_GENERAL, format, ##__VA_ARGS__)
#define LOG_GENERAL_ERROR(format, ...) LOG_ERROR(LOG_CAT_GENERAL, format, ##__VA_ARGS__)
#define LOG_GENERAL_FATAL(format, ...) LOG_FATAL(LOG_CAT_GENERAL, format, ##__VA_ARGS__)

#define LOG_GAME_TRACE(format, ...) LOG_TRACE(LOG_CAT_GAME, format, ##__VA_ARGS__)
#define LOG_GAME_DEBUG(format, ...) LOG_DEBUG(LOG_CAT_GAME, format, ##__VA_ARGS__)
#define LOG_GAME_INFO(format, ...)  LOG_INFO(LOG_CAT_GAME, format, ##__VA_ARGS__)
#define LOG_GAME_WARN(format, ...)  LOG_WARN(LOG_CAT_GAME, format, ##__VA_ARGS__)
#define LOG_GAME_ERROR(format, ...) LOG_ERROR(LOG_CAT_GAME, format, ##__VA_ARGS__)
#define LOG_GAME_FATAL(format, ...) LOG_FATAL(LOG_CAT_GAME, format, ##__VA_ARGS__)

#define LOG_PHYSICS_TRACE(format, ...) LOG_TRACE(LOG_CAT_PHYSICS, format, ##__VA_ARGS__)
#define LOG_PHYSICS_DEBUG(format, ...) LOG_DEBUG(LOG_CAT_PHYSICS, format, ##__VA_ARGS__)
#define LOG_PHYSICS_INFO(format, ...)  LOG_INFO(LOG_CAT_PHYSICS, format, ##__VA_ARGS__)
#define LOG_PHYSICS_WARN(format, ...)  LOG_WARN(LOG_CAT_PHYSICS, format, ##__VA_ARGS__)
#define LOG_PHYSICS_ERROR(format, ...) LOG_ERROR(LOG_CAT_PHYSICS, format, ##__VA_ARGS__)
#define LOG_PHYSICS_FATAL(format, ...) LOG_FATAL(LOG_CAT_PHYSICS, format, ##__VA_ARGS__)

#define LOG_GRAPHICS_TRACE(format, ...) LOG_TRACE(LOG_CAT_GRAPHICS, format, ##__VA_ARGS__)
#define LOG_GRAPHICS_DEBUG(format, ...) LOG_DEBUG(LOG_CAT_GRAPHICS, format, ##__VA_ARGS__)
#define LOG_GRAPHICS_INFO(format, ...)  LOG_INFO(LOG_CAT_GRAPHICS, format, ##__VA_ARGS__)
#define LOG_GRAPHICS_WARN(format, ...)  LOG_WARN(LOG_CAT_GRAPHICS, format, ##__VA_ARGS__)
#define LOG_GRAPHICS_ERROR(format, ...) LOG_ERROR(LOG_CAT_GRAPHICS, format, ##__VA_ARGS__)
#define LOG_GRAPHICS_FATAL(format, ...) LOG_FATAL(LOG_CAT_GRAPHICS, format, ##__VA_ARGS__)

#define LOG_RENDERER_TRACE(format, ...) LOG_TRACE(LOG_CAT_RENDERER, format, ##__VA_ARGS__)
#define LOG_RENDERER_DEBUG(format, ...) LOG_DEBUG(LOG_CAT_RENDERER, format, ##__VA_ARGS__)
#define LOG_RENDERER_INFO(format, ...)  LOG_INFO(LOG_CAT_RENDERER, format, ##__VA_ARGS__)
#define LOG_RENDERER_WARN(format, ...)  LOG_WARN(LOG_CAT_RENDERER, format, ##__VA_ARGS__)
#define LOG_RENDERER_ERROR(format, ...) LOG_ERROR(LOG_CAT_RENDERER, format, ##__VA_ARGS__)
#define LOG_RENDERER_FATAL(format, ...) LOG_FATAL(LOG_CAT_RENDERER, format, ##__VA_ARGS__)

#define LOG_AI_TRACE(format, ...) LOG_TRACE(LOG_CAT_AI, format, ##__VA_ARGS__)
#define LOG_AI_DEBUG(format, ...) LOG_DEBUG(LOG_CAT_AI, format, ##__VA_ARGS__)
#define LOG_AI_INFO(format, ...)  LOG_INFO(LOG_CAT_AI, format, ##__VA_ARGS__)
#define LOG_AI_WARN(format, ...)  LOG_WARN(LOG_CAT_AI, format, ##__VA_ARGS__)
#define LOG_AI_ERROR(format, ...) LOG_ERROR(LOG_CAT_AI, format, ##__VA_ARGS__)
#define LOG_AI_FATAL(format, ...) LOG_FATAL(LOG_CAT_AI, format, ##__VA_ARGS__)

#define LOG_NETWORK_TRACE(format, ...) LOG_TRACE(LOG_CAT_NETWORK, format, ##__VA_ARGS__)
#define LOG_NETWORK_DEBUG(format, ...) LOG_DEBUG(LOG_CAT_NETWORK, format, ##__VA_ARGS__)
#define LOG_NETWORK_INFO(format, ...)  LOG_INFO(LOG_CAT_NETWORK, format, ##__VA_ARGS__)
#define LOG_NETWORK_WARN(format, ...)  LOG_WARN(LOG_CAT_NETWORK, format, ##__VA_ARGS__)
#define LOG_NETWORK_ERROR(format, ...) LOG_ERROR(LOG_CAT_NETWORK, format, ##__VA_ARGS__)
#define LOG_NETWORK_FATAL(format, ...) LOG_FATAL(LOG_CAT_NETWORK, format, ##__VA_ARGS__)

#define LOG_AUDIO_TRACE(format, ...) LOG_TRACE(LOG_CAT_AUDIO, format, ##__VA_ARGS__)
#define LOG_AUDIO_DEBUG(format, ...) LOG_DEBUG(LOG_CAT_AUDIO, format, ##__VA_ARGS__)
#define LOG_AUDIO_INFO(format, ...)  LOG_INFO(LOG_CAT_AUDIO, format, ##__VA_ARGS__)
#define LOG_AUDIO_WARN(format, ...)  LOG_WARN(LOG_CAT_AUDIO, format, ##__VA_ARGS__)
#define LOG_AUDIO_ERROR(format, ...) LOG_ERROR(LOG_CAT_AUDIO, format, ##__VA_ARGS__)
#define LOG_AUDIO_FATAL(format, ...) LOG_FATAL(LOG_CAT_AUDIO, format, ##__VA_ARGS__)

#define LOG_MEMORY_TRACE(format, ...) LOG_TRACE(LOG_CAT_MEMORY, format, ##__VA_ARGS__)
#define LOG_MEMORY_DEBUG(format, ...) LOG_DEBUG(LOG_CAT_MEMORY, format, ##__VA_ARGS__)
#define LOG_MEMORY_INFO(format, ...)  LOG_INFO(LOG_CAT_MEMORY, format, ##__VA_ARGS__)
#define LOG_MEMORY_WARN(format, ...)  LOG_WARN(LOG_CAT_MEMORY, format, ##__VA_ARGS__)
#define LOG_MEMORY_ERROR(format, ...) LOG_ERROR(LOG_CAT_MEMORY, format, ##__VA_ARGS__)
#define LOG_MEMORY_FATAL(format, ...) LOG_FATAL(LOG_CAT_MEMORY, format, ##__VA_ARGS__)

// Legacy compatibility macros
#define LOG(format, ...) LOG_GENERAL_INFO(format, ##__VA_ARGS__)

// ============================================================================
// ADVANCED FEATURES
// ============================================================================

// Breadcrumb trail for debugging crashes
void logger_breadcrumb_add(const LogEntry* entry);
void logger_breadcrumb_clear(void);
const LogEntry* logger_breadcrumb_get_last(u32 count, u32* out_count);
void logger_breadcrumb_set_enabled(bool enabled);

// Performance logging
void logger_performance_frame_begin(void);
void logger_performance_frame_end(const LogPerformanceStats* stats);
void logger_performance_log_current_frame(void);
void logger_performance_log_average(void);
void logger_performance_reset_stats(void);

// Context tags for additional information
void logger_context_tag_add(const char* tag, const char* value);
void logger_context_tag_remove(const char* tag);
void logger_context_tag_clear(void);

// Message throttling to prevent log spam
void logger_throttle_reset(const char* key);
bool logger_throttle_check(const char* key, u32 max_per_second);

// Assertion macros with logging
#ifdef NDEBUG
#define ASSERT_LOG(condition, category, format, ...) \
    do { if (!(condition)) LOG_FATAL(category, "Assertion failed: " format, ##__VA_ARGS__); } while(0)
#define ASSERT_LOGF(condition, category, format, ...) \
    do { if (!(condition)) LOG_FATAL(category, "Assertion failed: " format, ##__VA_ARGS__); } while(0)
#else
#define ASSERT_LOG(condition, category, format, ...) \
    do { \
        if (!(condition)) { \
            LOG_FATAL(category, "Assertion failed: " format, ##__VA_ARGS__); \
            __builtin_trap(); \
        } \
    } while(0)
#define ASSERT_LOGF(condition, category, format, ...) \
    do { \
        if (!(condition)) { \
            LOG_FATAL(category, "Assertion failed: " format, ##__VA_ARGS__); \
            __builtin_trap(); \
        } \
    } while(0)
#endif

// ============================================================================
// FILE AND BUFFER MANAGEMENT
// ============================================================================

// File operations
void logger_set_file(const char* filename);
void logger_rotate_file(void);
void logger_flush(void);
void logger_flush_buffer(void);

// Buffer management
void logger_set_buffering(bool enabled, u32 buffer_size);
void logger_flush_interval_set(u32 interval_ms);

// ============================================================================
// STATISTICS AND MONITORING
// ============================================================================

// Get logging statistics
void logger_get_stats(u64* total_messages, u64* start_time, u32* breadcrumb_count);
void logger_print_stats(void);

// Export/import configuration
bool logger_config_save(const char* filename);
bool logger_config_load(const char* filename);

// ============================================================================
// PLATFORM-SPECIFIC EXTENSIONS
// ============================================================================

// Platform-specific logging
void logger_platform_init(void);
void logger_platform_shutdown(void);
void logger_platform_output(const char* message);

// Thread ID helpers
i32 logger_get_thread_id(void);
const char* logger_get_thread_name(void);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// String helpers
const char* logger_level_to_string(LogLevel level);
const char* logger_category_to_string(LogCategory category);
LogLevel logger_string_to_level(const char* str);
LogCategory logger_string_to_category(const char* str);

// Time helpers
u64 logger_get_timestamp_ms(void);
const char* logger_format_timestamp(u64 timestamp_ms, char* buffer, size_t buffer_size);

// Hex dumping
void logger_hex_dump(const void* data, size_t length, const char* label, LogLevel level, LogCategory category);

// Memory usage reporting
void logger_memory_usage_report(void);

// Session management
void logger_set_session_id(const char* session_id);
const char* logger_get_session_id(void);

// Version information
void logger_set_engine_version(const char* version);
void logger_set_build_info(const char* build_info);

// End of unified_logger.h
