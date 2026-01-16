// src/engine/core/logging/unified_logger.c
//
// Purpose: Unified logging system implementation consolidating all logging backends
// Provides comprehensive logging with multiple channels, categories, and advanced features

#include <core/logger/unified_logger.h>
#include <core/memory/unified_allocator.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define IS_CONSOLE(handle) (((handle) == STDIN_HANDLE) || ((handle) == STDOUT_HANDLE) || ((handle) == STDERR_HANDLE))
#else
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/time.h>
#endif

// ============================================================================
// CONSTANTS AND CONFIGURATION
// ============================================================================

#define MAX_LOG_MESSAGE_SIZE 4096
#define MAX_LOG_FILES 10
#define DEFAULT_MAX_FILE_SIZE_MB 50
#define DEFAULT_MAX_BACKUP_FILES 5
#define THROTTLE_MAP_SIZE 256

// ============================================================================
// INTERNAL STATE
// ============================================================================

typedef struct {
    char key[64];
    u32 count;
    u64 last_reset_time;
} ThrottleEntry;

static struct {
    Logger* instance;
    ThrottleEntry throttle_map[THROTTLE_MAP_SIZE];
    char session_id[32];
    char engine_version[64];
    char build_info[128];
    bool initialized;
} g_logging_state = {0};

// Global logger instance
Logger* g_logger = NULL;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static u32 hash_string(const char* str) {
    u32 hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static u64 get_time_ms(void) {
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

static i32 get_thread_id(void) {
#ifdef _WIN32
    return (i32)GetCurrentThreadId();
#else
    return (i32)syscall(SYS_gettid);
#endif
}

static const char* get_level_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_TRACE: return "TRACE";
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_FATAL: return "FATAL";
        default:              return "UNKNOWN";
    }
}

static const char* get_category_string(LogCategory category) {
    switch (category) {
        case LOG_CAT_GENERAL:    return "GENERAL";
        case LOG_CAT_GAME:       return "GAME";
        case LOG_CAT_PHYSICS:    return "PHYSICS";
        case LOG_CAT_GRAPHICS:   return "GRAPHICS";
        case LOG_CAT_RENDERER:   return "RENDERER";
        case LOG_CAT_AI:         return "AI";
        case LOG_CAT_NETWORK:    return "NETWORK";
        case LOG_CAT_AUDIO:      return "AUDIO";
        case LOG_CAT_MEMORY:     return "MEMORY";
        case LOG_CAT_IO:         return "IO";
        case LOG_CAT_SCRIPT:     return "SCRIPT";
        case LOG_CAT_PLATFORM:   return "PLATFORM";
        case LOG_CAT_EDITOR:     return "EDITOR";
        case LOG_CAT_ASSET:      return "ASSET";
        case LOG_CAT_ANIMATION:  return "ANIMATION";
        case LOG_CAT_INPUT:      return "INPUT";
        case LOG_CAT_UI:         return "UI";
        default:                 return "UNKNOWN";
    }
}

static void format_timestamp(u64 timestamp_ms, char* buffer, size_t buffer_size) {
    time_t seconds = timestamp_ms / 1000;
    int milliseconds = timestamp_ms % 1000;
    
    struct tm* tm_info = localtime(&seconds);
    snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, milliseconds);
}

static void get_color_code(LogLevel level, char* color_code, size_t buffer_size) {
#ifdef _WIN32
    // Windows console colors are handled differently
    (void)level;
    (void)color_code;
    (void)buffer_size;
#else
    switch (level) {
        case LOG_LEVEL_TRACE:   snprintf(color_code, buffer_size, "\033[0;37m"); break; // White
        case LOG_LEVEL_DEBUG:   snprintf(color_code, buffer_size, "\033[1;34m"); break; // Blue
        case LOG_LEVEL_INFO:    snprintf(color_code, buffer_size, "\033[1;36m"); break; // Cyan
        case LOG_LEVEL_WARN:    snprintf(color_code, buffer_size, "\033[1;33m"); break; // Yellow
        case LOG_LEVEL_ERROR:   snprintf(color_code, buffer_size, "\033[1;31m"); break; // Red
        case LOG_LEVEL_FATAL:   snprintf(color_code, buffer_size, "\033[1;35m"); break; // Magenta
        default:                snprintf(color_code, buffer_size, "\033[0m"); break;     // Reset
    }
#endif
}

static void reset_color(void) {
#ifndef _WIN32
    printf("\033[0m");
#endif
}

// ============================================================================
// LOG OUTPUT FUNCTIONS
// ============================================================================

static void output_to_console(LogLevel level, LogCategory category, const LogEntry* entry) {
    if (!(g_logger->config.active_channels & LOG_CHANNEL_CONSOLE)) {
        return;
    }
    
    char color_code[16];
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
            printf("[%s] [%s] %s\n", timestamp, get_level_string(level), entry->message);
            break;
            
        case LOG_FORMAT_PRODUCTION:
            printf("[%s] [%s] [%s] %s\n", timestamp, get_level_string(level), 
                   get_category_string(category), entry->message);
            break;
            
        case LOG_FORMAT_DEBUG:
            printf("[%s] [%s] [%s] %s:%d %s() %s\n", timestamp, get_level_string(level),
                   get_category_string(category), entry->file ? entry->file : "unknown",
                   entry->line, entry->function ? entry->function : "unknown", entry->message);
            break;
            
        case LOG_FORMAT_JSON:
            printf("{\"timestamp\":\"%s\",\"level\":\"%s\",\"category\":\"%s\",\"message\":\"%s\"}\n",
                   timestamp, get_level_string(level), get_category_string(category), entry->message);
            break;
    }
    
    if (g_logger->config.use_colors) {
        reset_color();
    }
}

static void output_to_file(LogLevel level, LogCategory category, const LogEntry* entry) {
    if (!(g_logger->config.active_channels & LOG_CHANNEL_FILE) || !g_logger->log_file) {
        return;
    }
    
    char timestamp[32];
    format_timestamp(entry->timestamp_ms, timestamp, sizeof(timestamp));
    
    switch (g_logger->config.format) {
        case LOG_FORMAT_MINIMAL:
            fprintf(g_logger->log_file, "%s\n", entry->message);
            break;
            
        case LOG_FORMAT_DEVELOPMENT:
            fprintf(g_logger->log_file, "[%s] [%s] %s\n", timestamp, get_level_string(level), entry->message);
            break;
            
        case LOG_FORMAT_PRODUCTION:
            fprintf(g_logger->log_file, "[%s] [%s] [%s] %s\n", timestamp, get_level_string(level),
                    get_category_string(category), entry->message);
            break;
            
        case LOG_FORMAT_DEBUG:
            fprintf(g_logger->log_file, "[%s] [%s] [%s] %s:%d %s() %s\n", timestamp, get_level_string(level),
                    get_category_string(category), entry->file ? entry->file : "unknown",
                    entry->line, entry->function ? entry->function : "unknown", entry->message);
            break;
            
        case LOG_FORMAT_JSON:
            fprintf(g_logger->log_file, "{\"timestamp\":\"%s\",\"level\":\"%s\",\"category\":\"%s\",\"message\":\"%s\"}\n",
                    timestamp, get_level_string(level), get_category_string(category), entry->message);
            break;
    }
    
    if (g_logger->config.flush_on_write) {
        fflush(g_logger->log_file);
    }
    
    g_logger->current_file_size += strlen(entry->message) + 64; // Rough estimate
    
    // Check if we need to rotate the file
    if (g_logger->current_file_size > g_logger->config.rotation.max_file_size_mb * 1024 * 1024) {
        logger_rotate_file();
    }
}

static void output_to_debugger(LogLevel level, const LogEntry* entry) {
    if (!(g_logger->config.active_channels & LOG_CHANNEL_DEBUGGER)) {
        return;
    }
    
    char output[MAX_LOG_MESSAGE_SIZE];
    snprintf(output, sizeof(output), "[%s] %s", get_level_string(level), entry->message);
    
#ifdef _WIN32
    OutputDebugStringA(output);
    OutputDebugStringA("\n");
#elif defined(__APPLE__)
    // macOS - write to system log
    fprintf(stderr, "%s\n", output);
#else
    // Linux - could use syslog or other mechanisms
    fprintf(stderr, "%s\n", output);
#endif
}

// ============================================================================
// THROTTLING SYSTEM
// ============================================================================

static bool check_throttle(const char* key, u32 max_per_second) {
    if (!g_logger->config.enable_message_throttling) {
        return true;
    }
    
    u32 hash = hash_string(key) % THROTTLE_MAP_SIZE;
    ThrottleEntry* entry = &g_logging_state.throttle_map[hash];
    
    u64 current_time = get_time_ms();
    
    // Check if this is the same key
    if (strcmp(entry->key, key) == 0) {
        // Check if we're within the same second
        if (current_time - entry->last_reset_time < 1000) {
            if (entry->count >= max_per_second) {
                return false; // Throttled
            }
            entry->count++;
        } else {
            // Reset for new second
            entry->count = 1;
            entry->last_reset_time = current_time;
        }
    } else {
        // New key
        strncpy(entry->key, key, sizeof(entry->key) - 1);
        entry->key[sizeof(entry->key) - 1] = '\0';
        entry->count = 1;
        entry->last_reset_time = current_time;
    }
    
    return true;
}

// ============================================================================
// CORE LOGGING API IMPLEMENTATION
// ============================================================================

bool logger_init(const LogConfig* config) {
    if (g_logging_state.initialized) {
        return true;
    }
    
    // Allocate logger instance
    g_logger = malloc(sizeof(Logger));
    if (!g_logger) {
        return false;
    }
    
    memset(g_logger, 0, sizeof(Logger));
    
    // Copy configuration
    if (config) {
        g_logger->config = *config;
    } else {
        // Default configuration
        memset(&g_logger->config, 0, sizeof(LogConfig));
        g_logger->config.global_level = LOG_LEVEL_INFO;
        g_logger->config.active_channels = LOG_CHANNEL_CONSOLE | LOG_CHANNEL_FILE;
        g_logger->config.format = LOG_FORMAT_DEVELOPMENT;
        g_logger->config.use_colors = true;
        g_logger->config.show_timestamp = true;
        g_logger->config.show_level = true;
        g_logger->config.show_category = true;
        g_logger->config.flush_on_write = false;
        g_logger->config.enable_breadcrumbs = true;
        g_logger->config.thread_safe = true;
        g_logger->config.rotation.max_file_size_mb = DEFAULT_MAX_FILE_SIZE_MB;
        g_logger->config.rotation.max_backup_files = DEFAULT_MAX_BACKUP_FILES;
        strcpy(g_logger->config.log_filename, "voxelforge.log");
        
        // Enable all categories by default
        for (int i = 0; i < LOG_CAT_COUNT; i++) {
            g_logger->config.category_filters[i].min_level = LOG_LEVEL_TRACE;
            g_logger->config.category_filters[i].enabled = true;
            g_logger->config.category_filters[i].channels = g_logger->config.active_channels;
        }
    }
    
    // Initialize platform-specific stuff
    logger_platform_init();
    
    // Open log file
    if (g_logger->config.active_channels & LOG_CHANNEL_FILE) {
        g_logger->log_file = fopen(g_logger->config.log_filename, "a");
        if (g_logger->log_file) {
            // Get current file size
            fseek(g_logger->log_file, 0, SEEK_END);
            g_logger->current_file_size = ftell(g_logger->log_file);
        }
    }
    
    // Initialize buffering
    if (g_logger->config.enable_buffering) {
        u32 buffer_size = g_logger->config.buffer_size > 0 ? g_logger->config.buffer_size : 64 * 1024;
        g_logger->log_buffer = malloc(buffer_size);
        g_logger->buffer_pos = 0;
    }
    
    // Initialize breadcrumbs
    if (g_logger->config.enable_breadcrumbs) {
        g_logger->breadcrumbs.enabled = true;
    }
    
    // Set start time
    g_logger->start_time_ms = get_time_ms();
    
    // Initialize session info
    snprintf(g_logging_state.session_id, sizeof(g_logging_state.session_id), "%llx", 
             (unsigned long long)g_logger->start_time_ms);
    
    g_logging_state.instance = g_logger;
    g_logging_state.initialized = true;
    
    // Log initialization message
    LOG_INFO(LOG_CAT_GENERAL, "Unified logging system initialized");
    LOG_INFO(LOG_CAT_GENERAL, "Session ID: %s", g_logging_state.session_id);
    
    return true;
}

void logger_shutdown(void) {
    if (!g_logging_state.initialized || !g_logger) {
        return;
    }
    
    LOG_INFO(LOG_CAT_GENERAL, "Shutting down unified logging system");
    LOG_INFO(LOG_CAT_GENERAL, "Total messages logged: %llu", g_logger->total_messages_logged);
    
    // Flush any remaining buffered messages
    if (g_logger->log_buffer) {
        logger_flush_buffer();
        free(g_logger->log_buffer);
    }
    
    // Close log file
    if (g_logger->log_file) {
        fclose(g_logger->log_file);
        g_logger->log_file = NULL;
    }
    
    // Platform cleanup
    logger_platform_shutdown();
    
    free(g_logger);
    g_logger = NULL;
    g_logging_state.instance = NULL;
    g_logging_state.initialized = false;
}

bool logger_is_initialized(void) {
    return g_logging_state.initialized;
}

void logger_log(LogLevel level, LogCategory category, const char* file, int line, 
               const char* function, const char* format, ...) {
    if (!g_logger || level < g_logger->config.global_level) {
        return;
    }
    
    // Check category filter
    if (category < LOG_CAT_COUNT) {
        const LogCategoryFilter* filter = &g_logger->config.category_filters[category];
        if (!filter->enabled || level < filter->min_level) {
            return;
        }
    }
    
    // Format message
    LogEntry entry = {0};
    entry.level = level;
    entry.category = category;
    entry.timestamp_ms = get_time_ms();
    entry.thread_id = get_thread_id();
    entry.file = file;
    entry.line = line;
    entry.function = function;
    
    va_list args;
    va_start(args, format);
    vsnprintf(entry.message, sizeof(entry.message), format, args);
    va_end(args);
    
    // Add to breadcrumbs if enabled
    if (g_logger->config.enable_breadcrumbs) {
        logger_breadcrumb_add(&entry);
    }
    
    // Output to all configured channels
    output_to_console(level, category, &entry);
    output_to_file(level, category, &entry);
    output_to_debugger(level, &entry);
    
    g_logger->total_messages_logged++;
}

void logger_log_entry(const LogEntry* entry) {
    if (!g_logger || !entry) {
        return;
    }
    
    output_to_console(entry->level, entry->category, entry);
    output_to_file(entry->level, entry->category, entry);
    output_to_debugger(entry->level, entry);
    
    g_logger->total_messages_logged++;
}

void logger_log_hex(LogLevel level, LogCategory category, const void* data, size_t length, 
                   const char* label) {
    if (!g_logger || !data || length == 0) {
        return;
    }
    
    const u8* bytes = (const u8*)data;
    char hex_line[128];
    
    logger_log(level, category, __FILE__, __LINE__, __func__, 
              "Hex dump: %s (%zu bytes)", label ? label : "Data", length);
    
    for (size_t i = 0; i < length; i += 16) {
        size_t line_length = MIN(length - i, 16);
        
        // Hex part
        int offset = 0;
        offset += snprintf(hex_line + offset, sizeof(hex_line) - offset, "%04zx: ", i);
        
        for (size_t j = 0; j < line_length; j++) {
            offset += snprintf(hex_line + offset, sizeof(hex_line) - offset, "%02x ", bytes[i + j]);
        }
        
        // Padding for incomplete lines
        for (size_t j = line_length; j < 16; j++) {
            offset += snprintf(hex_line + offset, sizeof(hex_line) - offset, "   ");
        }
        
        // ASCII part
        offset += snprintf(hex_line + offset, sizeof(hex_line) - offset, " ");
        for (size_t j = 0; j < line_length; j++) {
            char c = bytes[i + j];
            offset += snprintf(hex_line + offset, sizeof(hex_line) - offset, "%c", 
                             (c >= 32 && c <= 126) ? c : '.');
        }
        
        logger_log(level, category, __FILE__, __LINE__, __func__, "%s", hex_line);
    }
}

// ============================================================================
// CONFIGURATION FUNCTIONS
// ============================================================================

void logger_set_level(LogLevel level) {
    if (!g_logger) return;
    g_logger->config.global_level = level;
    LOG_INFO(LOG_CAT_GENERAL, "Log level set to: %s", get_level_string(level));
}

void logger_set_channels(LogChannel channels) {
    if (!g_logger) return;
    g_logger->config.active_channels = channels;
    LOG_INFO(LOG_CAT_GENERAL, "Log channels updated");
}

void logger_set_format(LogFormat format) {
    if (!g_logger) return;
    g_logger->config.format = format;
    LOG_INFO(LOG_CAT_GENERAL, "Log format set to: %d", format);
}

void logger_set_category_filter(LogCategory category, LogLevel min_level, bool enabled) {
    if (!g_logger || category >= LOG_CAT_COUNT) return;
    
    LogCategoryFilter* filter = &g_logger->config.category_filters[category];
    filter->min_level = min_level;
    filter->enabled = enabled;
    
    LOG_INFO(LOG_CAT_GENERAL, "Category filter updated: %s = %s (level: %s)",
             get_category_string(category), enabled ? "enabled" : "disabled", get_level_string(min_level));
}

void logger_get_config(LogConfig* out_config) {
    if (!g_logger || !out_config) return;
    *out_config = g_logger->config;
}

void logger_apply_config(const LogConfig* config) {
    if (!g_logger || !config) return;
    
    LogConfig old_config = g_logger->config;
    g_logger->config = *config;
    
    // Reopen log file if filename changed
    if (strcmp(old_config.log_filename, config->log_filename) != 0) {
        if (g_logger->log_file) {
            fclose(g_logger->log_file);
        }
        g_logger->log_file = fopen(config->log_filename, "a");
    }
    
    LOG_INFO(LOG_CAT_GENERAL, "Log configuration applied");
}

// ============================================================================
// BREADCRUMB TRAIL
// ============================================================================

void logger_breadcrumb_add(const LogEntry* entry) {
    if (!g_logger || !g_logger->breadcrumbs.enabled) return;
    
    LogBreadcrumbTrail* trail = &g_logger->breadcrumbs;
    
    // Add entry to circular buffer
    trail->entries[trail->head] = *entry;
    trail->head = (trail->head + 1) % LOG_BREADCRUMB_MAX;
    
    if (trail->count < LOG_BREADCRUMB_MAX) {
        trail->count++;
    }
}

void logger_breadcrumb_clear(void) {
    if (!g_logger) return;
    
    LogBreadcrumbTrail* trail = &g_logger->breadcrumbs;
    trail->head = 0;
    trail->count = 0;
}

const LogEntry* logger_breadcrumb_get_last(u32 count, u32* out_count) {
    if (!g_logger || !out_count) return NULL;
    
    LogBreadcrumbTrail* trail = &g_logger->breadcrumbs;
    *out_count = MIN(count, trail->count);
    
    if (*out_count == 0) return NULL;
    
    // Calculate start index
    u32 start_index;
    if (trail->count < LOG_BREADCRUMB_MAX) {
        start_index = 0;
    } else {
        start_index = trail->head;
    }
    
    return &trail->entries[start_index];
}

void logger_breadcrumb_set_enabled(bool enabled) {
    if (!g_logger) return;
    g_logger->breadcrumbs.enabled = enabled;
}

// ============================================================================
// FILE MANAGEMENT
// ============================================================================

void logger_set_file(const char* filename) {
    if (!g_logger || !filename) return;
    
    // Close current file
    if (g_logger->log_file) {
        fclose(g_logger->log_file);
    }
    
    // Open new file
    strncpy(g_logger->config.log_filename, filename, sizeof(g_logger->config.log_filename) - 1);
    g_logger->config.log_filename[sizeof(g_logger->config.log_filename) - 1] = '\0';
    
    g_logger->log_file = fopen(filename, "a");
    if (g_logger->log_file) {
        fseek(g_logger->log_file, 0, SEEK_END);
        g_logger->current_file_size = ftell(g_logger->log_file);
        LOG_INFO(LOG_CAT_GENERAL, "Log file opened: %s", filename);
    } else {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to open log file: %s", filename);
    }
}

void logger_rotate_file(void) {
    if (!g_logger || !g_logger->log_file) return;
    
    fclose(g_logger->log_file);
    
    // Create backup filename with timestamp
    char backup_filename[512];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    snprintf(backup_filename, sizeof(backup_filename), "%s.%04d%02d%02d_%02d%02d%02d",
             g_logger->config.log_filename,
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    
    // Rename current file
    rename(g_logger->config.log_filename, backup_filename);
    
    // Open new file
    g_logger->log_file = fopen(g_logger->config.log_filename, "w");
    if (g_logger->log_file) {
        g_logger->current_file_size = 0;
        LOG_INFO(LOG_CAT_GENERAL, "Log file rotated: %s -> %s", 
                 g_logger->config.log_filename, backup_filename);
    } else {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to open new log file after rotation");
    }
}

void logger_flush(void) {
    if (!g_logger) return;
    
    if (g_logger->log_file) {
        fflush(g_logger->log_file);
    }
    
    if (g_logger->log_buffer) {
        logger_flush_buffer();
    }
}

void logger_flush_buffer(void) {
    if (!g_logger || !g_logger->log_buffer || g_logger->buffer_pos == 0) return;
    
    if (g_logger->log_file) {
        fwrite(g_logger->log_buffer, 1, g_logger->buffer_pos, g_logger->log_file);
        fflush(g_logger->log_file);
    }
    
    g_logger->buffer_pos = 0;
    g_logger->last_flush_time = get_time_ms();
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

const char* logger_level_to_string(LogLevel level) {
    return get_level_string(level);
}

const char* logger_category_to_string(LogCategory category) {
    return get_category_string(category);
}

u64 logger_get_timestamp_ms(void) {
    return get_time_ms();
}

const char* logger_format_timestamp(u64 timestamp_ms, char* buffer, size_t buffer_size) {
    format_timestamp(timestamp_ms, buffer, buffer_size);
    return buffer;
}

void logger_set_session_id(const char* session_id) {
    if (!session_id) return;
    strncpy(g_logging_state.session_id, session_id, sizeof(g_logging_state.session_id) - 1);
    g_logging_state.session_id[sizeof(g_logging_state.session_id) - 1] = '\0';
}

const char* logger_get_session_id(void) {
    return g_logging_state.session_id;
}

void logger_set_engine_version(const char* version) {
    if (!version) return;
    strncpy(g_logging_state.engine_version, version, sizeof(g_logging_state.engine_version) - 1);
    g_logging_state.engine_version[sizeof(g_logging_state.engine_version) - 1] = '\0';
}

void logger_set_build_info(const char* build_info) {
    if (!build_info) return;
    strncpy(g_logging_state.build_info, build_info, sizeof(g_logging_state.build_info) - 1);
    g_logging_state.build_info[sizeof(g_logging_state.build_info) - 1] = '\0';
}

void logger_get_stats(u64* total_messages, u64* start_time, u32* breadcrumb_count) {
    if (!g_logger) return;
    
    if (total_messages) *total_messages = g_logger->total_messages_logged;
    if (start_time) *start_time = g_logger->start_time_ms;
    if (breadcrumb_count) *breadcrumb_count = g_logger->breadcrumbs.count;
}

void logger_print_stats(void) {
    if (!g_logger) return;
    
    u64 uptime = get_time_ms() - g_logger->start_time_ms;
    printf("=== Logger Statistics ===\n");
    printf("Total messages: %llu\n", g_logger->total_messages_logged);
    printf("Uptime: %llu ms\n", uptime);
    printf("Messages per second: %.2f\n", 
           (f32)g_logger->total_messages_logged / (uptime / 1000.0f));
    printf("Breadcrumbs: %u/%u\n", g_logger->breadcrumbs.count, LOG_BREADCRUMB_MAX);
    printf("========================\n");
}

// ============================================================================
// PLATFORM-SPECIFIC FUNCTIONS
// ============================================================================

void logger_platform_init(void) {
#ifdef _WIN32
    // Initialize Windows console for colors
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hConsole, &mode);
    SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

void logger_platform_shutdown(void) {
    // Platform-specific cleanup
}

void logger_platform_output(const char* message) {
#ifdef _WIN32
    OutputDebugStringA(message);
    OutputDebugStringA("\n");
#else
    fprintf(stderr, "%s\n", message);
#endif
}

i32 logger_get_thread_id(void) {
    return get_thread_id();
}

const char* logger_get_thread_name(void) {
    // Could be implemented with thread-local storage
    return "unknown";
}
