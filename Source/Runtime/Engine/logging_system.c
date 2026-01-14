// src/engine/core/logging_system.c
//
// Purpose: Unified logging system for the engine core
// Provides multiple output channels and severity levels

#include <core/logging_system.h>
#include <core/memory_allocator.h>
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
#endif

// Maximum log message size
#define MAX_LOG_MESSAGE_SIZE 4096
#define MAX_LOG_FILES 10
#define MAX_LOG_FILE_SIZE (10 * 1024 * 1024) // 10MB

typedef struct {
    FILE* file;
    char filename[256];
    size_t current_size;
    bool enabled;
} LogFile;

static struct {
    LogChannel active_channels;
    LogLevel min_level;
    LogFile log_file;
    bool initialized;
    u64 message_count;
    char buffer[MAX_LOG_MESSAGE_SIZE];
} g_logging_state = {0};

// Forward declarations
static void log_to_console(LogLevel level, const char* message);
static void log_to_file(LogLevel level, const char* message);
static void log_to_debugger(LogLevel level, const char* message);
static const char* get_level_string(LogLevel level);
static const char* get_timestamp(void);
static void rotate_log_file(void);

bool logging_system_init(void) {
    if (g_logging_state.initialized) {
        LOG_WARN("Logging system already initialized");
        return true;
    }

    LOG_INFO("Initializing unified logging system...");
    
    // Initialize default settings
    g_logging_state.active_channels = LOG_CHANNEL_CONSOLE | LOG_CHANNEL_FILE;
    g_logging_state.min_level = LOG_LEVEL_INFO;
    g_logging_state.message_count = 0;
    
    // Open log file
    const char* log_filename = "voxelforge.log";
    g_logging_state.log_file.file = fopen(log_filename, "a");
    if (g_logging_state.log_file.file) {
        strncpy(g_logging_state.log_file.filename, log_filename, sizeof(g_logging_state.log_file.filename) - 1);
        g_logging_state.log_file.filename[sizeof(g_logging_state.log_file.filename) - 1] = '\0';
        
        // Get current file size
        fseek(g_logging_state.log_file.file, 0, SEEK_END);
        g_logging_state.log_file.current_size = ftell(g_logging_state.log_file.file);
        g_logging_state.log_file.enabled = true;
        
        LOG_INFO("Log file opened: %s", log_filename);
    } else {
        LOG_WARN("Failed to open log file, continuing without file logging");
        g_logging_state.log_file.enabled = false;
    }
    
    g_logging_state.initialized = true;
    
    // Log initialization message
    LOG_INFO(" Logging system initialized successfully");
    LOG_INFO("Active channels: Console %s, File %s", 
             (g_logging_state.active_channels & LOG_CHANNEL_CONSOLE) ? "" : "",
             (g_logging_state.active_channels & LOG_CHANNEL_FILE) ? "" : "");
    
    return true;
}

void logging_system_shutdown(void) {
    if (!g_logging_state.initialized) {
        return;
    }

    LOG_INFO("Shutting down logging system...");
    LOG_INFO("Total messages logged: %llu", g_logging_state.message_count);
    
    // Close log file
    if (g_logging_state.log_file.file) {
        fclose(g_logging_state.log_file.file);
        g_logging_state.log_file.file = NULL;
    }
    
    g_logging_state.initialized = false;
    LOG_INFO("Logging system shutdown complete");
}

void logging_system_set_level(LogLevel level) {
    g_logging_state.min_level = level;
    LOG_INFO("Log level set to: %s", get_level_string(level));
}

void logging_system_set_channels(LogChannel channels) {
    g_logging_state.active_channels = channels;
    LOG_INFO("Log channels updated");
}

void logging_log(LogLevel level, const char* file, int line, const char* format, ...) {
    if (!g_logging_state.initialized || level < g_logging_state.min_level) {
        return;
    }

    // Format message
    va_list args;
    va_start(args, format);
    vsnprintf(g_logging_state.buffer, MAX_LOG_MESSAGE_SIZE, format, args);
    va_end(args);

    // Create full message with timestamp and location
    char full_message[MAX_LOG_MESSAGE_SIZE];
    snprintf(full_message, MAX_LOG_MESSAGE_SIZE, 
             "[%s] [%s] %s:%d: %s",
             get_timestamp(),
             get_level_string(level),
             file ? file : "unknown",
             line,
             g_logging_state.buffer);

    // Output to active channels
    if (g_logging_state.active_channels & LOG_CHANNEL_CONSOLE) {
        log_to_console(level, full_message);
    }
    
    if (g_logging_state.active_channels & LOG_CHANNEL_FILE && g_logging_state.log_file.enabled) {
        log_to_file(level, full_message);
    }
    
    if (g_logging_state.active_channels & LOG_CHANNEL_DEBUGGER) {
        log_to_debugger(level, full_message);
    }

    g_logging_state.message_count++;
}

// Static helper functions
static void log_to_console(LogLevel level, const char* message) {
    // Use colors for console output
    const char* color_code = "";
    
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    
    switch (level) {
        case LOG_LEVEL_ERROR:   color = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
        case LOG_LEVEL_WARN:    color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
        case LOG_LEVEL_INFO:    color = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
        case LOG_LEVEL_DEBUG:   color = FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
        case LOG_LEVEL_TRACE:   color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
    }
    
    SetConsoleTextAttribute(hConsole, color);
    printf("%s\n", message);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    switch (level) {
        case LOG_LEVEL_ERROR:   color_code = "\033[1;31m"; break; // Red
        case LOG_LEVEL_WARN:    color_code = "\033[1;33m"; break; // Yellow
        case LOG_LEVEL_INFO:    color_code = "\033[1;36m"; break; // Cyan
        case LOG_LEVEL_DEBUG:   color_code = "\033[1;34m"; break; // Blue
        case LOG_LEVEL_TRACE:   color_code = "\033[0;37m"; break; // White
    }
    
    printf("%s%s\033[0m\n", color_code, message);
#endif
}

static void log_to_file(LogLevel level, const char* message) {
    if (!g_logging_state.log_file.file) {
        return;
    }

    // Check if we need to rotate the file
    if (g_logging_state.log_file.current_size > MAX_LOG_FILE_SIZE) {
        rotate_log_file();
    }

    fprintf(g_logging_state.log_file.file, "%s\n", message);
    fflush(g_logging_state.log_file.file);
    
    g_logging_state.log_file.current_size += strlen(message) + 1;
}

static void log_to_debugger(LogLevel level, const char* message) {
#ifdef _WIN32
    OutputDebugStringA(message);
    OutputDebugStringA("\n");
#elif defined(__APPLE__)
    // macOS - write to system log
    fprintf(stderr, "%s\n", message);
#else
    // Linux - could use syslog or other debugging mechanisms
    // For now, just ignore
    (void)level;
    (void)message;
#endif
}

static const char* get_level_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_ERROR:   return "ERROR";
        case LOG_LEVEL_WARN:    return "WARN";
        case LOG_LEVEL_INFO:    return "INFO";
        case LOG_LEVEL_DEBUG:   return "DEBUG";
        case LOG_LEVEL_TRACE:   return "TRACE";
        default:                return "UNKNOWN";
    }
}

static const char* get_timestamp(void) {
    static char timestamp[32];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    return timestamp;
}

static void rotate_log_file(void) {
    if (!g_logging_state.log_file.file) {
        return;
    }

    // Close current file
    fclose(g_logging_state.log_file.file);
    
    // Rename current file
    char old_filename[512];
    snprintf(old_filename, sizeof(old_filename), "%s.old", g_logging_state.log_file.filename);
    rename(g_logging_state.log_file.filename, old_filename);
    
    // Open new file
    g_logging_state.log_file.file = fopen(g_logging_state.log_file.filename, "w");
    if (g_logging_state.log_file.file) {
        g_logging_state.log_file.current_size = 0;
        LOG_INFO("Log file rotated");
    } else {
        g_logging_state.log_file.enabled = false;
        LOG_ERROR("Failed to open new log file after rotation");
    }
}

// Convenience functions for different log levels
void log_error(const char* file, int line, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[MAX_LOG_MESSAGE_SIZE];
    vsnprintf(buffer, MAX_LOG_MESSAGE_SIZE, format, args);
    va_end(args);
    logging_log(LOG_LEVEL_ERROR, file, line, "%s", buffer);
}

void log_warn(const char* file, int line, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[MAX_LOG_MESSAGE_SIZE];
    vsnprintf(buffer, MAX_LOG_MESSAGE_SIZE, format, args);
    va_end(args);
    logging_log(LOG_LEVEL_WARN, file, line, "%s", buffer);
}

void log_info(const char* file, int line, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[MAX_LOG_MESSAGE_SIZE];
    vsnprintf(buffer, MAX_LOG_MESSAGE_SIZE, format, args);
    va_end(args);
    logging_log(LOG_LEVEL_INFO, file, line, "%s", buffer);
}

void log_debug(const char* file, int line, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[MAX_LOG_MESSAGE_SIZE];
    vsnprintf(buffer, MAX_LOG_MESSAGE_SIZE, format, args);
    va_end(args);
    logging_log(LOG_LEVEL_DEBUG, file, line, "%s", buffer);
}

void log_trace(const char* file, int line, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[MAX_LOG_MESSAGE_SIZE];
    vsnprintf(buffer, MAX_LOG_MESSAGE_SIZE, format, args);
    va_end(args);
    logging_log(LOG_LEVEL_TRACE, file, line, "%s", buffer);
}
