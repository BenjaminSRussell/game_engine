/**
 * @file logger.h
 * @brief Unified logging system API
 */

#ifndef VOXELFORGE_LOGGER_H
#define VOXELFORGE_LOGGER_H

#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Log Levels
// ============================================================================

typedef enum VF_LogLevel {
  VF_LOG_LEVEL_TRACE = 0,
  VF_LOG_LEVEL_DEBUG = 1,
  VF_LOG_LEVEL_INFO = 2,
  VF_LOG_LEVEL_WARN = 3,
  VF_LOG_LEVEL_ERROR = 4,
  VF_LOG_LEVEL_FATAL = 5,
  VF_LOG_LEVEL_OFF = 6,
} VF_LogLevel;

// ============================================================================
// Log Categories
// ============================================================================

typedef enum VF_LogCategory {
  VF_LOG_CATEGORY_CORE = 0,
  VF_LOG_CATEGORY_RENDERER,
  VF_LOG_CATEGORY_PHYSICS,
  VF_LOG_CATEGORY_AUDIO,
  VF_LOG_CATEGORY_ANIMATION,
  VF_LOG_CATEGORY_AI,
  VF_LOG_CATEGORY_NETWORKING,
  VF_LOG_CATEGORY_UI,
  VF_LOG_CATEGORY_SCENE,
  VF_LOG_CATEGORY_INPUT,
  VF_LOG_CATEGORY_ASSET,
  VF_LOG_CATEGORY_GAME,
  VF_LOG_CATEGORY_EDITOR,
  VF_LOG_CATEGORY_COUNT
} VF_LogCategory;

// ============================================================================
// API
// ============================================================================

VF_API VF_Result vf_logger_init(void);
VF_API void vf_logger_shutdown(void);

VF_API void vf_logger_set_level(VF_LogLevel level);
VF_API void vf_logger_set_category_level(VF_LogCategory category,
                                         VF_LogLevel level);
VF_API void vf_logger_enable_file_output(const char *filepath);
VF_API void vf_logger_enable_console_output(b8 enable);

VF_API void vf_log(VF_LogLevel level, VF_LogCategory category, const char *file,
                   i32 line, const char *fmt, ...);

// ============================================================================
// Convenience Macros
// ============================================================================

#define VF_LOG_TRACE(cat, ...)                                                 \
  vf_log(VF_LOG_LEVEL_TRACE, (cat), __FILE__, __LINE__, __VA_ARGS__)
#define VF_LOG_DEBUG(cat, ...)                                                 \
  vf_log(VF_LOG_LEVEL_DEBUG, (cat), __FILE__, __LINE__, __VA_ARGS__)
#define VF_LOG_INFO(cat, ...)                                                  \
  vf_log(VF_LOG_LEVEL_INFO, (cat), __FILE__, __LINE__, __VA_ARGS__)
#define VF_LOG_WARN(cat, ...)                                                  \
  vf_log(VF_LOG_LEVEL_WARN, (cat), __FILE__, __LINE__, __VA_ARGS__)
#define VF_LOG_ERROR(cat, ...)                                                 \
  vf_log(VF_LOG_LEVEL_ERROR, (cat), __FILE__, __LINE__, __VA_ARGS__)
#define VF_LOG_FATAL(cat, ...)                                                 \
  vf_log(VF_LOG_LEVEL_FATAL, (cat), __FILE__, __LINE__, __VA_ARGS__)

// Core-specific shortcuts
#define LOG_TRACE(...) VF_LOG_TRACE(VF_LOG_CATEGORY_CORE, __VA_ARGS__)
#define LOG_DEBUG(...) VF_LOG_DEBUG(VF_LOG_CATEGORY_CORE, __VA_ARGS__)
#define LOG_INFO(...) VF_LOG_INFO(VF_LOG_CATEGORY_CORE, __VA_ARGS__)
#define LOG_WARN(...) VF_LOG_WARN(VF_LOG_CATEGORY_CORE, __VA_ARGS__)
#define LOG_ERROR(...) VF_LOG_ERROR(VF_LOG_CATEGORY_CORE, __VA_ARGS__)
#define LOG_FATAL(...) VF_LOG_FATAL(VF_LOG_CATEGORY_CORE, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_LOGGER_H
