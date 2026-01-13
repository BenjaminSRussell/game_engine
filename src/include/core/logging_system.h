// include/core/logging_system.h
//
// Purpose: Unified logging system interface for the engine core

#ifndef LOGGING_SYSTEM_H
#define LOGGING_SYSTEM_H

#include <core/types.h>
#include <stdarg.h>

// Log levels
typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_DEBUG = 3,
    LOG_LEVEL_TRACE = 4
} LogLevel;

// Log channels (bit flags)
typedef enum {
    LOG_CHANNEL_CONSOLE = 1 << 0,
    LOG_CHANNEL_FILE = 1 << 1,
    LOG_CHANNEL_DEBUGGER = 1 << 2
} LogChannel;

// Logging system lifecycle
bool logging_system_init(void);
void logging_system_shutdown(void);

// Logging configuration
void logging_system_set_level(LogLevel level);
void logging_system_set_channels(LogChannel channels);

// Core logging function
void logging_log(LogLevel level, const char* file, int line, const char* format, ...);

// Convenience functions for different log levels
void log_error(const char* file, int line, const char* format, ...);
void log_warn(const char* file, int line, const char* format, ...);
void log_info(const char* file, int line, const char* format, ...);
void log_debug(const char* file, int line, const char* format, ...);
void log_trace(const char* file, int line, const char* format, ...);

// Convenience macros
#define LOG_ERROR(format, ...) log_error(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  log_warn(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)  log_info(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) log_debug(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_TRACE(format, ...) log_trace(__FILE__, __LINE__, format, ##__VA_ARGS__)

// Legacy compatibility with existing LOG_* macros
#define LOG_FATAL LOG_ERROR
#define LOG(level, format, ...) \
    do { \
        if (level == 0) LOG_ERROR(format, ##__VA_ARGS__); \
        else if (level == 1) LOG_WARN(format, ##__VA_ARGS__); \
        else if (level == 2) LOG_INFO(format, ##__VA_ARGS__); \
        else if (level == 3) LOG_DEBUG(format, ##__VA_ARGS__); \
        else LOG_TRACE(format, ##__VA_ARGS__); \
    } while(0)

#endif // LOGGING_SYSTEM_H
