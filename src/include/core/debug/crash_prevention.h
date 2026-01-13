// Crash Prevention System Header
// Prevents common crashes before they happen

#ifndef CRASH_PREVENTION_H
#define CRASH_PREVENTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <signal.h>

// Crash prevention state
typedef struct {
    bool initialized;
    size_t allocations_count;
    size_t total_allocated;
    size_t crashes_prevented;
    size_t crashes_detected;
} CrashPreventionState;

// Crash prevention statistics
typedef struct {
    size_t allocations_count;
    size_t total_allocated;
    size_t crashes_prevented;
    size_t crashes_detected;
} CrashPreventionStats;

// Function declarations
bool crash_prevention_init(void);
void crash_prevention_signal_handler(int signal, siginfo_t* info, void* context);
bool crash_prevention_handle_crash(int signal, siginfo_t* info);
bool crash_prevention_handle_segfault(siginfo_t* info);
bool crash_prevention_handle_fpe(siginfo_t* info);
bool crash_prevention_handle_abort(siginfo_t* info);

// Validation functions
bool crash_prevention_validate_pointer(const void* ptr, const char* operation);
bool crash_prevention_validate_array_access(size_t index, size_t size, const char* operation);
bool crash_prevention_validate_string(const char* str, const char* operation);

// Safe memory operations
void* crash_prevention_safe_malloc(size_t size, const char* operation);
void crash_prevention_safe_free(void* ptr, const char* operation);

// Statistics and cleanup
void crash_prevention_get_stats(CrashPreventionStats* stats);
void crash_prevention_cleanup(void);

// Convenience macros for crash prevention
#define SAFE_FREE(ptr) crash_prevention_safe_free(ptr, #ptr)
#define SAFE_MALLOC(size) crash_prevention_safe_malloc(size, #size)
#define VALIDATE_POINTER(ptr) crash_prevention_validate_pointer(ptr, #ptr)
#define VALIDATE_ARRAY_ACCESS(index, size) crash_prevention_validate_array_access(index, size, #index)
#define VALIDATE_STRING(str) crash_prevention_validate_string(str, #str)

// Safe array access macro
#define SAFE_ARRAY_ACCESS(arr, index, size) \
    (VALIDATE_ARRAY_ACCESS(index, size) ? (arr)[index] : (arr)[0])

// Safe string operations
#define SAFE_STRLEN(str) (VALIDATE_STRING(str) ? strlen(str) : 0)
#define SAFE_STRCPY(dest, src) (VALIDATE_STRING(src) ? strcpy(dest, src) : NULL)

#endif // CRASH_PREVENTION_H
