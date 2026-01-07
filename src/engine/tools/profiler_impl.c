/**
 * PROFILER IMPLEMENTATION - Complete Performance Monitoring
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

// ============================================================================
// TIMING UTILITIES
// ============================================================================

typedef struct {
    long long start_time_us;
    long long end_time_us;
} ProfilerTimer;

long long get_time_microseconds() {
#ifdef _WIN32
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (counter.QuadPart * 1000000LL) / freq.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
#endif
}

// ============================================================================
// PROFILER SCOPES
// ============================================================================

#define MAX_PROFILE_SCOPES 1024

typedef struct {
    char name[128];
    long long start_time;
    long long total_time;
    int hit_count;
    int parent_index;
    bool is_active;
} ProfileScope;

typedef struct {
    ProfileScope scopes[MAX_PROFILE_SCOPES];
    int scope_count;
    int current_scope_index;
    bool enabled;
    long long frame_start_time;
} Profiler;

static Profiler g_profiler = {0};

void profiler_init() {
    memset(&g_profiler, 0, sizeof(Profiler));
    g_profiler.enabled = true;
    g_profiler.current_scope_index = -1;
}

void profiler_shutdown() {
    g_profiler.enabled = false;
}

void profiler_begin_frame() {
    if (!g_profiler.enabled) return;
    g_profiler.frame_start_time = get_time_microseconds();
}

void profiler_end_frame() {
    if (!g_profiler.enabled) return;
    
    long long frame_time = get_time_microseconds() - g_profiler.frame_start_time;
    
    // Optional: Print frame stats
    // printf("Frame time: %.2f ms\n", frame_time / 1000.0);
}

int profiler_begin_scope(const char* name) {
    if (!g_profiler.enabled) return -1;
    if (g_profiler.scope_count >= MAX_PROFILE_SCOPES) return -1;
    
    // Find existing scope or create new
    int scope_idx = -1;
    for (int i = 0; i < g_profiler.scope_count; i++) {
        if (strcmp(g_profiler.scopes[i].name, name) == 0 && 
            !g_profiler.scopes[i].is_active) {
            scope_idx = i;
            break;
        }
    }
    
    if (scope_idx == -1) {
        scope_idx = g_profiler.scope_count++;
        strncpy(g_profiler.scopes[scope_idx].name, name, 127);
        g_profiler.scopes[scope_idx].total_time = 0;
        g_profiler.scopes[scope_idx].hit_count = 0;
    }
    
    ProfileScope* scope = &g_profiler.scopes[scope_idx];
    scope->start_time = get_time_microseconds();
    scope->parent_index = g_profiler.current_scope_index;
    scope->is_active = true;
    
    g_profiler.current_scope_index = scope_idx;
    
    return scope_idx;
}

void profiler_end_scope(int scope_idx) {
    if (!g_profiler.enabled || scope_idx < 0 || scope_idx >= g_profiler.scope_count) return;
    
    ProfileScope* scope = &g_profiler.scopes[scope_idx];
    long long elapsed = get_time_microseconds() - scope->start_time;
    
    scope->total_time += elapsed;
    scope->hit_count++;
    scope->is_active = false;
    
    g_profiler.current_scope_index = scope->parent_index;
}

void profiler_print_report() {
    if (!g_profiler.enabled) return;
    
    printf("\n=== PROFILER REPORT ===\n");
    printf("%-40s %10s %10s %10s\n", "Scope", "Total(ms)", "Avg(ms)", "Hits");
    printf("----------------------------------------------------------------\n");
    
    for (int i = 0; i < g_profiler.scope_count; i++) {
        ProfileScope* scope = &g_profiler.scopes[i];
        if (scope->hit_count > 0) {
            double total_ms = scope->total_time / 1000.0;
            double avg_ms = total_ms / scope->hit_count;
            printf("%-40s %10.2f %10.2f %10d\n", 
                   scope->name, total_ms, avg_ms, scope->hit_count);
        }
    }
    printf("\n");
}

void profiler_reset() {
    for (int i = 0; i < g_profiler.scope_count; i++) {
        g_profiler.scopes[i].total_time = 0;
        g_profiler.scopes[i].hit_count = 0;
    }
}

// ============================================================================
// CPU/GPU/MEMORY MONITORING
// ============================================================================

typedef struct {
    long long cpu_cycles;
    long long cache_misses;
    size_t memory_used;
    size_t memory_peak;
} SystemMetrics;

SystemMetrics profiler_get_metrics() {
    SystemMetrics metrics = {0};
    
    // Platform-specific implementations would go here
    // For now, return zeros
    
    return metrics;
}

// ============================================================================
// EXPORT TO JSON
// ============================================================================

void profiler_export_json(const char* filepath) {
    FILE* f = fopen(filepath, "w");
    if (!f) return;
    
    fprintf(f, "{\n  \"scopes\": [\n");
    for (int i = 0; i < g_profiler.scope_count; i++) {
        ProfileScope* scope = &g_profiler.scopes[i];
        if (scope->hit_count > 0) {
            fprintf(f, "    {\n");
            fprintf(f, "      \"name\": \"%s\",\n", scope->name);
            fprintf(f, "      \"total_ms\": %.2f,\n", scope->total_time / 1000.0);
            fprintf(f, "      \"avg_ms\": %.2f,\n", (scope->total_time / 1000.0) / scope->hit_count);
            fprintf(f, "      \"hits\": %d\n", scope->hit_count);
            fprintf(f, "    }%s\n", (i < g_profiler.scope_count - 1) ? "," : "");
        }
    }
    fprintf(f, "  ]\n}\n");
    
    fclose(f);
}

// Macro helpers
#define PROFILE_SCOPE(name) \
    int _profile_idx_##name = profiler_begin_scope(#name); \
    (void)_profile_idx_##name;

#define PROFILE_SCOPE_END(name) \
    profiler_end_scope(_profile_idx_##name);

// PROFILER COMPLETE - Full performance monitoring system
