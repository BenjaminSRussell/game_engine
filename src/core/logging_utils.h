// Logging Utilities
// Centralized logging utilities to reduce code duplication

#ifndef LOGGING_UTILS_H
#define LOGGING_UTILS_H

#include "core/types.h"
#include "core/logger.h"

#ifdef __cplusplus
extern "C" {
#endif

// Logging context for better categorization
typedef enum {
    LOG_CONTEXT_CORE = 0,
    LOG_CONTEXT_RENDERING,
    LOG_CONTEXT_PHYSICS,
    LOG_CONTEXT_AUDIO,
    LOG_CONTEXT_NETWORK,
    LOG_CONTEXT_ANIMATION,
    LOG_CONTEXT_AI,
    LOG_CONTEXT_ASSETS,
    LOG_CONTEXT_EDITOR,
    LOG_CONTEXT_PLATFORM,
    LOG_CONTEXT_COUNT
} LogContext;

// Logging levels with context
#define LOG_CONTEXT_ERROR(context, ...) logging_utils_log(LOG_LEVEL_ERROR, context, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_CONTEXT_WARN(context, ...)  logging_utils_log(LOG_LEVEL_WARN, context, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_CONTEXT_INFO(context, ...)  logging_utils_log(LOG_LEVEL_INFO, context, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_CONTEXT_DEBUG(context, ...) logging_utils_log(LOG_LEVEL_DEBUG, context, __FILE__, __LINE__, __VA_ARGS__)

// Context-specific logging macros
#define LOG_CORE_ERROR(...) LOG_CONTEXT_ERROR(LOG_CONTEXT_CORE, __VA_ARGS__)
#define LOG_CORE_WARN(...)  LOG_CONTEXT_WARN(LOG_CONTEXT_CORE, __VA_ARGS__)
#define LOG_CORE_INFO(...)  LOG_CONTEXT_INFO(LOG_CONTEXT_CORE, __VA_ARGS__)
#define LOG_CORE_DEBUG(...) LOG_CONTEXT_DEBUG(LOG_CONTEXT_CORE, __VA_ARGS__)

#define LOG_RENDER_ERROR(...) LOG_CONTEXT_ERROR(LOG_CONTEXT_RENDERING, __VA_ARGS__)
#define LOG_RENDER_WARN(...)  LOG_CONTEXT_WARN(LOG_CONTEXT_RENDERING, __VA_ARGS__)
#define LOG_RENDER_INFO(...)  LOG_CONTEXT_INFO(LOG_CONTEXT_RENDERING, __VA_ARGS__)
#define LOG_RENDER_DEBUG(...) LOG_CONTEXT_DEBUG(LOG_CONTEXT_RENDERING, __VA_ARGS__)

#define LOG_PHYSICS_ERROR(...) LOG_CONTEXT_ERROR(LOG_CONTEXT_PHYSICS, __VA_ARGS__)
#define LOG_PHYSICS_WARN(...)  LOG_CONTEXT_WARN(LOG_CONTEXT_PHYSICS, __VA_ARGS__)
#define LOG_PHYSICS_INFO(...)  LOG_CONTEXT_INFO(LOG_CONTEXT_PHYSICS, __VA_ARGS__)
#define LOG_PHYSICS_DEBUG(...) LOG_CONTEXT_DEBUG(LOG_CONTEXT_PHYSICS, __VA_ARGS__)

#define LOG_AUDIO_ERROR(...) LOG_CONTEXT_ERROR(LOG_CONTEXT_AUDIO, __VA_ARGS__)
#define LOG_AUDIO_WARN(...)  LOG_CONTEXT_WARN(LOG_CONTEXT_AUDIO, __VA_ARGS__)
#define LOG_AUDIO_INFO(...)  LOG_CONTEXT_INFO(LOG_CONTEXT_AUDIO, __VA_ARGS__)
#define LOG_AUDIO_DEBUG(...) LOG_CONTEXT_DEBUG(LOG_CONTEXT_AUDIO, __VA_ARGS__)

#define LOG_NETWORK_ERROR(...) LOG_CONTEXT_ERROR(LOG_CONTEXT_NETWORK, __VA_ARGS__)
#define LOG_NETWORK_WARN(...)  LOG_CONTEXT_WARN(LOG_CONTEXT_NETWORK, __VA_ARGS__)
#define LOG_NETWORK_INFO(...) LOG_CONTEXT_INFO(LOG_CONTEXT_NETWORK, __VA_ARGS__)
#define LOG_NETWORK_DEBUG(...) LOG_CONTEXT_DEBUG(LOG_CONTEXT_NETWORK, __VA_ARGS__)

#define LOG_ANIM_ERROR(...) LOG_CONTEXT_ERROR(LOG_CONTEXT_ANIMATION, __VA_ARGS__)
#define LOG_ANIM_WARN(...)  LOG_CONTEXT_WARN(LOG_CONTEXT_ANIMATION, __VA_ARGS__)
#define LOG_ANIM_INFO(...)  LOG_CONTEXT_INFO(LOG_CONTEXT_ANIMATION, __VA_ARGS__)
#define LOG_ANIM_DEBUG(...) LOG_CONTEXT_DEBUG(LOG_CONTEXT_ANIMATION, __VA_ARGS__)

#define LOG_AI_ERROR(...) LOG_CONTEXT_ERROR(LOG_CONTEXT_AI, __VA_ARGS__)
#define LOG_AI_WARN(...)  LOG_CONTEXT_WARN(LOG_CONTEXT_AI, __VA_ARGS__)
#define LOG_AI_INFO(...)  LOG_CONTEXT_INFO(LOG_CONTEXT_AI, __VA_ARGS__)
#define LOG_AI_DEBUG(...) LOG_CONTEXT_DEBUG(LOG_CONTEXT_AI, __VA_ARGS__)

#define LOG_ASSET_ERROR(...) LOG_CONTEXT_ERROR(LOG_CONTEXT_ASSETS, __VA_ARGS__)
#define LOG_ASSET_WARN(...)  LOG_CONTEXT_WARN(LOG_CONTEXT_ASSETS, __VA_ARGS__)
#define LOG_ASSET_INFO(...)  LOG_CONTEXT_INFO(LOG_CONTEXT_ASSETS, __VA_ARGS__)
#define LOG_ASSET_DEBUG(...) LOG_CONTEXT_DEBUG(LOG_CONTEXT_ASSETS, __VA_ARGS__)

#define LOG_EDITOR_ERROR(...) LOG_CONTEXT_ERROR(LOG_CONTEXT_EDITOR, __VA_ARGS__)
#define LOG_EDITOR_WARN(...)  LOG_CONTEXT_WARN(LOG_CONTEXT_EDITOR, __VA_ARGS__)
#define LOG_EDITOR_INFO(...)  LOG_CONTEXT_INFO(LOG_CONTEXT_EDITOR, __VA_ARGS__)
#define LOG_EDITOR_DEBUG(...) LOG_CONTEXT_DEBUG(LOG_CONTEXT_EDITOR, __VA_ARGS__)

#define LOG_PLATFORM_ERROR(...) LOG_CONTEXT_ERROR(LOG_CONTEXT_PLATFORM, __VA_ARGS__)
#define LOG_PLATFORM_WARN(...)  LOG_CONTEXT_WARN(LOG_CONTEXT_PLATFORM, __VA_ARGS__)
#define LOG_PLATFORM_INFO(...)  LOG_CONTEXT_INFO(LOG_CONTEXT_PLATFORM, __VA_ARGS__)
#define LOG_PLATFORM_DEBUG(...) LOG_CONTEXT_DEBUG(LOG_CONTEXT_PLATFORM, __VA_ARGS__)

// Performance logging
#define LOG_PERF_START(name) logging_utils_perf_start(name)
#define LOG_PERF_END(name) logging_utils_perf_end(name)
#define LOG_PERF_FRAME_START() logging_utils_perf_frame_start()
#define LOG_PERF_FRAME_END() logging_utils_perf_frame_end()

// Function entry/exit logging
#define LOG_FUNCTION_ENTRY() LOG_DEBUG("Entering %s", __FUNCTION__)
#define LOG_FUNCTION_EXIT()  LOG_DEBUG("Exiting %s", __FUNCTION__)
#define LOG_FUNCTION_EXIT_RET(ret) LOG_DEBUG("Exiting %s with return value", __FUNCTION__)

// Conditional logging
#define LOG_DEBUG_IF(condition, ...) do { if (condition) LOG_DEBUG(__VA_ARGS__); } while(0)
#define LOG_INFO_IF(condition, ...)  do { if (condition) LOG_INFO(__VA_ARGS__); } while(0)
#define LOG_WARN_IF(condition, ...)  do { if (condition) LOG_WARN(__VA_ARGS__); } while(0)
#define LOG_ERROR_IF(condition, ...) do { if (condition) LOG_ERROR(__VA_ARGS__); } while(0)

// Initialize logging utilities
void logging_utils_init(void);

// Shutdown logging utilities
void logging_utils_shutdown(void);

// Set logging level for specific context
void logging_utils_set_context_level(LogContext context, LogLevel level);

// Enable/disable specific context
void logging_utils_set_context_enabled(LogContext context, bool enabled);

// Log with context
void logging_utils_log(LogLevel level, LogContext context, const char *file, int line, const char *format, ...);

// Performance logging
void logging_utils_perf_start(const char *name);
void logging_utils_perf_end(const char *name);
void logging_utils_perf_frame_start(void);
void logging_utils_perf_frame_end(void);

// Get context name as string
const char *logging_utils_get_context_name(LogContext context);

// Print logging statistics
void logging_utils_print_stats(void);

// Enable/disable file logging
void logging_utils_set_file_logging(bool enabled, const char *filename);

// Flush all logs
void logging_utils_flush(void);

#ifdef __cplusplus
}
#endif

#endif // LOGGING_UTILS_H
