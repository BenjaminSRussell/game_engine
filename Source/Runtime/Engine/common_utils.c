// Common Utilities - Consolidated Code Duplication
// Shared functions for collision, memory, and logging systems

#include "common_utils.h"
#include "logger.h"
#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Common memory allocation wrapper with error checking
void* common_malloc(size_t size, const char* context) {
    if (size == 0) {
        LOG_WARN("Attempted to allocate 0 bytes in %s", context);
        return NULL;
    }
    
    void* ptr = malloc(size);
    if (!ptr) {
        LOG_ERROR("Failed to allocate %zu bytes in %s", size, context);
        return NULL;
    }
    
    // Clear memory for security
    memset(ptr, 0, size);
    
    LOG_DEBUG("Allocated %zu bytes in %s", size, context);
    return ptr;
}

// Common memory reallocation wrapper
void* common_realloc(void* ptr, size_t new_size, const char* context) {
    if (new_size == 0) {
        if (ptr) {
            common_free(ptr, context);
        }
        return NULL;
    }
    
    void* new_ptr = realloc(ptr, new_size);
    if (!new_ptr && new_size > 0) {
        LOG_ERROR("Failed to reallocate to %zu bytes in %s", new_size, context);
        return NULL;
    }
    
    LOG_DEBUG("Reallocated to %zu bytes in %s", new_size, context);
    return new_ptr;
}

// Common memory free wrapper
void common_free(void* ptr, const char* context) {
    if (ptr) {
        free(ptr);
        LOG_DEBUG("Freed memory in %s", context);
    }
}

// Common string duplication
char* common_strdup(const char* str, const char* context) {
    if (!str) {
        LOG_WARN("Attempted to duplicate NULL string in %s", context);
        return NULL;
    }
    
    size_t len = strlen(str);
    char* dup = common_malloc(len + 1, context);
    if (dup) {
        strcpy(dup, str);
    }
    
    return dup;
}

// Common bounds checking utility
bool common_bounds_check(size_t index, size_t max_index, const char* context) {
    if (index >= max_index) {
        LOG_ERROR("Index %zu out of bounds (max: %zu) in %s", index, max_index, context);
        return false;
    }
    return true;
}

// Common null pointer check
bool common_null_check(const void* ptr, const char* context) {
    if (!ptr) {
        LOG_ERROR("Null pointer encountered in %s", context);
        return false;
    }
    return true;
}

// Common safe string copy
bool common_strcpy_safe(char* dest, size_t dest_size, const char* src, const char* context) {
    if (!dest || !src || dest_size == 0) {
        LOG_ERROR("Invalid parameters for safe string copy in %s", context);
        return false;
    }
    
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        LOG_ERROR("String too long for destination buffer in %s (need %zu, have %zu)", 
                  context, src_len + 1, dest_size);
        return false;
    }
    
    strcpy(dest, src);
    return true;
}

// Common safe string concatenation
bool common_strcat_safe(char* dest, size_t dest_size, const char* src, const char* context) {
    if (!dest || !src || dest_size == 0) {
        LOG_ERROR("Invalid parameters for safe string concatenation in %s", context);
        return false;
    }
    
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    
    if (dest_len + src_len >= dest_size) {
        LOG_ERROR("String concatenation would overflow buffer in %s", context);
        return false;
    }
    
    strcat(dest, src);
    return true;
}

// Common min/max functions
int common_min_int(int a, int b) {
    return (a < b) ? a : b;
}

int common_max_int(int a, int b) {
    return (a > b) ? a : b;
}

float common_min_float(float a, float b) {
    return (a < b) ? a : b;
}

float common_max_float(float a, float b) {
    return (a > b) ? a : b;
}

size_t common_min_size(size_t a, size_t b) {
    return (a < b) ? a : b;
}

size_t common_max_size(size_t a, size_t b) {
    return (a > b) ? a : b;
}

// Common clamp functions
int common_clamp_int(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float common_clamp_float(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

size_t common_clamp_size(size_t value, size_t min, size_t max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Common linear interpolation
float common_lerp(float a, float b, float t) {
    return a + (b - a) * common_clamp_float(t, 0.0f, 1.0f);
}

// Common absolute value
int common_abs_int(int value) {
    return (value < 0) ? -value : value;
}

float common_abs_float(float value) {
    return (value < 0.0f) ? -value : value;
}

// Common power-of-two check
bool common_is_power_of_two(size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

// Common next power of two
size_t common_next_power_of_two(size_t value) {
    if (value == 0) return 1;
    
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32; // For 64-bit systems
    value++;
    
    return value;
}

// Common alignment utilities
size_t common_align_up(size_t value, size_t alignment) {
    if (alignment == 0) return value;
    return (value + alignment - 1) & ~(alignment - 1);
}

size_t common_align_down(size_t value, size_t alignment) {
    if (alignment == 0) return value;
    return value & ~(alignment - 1);
}

bool common_is_aligned(size_t value, size_t alignment) {
    if (alignment == 0) return true;
    return (value & (alignment - 1)) == 0;
}

// Common hash function for strings (djb2 algorithm)
size_t common_hash_string(const char* str) {
    if (!str) return 0;
    
    size_t hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    
    return hash;
}

// Common hash function for integers
size_t common_hash_int(int value) {
    return (size_t)value;
}

// Common hash function for pointers
size_t common_hash_ptr(const void* ptr) {
    return (size_t)ptr;
}

// Common timing utilities
#include <time.h>

static u64 common_start_time = 0;

void common_timer_init(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    common_start_time = (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
}

u64 common_timer_get_nanos(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    u64 current = (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
    return current - common_start_time;
}

f64 common_timer_get_seconds(void) {
    return (f64)common_timer_get_nanos() / 1000000000.0;
}

f64 common_timer_get_millis(void) {
    return (f64)common_timer_get_nanos() / 1000000.0;
}

// Common performance counter
static u64 common_perf_counter = 0;

u64 common_perf_counter_increment(void) {
    return ++common_perf_counter;
}

u64 common_perf_counter_get(void) {
    return common_perf_counter;
}

void common_perf_counter_reset(void) {
    common_perf_counter = 0;
}

// Common error handling
void common_error_handler(const char* function, const char* file, int line, const char* message) {
    LOG_ERROR("Error in %s (%s:%d): %s", function, file, line, message);
}

void common_warning_handler(const char* function, const char* file, int line, const char* message) {
    LOG_WARN("Warning in %s (%s:%d): %s", function, file, line, message);
}

void common_info_handler(const char* function, const char* file, int line, const char* message) {
    LOG_INFO("Info in %s (%s:%d): %s", function, file, line, message);
}

// Common assertion
void common_assert(bool condition, const char* expression, const char* function, const char* file, int line) {
    if (!condition) {
        LOG_ERROR("Assertion failed: %s in %s (%s:%d)", expression, function, file, line);
        // In debug builds, we might want to break here
        #ifdef DEBUG
        abort();
        #endif
    }
}

// Common validation utilities
bool common_validate_size(size_t size, size_t max_size, const char* context) {
    if (size > max_size) {
        LOG_ERROR("Size %zu exceeds maximum %zu in %s", size, max_size, context);
        return false;
    }
    return true;
}

bool common_validate_range(int value, int min, int max, const char* context) {
    if (value < min || value > max) {
        LOG_ERROR("Value %d out of range [%d, %d] in %s", value, min, max, context);
        return false;
    }
    return true;
}

bool common_validate_float_range(float value, float min, float max, const char* context) {
    if (value < min || value > max) {
        LOG_ERROR("Value %.3f out of range [%.3f, %.3f] in %s", value, min, max, context);
        return false;
    }
    return true;
}

// Common string utilities
bool common_string_equals(const char* a, const char* b) {
    if (!a || !b) return a == b;
    return strcmp(a, b) == 0;
}

bool common_string_equals_ignore_case(const char* a, const char* b) {
    if (!a || !b) return a == b;
    return strcasecmp(a, b) == 0;
}

bool common_string_starts_with(const char* str, const char* prefix) {
    if (!str || !prefix) return false;
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

bool common_string_ends_with(const char* str, const char* suffix) {
    if (!str || !suffix) return false;
    
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    
    if (suffix_len > str_len) return false;
    
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

bool common_string_contains(const char* str, const char* substr) {
    if (!str || !substr) return false;
    return strstr(str, substr) != NULL;
}

// Common file utilities
bool common_file_exists(const char* filepath) {
    if (!filepath) return false;
    
    FILE* file = fopen(filepath, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

size_t common_file_size(const char* filepath) {
    if (!filepath) return 0;
    
    FILE* file = fopen(filepath, "rb");
    if (!file) return 0;
    
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fclose(file);
    
    return size;
}

bool common_file_read_all(const char* filepath, void** buffer, size_t* size) {
    if (!filepath || !buffer || !size) return false;
    
    *size = common_file_size(filepath);
    if (*size == 0) {
        *buffer = NULL;
        return false;
    }
    
    *buffer = common_malloc(*size, "file_read_all");
    if (!*buffer) return false;
    
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        common_free(*buffer, "file_read_all");
        *buffer = NULL;
        *size = 0;
        return false;
    }
    
    size_t read_size = fread(*buffer, 1, *size, file);
    fclose(file);
    
    if (read_size != *size) {
        common_free(*buffer, "file_read_all");
        *buffer = NULL;
        *size = 0;
        return false;
    }
    
    return true;
}
