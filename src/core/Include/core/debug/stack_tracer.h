#ifndef CORE_DEBUG_STACK_TRACER_H
#define CORE_DEBUG_STACK_TRACER_H

#include <core/types.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_STACK_FRAMES 64
#define MAX_SYMBOL_LENGTH 512
#define MAX_MODULE_PATH 260

typedef struct {
    void* address;
    char symbol[MAX_SYMBOL_LENGTH];
    char module[MAX_MODULE_PATH];
    size_t offset;
    bool resolved;
} StackFrame;

typedef struct {
    StackFrame frames[MAX_STACK_FRAMES];
    size_t frame_count;
    u64 hash;
    f64 timestamp;
} StackTrace;

typedef struct {
    char name[MAX_MODULE_PATH];
    void* base_address;
    size_t size;
} ModuleInfo;

// Core functions
size_t stack_capture(void** frames, size_t max_frames, bool use_frame_pointers);

// Trace management
StackTrace* stack_trace_create(void** frames, size_t frame_count);
void stack_trace_format(const StackTrace* trace, char* buffer, size_t buffer_size, bool verbose);
void stack_trace_minimize(const StackTrace* trace, char* buffer, size_t buffer_size);
bool stack_trace_equals(const StackTrace* a, const StackTrace* b);
void stack_trace_destroy(StackTrace* trace);

// Formatting info
const char* stack_trace_get_module_name(void* address);

// Initialization
bool stack_tracer_init(void);
void stack_tracer_shutdown(void);

// Utils
size_t stack_tracer_get_module_count(void);
const ModuleInfo* stack_tracer_get_modules(void);
bool stack_tracer_are_symbols_loaded(void);

#ifdef __cplusplus
}
#endif

#endif // CORE_DEBUG_STACK_TRACER_H
