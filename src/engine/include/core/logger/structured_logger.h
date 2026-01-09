#ifndef STRUCTURED_LOGGER_H
#define STRUCTURED_LOGGER_H

#include <stdbool.h>

typedef enum { 
    LOG_DEBUG, 
    LOG_INFO, 
    LOG_WARN, 
    LOG_ERROR, 
    LOG_FATAL 
} LogLevel;

void logger_init(const char *log_path, int buffer_capacity);
void logger_log_structured(LogLevel level, const char *file, int line,
                           const char *message, const char *fields_json);
void logger_benchmark(void);
void logger_compress_log(const char *log_path);
const char *logger_color_code(LogLevel level);
const char *log_level_string(LogLevel level);

#endif // STRUCTURED_LOGGER_H
