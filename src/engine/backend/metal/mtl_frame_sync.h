/*
 * mtl_frame_sync.h
 * Advanced Metal frame synchronization and pacing
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MTL_FRAME_SYNC_H
#define MTL_FRAME_SYNC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <dispatch/dispatch.h>
#else
typedef void* id;
typedef void* dispatch_semaphore_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct metal_device metal_device_t;
typedef struct metal_command_buffer metal_command_buffer_t;

// ============================================================================
// Advanced Frame Pacing Manager
// ============================================================================

/**
 * Frame pacing configuration.
 */
typedef struct metal_frame_pacing_config {
    uint32_t target_fps;                   // Target frames per second (0 = unlimited)
    double frame_time_budget_ms;           // Frame time budget in milliseconds
    bool enable_vsync;                     // Enable VSync synchronization
    bool enable_adaptive_pacing;           // Enable adaptive frame rate
    uint32_t stats_window_size;            // Rolling statistics window size (frames)
    double adaptive_threshold_ms;          // GPU load threshold for adaptive pacing
} metal_frame_pacing_config_t;

/**
 * Frame timing statistics (rolling window).
 */
typedef struct metal_frame_timing_stats {
    double current_frame_time_ms;          // Current frame time
    double avg_frame_time_ms;              // Rolling average
    double min_frame_time_ms;              // Minimum in window
    double max_frame_time_ms;              // Maximum in window
    double percentile_95_ms;               // 95th percentile
    double percentile_99_ms;               // 99th percentile
    uint64_t total_frames;                 // Total frames rendered
    uint64_t dropped_frames;               // Frames exceeding budget
    uint32_t current_fps;                  // Current FPS estimate
} metal_frame_timing_stats_t;

/**
 * Frame pacing manager for advanced frame rate control.
 */
typedef struct metal_frame_pacer metal_frame_pacer_t;

/**
 * Create a frame pacing manager.
 * 
 * @param config Pacing configuration
 * @return Frame pacer or NULL on failure
 */
metal_frame_pacer_t* metal_frame_pacer_create(const metal_frame_pacing_config_t* config);

/**
 * Begin a new frame with the pacer.
 * Applies pacing delays as needed to meet target frame rate.
 * 
 * @param pacer Frame pacer
 * @return true if frame should be rendered, false if should skip
 */
bool metal_frame_pacer_begin_frame(metal_frame_pacer_t* pacer);

/**
 * End the current frame and record timing.
 * 
 * @param pacer Frame pacer
 * @param gpu_time_ms Optional GPU time in milliseconds (0 = use CPU time only)
 */
void metal_frame_pacer_end_frame(metal_frame_pacer_t* pacer, double gpu_time_ms);

/**
 * Get current frame timing statistics.
 * 
 * @param pacer Frame pacer
 * @param stats Output statistics structure
 */
void metal_frame_pacer_get_stats(metal_frame_pacer_t* pacer, metal_frame_timing_stats_t* stats);

/**
 * Reset statistics.
 * 
 * @param pacer Frame pacer
 */
void metal_frame_pacer_reset_stats(metal_frame_pacer_t* pacer);

/**
 * Update pacing configuration.
 * 
 * @param pacer Frame pacer
 * @param config New configuration
 */
void metal_frame_pacer_update_config(metal_frame_pacer_t* pacer, const metal_frame_pacing_config_t* config);

/**
 * Check if frame is within budget.
 * 
 * @param pacer Frame pacer
 * @return true if current frame is within time budget
 */
bool metal_frame_pacer_is_within_budget(metal_frame_pacer_t* pacer);

/**
 * Get recommended frame skip count based on current load.
 * For adaptive frame rate mode.
 * 
 * @param pacer Frame pacer
 * @return Number of frames to skip (0 = no skip)
 */
uint32_t metal_frame_pacer_get_skip_count(metal_frame_pacer_t* pacer);

/**
 * Destroy frame pacer.
 * 
 * @param pacer Frame pacer to destroy
 */
void metal_frame_pacer_destroy(metal_frame_pacer_t* pacer);

// ============================================================================
// Per-Frame Resource Tracking
// ============================================================================

/**
 * Per-frame resource data for triple buffering.
 */
typedef struct metal_frame_resources {
    uint64_t frame_index;                  // Frame index
    void** resources;                      // Array of resource pointers
    uint32_t resource_count;               // Number of resources
    uint32_t resource_capacity;            // Capacity of resources array
    bool in_use;                           // Frame is currently in use
} metal_frame_resources_t;

/**
 * Frame resource manager for managing per-frame resources.
 */
typedef struct metal_frame_resource_manager {
    metal_frame_resources_t* frames;       // Array of frame resources
    uint32_t max_frames_in_flight;         // Maximum concurrent frames
    uint32_t current_frame_index;          // Current frame index
} metal_frame_resource_manager_t;

/**
 * Create frame resource manager.
 * 
 * @param max_frames_in_flight Maximum frames in flight (typically 2-3)
 * @return Resource manager or NULL on failure
 */
metal_frame_resource_manager_t* metal_frame_resource_manager_create(uint32_t max_frames_in_flight);

/**
 * Get resources for current frame.
 * 
 * @param manager Resource manager
 * @return Current frame resources
 */
metal_frame_resources_t* metal_frame_resource_manager_get_current(metal_frame_resource_manager_t* manager);

/**
 * Advance to next frame.
 * 
 * @param manager Resource manager
 */
void metal_frame_resource_manager_advance(metal_frame_resource_manager_t* manager);

/**
 * Add resource to current frame.
 * 
 * @param manager Resource manager
 * @param resource Resource pointer to track
 */
void metal_frame_resource_manager_add_resource(metal_frame_resource_manager_t* manager, void* resource);

/**
 * Clear all resources from a frame.
 * 
 * @param manager Resource manager
 * @param frame_index Frame index to clear
 */
void metal_frame_resource_manager_clear_frame(metal_frame_resource_manager_t* manager, uint32_t frame_index);

/**
 * Destroy resource manager.
 * 
 * @param manager Resource manager to destroy
 */
void metal_frame_resource_manager_destroy(metal_frame_resource_manager_t* manager);

// ============================================================================
// Frame Drop Detection & Recovery
// ============================================================================

/**
 * Frame drop listener callback.
 * Called when a frame drop is detected.
 * 
 * @param frame_index Frame index that was dropped
 * @param exceeded_budget_ms How much the frame exceeded budget by
 * @param user_data User-provided data
 */
typedef void (*metal_frame_drop_callback_t)(uint64_t frame_index, double exceeded_budget_ms, void* user_data);

/**
 * Register frame drop callback.
 * 
 * @param pacer Frame pacer
 * @param callback Callback function
 * @param user_data User data to pass to callback
 */
void metal_frame_pacer_set_drop_callback(metal_frame_pacer_t* pacer, 
                                        metal_frame_drop_callback_t callback,
                                        void* user_data);

/**
 * Get frame drop rate (percentage).
 * 
 * @param pacer Frame pacer
 * @return Frame drop percentage (0.0 to 100.0)
 */
double metal_frame_pacer_get_drop_rate(metal_frame_pacer_t* pacer);

// ============================================================================
// VSync & Display Link Integration
// ============================================================================

/**
 * VSync callback function type.
 * Called at display refresh rate.
 * 
 * @param timestamp Display timestamp
 * @param user_data User-provided data
 */
typedef void (*metal_vsync_callback_t)(double timestamp, void* user_data);

/**
 * Enable VSync integration.
 * Note: Actual display link setup is platform-specific (CVDisplayLink on macOS).
 * 
 * @param pacer Frame pacer
 * @param enable Enable or disable VSync
 */
void metal_frame_pacer_set_vsync(metal_frame_pacer_t* pacer, bool enable);

/**
 * Set VSync callback for display synchronization.
 * 
 * @param pacer Frame pacer
 * @param callback Callback function
 * @param user_data User data
 */
void metal_frame_pacer_set_vsync_callback(metal_frame_pacer_t* pacer,
                                         metal_vsync_callback_t callback,
                                         void* user_data);

#ifdef __cplusplus
}
#endif

#endif // MTL_FRAME_SYNC_H
