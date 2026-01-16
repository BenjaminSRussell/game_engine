#include "Public/Log.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

bool Log_Init(void) {
  // printf("Log System Initialized\n");
  return true;
}

void Log_Shutdown(void) {
  // printf("Log System Shutdown\n");
}

void Log_Message(LogLevel level, const char *message, ...) {
  va_list args;
  va_start(args, message);

  // Simple console output
  const char *prefix = "[INFO]";
  if (level == LOG_LEVEL_DEBUG)
    prefix = "[DEBUG]";
  if (level == LOG_LEVEL_WARN)
    prefix = "[WARN]";
  if (level == LOG_LEVEL_ERROR)
    prefix = "[ERROR]";
  if (level == LOG_LEVEL_FATAL)
    prefix = "[FATAL]";

  printf("%s ", prefix);
  vprintf(message, args);
  printf("\n");

  va_end(args);
}
