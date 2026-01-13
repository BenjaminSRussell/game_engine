/*
 * unified_logging.h
 * Unified logging system - consolidates logging across all subsystems
 * 
 * This header provides common logging utilities, formatters, and output handlers
 * to eliminate code duplication across all engine systems.
 */

#ifndef UNIFIED_LOGGING_H
#define UNIFIED_LOGGING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * LOGGING CONSTANTS
 * ============================================================================ */

#define UNIFIED_LOG_MAX_MESSAGE_SIZE 4096
#define UNIFIED_LOG_MAX_CHANNELS 32
#define UNIFIED_LOG_MAX_FILES 10
#define UNIFIED_LOG_MAX_FILE_SIZE (10 * 1024 * 1024)  /* 10MB */
#define UNIFIED_LOG_MAX_MODULES 64
#define UNIFIED_LOG_MAX_TAGS 16

/* ============================================================================
 * LOG LEVELS
 * ============================================================================ */

typedef enum {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_COUNT
} log_level_t;

/* ============================================================================
 * LOG CHANNELS
 * ============================================================================ */

typedef enum {
    LOG_CHANNEL_CORE = 0,
    LOG_CHANNEL_RENDERING,
    LOG_CHANNEL_PHYSICS,
    LOG_CHANNEL_AUDIO,
    LOG_CHANNEL_ANIMATION,
    LOG_CHANNEL_AI,
    LOG_CHANNEL_NETWORK,
    LOG_CHANNEL_INPUT,
    LOG_CHANNEL_ASSETS,
    LOG_CHANNEL_MEMORY,
    LOG_CHANNEL_THREADING,
    LOG_CHANNEL_PLATFORM,
    LOG_CHANNEL_EDITOR,
    LOG_CHANNEL_GAMEPLAY,
    LOG_CHANNEL_SCRIPTING,
    LOG_CHANNEL_COUNT
} log_channel_t;

/* ============================================================================
 * LOG OUTPUT TARGETS
 * ============================================================================ */

typedef enum {
    LOG_OUTPUT_CONSOLE = (1 << 0),
    LOG_OUTPUT_FILE = (1 << 1),
    LOG_OUTPUT_DEBUGGER = (1 << 2),
    LOG_OUTPUT_NETWORK = (1 << 3),
    LOG_OUTPUT_MEMORY = (1 << 4),
    LOG_OUTPUT_ALL = 0xFF
} log_output_t;

/* ============================================================================
 * LOG MESSAGE STRUCTURE
 * ============================================================================ */

typedef struct {
    log_level_t level;
    log_channel_t channel;
    uint64_t timestamp;
    uint32_t thread_id;
    const char *module;
    const char *function;
    const char *file;
    int line;
    char message[UNIFIED_LOG_MAX_MESSAGE_SIZE];
    char tags[UNIFIED_LOG_MAX_TAGS][32];
    uint32_t tag_count;
} log_message_t;

/* ============================================================================
 * LOG FILE MANAGEMENT
 * ============================================================================ */

typedef struct {
    FILE *file;
    char filename[256];
    size_t current_size;
    bool enabled;
    log_level_t min_level;
    uint32_t rotation_count;
} log_file_t;

/* ============================================================================
 * LOG FILTER
 * ============================================================================ */

typedef struct {
    log_level_t min_level;
    log_level_t max_level;
    uint32_t channel_mask;
    char module_filter[64];
    char tag_filter[UNIFIED_LOG_MAX_TAGS][32];
    uint32_t tag_count;
    bool enabled;
} log_filter_t;

/* ============================================================================
 * LOG STATISTICS
 * ============================================================================ */

typedef struct {
    uint64_t total_messages;
    uint64_t messages_per_level[LOG_LEVEL_COUNT];
    uint64_t messages_per_channel[LOG_CHANNEL_COUNT];
    uint64_t dropped_messages;
    uint64_t filtered_messages;
    float average_message_size;
    uint64_t total_bytes_written;
    uint64_t peak_memory_usage;
} log_stats_t;

/* ============================================================================
 * UNIFIED LOGGING API
 * ============================================================================ */

/* Core logging functions */
void unified_log_init(void);
void unified_log_shutdown(void);
void unified_log_message(log_level_t level, log_channel_t channel, const char *module,
                        const char *function, const char *file, int line,
                        const char *format, ...);

/* Configuration functions */
void unified_log_set_level(log_level_t min_level);
void unified_log_set_output(log_output_t output_mask);
void unified_log_enable_channel(log_channel_t channel, bool enabled);
void unified_log_set_file_output(const char *filename, log_level_t min_level);
void unified_log_set_rotation_size(size_t max_size);

/* Filter functions */
void unified_log_set_filter(const log_filter_t *filter);
void unified_log_add_tag_filter(const char *tag);
void unified_log_clear_filters(void);

/* Statistics functions */
void unified_log_get_stats(log_stats_t *stats);
void unified_log_reset_stats(void);
void unified_log_dump_stats(void);

/* ============================================================================
 * CONVENIENCE MACROS (Consolidated from all systems)
 * ============================================================================ */

#define UNIFIED_LOG_TRACE(channel, format, ...) \
    unified_log_message(LOG_LEVEL_TRACE, channel, __FILE__, __FUNCTION__, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define UNIFIED_LOG_DEBUG(channel, format, ...) \
    unified_log_message(LOG_LEVEL_DEBUG, channel, __FILE__, __FUNCTION__, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define UNIFIED_LOG_INFO(channel, format, ...) \
    unified_log_message(LOG_LEVEL_INFO, channel, __FILE__, __FUNCTION__, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define UNIFIED_LOG_WARNING(channel, format, ...) \
    unified_log_message(LOG_LEVEL_WARNING, channel, __FILE__, __FUNCTION__, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define UNIFIED_LOG_ERROR(channel, format, ...) \
    unified_log_message(LOG_LEVEL_ERROR, channel, __FILE__, __FUNCTION__, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define UNIFIED_LOG_FATAL(channel, format, ...) \
    unified_log_message(LOG_LEVEL_FATAL, channel, __FILE__, __FUNCTION__, __FILE__, __LINE__, format, ##__VA_ARGS__)

/* Channel-specific macros */
#define LOG_CORE_TRACE(format, ...) UNIFIED_LOG_TRACE(LOG_CHANNEL_CORE, format, ##__VA_ARGS__)
#define LOG_CORE_DEBUG(format, ...) UNIFIED_LOG_DEBUG(LOG_CHANNEL_CORE, format, ##__VA_ARGS__)
#define LOG_CORE_INFO(format, ...) UNIFIED_LOG_INFO(LOG_CHANNEL_CORE, format, ##__VA_ARGS__)
#define LOG_CORE_WARNING(format, ...) UNIFIED_LOG_WARNING(LOG_CHANNEL_CORE, format, ##__VA_ARGS__)
#define LOG_CORE_ERROR(format, ...) UNIFIED_LOG_ERROR(LOG_CHANNEL_CORE, format, ##__VA_ARGS__)
#define LOG_CORE_FATAL(format, ...) UNIFIED_LOG_FATAL(LOG_CHANNEL_CORE, format, ##__VA_ARGS__)

#define LOG_RENDERING_TRACE(format, ...) UNIFIED_LOG_TRACE(LOG_CHANNEL_RENDERING, format, ##__VA_ARGS__)
#define LOG_RENDERING_DEBUG(format, ...) UNIFIED_LOG_DEBUG(LOG_CHANNEL_RENDERING, format, ##__VA_ARGS__)
#define LOG_RENDERING_INFO(format, ...) UNIFIED_LOG_INFO(LOG_CHANNEL_RENDERING, format, ##__VA_ARGS__)
#define LOG_RENDERING_WARNING(format, ...) UNIFIED_LOG_WARNING(LOG_CHANNEL_RENDERING, format, ##__VA_ARGS__)
#define LOG_RENDERING_ERROR(format, ...) UNIFIED_LOG_ERROR(LOG_CHANNEL_RENDERING, format, ##__VA_ARGS__)
#define LOG_RENDERING_FATAL(format, ...) UNIFIED_LOG_FATAL(LOG_CHANNEL_RENDERING, format, ##__VA_ARGS__)

#define LOG_PHYSICS_TRACE(format, ...) UNIFIED_LOG_TRACE(LOG_CHANNEL_PHYSICS, format, ##__VA_ARGS__)
#define LOG_PHYSICS_DEBUG(format, ...) UNIFIED_LOG_DEBUG(LOG_CHANNEL_PHYSICS, format, ##__VA_ARGS__)
#define LOG_PHYSICS_INFO(format, ...) UNIFIED_LOG_INFO(LOG_CHANNEL_PHYSICS, format, ##__VA_ARGS__)
#define LOG_PHYSICS_WARNING(format, ...) UNIFIED_LOG_WARNING(LOG_CHANNEL_PHYSICS, format, ##__VA_ARGS__)
#define LOG_PHYSICS_ERROR(format, ...) UNIFIED_LOG_ERROR(LOG_CHANNEL_PHYSICS, format, ##__VA_ARGS__)
#define LOG_PHYSICS_FATAL(format, ...) UNIFIED_LOG_FATAL(LOG_CHANNEL_PHYSICS, format, ##__VA_ARGS__)

#define LOG_AUDIO_TRACE(format, ...) UNIFIED_LOG_TRACE(LOG_CHANNEL_AUDIO, format, ##__VA_ARGS__)
#define LOG_AUDIO_DEBUG(format, ...) UNIFIED_LOG_DEBUG(LOG_CHANNEL_AUDIO, format, ##__VA_ARGS__)
#define LOG_AUDIO_INFO(format, ...) UNIFIED_LOG_INFO(LOG_CHANNEL_AUDIO, format, ##__VA_ARGS__)
#define LOG_AUDIO_WARNING(format, ...) UNIFIED_LOG_WARNING(LOG_CHANNEL_AUDIO, format, ##__VA_ARGS__)
#define LOG_AUDIO_ERROR(format, ...) UNIFIED_LOG_ERROR(LOG_CHANNEL_AUDIO, format, ##__VA_ARGS__)
#define LOG_AUDIO_FATAL(format, ...) UNIFIED_LOG_FATAL(LOG_CHANNEL_AUDIO, format, ##__VA_ARGS__)

#define LOG_MEMORY_TRACE(format, ...) UNIFIED_LOG_TRACE(LOG_CHANNEL_MEMORY, format, ##__VA_ARGS__)
#define LOG_MEMORY_DEBUG(format, ...) UNIFIED_LOG_DEBUG(LOG_CHANNEL_MEMORY, format, ##__VA_ARGS__)
#define LOG_MEMORY_INFO(format, ...) UNIFIED_LOG_INFO(LOG_CHANNEL_MEMORY, format, ##__VA_ARGS__)
#define LOG_MEMORY_WARNING(format, ...) UNIFIED_LOG_WARNING(LOG_CHANNEL_MEMORY, format, ##__VA_ARGS__)
#define LOG_MEMORY_ERROR(format, ...) UNIFIED_LOG_ERROR(LOG_CHANNEL_MEMORY, format, ##__VA_ARGS__)
#define LOG_MEMORY_FATAL(format, ...) UNIFIED_LOG_FATAL(LOG_CHANNEL_MEMORY, format, ##__VA_ARGS__)

/* ============================================================================
 * PERFORMANCE LOGGING
 * ============================================================================ */

typedef struct {
    const char *name;
    uint64_t start_time;
    uint64_t end_time;
    float duration_ms;
    bool active;
} log_profile_t;

void unified_log_profile_start(const char *name);
void unified_log_profile_end(const char *name);
void unified_log_profile_dump(void);

#define LOG_PROFILE_START(name) unified_log_profile_start(name)
#define LOG_PROFILE_END(name) unified_log_profile_end(name)

/* ============================================================================
 * CONDITIONAL LOGGING
 * ============================================================================ */

#ifdef DEBUG_BUILD
#define LOG_DEBUG_ENABLED 1
#else
#define LOG_DEBUG_ENABLED 0
#endif

#if LOG_DEBUG_ENABLED
#define LOG_DEBUG_TRACE(channel, format, ...) UNIFIED_LOG_TRACE(channel, format, ##__VA_ARGS__)
#define LOG_DEBUG_DEBUG(channel, format, ...) UNIFIED_LOG_DEBUG(channel, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG_TRACE(channel, format, ...) ((void)0)
#define LOG_DEBUG_DEBUG(channel, format, ...) ((void)0)
#endif

/* ============================================================================
 * THREAD-SAFE LOGGING
 * ============================================================================ */

#ifdef _WIN32
#include <windows.h>
#define LOG_MUTEX_TYPE CRITICAL_SECTION
#define LOG_MUTEX_INIT(mutex) InitializeCriticalSection(&(mutex))
#define LOG_MUTEX_LOCK(mutex) EnterCriticalSection(&(mutex))
#define LOG_MUTEX_UNLOCK(mutex) LeaveCriticalSection(&(mutex))
#define LOG_MUTEX_DESTROY(mutex) DeleteCriticalSection(&(mutex))
#else
#include <pthread.h>
#define LOG_MUTEX_TYPE pthread_mutex_t
#define LOG_MUTEX_INIT(mutex) pthread_mutex_init(&(mutex), NULL)
#define LOG_MUTEX_LOCK(mutex) pthread_mutex_lock(&(mutex))
#define LOG_MUTEX_UNLOCK(mutex) pthread_mutex_unlock(&(mutex))
#define LOG_MUTEX_DESTROY(mutex) pthread_mutex_destroy(&(mutex))
#endif

/* ============================================================================
 * LOG FORMATTING UTILITIES
 * ============================================================================ */

const char* unified_log_level_string(log_level_t level);
const char* unified_log_channel_string(log_channel_t channel);
const char* unified_log_timestamp(void);
void unified_log_format_message(const log_message_t *message, char *buffer, size_t buffer_size);

/* ============================================================================
 * LOG CALLBACKS
 * ============================================================================ */

typedef void (*log_callback_t)(const log_message_t *message, void *user_data);

void unified_log_set_callback(log_callback_t callback, void *user_data);
void unified_log_remove_callback(log_callback_t callback);

/* ============================================================================
 * COMPATIBILITY LAYER (For existing code)
 * ============================================================================ */

/* Simple logging for quick migration */
#define LOG_INFO(format, ...) LOG_CORE_INFO(format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...) LOG_CORE_WARNING(format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) LOG_CORE_ERROR(format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) LOG_CORE_DEBUG(format, ##__VA_ARGS__)

/* ============================================================================
 * STRUCTURED LOGGING
 * ============================================================================ */

typedef struct {
    const char *key;
    const char *value;
} log_field_t;

void unified_log_structured(log_level_t level, log_channel_t channel, const char *event_name,
                          const log_field_t *fields, uint32_t field_count);

#define LOG_STRUCTURED(level, channel, event, ...) \
    do { \
        log_field_t fields[] = {__VA_ARGS__}; \
        unified_log_structured(level, channel, event, fields, sizeof(fields) / sizeof(fields[0])); \
    } while(0)

/* Common structured logging helpers */
#define LOG_MEMORY_ALLOC(ptr, size) \
    LOG_STRUCTURED(LOG_LEVEL_DEBUG, LOG_CHANNEL_MEMORY, "memory_allocated", \
        {"pointer", #ptr}, {"size", #size})

#define LOG_MEMORY_FREE(ptr) \
    LOG_STRUCTURED(LOG_LEVEL_DEBUG, LOG_CHANNEL_MEMORY, "memory_freed", \
        {"pointer", #ptr})

#define LOG_RENDER_FRAME(frame_time) \
    LOG_STRUCTURED(LOG_LEVEL_DEBUG, LOG_CHANNEL_RENDERING, "frame_completed", \
        {"frame_time_ms", #frame_time})

#ifdef __cplusplus
}
#endif

#endif /* UNIFIED_LOGGING_H */
