/**
 * @file error_logging.h
 * @brief Rendering error logging and tracking system
 *
 * Comprehensive error tracking and logging for debugging
 * GPU error detection, validation, and reporting
 */

#ifndef RENDER_ERROR_LOGGING_H
#define RENDER_ERROR_LOGGING_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Error severity levels
typedef enum {
    ERROR_LEVEL_DEBUG = 0,
    ERROR_LEVEL_INFO = 1,
    ERROR_LEVEL_WARNING = 2,
    ERROR_LEVEL_ERROR = 3,
    ERROR_LEVEL_CRITICAL = 4,
    ERROR_LEVEL_COUNT
} ErrorLevel;

// Error categories
typedef enum {
    ERROR_CATEGORY_RENDERING = 0,
    ERROR_CATEGORY_SHADER = 1,
    ERROR_CATEGORY_TEXTURE = 2,
    ERROR_CATEGORY_MESH = 3,
    ERROR_CATEGORY_MEMORY = 4,
    ERROR_CATEGORY_GPU = 5,
    ERROR_CATEGORY_AUDIO = 6,
    ERROR_CATEGORY_INPUT = 7,
    ERROR_CATEGORY_NETWORK = 8,
    ERROR_CATEGORY_FILESYSTEM = 9,
    ERROR_CATEGORY_VALIDATION = 10,
    ERROR_CATEGORY_COUNT
} ErrorCategory;

// Error entry structure
typedef struct {
    uint64_t timestamp;
    ErrorLevel level;
    ErrorCategory category;
    uint32_t error_code;
    char message[512];
    char file_path[256];
    int line_number;
    char function_name[64];
    uint32_t thread_id;
    bool resolved;
    uint64_t resolved_timestamp;
    char resolution_notes[256];
} ErrorEntry;

// Error statistics
typedef struct {
    uint32_t total_errors;
    uint32_t errors_by_level[ERROR_LEVEL_COUNT];
    uint32_t errors_by_category[ERROR_CATEGORY_COUNT];
    uint32_t unresolved_errors;
    uint32_t resolved_errors;
    uint64_t first_error_time;
    uint64_t last_error_time;
} ErrorStats;

// Error logging context
typedef struct {
    ErrorEntry* entries;
    uint32_t capacity;
    uint32_t count;
    uint32_t head;
    uint32_t tail;
    bool circular_buffer;
    FILE* log_file;
    char log_file_path[256];
    bool file_logging_enabled;
    bool console_logging_enabled;
    ErrorLevel min_log_level;
    ErrorStats stats;
    uint32_t max_log_entries;
    bool auto_rotate_logs;
    size_t max_log_file_size;
} ErrorLoggingContext;

// Core logging functions
int error_logging_init(ErrorLoggingContext* ctx, const char* log_file_path, uint32_t max_entries);
int error_logging_shutdown(ErrorLoggingContext* ctx);
int error_logging_flush(ErrorLoggingContext* ctx);

// Error reporting functions
int error_log(ErrorLoggingContext* ctx, ErrorLevel level, ErrorCategory category, 
                uint32_t error_code, const char* message, const char* file_path, 
                int line_number, const char* function_name);

// Convenience macros
#define ERROR_LOG_DEBUG(ctx, category, code, msg) \
    error_log(ctx, ERROR_LEVEL_DEBUG, category, code, msg, __FILE__, __LINE__, __FUNCTION__)

#define ERROR_LOG_INFO(ctx, category, code, msg) \
    error_log(ctx, ERROR_LEVEL_INFO, category, code, msg, __FILE__, __LINE__, __FUNCTION__)

#define ERROR_LOG_WARN(ctx, category, code, msg) \
    error_log(ctx, ERROR_LEVEL_WARNING, category, code, msg, __FILE__, __LINE__, __FUNCTION__)

#define ERROR_LOG_ERROR(ctx, category, code, msg) \
    error_log(ctx, ERROR_LEVEL_ERROR, category, code, msg, __FILE__, __LINE__, __FUNCTION__)

#define ERROR_LOG_CRITICAL(ctx, category, code, msg) \
    error_log(ctx, ERROR_LEVEL_CRITICAL, category, code, msg, __FILE__, __LINE__, __FUNCTION__)

// GPU error tracking
int error_log_gpu_error(ErrorLoggingContext* ctx, uint32_t gpu_error_code, const char* gpu_error_string);
int error_log_shader_compilation_error(ErrorLoggingContext* ctx, const char* shader_name, 
                                         const char* compilation_log);
int error_log_texture_load_error(ErrorLoggingContext* ctx, const char* texture_path, 
                                   uint32_t texture_error);
int error_log_memory_allocation_error(ErrorLoggingContext* ctx, size_t requested_size, 
                                        const char* allocation_type);

// Error resolution functions
int error_mark_resolved(ErrorLoggingContext* ctx, uint64_t error_id, const char* resolution_notes);
int error_get_unresolved_errors(ErrorLoggingContext* ctx, ErrorEntry* out_errors, uint32_t max_count);
int error_get_errors_by_category(ErrorLoggingContext* ctx, ErrorCategory category, 
                                  ErrorEntry* out_errors, uint32_t max_count);

// Statistics and reporting
int error_get_stats(const ErrorLoggingContext* ctx, ErrorStats* out_stats);
int error_print_summary(const ErrorLoggingContext* ctx);
int error_export_to_file(const ErrorLoggingContext* ctx, const char* export_path);
int error_clear_all_errors(ErrorLoggingContext* ctx);

// Configuration functions
int error_set_min_log_level(ErrorLoggingContext* ctx, ErrorLevel level);
int error_enable_file_logging(ErrorLoggingContext* ctx, bool enabled);
int error_enable_console_logging(ErrorLoggingContext* ctx, bool enabled);
int error_set_auto_rotate(ErrorLoggingContext* ctx, bool enabled, size_t max_size);

// Validation functions
bool error_validate_context(const ErrorLoggingContext* ctx);
bool error_is_valid_error_level(ErrorLevel level);
bool error_is_valid_category(ErrorCategory category);

// Utility functions
const char* error_get_level_string(ErrorLevel level);
const char* error_get_category_string(ErrorCategory category);
uint64_t error_get_timestamp(void);
const char* error_format_timestamp(uint64_t timestamp, char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_ERROR_LOGGING_H */
