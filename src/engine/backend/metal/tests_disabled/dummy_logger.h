#ifndef DUMMY_LOGGER_H
#define DUMMY_LOGGER_H

#include <stdarg.h>
#include <stdio.h>

typedef enum {
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARN,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_FATAL
} LogLevel;

#define LOG_DEBUG(...)                                                         \
  printf("[DEBUG] " __VA_ARGS__);                                              \
  printf("\n")
#define LOG_INFO(...)                                                          \
  printf("[INFO] " __VA_ARGS__);                                               \
  printf("\n")
#define LOG_WARN(...)                                                          \
  printf("[WARN] " __VA_ARGS__);                                               \
  printf("\n")
#define LOG_ERROR(...)                                                         \
  printf("[ERROR] " __VA_ARGS__);                                              \
  printf("\n")
#define LOG_FATAL(...)                                                         \
  printf("[FATAL] " __VA_ARGS__);                                              \
  printf("\n")

inline void logger_log(LogLevel level, const char *category, const char *format,
                       ...) {
  va_list args;
  va_start(args, format);
  printf("[%d] [%s] ", level, category);
  vprintf(format, args);
  printf("\n");
  va_end(args);
}

#endif
