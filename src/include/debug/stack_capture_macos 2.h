#ifndef STACK_CAPTURE_MACOS_H
#define STACK_CAPTURE_MACOS_H

#include <core/types.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct Vec3 Vec3;

// Stack frame information
typedef struct {
    void* instruction_pointer;
    void* frame_pointer;
    void* stack_pointer;
    char function_name[256];
    char file_name[256];
    u32 line_number;
    char module_name[256];
    u64 module_base_address;
} StackFrame;

// Stack trace information
typedef struct {
    StackFrame* frames;
    u32 frame_count;
    u32 max_frames;
    u64 capture_time_ms;
    u32 thread_id;
    char thread_name[64];
} StackTrace;

// Memory allocation tracking
typedef struct {
    void* address;
    u64 size;
    u64 allocation_time_ms;
    u32 thread_id;
    StackTrace allocation_stack;
    char allocation_tag[64];
    bool is_freed;
    u64 free_time_ms;
    StackTrace free_stack;
} MemoryAllocation;

// Memory leak detection
typedef struct {
    MemoryAllocation* allocations;
    u32 allocation_count;
    u32 allocation_capacity;
    u64 total_allocated_bytes;
    u64 peak_allocated_bytes;
    u32 allocation_count_peak;
    
    // Leak detection settings
    bool enable_tracking;
    bool enable_stack_capture;
    u32 max_stack_frames;
    u64 leak_threshold_bytes;
    
    // Statistics
    u64 total_allocations;
    u64 total_frees;
    u64 failed_allocations;
} MemoryLeakDetector;

// Stack capture system
typedef struct {
    MemoryLeakDetector leak_detector;
    
    // Stack trace cache
    StackTrace* stack_cache;
    u32 stack_cache_count;
    u32 stack_cache_capacity;
    
    // Symbol resolution
    bool enable_symbol_resolution;
    bool enable_demangling;
    
    // Performance tracking
    u64 total_capture_time_ms;
    u64 capture_count;
    u64 failed_captures;
} StackCaptureSystem;

// MARK: - Stack Capture System Management

bool stack_capture_init(StackCaptureSystem* system, u32 max_allocations, u32 max_stack_frames);
void stack_capture_shutdown(StackCaptureSystem* system);

// MARK: - Stack Trace Capture

bool stack_capture_current_thread(StackTrace* trace, u32 max_frames);
bool stack_capture_thread(StackTrace* trace, u32 thread_id, u32 max_frames);
bool stack_capture_from_context(StackTrace* trace, void* context, u32 max_frames);

void stack_trace_clear(StackTrace* trace);
void stack_trace_print(const StackTrace* trace);
void stack_trace_print_to_file(const StackTrace* trace, const char* filename);

// MARK: - Memory Leak Detection

bool memory_leak_detector_start_tracking(MemoryLeakDetector* detector);
void memory_leak_detector_stop_tracking(MemoryLeakDetector* detector);

bool memory_leak_detector_record_allocation(MemoryLeakDetector* detector, void* address, u64 size, 
                                          const char* tag);
bool memory_leak_detector_record_free(MemoryLeakDetector* detector, void* address);

MemoryAllocation* memory_leak_detector_find_allocation(MemoryLeakDetector* detector, void* address);
void memory_leak_detector_report_leaks(MemoryLeakDetector* detector);
void memory_leak_detector_print_statistics(MemoryLeakDetector* detector);

// MARK: - Symbol Resolution

bool stack_capture_resolve_symbols(StackTrace* trace);
bool stack_capture_resolve_symbol(void* address, char* function_name, size_t func_size, 
                                 char* file_name, size_t file_size, u32* line_number);
bool stack_capture_demangle_symbol(const char* mangled_name, char* demangled_name, size_t size);

// MARK: - Utility Functions

void stack_capture_set_leak_threshold(StackCaptureSystem* system, u64 threshold_bytes);
void stack_capture_enable_symbol_resolution(StackCaptureSystem* system, bool enabled);
void stack_capture_enable_demangling(StackCaptureSystem* system, bool enabled);

void stack_capture_get_statistics(StackCaptureSystem* system, u64* total_allocations, 
                                 u64* current_allocations, u64* peak_allocations, 
                                 u64* total_leaks);

bool stack_capture_export_leaks_to_json(StackCaptureSystem* system, const char* filename);
bool stack_capture_export_allocations_to_csv(StackCaptureSystem* system, const char* filename);

// MARK: - macOS-specific implementations

bool macos_capture_stack_trace(StackTrace* trace, u32 max_frames);
bool macos_resolve_stack_symbols(StackTrace* trace);
bool macos_get_thread_stack_info(u32 thread_id, void** stack_base, u64* stack_size);
bool macos_read_memory_at_address(void* address, void* buffer, size_t size);

// MARK: - Debug Macros

#define STACK_CAPTURE_ENABLED 1

#if STACK_CAPTURE_ENABLED
    #define STACK_CAPTURE_TRACE(trace, max_frames) stack_capture_current_thread(trace, max_frames)
    #define MEMORY_LEAK_TRACK_ALLOC(address, size, tag) \
        if (g_stack_system) memory_leak_detector_record_allocation(&g_stack_system->leak_detector, address, size, tag)
    #define MEMORY_LEAK_TRACK_FREE(address) \
        if (g_stack_system) memory_leak_detector_record_free(&g_stack_system->leak_detector, address)
#else
    #define STACK_CAPTURE_TRACE(trace, max_frames) false
    #define MEMORY_LEAK_TRACK_ALLOC(address, size, tag)
    #define MEMORY_LEAK_TRACK_FREE(address)
#endif

// Global stack capture system instance
extern StackCaptureSystem* g_stack_system;

#endif // STACK_CAPTURE_MACOS_H
