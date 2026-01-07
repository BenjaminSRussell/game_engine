/*
 * mtl_sync_manager.h
 * Unified Metal synchronization manager
 * Coordinates frame sync, hazard tracking, timestamps, and VSync
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MTL_SYNC_MANAGER_H
#define MTL_SYNC_MANAGER_H

#include "mtl_sync.h"
#include "mtl_frame_sync.h"
#include "mtl_hazard_tracking.h"
#include "mtl_timestamp.h"
#include "mtl_vsync.h"

#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Unified Synchronization Manager
// ============================================================================

/**
 * Unified synchronization manager configuration.
 */
typedef struct metal_sync_manager_config {
    uint32_t max_frames_in_flight;            // Frames in flight (2-3)
    uint32_t target_fps;                      // Target frame rate
    bool enable_vsync;                        // Enable VSync
    bool enable_hazard_tracking;              // Enable automatic hazard detection
    bool enable_gpu_timestamps;               // Enable GPU profiling
    metal_barrier_policy_t barrier_policy;    // Barrier insertion policy
} metal_sync_manager_config_t;

/**
 * Unified synchronization manager.
 */
typedef struct metal_sync_manager metal_sync_manager_t;

/**
 * Create unified synchronization manager.
 * 
 * @param device Metal device
 * @param config Manager configuration
 * @return Synchronization manager or NULL on failure
 */
#ifdef __OBJC__
metal_sync_manager_t* metal_sync_manager_create(id<MTLDevice> device,
                                                const metal_sync_manager_config_t* config);
#else
metal_sync_manager_t* metal_sync_manager_create(id device,
                                                const metal_sync_manager_config_t* config);
#endif

/**
 * Begin a new frame with full synchronization.
 * 
 * @param manager Synchronization manager
 * @return true if frame should be rendered
 */
bool metal_sync_manager_begin_frame(metal_sync_manager_t* manager);

/**
 * End the current frame and submit command buffer.
 * 
 * @param manager Synchronization manager
 * @param cmd_buffer Command buffer to submit
 */
void metal_sync_manager_end_frame(metal_sync_manager_t* manager,
                                  metal_command_buffer_t* cmd_buffer);

/**
 * Get frame synchronization object.
 * 
 * @param manager Synchronization manager
 * @return Frame synchronization object
 */
metal_frame_sync_t* metal_sync_manager_get_frame_sync(metal_sync_manager_t* manager);

/**
 * Get frame pacer.
 * 
 * @param manager Synchronization manager
 * @return Frame pacer
 */
metal_frame_pacer_t* metal_sync_manager_get_frame_pacer(metal_sync_manager_t* manager);

/**
 * Get barrier manager.
 * 
 * @param manager Synchronization manager
 * @return Barrier manager
 */
metal_barrier_manager_t* metal_sync_manager_get_barrier_manager(metal_sync_manager_t* manager);

/**
 * Get timestamp pool.
 * 
 * @param manager Synchronization manager
 * @return Timestamp pool
 */
metal_timestamp_pool_t* metal_sync_manager_get_timestamp_pool(metal_sync_manager_t* manager);

/**
 * Get VSync manager.
 * 
 * @param manager Synchronization manager
 * @return VSync manager
 */
metal_vsync_manager_t* metal_sync_manager_get_vsync_manager(metal_sync_manager_t* manager);

/**
 * Get current frame index.
 * 
 * @param manager Synchronization manager
 * @return Current frame index
 */
uint64_t metal_sync_manager_get_frame_index(metal_sync_manager_t* manager);

/**
 * Get comprehensive synchronization statistics.
 * 
 * @param manager Synchronization manager
 * @param frame_stats Output frame statistics
 * @param hazard_report Output hazard report
 * @param vsync_stats Output VSync statistics
 */
void metal_sync_manager_get_stats(metal_sync_manager_t* manager,
                                  metal_frame_timing_stats_t* frame_stats,
                                  metal_hazard_report_t* hazard_report,
                                  metal_vsync_stats_t* vsync_stats);

/**
 * Reset all statistics.
 * 
 * @param manager Synchronization manager
 */
void metal_sync_manager_reset_stats(metal_sync_manager_t* manager);

/**
 * Print comprehensive statistics report.
 * 
 * @param manager Synchronization manager
 */
void metal_sync_manager_print_report(metal_sync_manager_t* manager);

/**
 * Destroy synchronization manager.
 * 
 * @param manager Synchronization manager to destroy
 */
void metal_sync_manager_destroy(metal_sync_manager_t* manager);

#ifdef __cplusplus
}
#endif

#endif // MTL_SYNC_MANAGER_H
