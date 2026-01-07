/*
 * mtl_swapchain.h
 * Metal swapchain (CAMetalLayer) integration
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_SWAPCHAIN_H
#define PLATFORM_MTL_SWAPCHAIN_H

#include "mtl_device.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <QuartzCore/CAMetalLayer.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
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

/* ============================================================================
 * ADVANCED LAYER CONFIGURATION
 * ============================================================================ */

/**
 * Set maximum drawable count (double or triple buffering).
 * @param swap The swapchain.
 * @param count Number of drawables (2 or 3 recommended).
 */
void metal_swapchain_set_max_drawable_count(metal_swapchain_t* swap, uint32_t count);

/**
 * Get current maximum drawable count.
 * @param swap The swapchain.
 * @return Maximum drawable count.
 */
uint32_t metal_swapchain_get_max_drawable_count(const metal_swapchain_t* swap);

/**
 * Set whether the layer's textures are used only for rendering.
 * @param swap The swapchain.
 * @param framebuffer_only If true, drawables can't be read by CPU or used in compute.
 */
void metal_swapchain_set_framebuffer_only(metal_swapchain_t* swap, bool framebuffer_only);

/**
 * Get content scale factor (for Retina displays).
 * @param swap The swapchain.
 * @return Scale factor (1.0 for non-Retina, 2.0 for Retina).
 */
float metal_swapchain_get_content_scale(const metal_swapchain_t* swap);

/**
 * Set layer debug label for Metal Frame Capture.
 * @param swap The swapchain.
 * @param label Debug label string.
 */
void metal_swapchain_set_debug_label(metal_swapchain_t* swap, const char* label);

/**
 * Enable/disable layer opacity.
 * @param swap The swapchain.
 * @param opaque If true, layer is fully opaque (optimization hint).
 */
void metal_swapchain_set_opaque(metal_swapchain_t* swap, bool opaque);

/**
 * Validate swapchain configuration.
 * @param swap The swapchain.
 * @return true if configuration is valid, false otherwise.
 */
bool metal_swapchain_validate(const metal_swapchain_t* swap);

/**
 * Get layer bounds in points.
 * @param swap The swapchain.
 * @param width Output width.
 * @param height Output height.
 */
void metal_swapchain_get_bounds(const metal_swapchain_t* swap, float* width, float* height);

/**
 * Get drawable size in pixels.
 * @param swap The swapchain.
 * @param width Output width.
 * @param height Output height.
 */
void metal_swapchain_get_drawable_size(const metal_swapchain_t* swap, uint32_t* width, uint32_t* height);

/* ============================================================================
 * ENHANCED DRAWABLE MANAGEMENT
 * ============================================================================ */

/**
 * Get number of available drawables in the pool.
 * @param swap The swapchain.
 * @return Number of drawables available for acquisition.
 */
uint32_t metal_swapchain_get_available_drawable_count(const metal_swapchain_t* swap);

/**
 * Check if a drawable is currently available.
 * @param swap The swapchain.
 * @return true if nextDrawable would succeed without blocking.
 */
bool metal_swapchain_has_available_drawable(const metal_swapchain_t* swap);

/**
 * Present drawable with scheduling options.
 * @param swap The swapchain.
 * @param cmd The command buffer.
 * @param present_at_time If > 0, present at specific time (CVTimeStamp).
 */
#ifdef __OBJC__
void metal_swapchain_present_at_time(metal_swapchain_t* swap, id<MTLCommandBuffer> cmd, double present_at_time);
#else
void metal_swapchain_present_at_time(metal_swapchain_t* swap, void* cmd, double present_at_time);
#endif

/**
 * Present drawable after minimum duration.
 * @param swap The swapchain.
 * @param cmd The command buffer.
 * @param duration Minimum duration in seconds before presenting.
 */
#ifdef __OBJC__
void metal_swapchain_present_after_minimum_duration(metal_swapchain_t* swap, id<MTLCommandBuffer> cmd, double duration);
#else
void metal_swapchain_present_after_minimum_duration(metal_swapchain_t* swap, void* cmd, double duration);
#endif

/**
 * Drawable presentation callback.
 * @param drawable_id Unique identifier for the drawable.
 * @param present_time Time when drawable was presented.
 * @param user_data User-provided data.
 */
typedef void (*metal_drawable_presented_callback_t)(uint64_t drawable_id, double present_time, void* user_data);

/**
 * Register callback for when drawable is presented.
 * @param swap The swapchain.
 * @param callback Callback function.
 * @param user_data User data passed to callback.
 */
void metal_swapchain_set_present_callback(metal_swapchain_t* swap, metal_drawable_presented_callback_t callback, void* user_data);

/**
 * Get the presented time of the last drawable.
 * @param swap The swapchain.
 * @return Time in seconds when last drawable was presented to screen.
 */
double metal_swapchain_get_last_presented_time(const metal_swapchain_t* swap);

/**
 * Flush the drawable queue (wait for all pending drawables to complete).
 * @param swap The swapchain.
 */
void metal_swapchain_flush(metal_swapchain_t* swap);

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

/* ============================================================================
 * VARIABLE REFRESH RATE (VRR) / PROMOTION SUPPORT
 * ============================================================================ */

/**
 * Set target frame rate for adaptive sync.
 * @param swap The swapchain.
 * @param target_fps Target frames per second (e.g., 30, 60, 120).
 */
void metal_swapchain_set_target_framerate(metal_swapchain_t* swap, uint32_t target_fps);

/**
 * Get current target frame rate.
 * @param swap The swapchain.
 * @return Target FPS.
 */
uint32_t metal_swapchain_get_target_framerate(const metal_swapchain_t* swap);

/**
 * Enable low latency mode (reduces buffering).
 * @param swap The swapchain.
 * @param enable If true, optimize for latency over throughput.
 */
void metal_swapchain_set_low_latency_mode(metal_swapchain_t* swap, bool enable);

/**
 * Get display refresh rate that swapchain is synchronized to.
 * @param swap The swapchain.
 * @return Display refresh rate in Hz.
 */
double metal_swapchain_get_display_refresh_rate(const metal_swapchain_t* swap);

/**
 * Check if display supports variable refresh rate.
 * @param swap The swapchain.
 * @return true if VRR/ProMotion is available.
 */
bool metal_swapchain_supports_vrr(const metal_swapchain_t* swap);

/* ============================================================================
 * PERFORMANCE MONITORING
 * ============================================================================ */

/**
 * Get detailed performance metrics.
 * @param swap The swapchain.
 * @param out_metrics Output structure for metrics.
 */
typedef struct metal_swapchain_performance {
    double fps;                          // Current frames per second
    double frame_time_min_ms;            // Minimum frame time
    double frame_time_max_ms;            // Maximum frame time
    double frame_time_std_dev_ms;        // Standard deviation
    uint64_t total_presents;             // Total presents since creation
    uint64_t dropped_drawables;          // Drawables that timed out
    double avg_drawable_wait_ms;         // Average time waiting for drawable
} metal_swapchain_performance_t;

void metal_swapchain_get_performance(const metal_swapchain_t* swap, metal_swapchain_performance_t* out_metrics);

/**
 * Capture a screenshot of the current drawable.
 * @param swap The swapchain.
 * @param output_path File path to save screenshot (PNG format).
 * @return true on success, false on failure.
 */
bool metal_swapchain_capture_screenshot(metal_swapchain_t* swap, const char* output_path);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_SWAPCHAIN_H */
