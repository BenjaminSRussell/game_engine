/**
 * @file performance_profiler.c
 * @brief Physics performance profiling system
 *
 * Implements comprehensive performance profiling for physics systems including
 * timing, memory usage, and performance metrics collection and analysis.
 */

#include "performance_profiler.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

// ========================================
// Constants
// ========================================

#define MAX_PROFILE_ZONES 64
#define MAX_FRAMES_HISTORY 1024
#define MAX_TIMERS_PER_ZONE 32
#define MAX_MEMORY_TRACKERS 32

// ========================================
// Internal Timer Structure
// ========================================

typedef struct ProfileTimer {
    char name[64];               // Timer name
    uint64_t start_time;         // Start timestamp
    uint64_t total_time;         // Accumulated time
    uint64_t min_time;           // Minimum time
    uint64_t max_time;           // Maximum time
    uint32_t call_count;          // Number of calls
    bool active;                  // Timer is currently active
    
} ProfileTimer;

// ========================================
// Memory Tracker Structure
// ========================================

typedef struct MemoryTracker {
    char name[64];               // Tracker name
    size_t total_allocated;      // Total allocated bytes
    size_t total_freed;          // Total freed bytes
    size_t current_usage;        // Current usage
    uint32_t allocation_count;    // Number of allocations
    uint32_t free_count;          // Number of frees
    size_t peak_usage;            // Peak memory usage
    
} MemoryTracker;

// ========================================
// Profile Zone Structure
// ========================================

typedef struct ProfileZone {
    char name[64];               // Zone name
    ProfileTimer timers[MAX_TIMERS_PER_ZONE]; // Zone-specific timers
    int timer_count;              // Number of active timers
    
    uint64_t frame_times[MAX_FRAMES_HISTORY]; // Frame time history
    int frame_count;             // Number of recorded frames
    int current_frame;           // Current frame index
    
    // Zone statistics
    uint64_t total_frame_time;   // Total time spent in zone
    uint64_t min_frame_time;     // Minimum frame time
    uint64_t max_frame_time;     // Maximum frame time
    float avg_frame_time;        // Average frame time
    
    bool enabled;                 // Zone is enabled
    
} ProfileZone;

// ========================================
// Profiler Structure
// ========================================

struct PhysicsProfiler {
    ProfileZone zones[MAX_PROFILE_ZONES]; // Profile zones
    int zone_count;               // Number of active zones
    
    MemoryTracker memory_trackers[MAX_MEMORY_TRACKERS]; // Memory trackers
    int memory_tracker_count;      // Number of memory trackers
    
    // Global statistics
    uint64_t total_profile_time;  // Total profiling time
    uint64_t frame_start_time;    // Current frame start time
    uint64_t last_frame_time;     // Last frame duration
    
    // Performance metrics
    float current_fps;            // Current frames per second
    float avg_fps;                // Average FPS
    float min_fps;                // Minimum FPS
    float max_fps;                // Maximum FPS
    
    // Memory statistics
    size_t total_memory_usage;     // Total memory usage
    size_t peak_memory_usage;      // Peak memory usage
    
    // Configuration
    bool enabled;                  // Profiler is enabled
    bool detailed_logging;          // Enable detailed logging
    bool auto_reset;               // Auto-reset statistics
    int reset_interval;            // Reset interval (frames)
    
    // Callbacks
    ProfilerFrameCallback frame_callback;
    ProfilerMemoryCallback memory_callback;
    void *user_data;
    
};

// ========================================
// High-Resolution Timer Functions
// ========================================

#ifdef _WIN32
#include <windows.h>

static uint64_t get_high_res_time(void) {
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (uint64_t)(counter.QuadPart * 1000000 / frequency.QuadPart); // Microseconds
}

#else
#include <time.h>

static uint64_t get_high_res_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000; // Microseconds
}

#endif

// ========================================
// Internal Helper Functions
// ========================================

static ProfileZone* get_zone(PhysicsProfiler *profiler, const char *zone_name) {
    if (!profiler || !zone_name) return NULL;
    
    for (int i = 0; i < profiler->zone_count; i++) {
        if (strcmp(profiler->zones[i].name, zone_name) == 0) {
            return &profiler->zones[i];
        }
    }
    
    return NULL;
}

static ProfileTimer* get_timer(ProfileZone *zone, const char *timer_name) {
    if (!zone || !timer_name) return NULL;
    
    for (int i = 0; i < zone->timer_count; i++) {
        if (strcmp(zone->timers[i].name, timer_name) == 0) {
            return &zone->timers[i];
        }
    }
    
    return NULL;
}

static ProfileTimer* create_timer(ProfileZone *zone, const char *timer_name) {
    if (!zone || !timer_name || zone->timer_count >= MAX_TIMERS_PER_ZONE) {
        return NULL;
    }
    
    ProfileTimer *timer = &zone->timers[zone->timer_count++];
    strncpy(timer->name, timer_name, sizeof(timer->name) - 1);
    timer->name[sizeof(timer->name) - 1] = '\0';
    
    timer->start_time = 0;
    timer->total_time = 0;
    timer->min_time = UINT64_MAX;
    timer->max_time = 0;
    timer->call_count = 0;
    timer->active = false;
    
    return timer;
}

static void update_frame_statistics(ProfileZone *zone) {
    if (zone->frame_count == 0) return;
    
    uint64_t total = 0;
    uint64_t min_time = UINT64_MAX;
    uint64_t max_time = 0;
    
    for (int i = 0; i < zone->frame_count; i++) {
        uint64_t frame_time = zone->frame_times[i];
        total += frame_time;
        min_time = (frame_time < min_time) ? frame_time : min_time;
        max_time = (frame_time > max_time) ? frame_time : max_time;
    }
    
    zone->total_frame_time = total;
    zone->min_frame_time = min_time;
    zone->max_frame_time = max_time;
    zone->avg_frame_time = (float)total / zone->frame_count;
}

// ========================================
// Public API Implementation
// ========================================

PhysicsProfiler* physics_profiler_create(void) {
    PhysicsProfiler *profiler = (PhysicsProfiler *)calloc(1, sizeof(PhysicsProfiler));
    if (!profiler) return NULL;
    
    profiler->enabled = true;
    profiler->detailed_logging = false;
    profiler->auto_reset = false;
    profiler->reset_interval = 60; // Reset every 60 frames
    
    return profiler;
}

void physics_profiler_destroy(PhysicsProfiler *profiler) {
    if (profiler) {
        free(profiler);
    }
}

void physics_profiler_enable(PhysicsProfiler *profiler, bool enabled) {
    if (profiler) {
        profiler->enabled = enabled;
    }
}

bool physics_profiler_is_enabled(PhysicsProfiler *profiler) {
    return profiler ? profiler->enabled : false;
}

int physics_profiler_create_zone(PhysicsProfiler *profiler, const char *zone_name) {
    if (!profiler || !zone_name || profiler->zone_count >= MAX_PROFILE_ZONES) {
        return -1;
    }
    
    // Check if zone already exists
    if (get_zone(profiler, zone_name)) {
        return -1; // Zone already exists
    }
    
    ProfileZone *zone = &profiler->zones[profiler->zone_count++];
    strncpy(zone->name, zone_name, sizeof(zone->name) - 1);
    zone->name[sizeof(zone->name) - 1] = '\0';
    
    zone->timer_count = 0;
    zone->frame_count = 0;
    zone->current_frame = 0;
    
    zone->total_frame_time = 0;
    zone->min_frame_time = UINT64_MAX;
    zone->max_frame_time = 0;
    zone->avg_frame_time = 0.0f;
    
    zone->enabled = true;
    
    return profiler->zone_count - 1;
}

void physics_profiler_start_zone(PhysicsProfiler *profiler, const char *zone_name) {
    if (!profiler || !profiler->enabled || !zone_name) return;
    
    ProfileZone *zone = get_zone(profiler, zone_name);
    if (!zone || !zone->enabled) return;
    
    profiler->frame_start_time = get_high_res_time();
}

void physics_profiler_end_zone(PhysicsProfiler *profiler, const char *zone_name) {
    if (!profiler || !profiler->enabled || !zone_name) return;
    
    ProfileZone *zone = get_zone(profiler, zone_name);
    if (!zone || !zone->enabled) return;
    
    uint64_t end_time = get_high_res_time();
    uint64_t frame_time = end_time - profiler->frame_start_time;
    
    // Add to frame history
    if (zone->frame_count < MAX_FRAMES_HISTORY) {
        zone->frame_times[zone->frame_count++] = frame_time;
    } else {
        // Circular buffer
        zone->frame_times[zone->current_frame] = frame_time;
        zone->current_frame = (zone->current_frame + 1) % MAX_FRAMES_HISTORY;
    }
    
    // Update statistics
    update_frame_statistics(zone);
    
    // Update global FPS
    if (frame_time > 0) {
        profiler->current_fps = 1000000.0f / frame_time; // Convert from microseconds to FPS
        
        if (profiler->frame_count == 0) {
            profiler->min_fps = profiler->max_fps = profiler->avg_fps = profiler->current_fps;
        } else {
            profiler->min_fps = fminf(profiler->min_fps, profiler->current_fps);
            profiler->max_fps = fmaxf(profiler->max_fps, profiler->current_fps);
            profiler->avg_fps = (profiler->avg_fps * profiler->frame_count + profiler->current_fps) / 
                              (profiler->frame_count + 1);
        }
        profiler->frame_count++;
    }
    
    profiler->last_frame_time = frame_time;
    
    // Call frame callback if set
    if (profiler->frame_callback) {
        profiler->frame_callback(zone_name, frame_time, profiler->user_data);
    }
}

void physics_profiler_start_timer(PhysicsProfiler *profiler, const char *zone_name, 
                                   const char *timer_name) {
    if (!profiler || !profiler->enabled || !zone_name || !timer_name) return;
    
    ProfileZone *zone = get_zone(profiler, zone_name);
    if (!zone || !zone->enabled) return;
    
    ProfileTimer *timer = get_timer(zone, timer_name);
    if (!timer) {
        timer = create_timer(zone, timer_name);
        if (!timer) return;
    }
    
    timer->start_time = get_high_res_time();
    timer->active = true;
}

void physics_profiler_end_timer(PhysicsProfiler *profiler, const char *zone_name, 
                                 const char *timer_name) {
    if (!profiler || !profiler->enabled || !zone_name || !timer_name) return;
    
    ProfileZone *zone = get_zone(profiler, zone_name);
    if (!zone || !zone->enabled) return;
    
    ProfileTimer *timer = get_timer(zone, timer_name);
    if (!timer || !timer->active) return;
    
    uint64_t end_time = get_high_res_time();
    uint64_t elapsed = end_time - timer->start_time;
    
    timer->total_time += elapsed;
    timer->min_time = (elapsed < timer->min_time) ? elapsed : timer->min_time;
    timer->max_time = (elapsed > timer->max_time) ? elapsed : timer->max_time;
    timer->call_count++;
    timer->active = false;
}

int physics_profiler_create_memory_tracker(PhysicsProfiler *profiler, const char *tracker_name) {
    if (!profiler || !tracker_name || profiler->memory_tracker_count >= MAX_MEMORY_TRACKERS) {
        return -1;
    }
    
    MemoryTracker *tracker = &profiler->memory_trackers[profiler->memory_tracker_count++];
    strncpy(tracker->name, tracker_name, sizeof(tracker->name) - 1);
    tracker->name[sizeof(tracker->name) - 1] = '\0';
    
    tracker->total_allocated = 0;
    tracker->total_freed = 0;
    tracker->current_usage = 0;
    tracker->allocation_count = 0;
    tracker->free_count = 0;
    tracker->peak_usage = 0;
    
    return profiler->memory_tracker_count - 1;
}

void physics_profiler_track_allocation(PhysicsProfiler *profiler, int tracker_id, size_t size) {
    if (!profiler || tracker_id < 0 || tracker_id >= profiler->memory_tracker_count) return;
    
    MemoryTracker *tracker = &profiler->memory_trackers[tracker_id];
    
    tracker->total_allocated += size;
    tracker->current_usage += size;
    tracker->allocation_count++;
    
    if (tracker->current_usage > tracker->peak_usage) {
        tracker->peak_usage = tracker->current_usage;
    }
    
    // Update global memory usage
    profiler->total_memory_usage += size;
    if (profiler->total_memory_usage > profiler->peak_memory_usage) {
        profiler->peak_memory_usage = profiler->total_memory_usage;
    }
    
    // Call memory callback if set
    if (profiler->memory_callback) {
        profiler->memory_callback(tracker->name, size, true, profiler->user_data);
    }
}

void physics_profiler_track_free(PhysicsProfiler *profiler, int tracker_id, size_t size) {
    if (!profiler || tracker_id < 0 || tracker_id >= profiler->memory_tracker_count) return;
    
    MemoryTracker *tracker = &profiler->memory_trackers[tracker_id];
    
    tracker->total_freed += size;
    tracker->current_usage -= size;
    tracker->free_count++;
    
    // Update global memory usage
    if (profiler->total_memory_usage >= size) {
        profiler->total_memory_usage -= size;
    }
    
    // Call memory callback if set
    if (profiler->memory_callback) {
        profiler->memory_callback(tracker->name, size, false, profiler->user_data);
    }
}

void physics_profiler_set_frame_callback(PhysicsProfiler *profiler, ProfilerFrameCallback callback, 
                                        void *user_data) {
    if (profiler) {
        profiler->frame_callback = callback;
        profiler->user_data = user_data;
    }
}

void physics_profiler_set_memory_callback(PhysicsProfiler *profiler, ProfilerMemoryCallback callback, 
                                          void *user_data) {
    if (profiler) {
        profiler->memory_callback = callback;
        profiler->user_data = user_data;
    }
}

void physics_profiler_get_zone_stats(PhysicsProfiler *profiler, const char *zone_name, 
                                    ProfileZoneStats *stats) {
    if (!profiler || !zone_name || !stats) return;
    
    ProfileZone *zone = get_zone(profiler, zone_name);
    if (!zone) return;
    
    strncpy(stats->zone_name, zone->name, sizeof(stats->zone_name) - 1);
    stats->zone_name[sizeof(stats->zone_name) - 1] = '\0';
    
    stats->avg_frame_time = zone->avg_frame_time;
    stats->min_frame_time = (float)zone->min_frame_time / 1000.0f; // Convert to milliseconds
    stats->max_frame_time = (float)zone->max_frame_time / 1000.0f;
    stats->frame_count = zone->frame_count;
    stats->timer_count = zone->timer_count;
    
    // Copy timer statistics
    for (int i = 0; i < zone->timer_count && i < MAX_TIMER_STATS; i++) {
        ProfileTimer *timer = &zone->timers[i];
        
        strncpy(stats->timers[i].name, timer->name, sizeof(stats->timers[i].name) - 1);
        stats->timers[i].name[sizeof(stats->timers[i].name) - 1] = '\0';
        
        stats->timers[i].avg_time = timer->call_count > 0 ? 
                                   (float)timer->total_time / timer->call_count / 1000.0f : 0.0f;
        stats->timers[i].min_time = (float)timer->min_time / 1000.0f;
        stats->timers[i].max_time = (float)timer->max_time / 1000.0f;
        stats->timers[i].call_count = timer->call_count;
    }
}

void physics_profiler_get_memory_stats(PhysicsProfiler *profiler, const char *tracker_name, 
                                       ProfileMemoryStats *stats) {
    if (!profiler || !tracker_name || !stats) return;
    
    for (int i = 0; i < profiler->memory_tracker_count; i++) {
        MemoryTracker *tracker = &profiler->memory_trackers[i];
        if (strcmp(tracker->name, tracker_name) == 0) {
            strncpy(stats->tracker_name, tracker->name, sizeof(stats->tracker_name) - 1);
            stats->tracker_name[sizeof(stats->tracker_name) - 1] = '\0';
            
            stats->total_allocated = tracker->total_allocated;
            stats->total_freed = tracker->total_freed;
            stats->current_usage = tracker->current_usage;
            stats->peak_usage = tracker->peak_usage;
            stats->allocation_count = tracker->allocation_count;
            stats->free_count = tracker->free_count;
            
            return;
        }
    }
    
    // Tracker not found - zero out stats
    memset(stats, 0, sizeof(ProfileMemoryStats));
}

void physics_profiler_get_global_stats(PhysicsProfiler *profiler, ProfileGlobalStats *stats) {
    if (!profiler || !stats) return;
    
    stats->current_fps = profiler->current_fps;
    stats->avg_fps = profiler->avg_fps;
    stats->min_fps = profiler->min_fps;
    stats->max_fps = profiler->max_fps;
    
    stats->total_memory_usage = profiler->total_memory_usage;
    stats->peak_memory_usage = profiler->peak_memory_usage;
    
    stats->zone_count = profiler->zone_count;
    stats->memory_tracker_count = profiler->memory_tracker_count;
    
    stats->last_frame_time = (float)profiler->last_frame_time / 1000.0f; // Convert to milliseconds
    stats->total_profile_time = (float)profiler->total_profile_time / 1000000.0f; // Convert to seconds
}

void physics_profiler_reset(PhysicsProfiler *profiler) {
    if (!profiler) return;
    
    // Reset all zones
    for (int i = 0; i < profiler->zone_count; i++) {
        ProfileZone *zone = &profiler->zones[i];
        
        zone->frame_count = 0;
        zone->current_frame = 0;
        zone->total_frame_time = 0;
        zone->min_frame_time = UINT64_MAX;
        zone->max_frame_time = 0;
        zone->avg_frame_time = 0.0f;
        
        // Reset timers
        for (int j = 0; j < zone->timer_count; j++) {
            ProfileTimer *timer = &zone->timers[j];
            timer->total_time = 0;
            timer->min_time = UINT64_MAX;
            timer->max_time = 0;
            timer->call_count = 0;
            timer->active = false;
        }
    }
    
    // Reset memory trackers
    for (int i = 0; i < profiler->memory_tracker_count; i++) {
        MemoryTracker *tracker = &profiler->memory_trackers[i];
        tracker->total_allocated = 0;
        tracker->total_freed = 0;
        tracker->current_usage = 0;
        tracker->allocation_count = 0;
        tracker->free_count = 0;
        tracker->peak_usage = 0;
    }
    
    // Reset global statistics
    profiler->total_profile_time = 0;
    profiler->frame_start_time = 0;
    profiler->last_frame_time = 0;
    
    profiler->current_fps = 0.0f;
    profiler->avg_fps = 0.0f;
    profiler->min_fps = 0.0f;
    profiler->max_fps = 0.0f;
    profiler->frame_count = 0;
    
    profiler->total_memory_usage = 0;
    profiler->peak_memory_usage = 0;
}

void physics_profiler_export_data(PhysicsProfiler *profiler, const char *filename) {
    if (!profiler || !filename) return;
    
    FILE *file = fopen(filename, "w");
    if (!file) return;
    
    fprintf(file, "Physics Performance Profile Report\n");
    fprintf(file, "=====================================\n\n");
    
    // Global statistics
    ProfileGlobalStats global_stats;
    physics_profiler_get_global_stats(profiler, &global_stats);
    
    fprintf(file, "Global Statistics:\n");
    fprintf(file, "  Current FPS: %.2f\n", global_stats.current_fps);
    fprintf(file, "  Average FPS: %.2f\n", global_stats.avg_fps);
    fprintf(file, "  Min FPS: %.2f\n", global_stats.min_fps);
    fprintf(file, "  Max FPS: %.2f\n", global_stats.max_fps);
    fprintf(file, "  Total Memory Usage: %zu bytes\n", global_stats.total_memory_usage);
    fprintf(file, "  Peak Memory Usage: %zu bytes\n", global_stats.peak_memory_usage);
    fprintf(file, "  Last Frame Time: %.3f ms\n", global_stats.last_frame_time);
    fprintf(file, "\n");
    
    // Zone statistics
    for (int i = 0; i < profiler->zone_count; i++) {
        ProfileZone *zone = &profiler->zones[i];
        if (!zone->enabled) continue;
        
        fprintf(file, "Zone: %s\n", zone->name);
        fprintf(file, "  Average Frame Time: %.3f ms\n", zone->avg_frame_time / 1000.0f);
        fprintf(file, "  Min Frame Time: %.3f ms\n", (float)zone->min_frame_time / 1000.0f);
        fprintf(file, "  Max Frame Time: %.3f ms\n", (float)zone->max_frame_time / 1000.0f);
        fprintf(file, "  Frame Count: %d\n", zone->frame_count);
        
        fprintf(file, "  Timers:\n");
        for (int j = 0; j < zone->timer_count; j++) {
            ProfileTimer *timer = &zone->timers[j];
            float avg_time = timer->call_count > 0 ? 
                           (float)timer->total_time / timer->call_count / 1000.0f : 0.0f;
            
            fprintf(file, "    %s: %.3f ms avg, %.3f ms min, %.3f ms max, %u calls\n",
                   timer->name, avg_time, (float)timer->min_time / 1000.0f,
                   (float)timer->max_time / 1000.0f, timer->call_count);
        }
        fprintf(file, "\n");
    }
    
    // Memory statistics
    fprintf(file, "Memory Statistics:\n");
    for (int i = 0; i < profiler->memory_tracker_count; i++) {
        MemoryTracker *tracker = &profiler->memory_trackers[i];
        fprintf(file, "  %s:\n", tracker->name);
        fprintf(file, "    Total Allocated: %zu bytes\n", tracker->total_allocated);
        fprintf(file, "    Total Freed: %zu bytes\n", tracker->total_freed);
        fprintf(file, "    Current Usage: %zu bytes\n", tracker->current_usage);
        fprintf(file, "    Peak Usage: %zu bytes\n", tracker->peak_usage);
        fprintf(file, "    Allocations: %u\n", tracker->allocation_count);
        fprintf(file, "    Frees: %u\n", tracker->free_count);
    }
    
    fclose(file);
}
