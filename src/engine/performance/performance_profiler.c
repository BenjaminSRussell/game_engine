/**
 * @file performance_profiler.c
 * @brief High-performance profiling system for Minecraft v2 Engine
 *
 * Provides real-time performance monitoring, profiling, and analysis
 * capabilities for all engine subsystems.
 */

#include "performance_profiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

// ========================================
// Internal Structures
// ========================================

typedef struct ProfileEntry {
    char name[64];                      // Profile name
    uint64_t start_time;                 // Start timestamp (nanoseconds)
    uint64_t end_time;                   // End timestamp (nanoseconds)
    uint64_t total_time;                 // Total accumulated time
    uint32_t call_count;                 // Number of calls
    uint64_t min_time;                   // Minimum time per call
    uint64_t max_time;                   // Maximum time per call
    double average_time;                 // Average time per call
    bool is_active;                      // Currently being profiled
    struct ProfileEntry *next;           // Next entry in hash table
} ProfileEntry;

typedef struct PerformanceFrame {
    uint64_t frame_start_time;           // Frame start timestamp
    uint64_t frame_end_time;             // Frame end timestamp
    uint64_t frame_time;                  // Frame duration
    uint32_t frame_number;               // Frame number
    ProfileEntry *active_profiles;       // Currently active profiles
    struct PerformanceFrame *next;        // Next frame in history
} PerformanceFrame;

struct PerformanceProfiler {
    ProfileEntry **profile_table;        // Hash table for profiles
    uint32_t table_size;                  // Hash table size
    uint32_t profile_count;              // Number of profiles
    pthread_mutex_t mutex;               // Thread safety mutex
    bool is_enabled;                      // Whether profiling is enabled
    uint64_t overhead_time;               // Profiling overhead time
    uint32_t frame_history_size;          // Frame history size
    PerformanceFrame *frame_history;      // Frame history buffer
    PerformanceFrame *current_frame;     // Current frame being recorded
    uint32_t frame_count;                // Total frame count
    uint64_t total_profile_time;         // Total time spent in profiling
    uint64_t profile_start_time;         // When profiling was started
};

// ========================================
// Global Profiler Instance
// ========================================

static PerformanceProfiler g_profiler = {0};
static bool g_profiler_initialized = false;

// ========================================
// Utility Functions
// ========================================

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint32_t hash_string(const char *str, uint32_t table_size) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % table_size;
}

static ProfileEntry* find_or_create_profile(const char *name) {
    uint32_t hash = hash_string(name, g_profiler.table_size);
    ProfileEntry *entry = g_profiler.profile_table[hash];
    
    // Search for existing entry
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    
    // Create new entry
    entry = malloc(sizeof(ProfileEntry));
    if (!entry) return NULL;
    
    memset(entry, 0, sizeof(ProfileEntry));
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->min_time = UINT64_MAX;
    entry->max_time = 0;
    entry->is_active = false;
    
    // Add to hash table
    entry->next = g_profiler.profile_table[hash];
    g_profiler.profile_table[hash] = entry;
    g_profiler.profile_count++;
    
    return entry;
}

// ========================================
// Profiler Initialization
// ========================================

bool performance_profiler_init(uint32_t table_size, uint32_t frame_history_size) {
    if (g_profiler_initialized) {
        return false;
    }
    
    memset(&g_profiler, 0, sizeof(g_profiler));
    
    // Allocate profile table
    g_profiler.table_size = table_size;
    g_profiler.profile_table = calloc(table_size, sizeof(ProfileEntry*));
    if (!g_profiler.profile_table) {
        return false;
    }
    
    // Initialize frame history
    g_profiler.frame_history_size = frame_history_size;
    g_profiler.frame_history = calloc(frame_history_size, sizeof(PerformanceFrame));
    if (!g_profiler.frame_history) {
        free(g_profiler.profile_table);
        return false;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&g_profiler.mutex, NULL) != 0) {
        free(g_profiler.profile_table);
        free(g_profiler.frame_history);
        return false;
    }
    
    g_profiler.is_enabled = true;
    g_profiler.profile_start_time = get_timestamp_ns();
    g_profiler_initialized = true;
    
    return true;
}

void performance_profiler_shutdown(void) {
    if (!g_profiler_initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_profiler.mutex);
    
    // Free all profile entries
    for (uint32_t i = 0; i < g_profiler.table_size; i++) {
        ProfileEntry *entry = g_profiler.profile_table[i];
        while (entry) {
            ProfileEntry *next = entry->next;
            free(entry);
            entry = next;
        }
    }
    
    // Free frame history
    if (g_profiler.frame_history) {
        free(g_profiler.frame_history);
    }
    
    pthread_mutex_unlock(&g_profiler.mutex);
    pthread_mutex_destroy(&g_profiler.mutex);
    
    memset(&g_profiler, 0, sizeof(g_profiler));
    g_profiler_initialized = false;
}

// ========================================
// Profiling Control
// ========================================

bool performance_profiler_start_profile(const char *name) {
    if (!g_profiler_initialized || !g_profiler.is_enabled || !name) {
        return false;
    }
    
    pthread_mutex_lock(&g_profiler.mutex);
    
    ProfileEntry *entry = find_or_create_profile(name);
    if (!entry) {
        pthread_mutex_unlock(&g_profiler.mutex);
        return false;
    }
    
    if (entry->is_active) {
        pthread_mutex_unlock(&g_profiler.mutex);
        return false; // Already profiling this
    }
    
    entry->start_time = get_timestamp_ns();
    entry->is_active = true;
    
    // Add to current frame's active profiles
    if (g_profiler.current_frame) {
        entry->next = g_profiler.current_frame->active_profiles;
        g_profiler.current_frame->active_profiles = entry;
    }
    
    pthread_mutex_unlock(&g_profiler.mutex);
    return true;
}

bool performance_profiler_end_profile(const char *name) {
    if (!g_profiler_initialized || !g_profiler.is_enabled || !name) {
        return false;
    }
    
    pthread_mutex_lock(&g_profiler.mutex);
    
    ProfileEntry *entry = find_or_create_profile(name);
    if (!entry || !entry->is_active) {
        pthread_mutex_unlock(&g_profiler.mutex);
        return false;
    }
    
    uint64_t end_time = get_timestamp_ns();
    uint64_t duration = end_time - entry->start_time;
    
    // Update statistics
    entry->end_time = end_time;
    entry->total_time += duration;
    entry->call_count++;
    
    if (duration < entry->min_time) {
        entry->min_time = duration;
    }
    if (duration > entry->max_time) {
        entry->max_time = duration;
    }
    
    entry->average_time = (double)entry->total_time / entry->call_count;
    entry->is_active = false;
    
    // Remove from current frame's active profiles
    if (g_profiler.current_frame) {
        ProfileEntry **current = &g_profiler.current_frame->active_profiles;
        while (*current) {
            if (*current == entry) {
                *current = entry->next;
                break;
            }
            current = &(*current)->next;
        }
        entry->next = NULL;
    }
    
    g_profiler.total_profile_time += duration;
    
    pthread_mutex_unlock(&g_profiler.mutex);
    return true;
}

// ========================================
// Frame Management
// ========================================

void performance_profiler_begin_frame(void) {
    if (!g_profiler_initialized || !g_profiler.is_enabled) {
        return;
    }
    
    pthread_mutex_lock(&g_profiler.mutex);
    
    // Get current frame buffer
    g_profiler.current_frame = &g_profiler.frame_history[g_profiler.frame_count % g_profiler.frame_history_size];
    
    // Initialize frame
    g_profiler.current_frame->frame_start_time = get_timestamp_ns();
    g_profiler.current_frame->frame_number = g_profiler.frame_count;
    g_profiler.current_frame->active_profiles = NULL;
    
    pthread_mutex_unlock(&g_profiler.mutex);
}

void performance_profiler_end_frame(void) {
    if (!g_profiler_initialized || !g_profiler.is_enabled || !g_profiler.current_frame) {
        return;
    }
    
    pthread_mutex_lock(&g_profiler.mutex);
    
    g_profiler.current_frame->frame_end_time = get_timestamp_ns();
    g_profiler.current_frame->frame_time = 
        g_profiler.current_frame->frame_end_time - g_profiler.current_frame->frame_start_time;
    
    g_profiler.frame_count++;
    g_profiler.current_frame = NULL;
    
    pthread_mutex_unlock(&g_profiler.mutex);
}

// ========================================
// Statistics and Reporting
// ========================================

bool performance_profiler_get_profile_stats(const char *name, ProfileStats *stats) {
    if (!g_profiler_initialized || !name || !stats) {
        return false;
    }
    
    pthread_mutex_lock(&g_profiler.mutex);
    
    ProfileEntry *entry = find_or_create_profile(name);
    if (!entry) {
        pthread_mutex_unlock(&g_profiler.mutex);
        return false;
    }
    
    strncpy(stats->name, entry->name, sizeof(stats->name) - 1);
    stats->call_count = entry->call_count;
    stats->total_time_ns = entry->total_time;
    stats->average_time_ns = entry->average_time;
    stats->min_time_ns = entry->min_time;
    stats->max_time_ns = entry->max_time;
    stats->is_active = entry->is_active;
    
    pthread_mutex_unlock(&g_profiler.mutex);
    return true;
}

uint32_t performance_profiler_get_all_profiles(ProfileStats *stats, uint32_t max_stats) {
    if (!g_profiler_initialized || !stats || max_stats == 0) {
        return 0;
    }
    
    pthread_mutex_lock(&g_profiler.mutex);
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < g_profiler.table_size && count < max_stats; i++) {
        ProfileEntry *entry = g_profiler.profile_table[i];
        while (entry && count < max_stats) {
            strncpy(stats[count].name, entry->name, sizeof(stats[count].name) - 1);
            stats[count].call_count = entry->call_count;
            stats[count].total_time_ns = entry->total_time;
            stats[count].average_time_ns = entry->average_time;
            stats[count].min_time_ns = entry->min_time;
            stats[count].max_time_ns = entry->max_time;
            stats[count].is_active = entry->is_active;
            
            count++;
            entry = entry->next;
        }
    }
    
    pthread_mutex_unlock(&g_profiler.mutex);
    return count;
}

void performance_profiler_get_frame_stats(FrameStats *stats) {
    if (!g_profiler_initialized || !stats) {
        return;
    }
    
    pthread_mutex_lock(&g_profiler.mutex);
    
    memset(stats, 0, sizeof(FrameStats));
    
    // Calculate frame statistics from history
    uint64_t total_time = 0;
    uint64_t min_time = UINT64_MAX;
    uint64_t max_time = 0;
    uint32_t frames_to_analyze = g_profiler.frame_count;
    
    if (frames_to_analyze > g_profiler.frame_history_size) {
        frames_to_analyze = g_profiler.frame_history_size;
    }
    
    for (uint32_t i = 0; i < frames_to_analyze; i++) {
        PerformanceFrame *frame = &g_profiler.frame_history[i];
        if (frame->frame_time > 0) {
            total_time += frame->frame_time;
            if (frame->frame_time < min_time) {
                min_time = frame->frame_time;
            }
            if (frame->frame_time > max_time) {
                max_time = frame->frame_time;
            }
        }
    }
    
    stats->total_frames = g_profiler.frame_count;
    stats->average_frame_time_ns = frames_to_analyze > 0 ? (double)total_time / frames_to_analyze : 0.0;
    stats->min_frame_time_ns = min_time == UINT64_MAX ? 0 : min_time;
    stats->max_frame_time_ns = max_time;
    stats->current_fps = stats->average_frame_time_ns > 0 ? 
        1000000000.0 / stats->average_frame_time_ns : 0.0;
    
    pthread_mutex_unlock(&g_profiler.mutex);
}

void performance_profiler_print_report(void) {
    if (!g_profiler_initialized) {
        printf("Performance profiler not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&g_profiler.mutex);
    
    printf("\n=== Performance Profiler Report ===\n");
    printf("Profiler Enabled: %s\n", g_profiler.is_enabled ? "Yes" : "No");
    printf("Total Profiles: %u\n", g_profiler.profile_count);
    printf("Total Frames: %u\n", g_profiler.frame_count);
    printf("Total Profile Time: %.3f ms\n", 
           (double)g_profiler.total_profile_time / 1000000.0);
    
    // Calculate overhead
    uint64_t total_run_time = get_timestamp_ns() - g_profiler.profile_start_time;
    double overhead_percent = total_run_time > 0 ? 
        (double)g_profiler.overhead_time / total_run_time * 100.0 : 0.0;
    printf("Profiling Overhead: %.3f%%\n", overhead_percent);
    
    printf("\nProfile Statistics:\n");
    printf("%-30s %-10s %-12s %-12s %-12s %-10s\n", 
           "Name", "Calls", "Total(ms)", "Avg(ms)", "Min(ms)", "Max(ms)");
    printf("%-30s %-10s %-12s %-12s %-12s %-10s\n", 
           "----", "-----", "----------", "--------", "-------", "-------");
    
    // Sort profiles by total time (simplified)
    ProfileEntry *sorted_entries[256];
    uint32_t sorted_count = 0;
    
    for (uint32_t i = 0; i < g_profiler.table_size && sorted_count < 256; i++) {
        ProfileEntry *entry = g_profiler.profile_table[i];
        while (entry && sorted_count < 256) {
            sorted_entries[sorted_count++] = entry;
            entry = entry->next;
        }
    }
    
    // Simple bubble sort by total time
    for (uint32_t i = 0; i < sorted_count - 1; i++) {
        for (uint32_t j = 0; j < sorted_count - i - 1; j++) {
            if (sorted_entries[j]->total_time < sorted_entries[j + 1]->total_time) {
                ProfileEntry *temp = sorted_entries[j];
                sorted_entries[j] = sorted_entries[j + 1];
                sorted_entries[j + 1] = temp;
            }
        }
    }
    
    // Print top 20 profiles
    uint32_t to_print = sorted_count < 20 ? sorted_count : 20;
    for (uint32_t i = 0; i < to_print; i++) {
        ProfileEntry *entry = sorted_entries[i];
        printf("%-30s %-10u %-12.3f %-12.6f %-12.6f %-10.6f\n",
               entry->name,
               entry->call_count,
               (double)entry->total_time / 1000000.0,
               entry->average_time / 1000000.0,
               (double)entry->min_time / 1000000.0,
               (double)entry->max_time / 1000000.0);
    }
    
    // Frame statistics
    FrameStats frame_stats;
    performance_profiler_get_frame_stats(&frame_stats);
    
    printf("\nFrame Statistics:\n");
    printf("Total Frames: %u\n", frame_stats.total_frames);
    printf("Average Frame Time: %.3f ms\n", frame_stats.average_frame_time_ns / 1000000.0);
    printf("Min Frame Time: %.3f ms\n", frame_stats.min_frame_time_ns / 1000000.0);
    printf("Max Frame Time: %.3f ms\n", frame_stats.max_frame_time_ns / 1000000.0);
    printf("Current FPS: %.1f\n", frame_stats.current_fps);
    
    printf("=====================================\n\n");
    
    pthread_mutex_unlock(&g_profiler.mutex);
}

// ========================================
// Profiler Control
// ========================================

void performance_profiler_set_enabled(bool enabled) {
    pthread_mutex_lock(&g_profiler.mutex);
    g_profiler.is_enabled = enabled;
    pthread_mutex_unlock(&g_profiler.mutex);
}

bool performance_profiler_is_enabled(void) {
    pthread_mutex_lock(&g_profiler.mutex);
    bool enabled = g_profiler.is_enabled;
    pthread_mutex_unlock(&g_profiler.mutex);
    return enabled;
}

void performance_profiler_reset(void) {
    if (!g_profiler_initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_profiler.mutex);
    
    // Reset all profile statistics
    for (uint32_t i = 0; i < g_profiler.table_size; i++) {
        ProfileEntry *entry = g_profiler.profile_table[i];
        while (entry) {
            entry->total_time = 0;
            entry->call_count = 0;
            entry->min_time = UINT64_MAX;
            entry->max_time = 0;
            entry->average_time = 0.0;
            entry->is_active = false;
            entry = entry->next;
        }
    }
    
    // Reset frame statistics
    g_profiler.frame_count = 0;
    g_profiler.total_profile_time = 0;
    g_profiler.profile_start_time = get_timestamp_ns();
    
    if (g_profiler.frame_history) {
        memset(g_profiler.frame_history, 0, 
               g_profiler.frame_history_size * sizeof(PerformanceFrame));
    }
    
    pthread_mutex_unlock(&g_profiler.mutex);
}
