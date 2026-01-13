#ifndef STRUCTURED_LOGGER_H
#define STRUCTURED_LOGGER_H

// Redirect to the consolidated unified logger
#include "unified_logger.h"

// Note: All structured_logger.h functionality has been consolidated into unified_logger.h
// This header is kept for backwards compatibility only

// Legacy compatibility aliases
#define logger_init(path, capacity) unified_logger_init(&(LoggerConfig){ \
    .min_level = LOG_LEVEL_INFO, \
    .enabled_channels = LOG_CHANNEL_ALL, \
    .use_colors = true, \
    .log_file_path = "", \
    .use_buffering = false \
})

#define logger_log_structured(level, file, line, message, fields_json) \
    unified_logger_log_structured(level, LOG_CAT_GENERAL, file, line, __func__, message, fields_json)

#define logger_benchmark unified_logger_benchmark
#define logger_compress_log(path) unified_logger_rotate() // Simplified mapping
#define logger_color_code unified_logger_get_color_code
#define log_level_string unified_logger_get_level_string

#endif // STRUCTURED_LOGGER_H
