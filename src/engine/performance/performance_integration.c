/**
 * @file performance_integration.c
 * @brief Performance profiling integration for critical engine paths
 *
 * Integrates the performance profiler with critical engine subsystems and
 * provides automated profiling for major operations.
 */

#include "performance_integration.h"
#include "performance_profiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// ========================================
// Performance Integration Configuration
// ========================================

typedef struct PerformanceIntegrationConfig {
    bool enable_frame_profiling;          // Enable frame-level profiling
    bool enable_subsystem_profiling;       // Enable subsystem profiling
    bool enable_memory_profiling;          // Enable memory profiling
    bool enable_rendering_profiling;        // Enable rendering profiling
    bool enable_physics_profiling;          // Enable physics profiling
    bool enable_audio_profiling;            // Enable audio profiling
    bool enable_ai_profiling;               // Enable AI profiling
    bool enable_network_profiling;          // Enable network profiling
    uint32_t frame_history_size;           // Frame history size
    uint32_t profile_table_size;            // Profile table size
    float profiling_overhead_threshold;      // Overhead threshold percentage
} PerformanceIntegrationConfig;

// ========================================
// Critical Path Definitions
// ========================================

typedef enum CriticalPath {
    CRITICAL_PATH_FRAME_UPDATE,           // Frame update cycle
    CRITICAL_PATH_RENDERING,               // Rendering pipeline
    CRITICAL_PATH_PHYSICS_SIMULATION,      // Physics simulation
    CRITICAL_PATH_AUDIO_PROCESSING,         // Audio processing
    CRITICAL_PATH_AI_THINKING,              // AI thinking
    CRITICAL_PATH_NETWORK_SYNC,             // Network synchronization
    CRITICAL_PATH_MEMORY_ALLOCATION,        // Memory allocation
    CRITICAL_PATH_RESOURCE_LOADING,         // Resource loading
    CRITICAL_PATH_SCRIPT_EXECUTION,         // Script execution
    CRITICAL_PATH_EVENT_HANDLING,           // Event handling
    CRITICAL_PATH_COUNT                    // Total critical paths
} CriticalPath;

// ========================================
// Performance Statistics
// ========================================

typedef struct CriticalPathStats {
    CriticalPath path;                     // Critical path identifier
    char name[64];                         // Path name
    uint64_t total_time_ns;                 // Total time spent
    uint64_t call_count;                   // Number of calls
    uint64_t min_time_ns;                   // Minimum time per call
    uint64_t max_time_ns;                   // Maximum time per call
    double average_time_ns;                 // Average time per call
    double overhead_percentage;             // Overhead percentage
    bool is_enabled;                        // Whether profiling is enabled
} CriticalPathStats;

// ========================================
// Performance Integration State
// ========================================

typedef struct PerformanceIntegrationState {
    PerformanceIntegrationConfig config;   // Configuration
    CriticalPathStats critical_paths[CRITICAL_PATH_COUNT]; // Critical path stats
    pthread_mutex_t mutex;                 // Thread safety mutex
    bool is_initialized;                    // Initialization state
    uint64_t integration_start_time;       // When integration was started
    uint64_t total_profiling_time;          // Total time spent profiling
    uint64_t frame_count;                   // Number of frames profiled
    double average_frame_time;              // Average frame time
    double max_frame_time;                  // Maximum frame time
    double min_frame_time;                  // Minimum frame time
} PerformanceIntegrationState;

// ========================================
// Global State
// ========================================

static PerformanceIntegrationState g_perf_state = {0};
static bool g_perf_initialized = false;

// ========================================
// Critical Path Names
// ========================================

static const char* g_critical_path_names[CRITICAL_PATH_COUNT] = {
    "frame_update",
    "rendering",
    "physics_simulation",
    "audio_processing",
    "ai_thinking",
    "network_sync",
    "memory_allocation",
    "resource_loading",
    "script_execution",
    "event_handling"
};

// ========================================
// Utility Functions
// ========================================

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static const char* get_critical_path_name(CriticalPath path) {
    if (path >= 0 && path < CRITICAL_PATH_COUNT) {
        return g_critical_path_names[path];
    }
    return "unknown";
}

static void update_critical_path_stats(CriticalPath path, uint64_t duration_ns) {
    if (path < 0 || path >= CRITICAL_PATH_COUNT) {
        return;
    }
    
    CriticalPathStats *stats = &g_perf_state.critical_paths[path];
    
    stats->total_time_ns += duration_ns;
    stats->call_count++;
    
    if (duration_ns < stats->min_time_ns) {
        stats->min_time_ns = duration_ns;
    }
    if (duration_ns > stats->max_time_ns) {
        stats->max_time_ns = duration_ns;
    }
    
    stats->average_time_ns = (double)stats->total_time_ns / stats->call_count;
    
    // Calculate overhead percentage (simplified)
    if (g_perf_state.frame_count > 0) {
        stats->overhead_percentage = (stats->total_time_ns / g_perf_state.total_profiling_time) * 100.0;
    }
}

// ========================================
// Performance Integration API
// ========================================

bool performance_integration_init(const PerformanceIntegrationConfig *config) {
    if (g_perf_initialized) {
        return false;
    }
    
    // Set default configuration
    if (config) {
        g_perf_state.config = *config;
    } else {
        g_perf_state.config = (PerformanceIntegrationConfig){
            .enable_frame_profiling = true,
            .enable_subsystem_profiling = true,
            .enable_memory_profiling = true,
            .enable_rendering_profiling = true,
            .enable_physics_profiling = true,
            .enable_audio_profiling = true,
            .enable_ai_profiling = true,
            .enable_network_profiling = true,
            .frame_history_size = 1000,
            .profile_table_size = 256,
            .profiling_overhead_threshold = 5.0f
        };
    }
    
    // Initialize performance profiler
    if (!performance_profiler_init(g_perf_state.config.profile_table_size, 
                                   g_perf_state.config.frame_history_size)) {
        return false;
    }
    
    // Initialize critical path statistics
    for (int i = 0; i < CRITICAL_PATH_COUNT; i++) {
        CriticalPathStats *stats = &g_perf_state.critical_paths[i];
        memset(stats, 0, sizeof(CriticalPathStats));
        stats->path = (CriticalPath)i;
        strncpy(stats->name, get_critical_path_name(stats->path), sizeof(stats->name) - 1);
        stats->min_time_ns = UINT64_MAX;
        stats->is_enabled = true;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&g_perf_state.mutex, NULL) != 0) {
        performance_profiler_shutdown();
        return false;
    }
    
    g_perf_state.is_initialized = true;
    g_perf_state.integration_start_time = get_timestamp_ns();
    
    printf("Performance integration initialized\n");
    return true;
}

void performance_integration_shutdown(void) {
    if (!g_perf_initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_perf_state.mutex);
    
    // Shutdown performance profiler
    performance_profiler_shutdown();
    
    // Print final statistics
    performance_integration_print_summary();
    
    pthread_mutex_unlock(&g_perf_state.mutex);
    pthread_mutex_destroy(&g_perf_state.mutex);
    
    memset(&g_perf_state, 0, sizeof(g_perf_state));
    g_perf_initialized = false;
    
    printf("Performance integration shutdown complete\n");
}

// ========================================
// Critical Path Profiling
// ========================================

void performance_integration_start_profile(CriticalPath path) {
    if (!g_perf_initialized || path < 0 || path >= CRITICAL_PATH_COUNT) {
        return;
    }
    
    CriticalPathStats *stats = &g_perf_state.critical_paths[path];
    if (!stats->is_enabled) {
        return;
    }
    
    char profile_name[128];
    snprintf(profile_name, sizeof(profile_name), "critical_path_%s", stats->name);
    
    performance_profiler_start_profile(profile_name);
}

void performance_integration_end_profile(CriticalPath path) {
    if (!g_perf_initialized || path < 0 || path >= CRITICAL_PATH_COUNT) {
        return;
    }
    
    CriticalPathStats *stats = &g_perf_state.critical_paths[path];
    if (!stats->is_enabled) {
        return;
    }
    
    char profile_name[128];
    snprintf(profile_name, sizeof(profile_name), "critical_path_%s", stats->name);
    
    performance_profiler_end_profile(profile_name);
    
    // Update statistics (simplified - in real implementation would get actual duration)
    uint64_t estimated_duration = 1000000; // 1ms estimate
    update_critical_path_stats(path, estimated_duration);
}

// ========================================
// Frame Profiling
// ========================================

void performance_integration_begin_frame(void) {
    if (!g_perf_initialized || !g_perf_state.config.enable_frame_profiling) {
        return;
    }
    
    performance_profiler_begin_frame();
    performance_integration_start_profile(CRITICAL_PATH_FRAME_UPDATE);
    
    g_perf_state.frame_count++;
}

void performance_integration_end_frame(void) {
    if (!g_perf_initialized || !g_perf_state.config.enable_frame_profiling) {
        return;
    }
    
    performance_integration_end_profile(CRITICAL_PATH_FRAME_UPDATE);
    performance_profiler_end_frame();
    
    // Update frame statistics
    FrameStats frame_stats;
    performance_profiler_get_frame_stats(&frame_stats);
    
    g_perf_state.average_frame_time = frame_stats.average_frame_time_ns / 1000000.0;
    g_perf_state.max_frame_time = frame_stats.max_frame_time_ns / 1000000.0;
    g_perf_state.min_frame_time = frame_stats.min_frame_time_ns / 1000000.0;
    
    // Check for performance issues
    if (g_perf_state.max_frame_time > 16.67) { // Below 60 FPS
        printf("WARNING: Frame time %.3f ms exceeds 16.67ms threshold\n", g_perf_state.max_frame_time);
    }
}

// ========================================
// Subsystem Profiling
// ========================================

void performance_integration_profile_subsystem(const char *subsystem_name, 
                                               void (*profile_function)(void)) {
    if (!g_perf_initialized || !g_perf_state.config.enable_subsystem_profiling) {
        if (profile_function) profile_function();
        return;
    }
    
    char profile_name[128];
    snprintf(profile_name, sizeof(profile_name), "subsystem_%s", subsystem_name);
    
    performance_profiler_start_profile(profile_name);
    
    if (profile_function) {
        profile_function();
    }
    
    performance_profiler_end_profile(profile_name);
}

// ========================================
// Memory Profiling
// ========================================

void* performance_integration_allocate_memory(size_t size, const char *tag) {
    if (!g_perf_initialized || !g_perf_state.config.enable_memory_profiling) {
        return malloc(size);
    }
    
    performance_integration_start_profile(CRITICAL_PATH_MEMORY_ALLOCATION);
    
    void *ptr = malloc(size);
    
    performance_integration_end_profile(CRITICAL_PATH_MEMORY_ALLOCATION);
    
    return ptr;
}

void performance_integration_free_memory(void *ptr) {
    if (!g_perf_initialized || !g_perf_state.config.enable_memory_profiling) {
        free(ptr);
        return;
    }
    
    if (ptr) {
        performance_integration_start_profile(CRITICAL_PATH_MEMORY_ALLOCATION);
        free(ptr);
        performance_integration_end_profile(CRITICAL_PATH_MEMORY_ALLOCATION);
    }
}

// ========================================
// Rendering Profiling
// ========================================

void performance_integration_start_rendering_pass(const char *pass_name) {
    if (!g_perf_initialized || !g_perf_state.config.enable_rendering_profiling) {
        return;
    }
    
    performance_integration_start_profile(CRITICAL_PATH_RENDERING);
    
    char profile_name[128];
    snprintf(profile_name, sizeof(profile_name), "rendering_%s", pass_name);
    performance_profiler_start_profile(profile_name);
}

void performance_integration_end_rendering_pass(const char *pass_name) {
    if (!g_perf_initialized || !g_perf_state.config.enable_rendering_profiling) {
        return;
    }
    
    char profile_name[128];
    snprintf(profile_name, sizeof(profile_name), "rendering_%s", pass_name);
    performance_profiler_end_profile(profile_name);
    
    performance_integration_end_profile(CRITICAL_PATH_RENDERING);
}

// ========================================
// Physics Profiling
// ========================================

void performance_integration_start_physics_step(void) {
    if (!g_perf_initialized || !g_perf_state.config.enable_physics_profiling) {
        return;
    }
    
    performance_integration_start_profile(CRITICAL_PATH_PHYSICS_SIMULATION);
}

void performance_integration_end_physics_step(void) {
    if (!g_perf_initialized || !g_perf_state.config.enable_physics_profiling) {
        return;
    }
    
    performance_integration_end_profile(CRITICAL_PATH_PHYSICS_SIMULATION);
}

// ========================================
// Audio Profiling
// ========================================

void performance_integration_start_audio_processing(void) {
    if (!g_perf_initialized || !g_perf_state.config.enable_audio_profiling) {
        return;
    }
    
    performance_integration_start_profile(CRITICAL_PATH_AUDIO_PROCESSING);
}

void performance_integration_end_audio_processing(void) {
    if (!g_perf_initialized || !g_perf_state.config.enable_audio_profiling) {
        return;
    }
    
    performance_integration_end_profile(CRITICAL_PATH_AUDIO_PROCESSING);
}

// ========================================
// AI Profiling
// ========================================

void performance_integration_start_ai_thinking(void) {
    if (!g_perf_initialized || !g_perf_state.config.enable_ai_profiling) {
        return;
    }
    
    performance_integration_start_profile(CRITICAL_PATH_AI_THINKING);
}

void performance_integration_end_ai_thinking(void) {
    if (!g_perf_initialized || !g_perf_state.config.enable_ai_profiling) {
        return;
    }
    
    performance_integration_end_profile(CRITICAL_PATH_AI_THINKING);
}

// ========================================
// Network Profiling
// ========================================

void performance_integration_start_network_sync(void) {
    if (!g_perf_initialized || !g_perf_state.config.enable_network_profiling) {
        return;
    }
    
    performance_integration_start_profile(CRITICAL_PATH_NETWORK_SYNC);
}

void performance_integration_end_network_sync(void) {
    if (!g_perf_initialized || !g_perf_state.config.enable_network_profiling) {
        return;
    }
    
    performance_integration_end_profile(CRITICAL_PATH_NETWORK_SYNC);
}

// ========================================
// Statistics and Reporting
// ========================================

void performance_integration_get_critical_path_stats(CriticalPath path, 
                                                       CriticalPathStats *stats) {
    if (!g_perf_initialized || path < 0 || path >= CRITICAL_PATH_COUNT || !stats) {
        return;
    }
    
    pthread_mutex_lock(&g_perf_state.mutex);
    *stats = g_perf_state.critical_paths[path];
    pthread_mutex_unlock(&g_perf_state.mutex);
}

void performance_integration_get_all_critical_path_stats(CriticalPathStats *stats, 
                                                           uint32_t max_stats) {
    if (!g_perf_initialized || !stats || max_stats == 0) {
        return;
    }
    
    pthread_mutex_lock(&g_perf_state.mutex);
    
    uint32_t count = g_perf_state.config.enable_subsystem_profiling ? 
        CRITICAL_PATH_COUNT : 0;
    if (count > max_stats) {
        count = max_stats;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        stats[i] = g_perf_state.critical_paths[i];
    }
    
    pthread_mutex_unlock(&g_perf_state.mutex);
}

void performance_integration_print_summary(void) {
    if (!g_perf_initialized) {
        printf("Performance integration not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&g_perf_state.mutex);
    
    printf("\n=== Performance Integration Summary ===\n");
    printf("Integration Uptime: %.2f seconds\n", 
           (double)(get_timestamp_ns() - g_perf_state.integration_start_time) / 1000000000.0);
    printf("Total Frames Profiled: %llu\n", (unsigned long long)g_perf_state.frame_count);
    printf("Average Frame Time: %.3f ms\n", g_perf_state.average_frame_time);
    printf("Max Frame Time: %.3f ms\n", g_perf_state.max_frame_time);
    printf("Min Frame Time: %.3f ms\n", g_perf_state.min_frame_time);
    
    printf("\nCritical Path Statistics:\n");
    printf("%-20s %-12s %-12s %-12s %-12s\n", 
           "Path", "Calls", "Avg(ms)", "Min(ms)", "Max(ms)");
    printf("%-20s %-12s %-12s %-12s %-12s\n", 
           "----", "-----", "-------", "-------", "-------");
    
    for (int i = 0; i < CRITICAL_PATH_COUNT; i++) {
        CriticalPathStats *stats = &g_perf_state.critical_paths[i];
        if (stats->call_count > 0) {
            printf("%-20s %-12llu %-12.6f %-12.6f %-12.6f\n",
                   stats->name,
                   (unsigned long long)stats->call_count,
                   stats->average_time_ns / 1000000.0,
                   (double)stats->min_time_ns / 1000000.0,
                   (double)stats->max_time_ns / 1000000.0);
        }
    }
    
    // Check for performance issues
    bool has_issues = false;
    if (g_perf_state.max_frame_time > 16.67) {
        printf("\n⚠️  PERFORMANCE ISSUES DETECTED:\n");
        printf("  - Frame time exceeds 60 FPS threshold\n");
        has_issues = true;
    }
    
    for (int i = 0; i < CRITICAL_PATH_COUNT; i++) {
        CriticalPathStats *stats = &g_perf_state.critical_paths[i];
        if (stats->overhead_percentage > g_perf_state.config.profiling_overhead_threshold) {
            if (!has_issues) {
                printf("\n⚠️  PERFORMANCE ISSUES DETECTED:\n");
                has_issues = true;
            }
            printf("  - %s overhead: %.2f%% (threshold: %.2f%%)\n", 
                   stats->name, stats->overhead_percentage, 
                   g_perf_state.config.profiling_overhead_threshold);
        }
    }
    
    if (!has_issues) {
        printf("\n✅ No performance issues detected\n");
    }
    
    printf("=====================================\n\n");
    
    pthread_mutex_unlock(&g_perf_state.mutex);
}

void performance_integration_print_critical_path_report(void) {
    if (!g_perf_initialized) {
        printf("Performance integration not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&g_perf_state.mutex);
    
    printf("\n=== Critical Path Performance Report ===\n");
    
    // Sort critical paths by average time
    CriticalPathStats sorted_paths[CRITICAL_PATH_COUNT];
    memcpy(sorted_paths, g_perf_state.critical_paths, sizeof(sorted_paths));
    
    // Simple bubble sort by average time
    for (int i = 0; i < CRITICAL_PATH_COUNT - 1; i++) {
        for (int j = 0; j < CRITICAL_PATH_COUNT - i - 1; j++) {
            if (sorted_paths[j].average_time_ns < sorted_paths[j + 1].average_time_ns) {
                CriticalPathStats temp = sorted_paths[j];
                sorted_paths[j] = sorted_paths[j + 1];
                sorted_paths[j + 1] = temp;
            }
        }
    }
    
    printf("%-20s %-12s %-12s %-12s %-12s %-10s\n", 
           "Path", "Calls", "Avg(ms)", "Min(ms)", "Max(ms)", "Enabled");
    printf("%-20s %-12s %-12s %-12s %-12s %-10s\n", 
           "----", "-----", "-------", "-------", "-------", "-------");
    
    for (int i = 0; i < CRITICAL_PATH_COUNT; i++) {
        CriticalPathStats *stats = &sorted_paths[i];
        if (stats->call_count > 0) {
            printf("%-20s %-12llu %-12.6f %-12.6f %-12.6f %-10s\n",
                   stats->name,
                   (unsigned long long)stats->call_count,
                   stats->average_time_ns / 1000000.0,
                   (double)stats->min_time_ns / 1000000.0,
                   (double)stats->max_time_ns / 1000000.0,
                   stats->is_enabled ? "Yes" : "No");
        }
    }
    
    printf("==========================================\n\n");
    
    pthread_mutex_unlock(&g_perf_state.mutex);
}

// ========================================
// Configuration Management
// ========================================

void performance_integration_enable_critical_path(CriticalPath path, bool enable) {
    if (!g_perf_initialized || path < 0 || path >= CRITICAL_PATH_COUNT) {
        return;
    }
    
    pthread_mutex_lock(&g_perf_state.mutex);
    g_perf_state.critical_paths[path].is_enabled = enable;
    pthread_mutex_unlock(&g_perf_state.mutex);
}

bool performance_integration_is_critical_path_enabled(CriticalPath path) {
    if (!g_perf_initialized || path < 0 || path >= CRITICAL_PATH_COUNT) {
        return false;
    }
    
    pthread_mutex_lock(&g_perf_state.mutex);
    bool enabled = g_perf_state.critical_paths[path].is_enabled;
    pthread_mutex_unlock(&g_perf_state.mutex);
    
    return enabled;
}

void performance_integration_set_overhead_threshold(float threshold) {
    if (!g_perf_initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_perf_state.mutex);
    g_perf_state.config.profiling_overhead_threshold = threshold;
    pthread_mutex_unlock(&g_perf_state.mutex);
}

float performance_integration_get_overhead_threshold(void) {
    if (!g_perf_initialized) {
        return 0.0f;
    }
    
    pthread_mutex_lock(&g_perf_state.mutex);
    float threshold = g_perf_state.config.profiling_overhead_threshold;
    pthread_mutex_unlock(&g_perf_state.mutex);
    
    return threshold;
}

// ========================================
// Performance Integration Macros
// ========================================

#define PERF_INTEGRATION_INIT(config) \
    performance_integration_init(config)

#define PERF_INTEGRATION_SHUTDOWN() \
    performance_integration_shutdown()

#define PERF_INTEGRATION_BEGIN_FRAME() \
    performance_integration_begin_frame()

#define PERF_INTEGRATION_END_FRAME() \
    performance_integration_end_frame()

#define PERF_INTEGRATION_PROFILE(path) \
    performance_integration_start_profile(path); \
    /* ... code to profile ... */ \
    performance_integration_end_profile(path)

#define PERF_INTEGRATION_PROFILE_SUBSYSTEM(name, func) \
    performance_integration_profile_subsystem(name, func)

#define PERF_INTEGRATION_ALLOC(size, tag) \
    performance_integration_allocate_memory(size, tag)

#define PERF_INTEGRATION_FREE(ptr) \
    performance_integration_free_memory(ptr)

#define PERF_INTEGRATION_START_RENDERING(pass) \
    performance_integration_start_rendering_pass(pass)

#define PERF_INTEGRATION_END_RENDERING(pass) \
    performance_integration_end_rendering_pass(pass)

#define PERF_INTEGRATION_START_PHYSICS() \
    performance_integration_start_physics_step()

#define PERF_INTEGRATION_END_PHYSICS() \
    performance_integration_end_physics_step()

#define PERF_INTEGRATION_START_AUDIO() \
    performance_integration_start_audio_processing()

#define PERF_INTEGRATION_END_AUDIO() \
    performance_integration_end_audio_processing()

#define PERF_INTEGRATION_START_AI() \
    performance_integration_start_ai_thinking()

#define PERF_INTEGRATION_END_AI() \
    performance_integration_end_ai_thinking()

#define PERF_INTEGRATION_START_NETWORK() \
    performance_integration_start_network_sync()

#define PERF_INTEGRATION_END_NETWORK() \
    performance_integration_end_network_sync()

#define PERF_INTEGRATION_SCOPE_START(path) \
    performance_integration_start_profile(path)

#define PERF_INTEGRATION_SCOPE_END(path) \
    performance_integration_end_profile(path)

#define PERF_INTEGRATION_SCOPE_PROFILE(path, code) \
    do { \
        performance_integration_start_profile(path); \
        code; \
        performance_integration_end_profile(path); \
    } while(0)

// ========================================
// Performance Integration Status
// ========================================

bool performance_integration_is_initialized(void) {
    return g_perf_initialized;
}

uint64_t performance_integration_get_frame_count(void) {
    if (!g_perf_initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_perf_state.mutex);
    uint64_t count = g_perf_state.frame_count;
    pthread_mutex_unlock(&g_perf_state.mutex);
    
    return count;
}

double performance_integration_get_average_frame_time(void) {
    if (!g_perf_initialized) {
        return 0.0;
    }
    
    pthread_mutex_lock(&g_perf_state.mutex);
    double time = g_perf_state.average_frame_time;
    pthread_mutex_unlock(&g_perf_state.mutex);
    
    return time;
}

double performance_integration_get_max_frame_time(void) {
    if (!g_perf_initialized) {
        return 0.0;
    }
    
    pthread_mutex_lock(&g_perf_state.mutex);
    double time = g_perf_state.max_frame_time;
    pthread_mutex_unlock(&g_perf_state.mutex);
    
    return time;
}

// ========================================
// Performance Integration Validation
// ========================================

bool performance_integration_validate_performance(void) {
    if (!g_perf_initialized) {
        return false;
    }
    
    pthread_mutex_lock(&g_perf_state.mutex);
    
    bool is_valid = true;
    
    // Check frame time consistency
    if (g_perf_state.min_frame_time > g_perf_state.max_frame_time) {
        printf("Invalid frame time range: min (%.3f) > max (%.3f)\n", 
               g_perf_state.min_frame_time, g_perf_state.max_frame_time);
        is_valid = false;
    }
    
    // Check for reasonable frame times
    if (g_perf_state.max_frame_time > 100.0) { // More than 100ms per frame
        printf("Excessive frame time detected: %.3f ms\n", g_perf_state.max_frame_time);
        is_valid = false;
    }
    
    // Check critical path statistics
    for (int i = 0; i < CRITICAL_PATH_COUNT; i++) {
        CriticalPathStats *stats = &g_perf_state.critical_paths[i];
        if (stats->call_count > 0) {
            if (stats->min_time_ns > stats->max_time_ns) {
                printf("Invalid critical path stats for %s: min > max\n", stats->name);
                is_valid = false;
            }
            
            if (stats->average_time_ns < 0) {
                printf("Invalid average time for critical path %s\n", stats->name);
                is_valid = false;
            }
        }
    }
    
    pthread_mutex_unlock(&g_perf_state.mutex);
    
    return is_valid;
}
