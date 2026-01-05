#include "core/debug/profiler_hooks.h"
#include "core/core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ✅ COMPLETED: Profiler Hooks Implementation - AGENT_CORE_1
// Macro hooks for measuring scope duration

#define MAX_THREADS 64
#define MAX_SCOPES_PER_THREAD 1024
#define SCOPE_NAME_LENGTH 64
#define CALIBRATION_SAMPLES 1000

// Thread-local storage for profiler data
#ifdef _WIN32
__declspec(thread) static int g_thread_id = -1;
#else
__thread static int g_thread_id = -1;
#endif

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

typedef struct {
    ProfileScope scopes[MAX_SCOPES_PER_THREAD];
    size_t scope_count;
    size_t current_depth;
    f64 frame_start_time;
    u64 frame_number;
    char thread_name[32];
    u32 thread_color;
} ThreadProfileData;

static ThreadProfileData g_thread_data[MAX_THREADS];
static size_t g_thread_count = 0;
static bool g_profiler_enabled = false;
static f64 g_overhead_calibration = 0.0;
static u64 g_global_frame_counter = 0;

// ✅ COMPLETED: High-resolution time
static f64 get_time(void) {
#ifdef _WIN32
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (f64)count.QuadPart / (f64)freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (f64)ts.tv_sec + (f64)ts.tv_nsec * 1e-9;
#endif
}

// ✅ COMPLETED: Visual color hashing
static u32 hash_color(const char* name) {
    u32 hash = 0;
    
    for (size_t i = 0; name[i]; i++) {
        hash = hash * 31 + name[i];
    }
    
    // Convert to RGB with good saturation
    u32 hue = hash % 360;
    f32 h = hue / 60.0f;
    f32 c = 0.8f; // Saturation
    f32 x = c * (1.0f - fabsf(fmodf(h, 2.0f) - 1.0f));
    f32 m = 0.2f; // Lightness
    
    f32 r, g, b;
    if (h < 1.0f) { r = c; g = x; b = 0; }
    else if (h < 2.0f) { r = x; g = c; b = 0; }
    else if (h < 3.0f) { r = 0; g = c; b = x; }
    else if (h < 4.0f) { r = 0; g = x; b = c; }
    else if (h < 5.0f) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }
    
    r += m; g += m; b += m;
    
    return ((u32)(r * 255) << 16) | ((u32)(g * 255) << 8) | ((u32)(b * 255));
}

// ✅ COMPLETED: Thread registration
int profiler_register_thread(const char* thread_name) {
    if (!thread_name) thread_name = "Unknown";
    
    // Find existing thread
    for (size_t i = 0; i < g_thread_count; i++) {
        if (strcmp(g_thread_data[i].thread_name, thread_name) == 0) {
            g_thread_id = (int)i;
            return g_thread_id;
        }
    }
    
    // Register new thread
    if (g_thread_count >= MAX_THREADS) return -1;
    
    size_t index = g_thread_count++;
    ThreadProfileData* data = &g_thread_data[index];
    
    memset(data, 0, sizeof(ThreadProfileData));
    strncpy(data->thread_name, thread_name, sizeof(data->thread_name) - 1);
    data->thread_color = hash_color(thread_name);
    data->frame_start_time = get_time();
    
    g_thread_id = (int)index;
    return g_thread_id;
}

// ✅ COMPLETED: Automatic calibration
static void calibrate_overhead(void) {
    f64 total_overhead = 0.0;
    
    // Measure overhead of scope operations
    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        f64 start = get_time();
        
        // Simulate scope operations
        volatile f64 dummy = 0.0;
        dummy += get_time();
        dummy += get_time();
        
        f64 end = get_time();
        total_overhead += (end - start);
    }
    
    g_overhead_calibration = total_overhead / CALIBRATION_SAMPLES;
}

// ✅ COMPLETED: Scope enter
int profiler_scope_enter(const char* name) {
    if (!g_profiler_enabled || g_thread_id < 0) return -1;
    
    ThreadProfileData* data = &g_thread_data[g_thread_id];
    
    if (data->scope_count >= MAX_SCOPES_PER_THREAD) return -1;
    
    // Find existing scope or create new one
    int scope_index = -1;
    for (size_t i = 0; i < data->scope_count; i++) {
        if (strcmp(data->scopes[i].name, name) == 0) {
            scope_index = (int)i;
            break;
        }
    }
    
    if (scope_index == -1) {
        scope_index = (int)data->scope_count++;
        ProfileScope* scope = &data->scopes[scope_index];
        memset(scope, 0, sizeof(ProfileScope));
        strncpy(scope->name, name, SCOPE_NAME_LENGTH - 1);
        scope->color = hash_color(name);
        scope->min_time = FLT_MAX;
    }
    
    ProfileScope* scope = &data->scopes[scope_index];
    scope->start_time = get_time();
    scope->active = true;
    scope->call_count++;
    
    data->current_depth++;
    return scope_index;
}

// ✅ COMPLETED: Scope exit
void profiler_scope_exit(int scope_id) {
    if (!g_profiler_enabled || g_thread_id < 0 || scope_id < 0) return;
    
    ThreadProfileData* data = &g_thread_data[g_thread_id];
    
    if (scope_id >= (int)data->scope_count) return;
    
    ProfileScope* scope = &data->scopes[scope_id];
    if (!scope->active) return;
    
    scope->end_time = get_time();
    scope->duration = scope->end_time - scope->start_time - g_overhead_calibration;
    
    // Update statistics
    scope->total_time += scope->duration;
    if (scope->duration < scope->min_time) scope->min_time = scope->duration;
    if (scope->duration > scope->max_time) scope->max_time = scope->duration;
    
    scope->active = false;
    
    if (data->current_depth > 0) data->current_depth--;
}

// ✅ COMPLETED: Frame marker injection
void profiler_frame_marker(void) {
    if (!g_profiler_enabled) return;
    
    g_global_frame_counter++;
    
    if (g_thread_id >= 0) {
        ThreadProfileData* data = &g_thread_data[g_thread_id];
        
        f64 now = get_time();
        f64 frame_duration = now - data->frame_start_time;
        data->frame_start_time = now;
        data->frame_number = g_global_frame_counter;
        
        // Reset active scopes for new frame
        for (size_t i = 0; i < data->scope_count; i++) {
            data->scopes[i].active = false;
        }
    }
}

// ✅ COMPLETED: Profiler control
void profiler_enable(bool enabled) {
    if (enabled && !g_profiler_enabled) {
        calibrate_overhead();
    }
    
    g_profiler_enabled = enabled;
}

bool profiler_is_enabled(void) {
    return g_profiler_enabled;
}

// ✅ COMPLETED: Statistics retrieval
void profiler_get_thread_stats(int thread_id, ProfileScope* scopes, size_t* scope_count) {
    if (thread_id < 0 || thread_id >= (int)g_thread_count || !scopes || !scope_count) {
        if (scope_count) *scope_count = 0;
        return;
    }
    
    ThreadProfileData* data = &g_thread_data[thread_id];
    *scope_count = data->scope_count;
    
    memcpy(scopes, data->scopes, data->scope_count * sizeof(ProfileScope));
}

void profiler_get_global_stats(ProfileScope* scopes, size_t* scope_count, size_t max_scopes) {
    if (!scopes || !scope_count) {
        if (scope_count) *scope_count = 0;
        return;
    }
    
    // Aggregate stats across all threads
    size_t total_scopes = 0;
    
    for (size_t i = 0; i < g_thread_count && total_scopes < max_scopes; i++) {
        ThreadProfileData* data = &g_thread_data[i];
        
        for (size_t j = 0; j < data->scope_count && total_scopes < max_scopes; j++) {
            // Check if scope already exists in global list
            bool found = false;
            for (size_t k = 0; k < total_scopes; k++) {
                if (strcmp(scopes[k].name, data->scopes[j].name) == 0) {
                    // Aggregate statistics
                    scopes[k].call_count += data->scopes[j].call_count;
                    scopes[k].total_time += data->scopes[j].total_time;
                    if (data->scopes[j].min_time < scopes[k].min_time) {
                        scopes[k].min_time = data->scopes[j].min_time;
                    }
                    if (data->scopes[j].max_time > scopes[k].max_time) {
                        scopes[k].max_time = data->scopes[j].max_time;
                    }
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                scopes[total_scopes] = data->scopes[j];
                total_scopes++;
            }
        }
    }
    
    *scope_count = total_scopes;
}

// ✅ COMPLETED: Thread information
const char* profiler_get_thread_name(int thread_id) {
    if (thread_id < 0 || thread_id >= (int)g_thread_count) return NULL;
    return g_thread_data[thread_id].thread_name;
}

u32 profiler_get_thread_color(int thread_id) {
    if (thread_id < 0 || thread_id >= (int)g_thread_count) return 0xFFFFFF;
    return g_thread_data[thread_id].thread_color;
}

size_t profiler_get_thread_count(void) {
    return g_thread_count;
}

// ✅ COMPLETED: Overhead testing
f64 profiler_measure_overhead(int iterations) {
    if (iterations <= 0) iterations = 10000;
    
    f64 start = get_time();
    
    for (int i = 0; i < iterations; i++) {
        int scope_id = profiler_scope_enter("TestScope");
        profiler_scope_exit(scope_id);
    }
    
    f64 end = get_time();
    f64 total_time = end - start;
    
    return total_time / (f64)iterations;
}

// ✅ COMPLETED: Reset statistics
void profiler_reset_stats(void) {
    for (size_t i = 0; i < g_thread_count; i++) {
        ThreadProfileData* data = &g_thread_data[i];
        
        for (size_t j = 0; j < data->scope_count; j++) {
            ProfileScope* scope = &data->scopes[j];
            scope->call_count = 0;
            scope->total_time = 0.0;
            scope->min_time = FLT_MAX;
            scope->max_time = 0.0;
            scope->active = false;
        }
        
        data->frame_number = 0;
        data->frame_start_time = get_time();
    }
    
    g_global_frame_counter = 0;
}

// ✅ COMPLETED: Initialization and cleanup
void profiler_init(void) {
    memset(g_thread_data, 0, sizeof(g_thread_data));
    g_thread_count = 0;
    g_profiler_enabled = false;
    g_overhead_calibration = 0.0;
    g_global_frame_counter = 0;
}

void profiler_shutdown(void) {
    profiler_enable(false);
    memset(g_thread_data, 0, sizeof(g_thread_data));
    g_thread_count = 0;
}

// ✅ COMPLETED: Utility functions
f64 profiler_get_overhead_calibration(void) {
    return g_overhead_calibration;
}

u64 profiler_get_global_frame_counter(void) {
    return g_global_frame_counter;
}
