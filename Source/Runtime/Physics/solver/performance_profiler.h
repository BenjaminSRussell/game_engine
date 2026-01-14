/**
 * @file performance_profiler.h
 * @brief Physics performance profiling system
 *
 * Provides comprehensive performance profiling for physics systems including
 * timing, memory usage, and performance metrics collection and analysis.
 */

#ifndef PERFORMANCE_PROFILER_H
#define PERFORMANCE_PROFILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Forward Declarations
// ========================================

typedef struct PhysicsProfiler PhysicsProfiler;

// ========================================
// Callback Types
// ========================================

/**
 * Frame completion callback
 * @param zone_name Name of the profile zone
 * @param frame_time Time spent in zone (microseconds)
 * @param user_data User-provided data pointer
 */
typedef void (*ProfilerFrameCallback)(const char *zone_name, uint64_t frame_time, void *user_data);

/**
 * Memory allocation callback
 * @param tracker_name Name of the memory tracker
 * @param size Size of allocation/deallocation
 * @param is_allocation True for allocation, false for deallocation
 * @param user_data User-provided data pointer
 */
typedef void (*ProfilerMemoryCallback)(const char *tracker_name, size_t size, bool is_allocation, void *user_data);

// ========================================
// Statistics Structures
// ========================================

#define MAX_TIMER_STATS 16

typedef struct ProfileTimerStats {
    char name[64];               // Timer name
    float avg_time;               // Average time (milliseconds)
    float min_time;               // Minimum time (milliseconds)
    float max_time;               // Maximum time (milliseconds)
    uint32_t call_count;          // Number of calls
    
} ProfileTimerStats;

typedef struct ProfileZoneStats {
    char zone_name[64];           // Zone name
    float avg_frame_time;         // Average frame time (milliseconds)
    float min_frame_time;         // Minimum frame time (milliseconds)
    float max_frame_time;         // Maximum frame time (milliseconds)
    int frame_count;              // Number of recorded frames
    int timer_count;              // Number of timers
    ProfileTimerStats timers[MAX_TIMER_STATS]; // Timer statistics
    
} ProfileZoneStats;

typedef struct ProfileMemoryStats {
    char tracker_name[64];        // Tracker name
    size_t total_allocated;       // Total allocated bytes
    size_t total_freed;           // Total freed bytes
    size_t current_usage;         // Current usage
    size_t peak_usage;            // Peak memory usage
    uint32_t allocation_count;    // Number of allocations
    uint32_t free_count;          // Number of frees
    
} ProfileMemoryStats;

typedef struct ProfileGlobalStats {
    float current_fps;            // Current frames per second
    float avg_fps;                // Average FPS
    float min_fps;                // Minimum FPS
    float max_fps;                // Maximum FPS
    
    size_t total_memory_usage;     // Total memory usage
    size_t peak_memory_usage;      // Peak memory usage
    
    int zone_count;               // Number of profile zones
    int memory_tracker_count;      // Number of memory trackers
    
    float last_frame_time;        // Last frame time (milliseconds)
    float total_profile_time;      // Total profiling time (seconds)
    
} ProfileGlobalStats;

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create physics performance profiler
 */
PhysicsProfiler* physics_profiler_create(void);

/**
 * Destroy physics profiler
 */
void physics_profiler_destroy(PhysicsProfiler *profiler);

// ========================================
// Profiler Control
// ========================================

/**
 * Enable or disable profiling
 */
void physics_profiler_enable(PhysicsProfiler *profiler, bool enabled);

/**
 * Check if profiler is enabled
 */
bool physics_profiler_is_enabled(PhysicsProfiler *profiler);

// ========================================
// Zone Management
// ========================================

/**
 * Create a new profile zone
 * @param profiler Profiler instance
 * @param zone_name Name of the zone
 * @return Zone ID or -1 on error
 */
int physics_profiler_create_zone(PhysicsProfiler *profiler, const char *zone_name);

/**
 * Start profiling a zone
 * @param profiler Profiler instance
 * @param zone_name Name of the zone
 */
void physics_profiler_start_zone(PhysicsProfiler *profiler, const char *zone_name);

/**
 * End profiling a zone
 * @param profiler Profiler instance
 * @param zone_name Name of the zone
 */
void physics_profiler_end_zone(PhysicsProfiler *profiler, const char *zone_name);

// ========================================
// Timer Management
// ========================================

/**
 * Start profiling a timer within a zone
 * @param profiler Profiler instance
 * @param zone_name Name of the zone
 * @param timer_name Name of the timer
 */
void physics_profiler_start_timer(PhysicsProfiler *profiler, const char *zone_name, 
                                   const char *timer_name);

/**
 * End profiling a timer
 * @param profiler Profiler instance
 * @param zone_name Name of the zone
 * @param timer_name Name of the timer
 */
void physics_profiler_end_timer(PhysicsProfiler *profiler, const char *zone_name, 
                                 const char *timer_name);

// ========================================
// Memory Profiling
// ========================================

/**
 * Create a memory tracker
 * @param profiler Profiler instance
 * @param tracker_name Name of the tracker
 * @return Tracker ID or -1 on error
 */
int physics_profiler_create_memory_tracker(PhysicsProfiler *profiler, const char *tracker_name);

/**
 * Track memory allocation
 * @param profiler Profiler instance
 * @param tracker_id Tracker ID
 * @param size Size of allocation
 */
void physics_profiler_track_allocation(PhysicsProfiler *profiler, int tracker_id, size_t size);

/**
 * Track memory deallocation
 * @param profiler Profiler instance
 * @param tracker_id Tracker ID
 * @param size Size of deallocation
 */
void physics_profiler_track_free(PhysicsProfiler *profiler, int tracker_id, size_t size);

// ========================================
// Callbacks
// ========================================

/**
 * Set frame completion callback
 * @param profiler Profiler instance
 * @param callback Callback function
 * @param user_data User data pointer
 */
void physics_profiler_set_frame_callback(PhysicsProfiler *profiler, ProfilerFrameCallback callback, 
                                        void *user_data);

/**
 * Set memory allocation callback
 * @param profiler Profiler instance
 * @param callback Callback function
 * @param user_data User data pointer
 */
void physics_profiler_set_memory_callback(PhysicsProfiler *profiler, ProfilerMemoryCallback callback, 
                                          void *user_data);

// ========================================
// Statistics Retrieval
// ========================================

/**
 * Get statistics for a specific zone
 * @param profiler Profiler instance
 * @param zone_name Name of the zone
 * @param stats Output statistics structure
 */
void physics_profiler_get_zone_stats(PhysicsProfiler *profiler, const char *zone_name, 
                                    ProfileZoneStats *stats);

/**
 * Get statistics for a memory tracker
 * @param profiler Profiler instance
 * @param tracker_name Name of the tracker
 * @param stats Output statistics structure
 */
void physics_profiler_get_memory_stats(PhysicsProfiler *profiler, const char *tracker_name, 
                                       ProfileMemoryStats *stats);

/**
 * Get global statistics
 * @param profiler Profiler instance
 * @param stats Output statistics structure
 */
void physics_profiler_get_global_stats(PhysicsProfiler *profiler, ProfileGlobalStats *stats);

// ========================================
// Data Management
// ========================================

/**
 * Reset all profiling statistics
 * @param profiler Profiler instance
 */
void physics_profiler_reset(PhysicsProfiler *profiler);

/**
 * Export profiling data to file
 * @param profiler Profiler instance
 * @param filename Output filename
 */
void physics_profiler_export_data(PhysicsProfiler *profiler, const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* PERFORMANCE_PROFILER_H */
