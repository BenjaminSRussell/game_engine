/*
 * mtl_swapchain.h
 * Metal swapchain (CAMetalLayer) integration
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_SWAPCHAIN_H
#define PLATFORM_MTL_SWAPCHAIN_H

#include "backend/metal/mtl_device.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <QuartzCore/CAMetalLayer.h>
#import <Cocoa/Cocoa.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

/**
 * Present mode for controlling vsync and frame pacing.
 */
typedef enum metal_present_mode {
    METAL_PRESENT_MODE_VSYNC,      // Wait for vsync (displaySyncEnabled = YES)
    METAL_PRESENT_MODE_IMMEDIATE,  // Present immediately (displaySyncEnabled = NO)
    METAL_PRESENT_MODE_ADAPTIVE    // Adaptive vsync based on frame time
} metal_present_mode_t;

/**
 * Swapchain statistics for performance monitoring.
 */
typedef struct metal_swapchain_stats {
    uint64_t total_frames;          // Total frames presented
    uint64_t dropped_frames;        // Frames that failed to acquire drawable
    double avg_frame_time_ms;       // Average frame time in milliseconds
    double last_frame_time_ms;      // Last frame time
    double drawable_acquire_time_ms; // Time spent acquiring drawable
} metal_swapchain_stats_t;

#ifdef __OBJC__
typedef struct metal_swapchain {
    CAMetalLayer* layer;
    id<CAMetalDrawable> current_drawable;
    MTLPixelFormat pixel_format;
    CGSize size;
    bool hdr_enabled;
    
    // Present mode and timing
    metal_present_mode_t present_mode;
    double drawable_timeout;        // Timeout for nextDrawable (seconds)
    
    // Statistics
    metal_swapchain_stats_t stats;
    uint64_t frame_start_time;      // Mach absolute time for frame timing
} metal_swapchain_t;
#else
typedef struct metal_swapchain metal_swapchain_t;
#endif

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Creates a Metal swapchain backed by a CAMetalLayer for the given NSView.
 * @param dev The Metal device to use.
 * @param ns_view Pointer to the NSView (void* for C compatibility).
 * @param hdr If true, configures for HDR output (P3 color space, Float16 format).
 * @return A new swapchain instance or NULL on failure.
 */
metal_swapchain_t* metal_swapchain_create(metal_device_t* dev, void* ns_view, bool hdr);

/**
 * Destroys the swapchain.
 * @param swap The swapchain to destroy.
 */
void metal_swapchain_destroy(metal_swapchain_t* swap);

/**
 * Updates the swapchain size. Should be called when the window checks resize.
 * @param swap The swapchain to resize.
 * @param width New width in points (or pixels if layer scaling is set).
 * @param height New height.
 */
void metal_swapchain_resize(metal_swapchain_t* swap, uint32_t width, uint32_t height);

/**
 * Acquires the next drawable for rendering.
 * @param swap The swapchain.
 * @return The MTLTexture for the next frame (void* for C).
 */
#ifdef __OBJC__
id<MTLTexture> metal_swapchain_get_texture(metal_swapchain_t* swap);
#else
void* metal_swapchain_get_texture(metal_swapchain_t* swap);
#endif

/**
 * Presents the current drawable.
 * @param swap The swapchain.
 * @param cmd The command buffer that rendered to the drawable (to schedule presentation).
 */
#ifdef __OBJC__
void metal_swapchain_present(metal_swapchain_t* swap, id<MTLCommandBuffer> cmd);
#else
void metal_swapchain_present(metal_swapchain_t* swap, void* cmd);
#endif

/**
 * Set the present mode (vsync, immediate, adaptive).
 * @param swap The swapchain.
 * @param mode Present mode to set.
 */
void metal_swapchain_set_present_mode(metal_swapchain_t* swap, metal_present_mode_t mode);

/**
 * Get current present mode.
 * @param swap The swapchain.
 * @return Current present mode.
 */
metal_present_mode_t metal_swapchain_get_present_mode(const metal_swapchain_t* swap);

/**
 * Set drawable acquisition timeout.
 * @param swap The swapchain.
 * @param timeout_seconds Timeout in seconds (0 = infinite wait).
 */
void metal_swapchain_set_drawable_timeout(metal_swapchain_t* swap, double timeout_seconds);

/**
 * Get swapchain statistics.
 * @param swap The swapchain.
 * @param stats Pointer to stats structure to fill.
 */
void metal_swapchain_get_statistics(const metal_swapchain_t* swap, metal_swapchain_stats_t* stats);

/**
 * Reset swapchain statistics.
 * @param swap The swapchain.
 */
void metal_swapchain_reset_statistics(metal_swapchain_t* swap);

/**
 * Begin frame timing measurement.
 * @param swap The swapchain.
 */
void metal_swapchain_begin_frame(metal_swapchain_t* swap);

/**
 * End frame timing measurement.
 * @param swap The swapchain.
 */
void metal_swapchain_end_frame(metal_swapchain_t* swap);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_SWAPCHAIN_H */
