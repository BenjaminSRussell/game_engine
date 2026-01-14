// Common Utilities Header - Consolidated Code Duplication
// Shared functions for collision, memory, and logging systems

#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include "core/types.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Memory Management Utilities
void* common_malloc(size_t size, const char* context);
void* common_realloc(void* ptr, size_t new_size, const char* context);
void common_free(void* ptr, const char* context);
char* common_strdup(const char* str, const char* context);

// Validation Utilities
bool common_bounds_check(size_t index, size_t max_index, const char* context);
bool common_null_check(const void* ptr, const char* context);
bool common_validate_size(size_t size, size_t max_size, const char* context);
bool common_validate_range(int value, int min, int max, const char* context);
bool common_validate_float_range(float value, float min, float max, const char* context);

// String Utilities
bool common_strcpy_safe(char* dest, size_t dest_size, const char* src, const char* context);
bool common_strcat_safe(char* dest, size_t dest_size, const char* src, const char* context);
bool common_string_equals(const char* a, const char* b);
bool common_string_equals_ignore_case(const char* a, const char* b);
bool common_string_starts_with(const char* str, const char* prefix);
bool common_string_ends_with(const char* str, const char* suffix);
bool common_string_contains(const char* str, const char* substr);

// Math Utilities
int common_min_int(int a, int b);
int common_max_int(int a, int b);
float common_min_float(float a, float b);
float common_max_float(float a, float b);
size_t common_min_size(size_t a, size_t b);
size_t common_max_size(size_t a, size_t b);

int common_clamp_int(int value, int min, int max);
float common_clamp_float(float value, float min, float max);
size_t common_clamp_size(size_t value, size_t min, size_t max);

float common_lerp(float a, float b, float t);
int common_abs_int(int value);
float common_abs_float(float value);

// Power-of-two and Alignment Utilities
bool common_is_power_of_two(size_t value);
size_t common_next_power_of_two(size_t value);
size_t common_align_up(size_t value, size_t alignment);
size_t common_align_down(size_t value, size_t alignment);
bool common_is_aligned(size_t value, size_t alignment);

// Hash Utilities
size_t common_hash_string(const char* str);
size_t common_hash_int(int value);
size_t common_hash_ptr(const void* ptr);

// Timing Utilities
void common_timer_init(void);
u64 common_timer_get_nanos(void);
f64 common_timer_get_seconds(void);
f64 common_timer_get_millis(void);

// Performance Counter
u64 common_perf_counter_increment(void);
u64 common_perf_counter_get(void);
void common_perf_counter_reset(void);

// Error Handling Utilities
void common_error_handler(const char* function, const char* file, int line, const char* message);
void common_warning_handler(const char* function, const char* file, int line, const char* message);
void common_info_handler(const char* function, const char* file, int line, const char* message);

// Assertion Utility
void common_assert(bool condition, const char* expression, const char* function, const char* file, int line);

// File Utilities
bool common_file_exists(const char* filepath);
size_t common_file_size(const char* filepath);
bool common_file_read_all(const char* filepath, void** buffer, size_t* size);

// Convenience macros
#define COMMON_MALLOC(size) common_malloc(size, __func__)
#define COMMON_REALLOC(ptr, size) common_realloc(ptr, size, __func__)
#define COMMON_FREE(ptr) common_free(ptr, __func__)
#define COMMON_STRDUP(str) common_strdup(str, __func__)

#define COMMON_BOUNDS_CHECK(index, max) common_bounds_check(index, max, __func__)
#define COMMON_NULL_CHECK(ptr) common_null_check(ptr, __func__)
#define COMMON_VALIDATE_SIZE(size, max) common_validate_size(size, max, __func__)
#define COMMON_VALIDATE_RANGE(value, min, max) common_validate_range(value, min, max, __func__)
#define COMMON_VALIDATE_FLOAT_RANGE(value, min, max) common_validate_float_range(value, min, max, __func__)

#define COMMON_STRCPY_SAFE(dest, dest_size, src) common_strcpy_safe(dest, dest_size, src, __func__)
#define COMMON_STRCAT_SAFE(dest, dest_size, src) common_strcat_safe(dest, dest_size, src, __func__)

#define COMMON_ERROR(message) common_error_handler(__func__, __FILE__, __LINE__, message)
#define COMMON_WARNING(message) common_warning_handler(__func__, __FILE__, __LINE__, message)
#define COMMON_INFO(message) common_info_handler(__func__, __FILE__, __LINE__, message)

#define COMMON_ASSERT(condition) common_assert(condition, #condition, __func__, __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif // COMMON_UTILS_H
