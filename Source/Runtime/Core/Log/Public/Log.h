#ifndef ULTIMATE_ENGINE_LOG_H
#define ULTIMATE_ENGINE_LOG_H

#include "../../Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  LOG_LEVEL_FATAL = 0,
  LOG_LEVEL_ERROR = 1,
  LOG_LEVEL_WARN = 2,
  LOG_LEVEL_INFO = 3,
  LOG_LEVEL_DEBUG = 4,
  LOG_LEVEL_TRACE = 5
} LogLevel;

VF_API bool Log_Init(void);
VF_API void Log_Shutdown(void);

VF_API void Log_Message(LogLevel level, const char *message, ...);

// Convenience macros
#define VF_FATAL(message, ...)                                                 \
  Log_Message(LOG_LEVEL_FATAL, message, ##__VA_ARGS__);
#define VF_ERROR(message, ...)                                                 \
  Log_Message(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);
#define VF_WARN(message, ...)                                                  \
  Log_Message(LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#define VF_INFO(message, ...)                                                  \
  Log_Message(LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#define VF_DEBUG(message, ...)                                                 \
  Log_Message(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#define VF_TRACE(message, ...)                                                 \
  Log_Message(LOG_LEVEL_TRACE, message, ##__VA_ARGS__);

#ifdef __cplusplus
}
#endif

#endif // ULTIMATE_ENGINE_LOG_H
