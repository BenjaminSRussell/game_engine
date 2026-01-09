#ifndef CORE_DEBUG_PROFILER_HOOKS_H
#define CORE_DEBUG_PROFILER_HOOKS_H

#include <core/types.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCOPE_NAME_LENGTH 64

typedef struct {
    char name[SCOPE_NAME_LENGTH];
    f64 start_time;
    f64 end_time;
    f64 duration;
    u32 color;
    bool active;
    u64 call_count;
    f64 total_time;
    f64 min_time;
    f64 max_time;
} ProfileScope;

// Thread registration
int profiler_register_thread(const char* thread_name);

// Scope control
int internal_profiler_scope_enter(const char* name);
void internal_profiler_scope_exit(int scope_id);
void profiler_frame_marker(void);

// Profiler control
void profiler_enable(bool enabled);
bool profiler_is_enabled(void);

// Statistics
void profiler_get_thread_stats(int thread_id, ProfileScope* scopes, size_t* scope_count);
void profiler_get_global_stats(ProfileScope* scopes, size_t* scope_count, size_t max_scopes);
void profiler_reset_stats(void);

// Information
const char* profiler_get_thread_name(int thread_id);
u32 profiler_get_thread_color(int thread_id);
size_t profiler_get_thread_count(void);
f64 profiler_measure_overhead(int iterations);
f64 profiler_get_overhead_calibration(void);
u64 profiler_get_global_frame_counter(void);

// Lifecycle
void profiler_init(void);
void profiler_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // CORE_DEBUG_PROFILER_HOOKS_H
