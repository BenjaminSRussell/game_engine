// src/engine/core/logging/consolidated_logging.c
//
// Purpose: Consolidated logging system that merges all existing logging backends
// into a single, unified, high-performance logging infrastructure

#include "core/logger.h"
#include "core/logger/unified_logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <zlib.h>

// ============================================================================
// GLOBAL STATE AND CONFIGURATION
// ============================================================================

// Global logger instance
static struct {
    bool initialized;
    LoggerConfig config;
    LoggerStats stats;
    
    // Output channels
    FILE* console_file;
    FILE* log_file;
    int log_fd;
    
    // Buffering system
    LogEntry* entry_buffer;
    size_t buffer_size;
    size_t buffer_count;
    pthread_mutex_t buffer_mutex;
    
    // File rotation
    size_t current_file_size;
    int current_backup_index;
    
    // Network logging
    bool network_enabled;
    int network_socket;
    
    // Thread safety
    pthread_mutex_t logger_mutex;
    pthread_cond_t flush_condition;
    pthread_t flush_thread;
    bool flush_thread_running;
    
    // Performance monitoring
    u64 total_log_time;
    u64 max_log_time;
    u64 min_log_time;
    
} g_logger = {0};

// Log level names and colors
static const char* g_level_names[LOG_LEVEL_COUNT] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static const char* g_level_colors[LOG_LEVEL_COUNT] = {
    "\033[37m",  // TRACE - White
    "\033[36m",  // DEBUG - Cyan
    "\033[32m",  // INFO - Green
    "\033[33m",  // WARN - Yellow
    "\033[31m",  // ERROR - Red
    "\033[35m"   // FATAL - Magenta
};

static const char* g_category_names[LOG_CAT_COUNT] = {
    "GENERAL", "GAME", "PHYSICS", "GRAPHICS", "RENDERER",
    "AI", "NETWORK", "AUDIO", "MEMORY", "IO", "SCRIPT",
    "PLATFORM", "EDITOR", "ASSET", "ANIMATION", "INPUT"
};

// ============================================================================
// INTERNAL HELPER FUNCTIONS
// ============================================================================

static inline u64 get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
}

static inline const char* get_level_color(LogLevel level) {
    if (level < LOG_LEVEL_COUNT) {
        return g_level_colors[level];
    }
    return "\033[0m";
}

static inline const char* get_level_name(LogLevel level) {
    if (level < LOG_LEVEL_COUNT) {
        return g_level_names[level];
    }
    return "UNKNOWN";
}

static inline const char* get_category_name(LogCategory category) {
    if (category < LOG_CAT_COUNT) {
        return g_category_names[category];
    }
    return "UNKNOWN";
}

static void format_timestamp(time_t timestamp, char* buffer, size_t buffer_size) {
    struct tm* tm_info = localtime(&timestamp);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);
}

static void format_log_entry(const LogEntry* entry, char* buffer, size_t buffer_size) {
    char timestamp[32];
    format_timestamp(entry->timestamp, timestamp, sizeof(timestamp));
    
    const char* level_color = g_logger.config.use_colors ? get_level_color(entry->level) : "";
    const char* reset_color = g_logger.config.use_colors ? "\033[0m" : "";
    const char* level_name = get_level_name(entry->level);
    const char* category_name = get_category_name(entry->category);
    
    if (g_logger.config.show_file_line && entry->file && entry->line > 0) {
        if (g_logger.config.show_function && entry->function) {
            snprintf(buffer, buffer_size,
                "%s[%s] %s [%s] %s:%d in %s(): %s%s",
                level_color, timestamp, level_name, category_name,
                entry->file, entry->line, entry->function,
                entry->message, reset_color);
        } else {
            snprintf(buffer, buffer_size,
                "%s[%s] %s [%s] %s:%d: %s%s",
                level_color, timestamp, level_name, category_name,
                entry->file, entry->line, entry->message, reset_color);
        }
    } else {
        snprintf(buffer, buffer_size,
            "%s[%s] %s [%s]: %s%s",
            level_color, timestamp, level_name, category_name,
            entry->message, reset_color);
    }
}

static bool rotate_log_file(void) {
    if (!g_logger.log_file) {
        return false;
    }
    
    fclose(g_logger.log_file);
    g_logger.log_file = NULL;
    
    // Create backup files
    for (int i = g_logger.config.max_backup_files - 1; i > 0; i--) {
        char old_name[512], new_name[512];
        snprintf(old_name, sizeof(old_name), "%s.%d", g_logger.config.log_file_path, i - 1);
        snprintf(new_name, sizeof(new_name), "%s.%d", g_logger.config.log_file_path, i);
        rename(old_name, new_name);
    }
    
    // Move current log to .0
    char backup_name[512];
    snprintf(backup_name, sizeof(backup_name), "%s.0", g_logger.config.log_file_path);
    rename(g_logger.config.log_file_path, backup_name);
    
    // Open new log file
    g_logger.log_file = fopen(g_logger.config.log_file_path, "w");
    if (g_logger.log_file) {
        g_logger.current_file_size = 0;
        g_logger.stats.files_rotated++;
        return true;
    }
    
    return false;
}

static void write_to_console(const LogEntry* entry) {
    if (!(entry->channels & LOG_CHANNEL_CONSOLE)) {
        return;
    }
    
    char formatted[8192];
    format_log_entry(entry, formatted, sizeof(formatted));
    
    if (g_logger.config.use_colors) {
        fprintf(g_logger.console_file, "%s\n", formatted);
    } else {
        // Strip color codes for non-color output
        char clean[8192];
        char* src = formatted;
        char* dst = clean;
        while (*src) {
            if (*src == '\033') {
                // Skip ANSI escape sequence
                while (*src && *src != 'm') src++;
                if (*src) src++;
            } else {
                *dst++ = *src++;
            }
        }
        *dst = '\0';
        fprintf(g_logger.console_file, "%s\n", clean);
    }
    
    fflush(g_logger.console_file);
}

static void write_to_file(const LogEntry* entry) {
    if (!(entry->channels & LOG_CHANNEL_FILE) || !g_logger.log_file) {
        return;
    }
    
    char formatted[8192];
    format_log_entry(entry, formatted, sizeof(formatted));
    
    // Remove color codes for file output
    char clean[8192];
    char* src = formatted;
    char* dst = clean;
    while (*src) {
        if (*src == '\033') {
            // Skip ANSI escape sequence
            while (*src && *src != 'm') src++;
            if (*src) src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
    
    // Add newline
    *dst++ = '\n';
    *dst = '\0';
    
    size_t len = strlen(clean);
    fwrite(clean, 1, len, g_logger.log_file);
    
    g_logger.current_file_size += len;
    g_logger.stats.bytes_written += len;
    
    // Check if we need to rotate
    if (g_logger.config.enable_rotation && 
        g_logger.current_file_size >= g_logger.config.max_file_size) {
        rotate_log_file();
    }
    
    // Flush if buffering is disabled
    if (!g_logger.config.use_buffering) {
        fflush(g_logger.log_file);
    }
}

static void write_to_network(const LogEntry* entry) {
    if (!(entry->channels & LOG_CHANNEL_NETWORK) || !g_logger.network_enabled) {
        return;
    }
    
    // This would implement network logging to a remote endpoint
    // For now, we'll just increment the counter
    g_logger.stats.network_transmissions++;
}

static void* flush_thread_func(void* arg) {
    (void)arg;
    
    while (g_logger.flush_thread_running) {
        pthread_mutex_lock(&g_logger.buffer_mutex);
        
        // Wait for signal or timeout
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1; // 1 second timeout
        
        pthread_cond_timedwait(&g_logger.flush_condition, &g_logger.buffer_mutex, &timeout);
        
        // Flush buffered entries
        if (g_logger.buffer_count > 0) {
            for (size_t i = 0; i < g_logger.buffer_count; i++) {
                LogEntry* entry = &g_logger.entry_buffer[i];
                
                write_to_console(entry);
                write_to_file(entry);
                write_to_network(entry);
            }
            
            g_logger.buffer_count = 0;
        }
        
        pthread_mutex_unlock(&g_logger.buffer_mutex);
    }
    
    return NULL;
}

static void process_log_entry(const LogEntry* entry) {
    u64 start_time = get_timestamp_ns();
    
    // Update statistics
    g_logger.stats.total_entries++;
    g_logger.stats.entries_per_level[entry->level]++;
    g_logger.stats.entries_per_category[entry->category]++;
    g_logger.stats.last_entry_time = entry->timestamp;
    
    if (g_logger.config.use_buffering) {
        // Add to buffer
        pthread_mutex_lock(&g_logger.buffer_mutex);
        
        if (g_logger.buffer_count < g_logger.buffer_size) {
            g_logger.entry_buffer[g_logger.buffer_count] = *entry;
            g_logger.buffer_count++;
        } else {
            // Buffer full, force flush
            pthread_cond_signal(&g_logger.flush_condition);
        }
        
        pthread_mutex_unlock(&g_logger.buffer_mutex);
    } else {
        // Write immediately
        write_to_console(entry);
        write_to_file(entry);
        write_to_network(entry);
    }
    
    // Update performance metrics
    u64 end_time = get_timestamp_ns();
    u64 log_time = end_time - start_time;
    g_logger.total_log_time += log_time;
    
    if (log_time > g_logger.max_log_time) {
        g_logger.max_log_time = log_time;
    }
    if (log_time < g_logger.min_log_time || g_logger.min_log_time == 0) {
        g_logger.min_log_time = log_time;
    }
}

// ============================================================================
// CONSOLIDATED LOGGER API IMPLEMENTATION
// ============================================================================

bool unified_logger_init(const LoggerConfig* config) {
    if (g_logger.initialized) {
        return true; // Already initialized
    }
    
    // Set configuration
    if (config) {
        g_logger.config = *config;
    } else {
        // Default configuration
        g_logger.config = (LoggerConfig){
            .min_level = LOG_LEVEL_INFO,
            .enabled_channels = LOG_CHANNEL_CONSOLE | LOG_CHANNEL_FILE,
            .use_colors = true,
            .show_timestamp = true,
            .show_file_line = true,
            .show_function = false,
            .use_buffering = true,
            .enable_rotation = true,
            .enable_compression = false,
            .enable_remote = false,
            .enable_encryption = false,
            .log_file_path = "engine.log",
            .max_file_size = 10 * 1024 * 1024, // 10MB
            .max_backup_files = 5
        };
        
        // Enable all categories by default
        for (int i = 0; i < LOG_CAT_COUNT; i++) {
            g_logger.config.enabled_categories[i] = true;
        }
    }
    
    // Initialize mutexes
    if (pthread_mutex_init(&g_logger.logger_mutex, NULL) != 0) return false;
    if (pthread_mutex_init(&g_logger.buffer_mutex, NULL) != 0) return false;
    if (pthread_cond_init(&g_logger.flush_condition, NULL) != 0) return false;
    
    // Initialize console output
    g_logger.console_file = stdout;
    
    // Initialize file output
    if (g_logger.config.enabled_channels & LOG_CHANNEL_FILE) {
        g_logger.log_file = fopen(g_logger.config.log_file_path, "a");
        if (!g_logger.log_file) {
            g_logger.log_file = fopen(g_logger.config.log_file_path, "w");
        }
        
        if (g_logger.log_file) {
            // Get current file size
            fseek(g_logger.log_file, 0, SEEK_END);
            g_logger.current_file_size = ftell(g_logger.log_file);
            fseek(g_logger.log_file, 0, SEEK_SET);
        }
    }
    
    // Initialize buffer
    if (g_logger.config.use_buffering) {
        g_logger.buffer_size = 1024; // Buffer 1024 entries
        g_logger.entry_buffer = malloc(g_logger.buffer_size * sizeof(LogEntry));
        if (!g_logger.entry_buffer) {
            return false;
        }
    }
    
    // Initialize network logging
    if (g_logger.config.enable_remote) {
        // This would initialize network connection to remote endpoint
        g_logger.network_enabled = false; // Disabled for now
    }
    
    // Start flush thread
    if (g_logger.config.use_buffering) {
        g_logger.flush_thread_running = true;
        if (pthread_create(&g_logger.flush_thread, NULL, flush_thread_func, NULL) != 0) {
            return false;
        }
    }
    
    // Initialize statistics
    memset(&g_logger.stats, 0, sizeof(LoggerStats));
    g_logger.stats.start_time = time(NULL);
    
    g_logger.initialized = true;
    return true;
}

void unified_logger_shutdown(void) {
    if (!g_logger.initialized) {
        return;
    }
    
    // Stop flush thread
    if (g_logger.flush_thread_running) {
        g_logger.flush_thread_running = false;
        pthread_cond_signal(&g_logger.flush_condition);
        pthread_join(g_logger.flush_thread, NULL);
    }
    
    // Flush remaining entries
    if (g_logger.buffer_count > 0) {
        for (size_t i = 0; i < g_logger.buffer_count; i++) {
            LogEntry* entry = &g_logger.entry_buffer[i];
            write_to_console(entry);
            write_to_file(entry);
            write_to_network(entry);
        }
        g_logger.buffer_count = 0;
    }
    
    // Close files
    if (g_logger.log_file) {
        fclose(g_logger.log_file);
        g_logger.log_file = NULL;
    }
    
    // Clean up network
    if (g_logger.network_socket >= 0) {
        close(g_logger.network_socket);
        g_logger.network_socket = -1;
    }
    
    // Free buffer
    if (g_logger.entry_buffer) {
        free(g_logger.entry_buffer);
        g_logger.entry_buffer = NULL;
    }
    
    // Destroy mutexes
    pthread_mutex_destroy(&g_logger.logger_mutex);
    pthread_mutex_destroy(&g_logger.buffer_mutex);
    pthread_cond_destroy(&g_logger.flush_condition);
    
    g_logger.initialized = false;
}

void unified_logger_log(LogLevel level, LogCategory category, LogChannel channels,
                        const char* file, int line, const char* function,
                        const char* format, ...) {
    if (!g_logger.initialized || level < g_logger.config.min_level) {
        return;
    }
    
    // Check if category is enabled
    if (category >= LOG_CAT_COUNT || !g_logger.config.enabled_categories[category]) {
        return;
    }
    
    // Create log entry
    LogEntry entry = {0};
    entry.level = level;
    entry.category = category;
    entry.channels = channels;
    entry.timestamp = time(NULL);
    entry.file = file;
    entry.line = line;
    entry.function = function;
    
    // Format message
    va_list args;
    va_start(args, format);
    vsnprintf(entry.message, sizeof(entry.message), format, args);
    va_end(args);
    
    // Process the entry
    pthread_mutex_lock(&g_logger.logger_mutex);
    process_log_entry(&entry);
    pthread_mutex_unlock(&g_logger.logger_mutex);
}

void unified_logger_flush(void) {
    if (!g_logger.initialized) {
        return;
    }
    
    if (g_logger.config.use_buffering) {
        pthread_mutex_lock(&g_logger.buffer_mutex);
        pthread_cond_signal(&g_logger.flush_condition);
        pthread_mutex_unlock(&g_logger.buffer_mutex);
        
        // Wait for flush to complete
        usleep(10000); // 10ms
    } else {
        if (g_logger.log_file) {
            fflush(g_logger.log_file);
        }
        fflush(g_logger.console_file);
    }
}

void unified_logger_get_stats(LoggerStats* stats) {
    if (!stats || !g_logger.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_logger.logger_mutex);
    *stats = g_logger.stats;
    pthread_mutex_unlock(&g_logger.logger_mutex);
}

void unified_logger_benchmark(int iterations, const char* message) {
    if (!g_logger.initialized) {
        return;
    }
    
    u64 start_time = get_timestamp_ns();
    
    for (int i = 0; i < iterations; i++) {
        unified_logger_log(LOG_LEVEL_DEBUG, LOG_CAT_GENERAL, LOG_CHANNEL_CONSOLE,
                          __FILE__, __LINE__, __FUNCTION__,
                          "Benchmark iteration %d: %s", i, message);
    }
    
    u64 end_time = get_timestamp_ns();
    u64 total_time = end_time - start_time;
    u64 avg_time = total_time / iterations;
    
    unified_logger_log(LOG_LEVEL_INFO, LOG_CAT_GENERAL, LOG_CHANNEL_CONSOLE,
                      __FILE__, __LINE__, __FUNCTION__,
                      "Benchmark completed: %d iterations in %llu ns (avg: %llu ns per log)",
                      iterations, total_time, avg_time);
}

// ============================================================================
// CONVENIENCE MACROS IMPLEMENTATION
// ============================================================================

// These macros are defined in the header file for convenience
// LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL

// Category-specific macros
#define LOG_GAME(level, format, ...) unified_logger_log(level, LOG_CAT_GAME, LOG_CHANNEL_ALL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_PHYSICS(level, format, ...) unified_logger_log(level, LOG_CAT_PHYSICS, LOG_CHANNEL_ALL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_GRAPHICS(level, format, ...) unified_logger_log(level, LOG_CAT_GRAPHICS, LOG_CHANNEL_ALL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_RENDERER(level, format, ...) unified_logger_log(level, LOG_CAT_RENDERER, LOG_CHANNEL_ALL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_AI(level, format, ...) unified_logger_log(level, LOG_CAT_AI, LOG_CHANNEL_ALL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_NETWORK(level, format, ...) unified_logger_log(level, LOG_CAT_NETWORK, LOG_CHANNEL_ALL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_AUDIO(level, format, ...) unified_logger_log(level, LOG_CAT_AUDIO, LOG_CHANNEL_ALL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_MEMORY(level, format, ...) unified_logger_log(level, LOG_CAT_MEMORY, LOG_CHANNEL_ALL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_IO(level, format, ...) unified_logger_log(level, LOG_CAT_IO, LOG_CHANNEL_ALL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_SCRIPT(level, format, ...) unified_logger_log(level, LOG_CAT_SCRIPT, LOG_CHANNEL_ALL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_PLATFORM(level, format, ...) unified_logger_log(level, LOG_CAT_PLATFORM, LOG_CHANNEL_ALL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_EDITOR(level, format, ...) unified_logger_log(level, LOG_CAT_EDITOR, LOG_CHANNEL_ALL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_ASSET(level, format, ...) unified_logger_log(level, LOG_CAT_ASSET, LOG_CHANNEL_ALL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_ANIMATION(level, format, ...) unified_logger_log(level, LOG_CAT_ANIMATION, LOG_CHANNEL_ALL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_INPUT(level, format, ...) unified_logger_log(level, LOG_CAT_INPUT, LOG_CHANNEL_ALL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)

// Channel-specific macros
#define LOG_TO_CONSOLE(level, category, format, ...) unified_logger_log(level, category, LOG_CHANNEL_CONSOLE, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_TO_FILE(level, category, format, ...) unified_logger_log(level, category, LOG_CHANNEL_FILE, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOG_TO_NETWORK(level, category, format, ...) unified_logger_log(level, category, LOG_CHANNEL_NETWORK, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
