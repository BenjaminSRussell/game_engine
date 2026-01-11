/**
 * @file error_logging.c
 * @brief Rendering error logging and tracking system implementation
 */

#include "error_logging.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>

// Global error logging context
static ErrorLoggingContext g_error_logging_ctx = {0};

// Internal helper functions
static uint32_t get_thread_id(void);
static void write_error_to_file(FILE* file, const ErrorEntry* entry);
static void write_error_to_console(const ErrorEntry* entry);
static void update_statistics(ErrorLoggingContext* ctx, const ErrorEntry* entry);
static bool should_log_error(const ErrorLoggingContext* ctx, ErrorLevel level);
static void rotate_log_file_if_needed(ErrorLoggingContext* ctx);

// Core logging functions
int error_logging_init(ErrorLoggingContext* ctx, const char* log_file_path, uint32_t max_entries) {
    if (!ctx || !log_file_path || max_entries == 0) {
        return -1;
    }
    
    // Initialize context
    memset(ctx, 0, sizeof(ErrorLoggingContext));
    ctx->capacity = max_entries;
    ctx->min_log_level = ERROR_LEVEL_INFO;
    ctx->file_logging_enabled = true;
    ctx->console_logging_enabled = true;
    ctx->max_log_entries = max_entries;
    ctx->auto_rotate_logs = true;
    ctx->max_log_file_size = 10 * 1024 * 1024; // 10MB default
    
    // Copy log file path
    strncpy(ctx->log_file_path, log_file_path, sizeof(ctx->log_file_path) - 1);
    
    // Allocate error entries array
    ctx->entries = (ErrorEntry*)calloc(max_entries, sizeof(ErrorEntry));
    if (!ctx->entries) {
        return -1;
    }
    
    // Open log file
    ctx->log_file = fopen(ctx->log_file_path, "w");
    if (!ctx->log_file) {
        free(ctx->entries);
        ctx->entries = NULL;
        return -1;
    }
    
    // Write header to log file
    fprintf(ctx->log_file, "# Error Logging System - Started at %s\n", error_format_timestamp(error_get_timestamp(), (char[64]){0}, 64));
    fprintf(ctx->log_file, "# Format: Timestamp | Level | Category | Code | Message | File:Line | Function | Thread | Resolved\n");
    fflush(ctx->log_file);
    
    // Initialize statistics
    ctx->stats.first_error_time = error_get_timestamp();
    ctx->stats.last_error_time = ctx->stats.first_error_time;
    
    printf("Error logging system initialized: %s (max entries: %u)\n", log_file_path, max_entries);
    return 0;
}

int error_logging_shutdown(ErrorLoggingContext* ctx) {
    if (!ctx) return -1;
    
    // Write final statistics to log file
    if (ctx->log_file) {
        fprintf(ctx->log_file, "\n# Error Logging System - Shutdown at %s\n", error_format_timestamp(error_get_timestamp(), (char[64]){0}, 64}));
        fprintf(ctx->log_file, "# Total errors: %u\n", ctx->stats.total_errors);
        fprintf(ctx->log_file, "# Unresolved errors: %u\n", ctx->stats.unresolved_errors);
        fprintf(ctx->log_file, "# Resolved errors: %u\n", ctx->stats.resolved_errors);
        fclose(ctx->log_file);
        ctx->log_file = NULL;
    }
    
    // Free entries array
    if (ctx->entries) {
        free(ctx->entries);
        ctx->entries = NULL;
    }
    
    memset(ctx, 0, sizeof(ErrorLoggingContext));
    printf("Error logging system shutdown complete\n");
    return 0;
}

int error_logging_flush(ErrorLoggingContext* ctx) {
    if (!ctx || !ctx->log_file) return -1;
    
    fflush(ctx->log_file);
    return 0;
}

// Error reporting functions
int error_log(ErrorLoggingContext* ctx, ErrorLevel level, ErrorCategory category, 
                uint32_t error_code, const char* message, const char* file_path, 
                int line_number, const char* function_name) {
    if (!ctx || !message || !error_is_valid_error_level(level) || !error_is_valid_category(category)) {
        return -1;
    }
    
    // Check if we should log this error level
    if (!should_log_error(ctx, level)) {
        return 0;
    }
    
    // Use global context if none provided
    if (ctx == NULL) {
        ctx = &g_error_logging_ctx;
        if (!ctx->entries) {
            // Initialize with defaults if not already initialized
            error_logging_init(ctx, "error_log.txt", 1000);
        }
    }
    
    // Handle circular buffer
    if (ctx->count >= ctx->capacity) {
        if (ctx->circular_buffer) {
            // Overwrite oldest entry
            ctx->head = (ctx->head + 1) % ctx->capacity;
        } else {
            // Don't log if buffer is full and not circular
            return -2;
        }
    } else {
        ctx->tail = (ctx->tail + 1) % ctx->capacity;
    }
    
    // Create new error entry
    ErrorEntry* entry = &ctx->entries[ctx->tail];
    memset(entry, 0, sizeof(ErrorEntry));
    
    entry->timestamp = error_get_timestamp();
    entry->level = level;
    entry->category = category;
    entry->error_code = error_code;
    entry->line_number = line_number;
    entry->thread_id = get_thread_id();
    entry->resolved = false;
    
    // Copy strings safely
    strncpy(entry->message, message, sizeof(entry->message) - 1);
    if (file_path) {
        strncpy(entry->file_path, file_path, sizeof(entry->file_path) - 1);
    }
    if (function_name) {
        strncpy(entry->function_name, function_name, sizeof(entry->function_name) - 1);
    }
    
    // Update statistics
    update_statistics(ctx, entry);
    
    // Write to outputs
    if (ctx->file_logging_enabled && ctx->log_file) {
        write_error_to_file(ctx->log_file, entry);
    }
    
    if (ctx->console_logging_enabled) {
        write_error_to_console(entry);
    }
    
    // Rotate log file if needed
    rotate_log_file_if_needed(ctx);
    
    ctx->count++;
    return 0;
}

// GPU error tracking
int error_log_gpu_error(ErrorLoggingContext* ctx, uint32_t gpu_error_code, const char* gpu_error_string) {
    char message[512];
    snprintf(message, sizeof(message), "GPU Error 0x%08X: %s", gpu_error_code, gpu_error_string ? gpu_error_string : "Unknown GPU error");
    return error_log(ctx, ERROR_LEVEL_ERROR, ERROR_CATEGORY_GPU, gpu_error_code, message, NULL, 0, "error_log_gpu_error");
}

int error_log_shader_compilation_error(ErrorLoggingContext* ctx, const char* shader_name, 
                                         const char* compilation_log) {
    char message[512];
    snprintf(message, sizeof(message), "Shader compilation failed: %s\nCompilation log:\n%s", 
             shader_name ? shader_name : "Unknown shader", 
             compilation_log ? compilation_log : "No compilation log available");
    return error_log(ctx, ERROR_LEVEL_ERROR, ERROR_CATEGORY_SHADER, 1001, message, NULL, 0, "error_log_shader_compilation_error");
}

int error_log_texture_load_error(ErrorLoggingContext* ctx, const char* texture_path, 
                                   uint32_t texture_error) {
    char message[512];
    snprintf(message, sizeof(message), "Failed to load texture: %s (error code: %u)", 
             texture_path ? texture_path : "Unknown texture", texture_error);
    return error_log(ctx, ERROR_LEVEL_ERROR, ERROR_CATEGORY_TEXTURE, texture_error, message, NULL, 0, "error_log_texture_load_error");
}

int error_log_memory_allocation_error(ErrorLoggingContext* ctx, size_t requested_size, 
                                        const char* allocation_type) {
    char message[512];
    snprintf(message, sizeof(message), "Memory allocation failed: %zu bytes for %s", 
             requested_size, allocation_type ? allocation_type : "Unknown type");
    return error_log(ctx, ERROR_LEVEL_CRITICAL, ERROR_CATEGORY_MEMORY, 2001, message, NULL, 0, "error_log_memory_allocation_error");
}

// Error resolution functions
int error_mark_resolved(ErrorLoggingContext* ctx, uint64_t error_id, const char* resolution_notes) {
    if (!ctx || !ctx->entries) return -1;
    
    // Find error by timestamp (simplified - in real implementation would use proper IDs)
    for (uint32_t i = 0; i < ctx->count; i++) {
        uint32_t index = (ctx->head + i) % ctx->capacity;
        ErrorEntry* entry = &ctx->entries[index];
        
        if (entry->timestamp == error_id && !entry->resolved) {
            entry->resolved = true;
            entry->resolved_timestamp = error_get_timestamp();
            if (resolution_notes) {
                strncpy(entry->resolution_notes, resolution_notes, sizeof(entry->resolution_notes) - 1);
            }
            
            // Update statistics
            ctx->stats.unresolved_errors--;
            ctx->stats.resolved_errors++;
            
            // Log resolution
            char message[512];
            snprintf(message, sizeof(message), "Error resolved: %s", entry->message);
            error_log(ctx, ERROR_LEVEL_INFO, ERROR_CATEGORY_VALIDATION, 0, message, NULL, 0, "error_mark_resolved");
            
            return 0;
        }
    }
    
    return -1; // Error not found
}

int error_get_unresolved_errors(ErrorLoggingContext* ctx, ErrorEntry* out_errors, uint32_t max_count) {
    if (!ctx || !out_errors || max_count == 0) return -1;
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < ctx->count && count < max_count; i++) {
        uint32_t index = (ctx->head + i) % ctx->capacity;
        ErrorEntry* entry = &ctx->entries[index];
        
        if (!entry->resolved) {
            out_errors[count] = *entry;
            count++;
        }
    }
    
    return count;
}

int error_get_errors_by_category(ErrorLoggingContext* ctx, ErrorCategory category, 
                                  ErrorEntry* out_errors, uint32_t max_count) {
    if (!ctx || !out_errors || max_count == 0 || !error_is_valid_category(category)) return -1;
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < ctx->count && count < max_count; i++) {
        uint32_t index = (ctx->head + i) % ctx->capacity;
        ErrorEntry* entry = &ctx->entries[index];
        
        if (entry->category == category) {
            out_errors[count] = *entry;
            count++;
        }
    }
    
    return count;
}

// Statistics and reporting
int error_get_stats(const ErrorLoggingContext* ctx, ErrorStats* out_stats) {
    if (!ctx || !out_stats) return -1;
    
    *out_stats = ctx->stats;
    return 0;
}

int error_print_summary(const ErrorLoggingContext* ctx) {
    if (!ctx) return -1;
    
    printf("\n=== Error Logging Summary ===\n");
    printf("Total errors: %u\n", ctx->stats.total_errors);
    printf("Unresolved errors: %u\n", ctx->stats.unresolved_errors);
    printf("Resolved errors: %u\n", ctx->stats.resolved_errors);
    printf("First error: %s\n", error_format_timestamp(ctx->stats.first_error_time, (char[64]){0}, 64));
    printf("Last error: %s\n", error_format_timestamp(ctx->stats.last_error_time, (char[64]){0}, 64));
    
    printf("\nErrors by level:\n");
    for (int i = 0; i < ERROR_LEVEL_COUNT; i++) {
        if (ctx->stats.errors_by_level[i] > 0) {
            printf("  %s: %u\n", error_get_level_string((ErrorLevel)i), ctx->stats.errors_by_level[i]);
        }
    }
    
    printf("\nErrors by category:\n");
    for (int i = 0; i < ERROR_CATEGORY_COUNT; i++) {
        if (ctx->stats.errors_by_category[i] > 0) {
            printf("  %s: %u\n", error_get_category_string((ErrorCategory)i), ctx->stats.errors_by_category[i]);
        }
    }
    
    printf("============================\n\n");
    return 0;
}

int error_export_to_file(const ErrorLoggingContext* ctx, const char* export_path) {
    if (!ctx || !export_path) return -1;
    
    FILE* export_file = fopen(export_path, "w");
    if (!export_file) {
        return -1;
    }
    
    fprintf(export_file, "Error Log Export\n");
    fprintf(export_file, "Generated: %s\n", error_format_timestamp(error_get_timestamp(), (char[64]){0}, 64}));
    fprintf(export_file, "Total errors: %u\n\n", ctx->stats.total_errors);
    
    // Export all errors
    for (uint32_t i = 0; i < ctx->count; i++) {
        uint32_t index = (ctx->head + i) % ctx->capacity;
        ErrorEntry* entry = &ctx->entries[index];
        
        fprintf(export_file, "[%s] %s | %s | %u | %s | %s:%d | %s | Thread:%u | %s\n",
                error_format_timestamp(entry->timestamp, (char[64]){0}, 64),
                error_get_level_string(entry->level),
                error_get_category_string(entry->category),
                entry->error_code,
                entry->message,
                entry->file_path,
                entry->line_number,
                entry->function_name,
                entry->thread_id,
                entry->resolved ? "RESOLVED" : "UNRESOLVED");
        
        if (entry->resolved) {
            fprintf(export_file, "  Resolution: %s at %s\n", 
                    entry->resolution_notes,
                    error_format_timestamp(entry->resolved_timestamp, (char[64]){0}, 64}));
        }
    }
    
    fclose(export_file);
    printf("Error log exported to: %s\n", export_path);
    return 0;
}

int error_clear_all_errors(ErrorLoggingContext* ctx) {
    if (!ctx || !ctx->entries) return -1;
    
    // Clear all entries
    memset(ctx->entries, 0, ctx->capacity * sizeof(ErrorEntry));
    ctx->count = 0;
    ctx->head = 0;
    ctx->tail = 0;
    
    // Reset statistics
    memset(&ctx->stats, 0, sizeof(ErrorStats));
    ctx->stats.first_error_time = error_get_timestamp();
    ctx->stats.last_error_time = ctx->stats.first_error_time;
    
    printf("All errors cleared from logging system\n");
    return 0;
}

// Configuration functions
int error_set_min_log_level(ErrorLoggingContext* ctx, ErrorLevel level) {
    if (!ctx || !error_is_valid_error_level(level)) return -1;
    
    ctx->min_log_level = level;
    printf("Minimum log level set to: %s\n", error_get_level_string(level));
    return 0;
}

int error_enable_file_logging(ErrorLoggingContext* ctx, bool enabled) {
    if (!ctx) return -1;
    
    ctx->file_logging_enabled = enabled;
    printf("File logging %s\n", enabled ? "enabled" : "disabled");
    return 0;
}

int error_enable_console_logging(ErrorLoggingContext* ctx, bool enabled) {
    if (!ctx) return -1;
    
    ctx->console_logging_enabled = enabled;
    printf("Console logging %s\n", enabled ? "enabled" : "disabled");
    return 0;
}

int error_set_auto_rotate(ErrorLoggingContext* ctx, bool enabled, size_t max_size) {
    if (!ctx) return -1;
    
    ctx->auto_rotate_logs = enabled;
    ctx->max_log_file_size = max_size;
    printf("Auto-rotate logs %s (max size: %zu bytes)\n", enabled ? "enabled" : "disabled", max_size);
    return 0;
}

// Validation functions
bool error_validate_context(const ErrorLoggingContext* ctx) {
    if (!ctx) return false;
    
    if (!ctx->entries || ctx->capacity == 0) return false;
    if (ctx->count > ctx->capacity) return false;
    if (!error_is_valid_error_level(ctx->min_log_level)) return false;
    
    return true;
}

bool error_is_valid_error_level(ErrorLevel level) {
    return level >= 0 && level < ERROR_LEVEL_COUNT;
}

bool error_is_valid_category(ErrorCategory category) {
    return category >= 0 && category < ERROR_CATEGORY_COUNT;
}

// Utility functions
const char* error_get_level_string(ErrorLevel level) {
    switch (level) {
        case ERROR_LEVEL_DEBUG: return "DEBUG";
        case ERROR_LEVEL_INFO: return "INFO";
        case ERROR_LEVEL_WARNING: return "WARNING";
        case ERROR_LEVEL_ERROR: return "ERROR";
        case ERROR_LEVEL_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

const char* error_get_category_string(ErrorCategory category) {
    switch (category) {
        case ERROR_CATEGORY_RENDERING: return "RENDERING";
        case ERROR_CATEGORY_SHADER: return "SHADER";
        case ERROR_CATEGORY_TEXTURE: return "TEXTURE";
        case ERROR_CATEGORY_MESH: return "MESH";
        case ERROR_CATEGORY_MEMORY: return "MEMORY";
        case ERROR_CATEGORY_GPU: return "GPU";
        case ERROR_CATEGORY_AUDIO: return "AUDIO";
        case ERROR_CATEGORY_INPUT: return "INPUT";
        case ERROR_CATEGORY_NETWORK: return "NETWORK";
        case ERROR_CATEGORY_FILESYSTEM: return "FILESYSTEM";
        case ERROR_CATEGORY_VALIDATION: return "VALIDATION";
        default: return "UNKNOWN";
    }
}

uint64_t error_get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

const char* error_format_timestamp(uint64_t timestamp, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return "";
    
    time_t raw_time = timestamp / 1000000;
    int milliseconds = timestamp % 1000000;
    
    struct tm* timeinfo = localtime(&raw_time);
    if (timeinfo) {
        snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, milliseconds);
    } else {
        strncpy(buffer, "Invalid timestamp", buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }
    
    return buffer;
}

// Internal helper functions
static uint32_t get_thread_id(void) {
    return (uint32_t)pthread_self();
}

static void write_error_to_file(FILE* file, const ErrorEntry* entry) {
    if (!file || !entry) return;
    
    char timestamp[64];
    error_format_timestamp(entry->timestamp, timestamp, sizeof(timestamp));
    
    fprintf(file, "[%s] %s | %s | %u | %s | %s:%d | %s | Thread:%u | %s\n",
            timestamp,
            error_get_level_string(entry->level),
            error_get_category_string(entry->category),
            entry->error_code,
            entry->message,
            entry->file_path,
            entry->line_number,
            entry->function_name,
            entry->thread_id,
            entry->resolved ? "RESOLVED" : "UNRESOLVED");
    
    if (entry->resolved) {
        fprintf(file, "  Resolution: %s at %s\n", 
                entry->resolution_notes,
                error_format_timestamp(entry->resolved_timestamp, timestamp, sizeof(timestamp)));
    }
    
    fflush(file);
}

static void write_error_to_console(const ErrorEntry* entry) {
    if (!entry) return;
    
    char timestamp[64];
    error_format_timestamp(entry->timestamp, timestamp, sizeof(timestamp));
    
    // Use color codes for different levels
    const char* color_code = "";
    switch (entry->level) {
        case ERROR_LEVEL_DEBUG: color_code = "\033[0;36m"; break;    // Cyan
        case ERROR_LEVEL_INFO: color_code = "\033[0;32m"; break;     // Green
        case ERROR_LEVEL_WARNING: color_code = "\033[0;33m"; break;  // Yellow
        case ERROR_LEVEL_ERROR: color_code = "\033[0;31m"; break;     // Red
        case ERROR_LEVEL_CRITICAL: color_code = "\033[1;31m"; break;   // Bold Red
        default: color_code = "\033[0m"; break;                   // Reset
    }
    
    printf("%s[%s] %s | %s | %u | %s\033[0m\n",
           color_code,
           timestamp,
           error_get_level_string(entry->level),
           error_get_category_string(entry->category),
           entry->error_code,
           entry->message);
}

static void update_statistics(ErrorLoggingContext* ctx, const ErrorEntry* entry) {
    if (!ctx || !entry) return;
    
    ctx->stats.total_errors++;
    ctx->stats.errors_by_level[entry->level]++;
    ctx->stats.errors_by_category[entry->category]++;
    
    if (!entry->resolved) {
        ctx->stats.unresolved_errors++;
    } else {
        ctx->stats.resolved_errors++;
    }
    
    ctx->stats.last_error_time = entry->timestamp;
    
    if (ctx->stats.total_errors == 1) {
        ctx->stats.first_error_time = entry->timestamp;
    }
}

static bool should_log_error(const ErrorLoggingContext* ctx, ErrorLevel level) {
    if (!ctx) return false;
    
    return level >= ctx->min_log_level;
}

static void rotate_log_file_if_needed(ErrorLoggingContext* ctx) {
    if (!ctx || !ctx->log_file || !ctx->auto_rotate_logs) return;
    
    // Get current file size
    fseek(ctx->log_file, 0, SEEK_END);
    long current_size = ftell(ctx->log_file);
    
    if (current_size >= (long)ctx->max_log_file_size) {
        // Close current file
        fclose(ctx->log_file);
        
        // Generate new filename with timestamp
        char new_filename[512];
        char timestamp[64];
        error_format_timestamp(error_get_timestamp(), timestamp, sizeof(timestamp));
        snprintf(new_filename, sizeof(new_filename), "%s.%s", ctx->log_file_path, timestamp);
        
        // Open new file
        ctx->log_file = fopen(new_filename, "w");
        if (ctx->log_file) {
            fprintf(ctx->log_file, "# Error Logging System - Rotated at %s\n", timestamp);
            fprintf(ctx->log_file, "# Previous file: %s\n", ctx->log_file_path);
            fprintf(ctx->log_file, "# Format: Timestamp | Level | Category | Code | Message | File:Line | Function | Thread | Resolved\n");
            fflush(ctx->log_file);
            
            printf("Log file rotated to: %s\n", new_filename);
        }
    }
}
