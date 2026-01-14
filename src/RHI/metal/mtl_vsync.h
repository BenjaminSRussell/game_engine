/*
 * mtl_vsync.h
 * VSync integration using CVDisplayLink for macOS
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MTL_VSYNC_H
#define MTL_VSYNC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct metal_vsync_manager metal_vsync_manager_t;

/**
 * VSync callback function type.
 * Called at display refresh rate.
 * 
 * @param timestamp Display timestamp in seconds
 * @param frame_duration Duration since last frame in seconds
 * @param user_data User-provided data
 */
typedef void (*metal_vsync_callback_t)(double timestamp, double frame_duration, void* user_data);

/**
 * VSync configuration.
 */
typedef struct metal_vsync_config {
    uint32_t display_id;              // Display ID (0 = main display)
    bool enable_adaptive_sync;        // Enable adaptive sync (VRR)
    double target_frame_rate;         // Target frame rate (0 = use display refresh rate)
} metal_vsync_config_t;

/**
 * VSync statistics.
 */
typedef struct metal_vsync_stats {
    double refresh_rate_hz;           // Display refresh rate
    double actual_frame_time_ms;      // Actual frame time
    uint64_t total_vblanks;           // Total VBlanks received
    uint64_t missed_vblanks;          // Missed VBlanks
    double jitter_ms;                 // Frame time jitter (stddev)
} metal_vsync_stats_t;

/**
 * Create VSync manager.
 * 
 * @param config VSync configuration
 * @return VSync manager or NULL on failure
 */
metal_vsync_manager_t* metal_vsync_manager_create(const metal_vsync_config_t* config);

/**
 * Set VSync callback.
 * 
 * @param manager VSync manager
 * @param callback Callback function
 * @param user_data User data to pass to callback
 */
void metal_vsync_manager_set_callback(metal_vsync_manager_t* manager,
                                     metal_vsync_callback_t callback,
                                     void* user_data);

/**
 * Start VSync notifications.
 * 
 * @param manager VSync manager
 * @return true if started successfully
 */
bool metal_vsync_manager_start(metal_vsync_manager_t* manager);

/**
 * Stop VSync notifications.
 * 
 * @param manager VSync manager
 */
void metal_vsync_manager_stop(metal_vsync_manager_t* manager);

/**
 * Get VSync statistics.
 * 
 * @param manager VSync manager
 * @param stats Output statistics
 */
void metal_vsync_manager_get_stats(metal_vsync_manager_t* manager,
                                  metal_vsync_stats_t* stats);

/**
 * Reset VSync statistics.
 * 
 * @param manager VSync manager
 */
void metal_vsync_manager_reset_stats(metal_vsync_manager_t* manager);

/**
 * Get current display refresh rate.
 * 
 * @param manager VSync manager
 * @return Refresh rate in Hz
 */
double metal_vsync_manager_get_refresh_rate(metal_vsync_manager_t* manager);

/**
 * Wait for next VBlank (blocking).
 * 
 * @param manager VSync manager
 * @param timeout_ms Timeout in milliseconds (0 = infinite)
 * @return true if VBlank occurred, false if timeout
 */
bool metal_vsync_manager_wait_for_vblank(metal_vsync_manager_t* manager,
                                        uint32_t timeout_ms);

/**
 * Destroy VSync manager.
 * 
 * @param manager VSync manager to destroy
 */
void metal_vsync_manager_destroy(metal_vsync_manager_t* manager);

#ifdef __cplusplus
}
#endif

#endif // MTL_VSYNC_H
