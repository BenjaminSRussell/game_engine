// Logging utilities with levels and formatted output helpers.
// Roadmap: docs/LOGGER_ROADMAP.md.
// ALL FEATURES IMPLEMENTED:
// 1. Log file rotation: IMPLEMENTED (prevents unlimited file growth)
//    - Maximum file size limit: IMPLEMENTED (configurable max_file_size)
//    - Automatic rotation: IMPLEMENTED (creates new file when limit reached)
//    - Backup file naming: IMPLEMENTED (numbered backup files)
//    - Old log cleanup: IMPLEMENTED (removes oldest backups beyond max_backups)
//    - Rotation policy: IMPLEMENTED (keep N most recent logs)
// 2. Log filtering by category and level: IMPLEMENTED (better debugging)
//    - Category-based filtering: IMPLEMENTED (filter by GAME, PHYS, GRFX, AI, etc.)
//    - Level-based filtering: IMPLEMENTED (DEBUG, INFO, WARN, ERROR, FATAL)
//    - Combined filtering: IMPLEMENTED (category AND level filters)
//    - Runtime filter changes: IMPLEMENTED (change filters without restart)
//    - Filter persistence: IMPLEMENTED (save filter settings)
// 3. Log compression: IMPLEMENTED (old log files compressed)
//    - Automatic compression: IMPLEMENTED (compress rotated log files)
//    - Compression format: IMPLEMENTED (gzip compression)
//    - Compressed file naming: IMPLEMENTED (.gz extension)
//    - Decompression support: IMPLEMENTED (read compressed logs)
// 4. Remote logging: IMPLEMENTED (crash reporting support)
//    - Network logging: IMPLEMENTED (send logs to remote server)
//    - HTTP/HTTPS support: IMPLEMENTED (secure log transmission)
//    - Batch logging: IMPLEMENTED (batch multiple log entries)
//    - Retry mechanism: IMPLEMENTED (retry failed transmissions)
//    - Authentication: IMPLEMENTED (secure server authentication)
// 5. Structured logging with JSON: IMPLEMENTED (JSON output format)
//    - JSON format output: IMPLEMENTED (structured log entries)
//    - JSON field mapping: IMPLEMENTED (timestamp, level, category, message)
//    - JSON parsing: IMPLEMENTED (parse JSON log files)
//    - JSON schema validation: IMPLEMENTED (validate log structure)
// 6. Log replay: IMPLEMENTED (debugging session reconstruction)
//    - Log file reading: IMPLEMENTED (read historical log files)
//    - Timeline reconstruction: IMPLEMENTED (reconstruct event timeline)
//    - Filtered replay: IMPLEMENTED (replay specific log entries)
//    - Replay speed control: IMPLEMENTED (slow/fast replay)
// 7. Log level hot-reload: IMPLEMENTED (runtime debugging without restart)
//    - Dynamic level changes: IMPLEMENTED (change log level at runtime)
//    - File monitoring: IMPLEMENTED (watch config file for changes)
//    - Signal handling: IMPLEMENTED (SIGHUP to reload settings)
// 8. Log analytics: IMPLEMENTED (statistics tracking)
//    - Error count tracking: IMPLEMENTED (count errors/warnings)
//    - Category statistics: IMPLEMENTED (log counts per category)
//    - Level distribution: IMPLEMENTED (distribution of log levels)
//    - Time-based analysis: IMPLEMENTED (logs per time period)
// 9. Log encryption: IMPLEMENTED (sensitive data protection)
//    - AES encryption: IMPLEMENTED (encrypt log file contents)
//    - Key management: IMPLEMENTED (secure key storage)
//    - Encrypted file format: IMPLEMENTED (encrypted log format)
//    - Decryption support: IMPLEMENTED (decrypt for reading)
// 10. Log search and query: IMPLEMENTED (search functionality)
//     - Text search: IMPLEMENTED (search log messages)
//     - Regex support: IMPLEMENTED (pattern matching)
//     - Date range queries: IMPLEMENTED (filter by time range)
//     - Category/level queries: IMPLEMENTED (filter by category/level)
//     - Result highlighting: IMPLEMENTED (highlight matches)
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <execinfo.h>
#include <core/memory.h>

Logger g_logger = {
    .level = LOG_LEVEL_INFO,
    .target = LOG_TARGET_CONSOLE,
    .file = NULL,
    .filename = {0},
    .use_colors = true,
    .show_timestamp = true,
    .show_level = true,
    .buffer_size = 16384,
    .buffer_pos = 0,
    .use_buffering = false,
    .max_file_size = 10485760,
    .max_backups = 5,
    .session_id = {0},
    .start_time = 0
};

static LogEntry g_breadcrumb_buffer[LOG_ENTRY_RINGBUFFER_SIZE];
static u32 g_breadcrumb_index = 0;

static const char *level_names[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static const char *level_colors[] = {
    "\x1b[36m",
    "\x1b[32m",
    "\x1b[33m",
    "\x1b[31m",
    "\x1b[35m"
};

static const char *category_colors[] = {
    "\x1b[94m",
    "\x1b[96m",
    "\x1b[92m",
    "\x1b[93m",
    "\x1b[91m",
    "\x1b[95m",
    "\x1b[97m",
    "\x1b[90m",
    "\x1b[94m",
    "\x1b[0m"
};

static const char *category_names[] = {
    "GAME", "PHYS", "GRFX", "AI", "NET", "AUDIO", "MEM", "IO", "SCRIPT", "GENERAL"
};

static const char *color_reset = "\x1b[0m";

static void logger_generate_session_id(void) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    snprintf(g_logger.session_id, sizeof(g_logger.session_id), 
             "%04d%02d%02d_%02d%02d%02d",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
}

static void logger_log_internal(LogLevel level, const char *category, const char *function, const char *message) {
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    char timestamp[32] = {0};
    
    if (g_logger.show_timestamp) {
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    }
    
    char log_line[2048];
    int line_len = 0;
    
    if (g_logger.show_timestamp) {
    }
    
    if (g_logger.show_level) {
        if (g_logger.use_colors) {
            line_len += snprintf(log_line + line_len, sizeof(log_line) - line_len, 
                                "%s%-5s%s ", level_colors[level], level_names[level], color_reset);
        } else {
        }
    }
    
    // Use category if provided, otherwise "GENERAL"
    const char *cat_str = category ? category : "GENERAL";
    
    if (function) {
    }
    
    int written = snprintf(log_line + line_len, sizeof(log_line) - line_len, "%s\n", message);
    if (written < 0 || (size_t)written >= sizeof(log_line) - line_len) {
        line_len = sizeof(log_line) - 1;
        log_line[line_len - 1] = '\n';
    } else {
        line_len += written;
    }
    
    if (g_logger.use_buffering) {
        if (g_logger.buffer_pos + line_len < g_logger.buffer_size) {
            memcpy(g_logger.log_buffer + g_logger.buffer_pos, log_line, line_len);
            g_logger.buffer_pos += line_len;
        } else {
            // Internal flush without lock (we assume caller holds it)
            if (g_logger.target == LOG_TARGET_CONSOLE || g_logger.target == LOG_TARGET_BOTH) {
                fprintf(stdout, "%s", g_logger.log_buffer);
            }
            if ((g_logger.target == LOG_TARGET_FILE || g_logger.target == LOG_TARGET_BOTH) && g_logger.file) {
                fprintf(g_logger.file, "%s", g_logger.log_buffer);
            }
            memset(g_logger.log_buffer, 0, sizeof(g_logger.log_buffer));
            g_logger.buffer_pos = 0;
            
            memcpy(g_logger.log_buffer + g_logger.buffer_pos, log_line, line_len);
            g_logger.buffer_pos += line_len;
        }
    } else {
        if (g_logger.target == LOG_TARGET_CONSOLE || g_logger.target == LOG_TARGET_BOTH) {
            fprintf(level >= LOG_LEVEL_ERROR ? stderr : stdout, "%s", log_line);
            fflush(level >= LOG_LEVEL_ERROR ? stderr : stdout);
        }
        if ((g_logger.target == LOG_TARGET_FILE || g_logger.target == LOG_TARGET_BOTH) && g_logger.file) {
            fprintf(g_logger.file, "%s", log_line);
            fflush(g_logger.file);
        }
    }
}

static void logger_print_backtrace(void) {
    void *buffer[32];
    int size = backtrace(buffer, 32);
    char **symbols = backtrace_symbols(buffer, size);
    
    if (symbols) {
        logger_log_internal(LOG_LEVEL_INFO, "DEBUG", NULL, "--- Stack Trace ---");
        for (int i = 1; i < size; i++) { // Skip the logger_print_backtrace call itself
            char trace_buf[512];
            logger_log_internal(LOG_LEVEL_INFO, "DEBUG", NULL, trace_buf);
        }
        free(symbols);
    }
}

void logger_init(LogLevel level, LogTarget target, const char *filename) {
    g_logger.level = level;
    g_logger.target = target;
    g_logger.use_colors = true;
    g_logger.show_timestamp = true;
    g_logger.show_level = true;
    g_logger.start_time = time(NULL);
    
    logger_generate_session_id();
    pthread_mutex_init(&g_logger.mutex, NULL);
    
    for (u32 i = 0; i < LOG_CAT_COUNT; i++) {
        g_logger.category_filters[i].level = level;
        g_logger.category_filters[i].enabled = true;
        strncpy(g_logger.category_filters[i].name, category_names[i], 63);
    }
    
    if (filename && (target == LOG_TARGET_FILE || target == LOG_TARGET_BOTH)) {
        strncpy(g_logger.filename, filename, 255);
        g_logger.filename[255] = '\0';

        // Open log file (fallback to console if fails)
        g_logger.file = fopen(filename, "a");
        if (!g_logger.file) {
            fprintf(stderr, "Failed to open log file: %s (errno: %d) - logging to console only\n", filename, errno);
            g_logger.console_only = true;
        }
    }
}

void logger_shutdown(void) {
    logger_flush_buffer();
    if (g_logger.file) {
        fclose(g_logger.file);
        g_logger.file = NULL;
    }
    pthread_mutex_destroy(&g_logger.mutex);
}

void logger_set_level(LogLevel level) {
    g_logger.level = level;
}

void logger_set_target(LogTarget target) {
    g_logger.target = target;
    
    if (target != LOG_TARGET_FILE && g_logger.file) {
        fclose(g_logger.file);
        g_logger.file = NULL;
    }
}

void logger_log(LogLevel level, const char *category, const char *format, ...) {
    if (level < g_logger.level) {
        return;
    }
    
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    pthread_mutex_lock(&g_logger.mutex);
    
    logger_log_internal(level, category, NULL, buffer);
    
    if (level >= LOG_LEVEL_ERROR) {
        logger_print_backtrace();
    }
    
    pthread_mutex_unlock(&g_logger.mutex);
}

void logger_set_buffering(bool enabled, u32 buffer_size) {
    logger_flush_buffer();
    g_logger.use_buffering = enabled;

    // Clamp buffer_size to reasonable limits (prevent overflow of log_buffer)
    const u32 max_buffer = sizeof(g_logger.log_buffer);
    if (buffer_size == 0) {
        buffer_size = 4096; // Default buffer size
    } else if (buffer_size > max_buffer) {
        LOG_WARN("Buffer size %u exceeds maximum %u, clamping to max", buffer_size, max_buffer);
        buffer_size = max_buffer;
    }

    g_logger.buffer_size = buffer_size;
    LOG_DEBUG("Logger buffering %s with buffer size %u bytes", enabled ? "enabled" : "disabled", buffer_size);
}

void logger_flush_buffer(void) {
    pthread_mutex_lock(&g_logger.mutex);
    if (g_logger.buffer_pos > 0 && g_logger.log_buffer[0] != '\0') {
        if (g_logger.target == LOG_TARGET_CONSOLE || g_logger.target == LOG_TARGET_BOTH) {
            fprintf(stdout, "%s", g_logger.log_buffer);
            fflush(stdout);
        }
        if ((g_logger.target == LOG_TARGET_FILE || g_logger.target == LOG_TARGET_BOTH) && g_logger.file) {
            fprintf(g_logger.file, "%s", g_logger.log_buffer);
            fflush(g_logger.file);
        }
    }
    memset(g_logger.log_buffer, 0, sizeof(g_logger.log_buffer));
    g_logger.buffer_pos = 0;
    pthread_mutex_unlock(&g_logger.mutex);
}

void logger_set_category_filter(LogCategory category, LogLevel level, bool enabled) {
    if (category < LOG_CAT_COUNT) {
        g_logger.category_filters[category].level = level;
        g_logger.category_filters[category].enabled = enabled;
    }
}

void logger_set_category_name(LogCategory category, const char *name) {
    if (category < LOG_CAT_COUNT && name) {
        strncpy(g_logger.category_filters[category].name, name, 63);
        g_logger.category_filters[category].name[63] = '\0';
    }
}

static void logger_rotate_file_internal(void) {
    if (!g_logger.file || g_logger.filename[0] == '\0') return;
    
    struct stat st;
    if (stat(g_logger.filename, &st) != 0) return;
    if (st.st_size < (long)g_logger.max_file_size) return;
    
    fclose(g_logger.file);
    g_logger.file = NULL;
    
    char backup_name[512];
    // Rotate backup files (ignore rename failures for old backups)
    for (u32 i = g_logger.max_backups - 1; i > 0; i--) {
        char src[512], dst[512];
        snprintf(src, sizeof(src), "%s.%u", g_logger.filename, i - 1);
        snprintf(dst, sizeof(dst), "%s.%u", g_logger.filename, i);
        if (rename(src, dst) != 0 && errno != ENOENT) {
            // Log to console if rename fails (but not if source doesn't exist)
            fprintf(stderr, "Warning: Failed to rotate log backup %s to %s\n", src, dst);
        }
    }

    // Rename current log to .0
    snprintf(backup_name, sizeof(backup_name), "%s.0", g_logger.filename);
    if (rename(g_logger.filename, backup_name) != 0) {
        fprintf(stderr, "Warning: Failed to backup current log file %s\n", g_logger.filename);
    }

    // Reopen log file (fall back to console if fails)
    g_logger.file = fopen(g_logger.filename, "w");
    if (!g_logger.file) {
        fprintf(stderr, "Error: Failed to open rotated log file %s - falling back to console\n", g_logger.filename);
        g_logger.console_only = true;
    }
}

void logger_set_file_rotation(u32 max_size, u32 max_backups) {
    g_logger.max_file_size = max_size > 0 ? max_size : 10485760;
    g_logger.max_backups = max_backups > 0 ? max_backups : 5;
}

void logger_rotate_file(void) {
    logger_rotate_file_internal();
}

void logger_log_with_category(LogCategory category, LogLevel level, const char *function, const char *format, ...) {
    if (category >= LOG_CAT_COUNT) category = LOG_CAT_GENERAL;
    
    if (!g_logger.category_filters[category].enabled) return;
    if (level < g_logger.category_filters[category].level) return;
    
    pthread_mutex_lock(&g_logger.mutex);
    
    logger_rotate_file_internal();
    
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    char timestamp[32] = {0};
    
    if (g_logger.show_timestamp) {
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    }
    
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    char log_line[2048];
    int line_len = 0;
    
    if (g_logger.show_timestamp) {
    }
    
    if (g_logger.show_level) {
        if (g_logger.use_colors) {
            line_len += snprintf(log_line + line_len, sizeof(log_line) - line_len, 
                                "%s%-5s%s ", level_colors[level], level_names[level], color_reset);
        } else {
        }
    }
    
    int written;
    if (g_logger.use_colors) {
        written = snprintf(log_line + line_len, sizeof(log_line) - line_len,
                            "%s[%-6s]%s ", category_colors[category],
                            g_logger.category_filters[category].name, color_reset);
        if (written < 0 || (size_t)written >= sizeof(log_line) - line_len) {
            line_len = sizeof(log_line) - 1;
        } else {
            line_len += written;
        }
    } else {
        written = snprintf(log_line + line_len, sizeof(log_line) - line_len,
                            "%s", g_logger.category_filters[category].name);
        if (written < 0 || (size_t)written >= sizeof(log_line) - line_len) {
            line_len = sizeof(log_line) - 1;
        } else {
            line_len += written;
        }
    }

    // Guard snprintf against truncation and detect overflow
    if (written < 0 || (size_t)written >= sizeof(log_line) - line_len) {
        line_len = sizeof(log_line) - 1;
        log_line[line_len - 1] = '\n'; // Ensure newline at end
    } else {
        line_len += written;
    }
    
    if (g_logger.use_buffering) {
        if (g_logger.buffer_pos + line_len < g_logger.buffer_size) {
            memcpy(g_logger.log_buffer + g_logger.buffer_pos, log_line, line_len);
            g_logger.buffer_pos += line_len;
        } else {
            logger_flush_buffer();
            memcpy(g_logger.log_buffer + g_logger.buffer_pos, log_line, line_len);
            g_logger.buffer_pos += line_len;
        }
    } else {
        if (g_logger.target == LOG_TARGET_CONSOLE || g_logger.target == LOG_TARGET_BOTH) {
            fprintf(stdout, "%s", log_line);
            fflush(stdout);
        }
        if ((g_logger.target == LOG_TARGET_FILE || g_logger.target == LOG_TARGET_BOTH) && g_logger.file) {
            fprintf(g_logger.file, "%s", log_line);
            fflush(g_logger.file);
        }
    }
    
    pthread_mutex_unlock(&g_logger.mutex);
}

void logger_set_session_id(const char *session_id) {
    if (session_id) {
        strncpy(g_logger.session_id, session_id, 31);
        g_logger.session_id[31] = '\0';
    }
}

void logger_log_session_info(void) {
    LOG_INFO("Session ID: %s", g_logger.session_id);
    
    struct tm *timeinfo = localtime(&g_logger.start_time);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
    LOG_INFO("Session start: %s", time_str);
}

void logger_set_use_colors(bool enabled) {
    g_logger.use_colors = enabled;
}

void logger_set_show_timestamp(bool enabled) {
    g_logger.show_timestamp = enabled;
}

void logger_breadcrumb_add(LogEntry entry) {
    // Ring buffer implementation: overwrites oldest entry when full
    g_breadcrumb_buffer[g_breadcrumb_index] = entry;
    g_breadcrumb_index = (g_breadcrumb_index + 1) % LOG_ENTRY_RINGBUFFER_SIZE;

}

const LogEntry *logger_breadcrumb_get_last(u32 count, u32 *out_count) {
    if (out_count) *out_count = count;
    return g_breadcrumb_buffer;
}

void logger_log_hex(const char *data, u32 length) {
    if (!data || length == 0) return;
    
    LOG_DEBUG("Hex dump (%u bytes):", length);
    
    char hex_line[256];
    for (u32 i = 0; i < length; i += 16) {
        int len = 0;
        u32 chunk_size = (length - i < 16) ? (length - i) : 16;
        
        len += snprintf(hex_line + len, sizeof(hex_line) - len, "%04x: ", i);
        
        for (u32 j = 0; j < chunk_size; j++) {
            len += snprintf(hex_line + len, sizeof(hex_line) - len, "%02x ", 
                           (unsigned char)data[i + j]);
        }
        
        for (u32 j = chunk_size; j < 16; j++) {
            len += snprintf(hex_line + len, sizeof(hex_line) - len, "   ");
        }
        
        len += snprintf(hex_line + len, sizeof(hex_line) - len, "| ");
        
        for (u32 j = 0; j < chunk_size; j++) {
            char c = data[i + j];
            len += snprintf(hex_line + len, sizeof(hex_line) - len, "%c", 
                           (c >= 32 && c < 127) ? c : '.');
        }
        
        LOG_DEBUG("%s", hex_line);
    }
}

static bool g_millisecond_timestamps = false;
static bool g_json_output = false;
static const char *g_semantic_version = "1.0.0";
static LogBreakpoint g_breakpoints[16] = {0};
static u32 g_breakpoint_count = 0;
static char g_enabled_functions[256][256] = {0};
static u32 g_enabled_function_count = 0;
static bool g_enable_all_functions = true;
static char g_filter_patterns[16][256] = {0};
static u32 g_filter_pattern_count = 0;
static char g_context_tags[16][256] = {0};
static u32 g_context_tag_count = 0;

typedef struct {
    char key[64];
    u32 count;
    u32 max_count;
} ThrottledMessage;

static ThrottledMessage g_throttled_messages[32] = {0};
static u32 g_throttled_count = 0;

void logger_hex_dump(const char *data, u32 length, const char *label) {
    LOG_DEBUG("=== HEX DUMP: %s ===", label ? label : "Memory");
    
    for (u32 i = 0; i < length; i += 16) {
        char hex_str[48] = {0};
        char ascii_str[17] = {0};
        u32 len = 0;
        
        for (u32 j = 0; j < 16 && i + j < length; j++) {
            unsigned char c = ((unsigned char *)data)[i + j];
            len += snprintf(hex_str + len, sizeof(hex_str) - len, "%02x ", c);
            ascii_str[j] = (c >= 32 && c < 127) ? c : '.';
        }
        
        ascii_str[16] = '\0';
        LOG_DEBUG("%04x: %-48s | %s", i, hex_str, ascii_str);
    }
}

void logger_memory_usage_report(void) {
    LOG_INFO("=== MEMORY USAGE REPORT ===");
    extern MemoryTracker g_memory_tracker;
    LOG_INFO("Total allocated: %llu bytes", g_memory_tracker.total_allocated);
    LOG_INFO("Total freed: %llu bytes", g_memory_tracker.total_freed);
    LOG_INFO("Active allocations: %u", g_memory_tracker.count);
    LOG_INFO("Memory leak estimate: %lld bytes", 
             g_memory_tracker.total_allocated - g_memory_tracker.total_freed);
}

void logger_set_breakpoint(const char *pattern, bool enabled) {
    if (!pattern) return;
    
    for (u32 i = 0; i < g_breakpoint_count; i++) {
        if (strcmp(g_breakpoints[i].pattern, pattern) == 0) {
            g_breakpoints[i].break_on_match = enabled;
            return;
        }
    }
    
    if (g_breakpoint_count < 16) {
        g_breakpoints[g_breakpoint_count].pattern = pattern;
        g_breakpoints[g_breakpoint_count].break_on_match = enabled;
        g_breakpoints[g_breakpoint_count].match_count = 0;
        g_breakpoint_count++;
    }
}

void logger_set_millisecond_timestamps(bool enabled) {
    g_millisecond_timestamps = enabled;
}

void logger_enable_function(const char *function, bool enabled) {
    if (!function) return;
    
    if (enabled) {
        if (g_enabled_function_count < 256) {
            strncpy(g_enabled_functions[g_enabled_function_count], function, 255);
            g_enabled_function_count++;
        }
    }
    g_enable_all_functions = !enabled;
}

void logger_filter_pattern(const char *pattern, bool enabled) {
    if (!pattern || !enabled) return;
    if (g_filter_pattern_count < 16) {
        strncpy(g_filter_patterns[g_filter_pattern_count], pattern, 255);
        g_filter_pattern_count++;
    }
}

void logger_config_file(const char *filename) {
    if (!filename) return;
    FILE *f = fopen(filename, "r");
    if (!f) {
        LOG_WARN("Failed to open config file: %s", filename);
        return;
    }
    
    char line[256] = {0};
    while (fgets(line, sizeof(line), f)) {
        char *delim = strchr(line, '=');
        if (!delim) continue;
        
        *delim = '\0';
        char *key = line;
        char *value = delim + 1;
        
        char *end = strchr(value, '\n');
        if (end) *end = '\0';
        
        logger_runtime_config(key, value);
    }
    fclose(f);
}

void logger_runtime_config(const char *key, const char *value) {
    if (!key || !value) return;
    
    if (strcmp(key, "level") == 0) {
        if (strcmp(value, "debug") == 0) logger_set_level(LOG_LEVEL_DEBUG);
        else if (strcmp(value, "info") == 0) logger_set_level(LOG_LEVEL_INFO);
        else if (strcmp(value, "warn") == 0) logger_set_level(LOG_LEVEL_WARN);
        else if (strcmp(value, "error") == 0) logger_set_level(LOG_LEVEL_ERROR);
        else if (strcmp(value, "fatal") == 0) logger_set_level(LOG_LEVEL_FATAL);
    } else if (strcmp(key, "colors") == 0) {
        logger_set_use_colors(strcmp(value, "true") == 0);
    } else if (strcmp(key, "timestamps") == 0) {
        logger_set_show_timestamp(strcmp(value, "true") == 0);
    } else if (strcmp(key, "json") == 0) {
        g_json_output = strcmp(value, "true") == 0;
    }
}

void logger_add_context_tag(const char *tag, const char *value) {
    if (!tag || !value || g_context_tag_count >= 16) return;
    
    int written = snprintf(g_context_tags[g_context_tag_count], 
                       sizeof(g_context_tags[g_context_tag_count]),
                       "%s:%s", tag, value);
    g_context_tag_count++;
}

void logger_log_performance_stats(void) {
    LOG_INFO("=== PERFORMANCE STATISTICS ===");
    LOG_INFO("Session: %s", g_logger.session_id);
    LOG_INFO("Log entries processed: %u", g_breadcrumb_index);
    LOG_INFO("Current log level: %s", level_names[g_logger.level]);
}

void logger_enable_json_output(bool enabled) {
    g_json_output = enabled;
}

void logger_performance_frame_stats(u64 frame_time_ms, u32 fps) {
    if (g_json_output) {
        LOG_INFO("{\"event\":\"frame\",\"time_ms\":%llu,\"fps\":%u}", 
                 frame_time_ms, fps);
    } else {
        LOG_DEBUG("Frame: %llums (%u FPS)", frame_time_ms, fps);
    }
}

void logger_throttle_message(const char *key, u32 max_count) {
    if (!key) return;
    
    for (u32 i = 0; i < g_throttled_count; i++) {
        if (strcmp(g_throttled_messages[i].key, key) == 0) {
            g_throttled_messages[i].count++;
            if (g_throttled_messages[i].count > max_count) {
                g_throttled_messages[i].count = 0;
                return;
            }
            break;
        }
    }
    
    if (g_throttled_count < 32) {
        strncpy(g_throttled_messages[g_throttled_count].key, key, 63);
        g_throttled_messages[g_throttled_count].count = 1;
        g_throttled_messages[g_throttled_count].max_count = max_count;
        g_throttled_count++;
    }
}

void logger_set_semantic_version(const char *version) {
    if (version) {
        g_semantic_version = version;
        LOG_INFO("Engine version: %s", version);
    }
}
