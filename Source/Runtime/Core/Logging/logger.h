/**
 * @file logger.h
 * @brief Unified logging system for the engine
 * @details Provides colored console output, file logging, and filtering
 *
 * Usage:
 *   LOG_INFO("SYSTEM", "Message: %d", 42);
 *   LOG_ERROR("MODULE", "Error occurred");
 *   LOG_DEBUG("PHYSICS", "Detailed info");
 *
 * Dependencies:
 *   - sys/terminal.h (for colored output)
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ============================================================================
 * LOG LEVELS
 * ============================================================================
 */

typedef enum {
    LOG_TRACE = 0,      // Very detailed information
    LOG_DEBUG,          // Detailed information for debugging
    LOG_INFO,           // General information
    LOG_WARN,           // Warning messages
    LOG_ERROR,          // Error messages
    LOG_FATAL,          // Critical errors
    LOG_LEVEL_COUNT
} LogLevel;

/**
 * ============================================================================
 * INITIALIZATION & CONFIGURATION
 * ============================================================================
 */

/**
 * Initialize logging system
 * @param log_file Optional file path for file logging (NULL to disable)
 * @return true on success
 */
bool logger_init(const char* log_file);

/**
 * Shutdown logging system
 */
void logger_shutdown(void);

/**
 * Set minimum log level (messages below this are filtered)
 * @param level Minimum level to display
 */
void logger_set_level(LogLevel level);

/**
 * Get current log level
 * @return Current minimum log level
 */
LogLevel logger_get_level(void);

/**
 * Enable/disable file logging
 * @param enabled Whether to log to file
 */
void logger_set_file_logging(bool enabled);

/**
 * Enable/disable console logging
 * @param enabled Whether to log to console
 */
void logger_set_console_logging(bool enabled);

/**
 * Set module filter (only log specific modules)
 * @param module_filter Comma-separated module names
 */
void logger_set_module_filter(const char* module_filter);

/**
 * Clear module filter (log all modules)
 */
void logger_clear_module_filter(void);

/**
 * ============================================================================
 * CORE LOGGING FUNCTION
 * ============================================================================
 */

/**
 * Log a message
 * @param level Log level
 * @param module Module name (e.g., "PHYSICS", "RENDER")
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
void logger_log(LogLevel level, const char* module, const char* fmt, ...);

/**
 * Log a message with variable argument list
 * @param level Log level
 * @param module Module name
 * @param fmt Format string
 * @param args Variable argument list
 */
void logger_logv(LogLevel level, const char* module, const char* fmt, va_list args);

/**
 * ============================================================================
 * CONVENIENCE MACROS
 * ============================================================================
 */

/** Log trace-level message */
#define LOG_TRACE(module, fmt, ...) \
    logger_log(LOG_TRACE, module, fmt, ##__VA_ARGS__)

/** Log debug-level message */
#define LOG_DEBUG(module, fmt, ...) \
    logger_log(LOG_DEBUG, module, fmt, ##__VA_ARGS__)

/** Log info-level message */
#define LOG_INFO(module, fmt, ...) \
    logger_log(LOG_INFO, module, fmt, ##__VA_ARGS__)

/** Log warning-level message */
#define LOG_WARN(module, fmt, ...) \
    logger_log(LOG_WARN, module, fmt, ##__VA_ARGS__)

/** Log error-level message */
#define LOG_ERROR(module, fmt, ...) \
    logger_log(LOG_ERROR, module, fmt, ##__VA_ARGS__)

/** Log fatal-level message */
#define LOG_FATAL(module, fmt, ...) \
    logger_log(LOG_FATAL, module, fmt, ##__VA_ARGS__)

/**
 * ============================================================================
 * ADVANCED FEATURES
 * ============================================================================
 */

/**
 * Begin a collapsible log group
 * @param name Group name
 */
void logger_group_begin(const char* name);

/**
 * End current log group
 */
void logger_group_end(void);

/**
 * Flush pending log messages
 */
void logger_flush(void);

/**
 * Dump all logs to file
 * @param filepath Output file path
 */
void logger_dump_to_file(const char* filepath);

/**
 * Get number of messages logged at each level
 * @param level Log level
 * @return Count of messages at this level
 */
uint32_t logger_get_message_count(LogLevel level);

/**
 * Clear all message counts
 */
void logger_clear_stats(void);

/**
 * Print logging statistics
 */
void logger_print_stats(void);

/**
 * ============================================================================
 * ASSERTION SUPPORT
 * ============================================================================
 */

/**
 * Assert with logging
 * @param condition Condition to check
 * @param module Module name
 * @param message Message if assertion fails
 */
#define LOG_ASSERT(condition, module, message) \
    do { \
        if (!(condition)) { \
            LOG_FATAL(module, "ASSERTION FAILED: %s", message); \
            __builtin_trap(); \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif // LOGGER_H
