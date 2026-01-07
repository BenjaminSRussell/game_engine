/*
 * mtl_swapchain.c
 * Metal swapchain (CAMetalLayer) implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_swapchain.h"
#include "mtl_display.h"
#include <stdlib.h>
#include <stdio.h>
#include <mach/mach_time.h>
#include <string.h>
#include <math.h>

#ifdef __OBJC__

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

/* 
 * Local definition of metal_device struct to access its internal MTLDevice.
 * This MUST match the layout in mtl_device.c
 */
struct metal_device {
    id<MTLDevice> device;              // __bridge retained
    id<MTLCommandQueue> command_queue; // __bridge retained
    MTLSize max_threads_per_group;
    uint64_t recommended_working_set_size;
    bool supports_raytracing;
    bool supports_mesh_shaders;
};

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

static double mach_time_to_ms(uint64_t mach_time) {
    static mach_timebase_info_data_t timebase = {0, 0};
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }
    return (double)mach_time * timebase.numer / timebase.denom / 1000000.0;
}

static uint64_t get_current_mach_time(void) {
    return mach_absolute_time();
}

/* ============================================================================
 * API IMPLEMENTATION
 * ============================================================================ */

metal_swapchain_t* metal_swapchain_create(metal_device_t* dev, void* ns_view, bool hdr) {
    if (!dev || !ns_view) return NULL;
    
    metal_swapchain_t* swap = (metal_swapchain_t*)calloc(1, sizeof(metal_swapchain_t));
    if (!swap) return NULL;

    struct metal_device* internal_dev = (struct metal_device*)dev;
    id<MTLDevice> mtl_device = internal_dev->device;

    if (!mtl_device) {
        free(swap);
        return NULL;
    }

    // Configure Layer
    swap->layer = [CAMetalLayer layer];
    swap->layer.device = mtl_device;
    swap->layer.pixelFormat = hdr ? MTLPixelFormatRGBA16Float : MTLPixelFormatBGRA8Unorm;
    swap->layer.framebufferOnly = YES; // Set to NO if you need to read back or use as compute target
    swap->layer.displaySyncEnabled = YES; // V-Sync
    
    // HDR Configuration
    if (hdr) {
        if (@available(macOS 10.11, *)) {
            swap->layer.wantsExtendedDynamicRangeContent = YES;
            swap->layer.colorspace = CGColorSpaceCreateWithName(kCGColorSpaceExtendedLinearDisplayP3);
        } else {
             // Fallback for older macOS if necessary (though engine likely targets 10.15+)
            swap->layer.colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        }
    } else {
        swap->layer.colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    }

    // Attach to NSView
    NSView* view = (__bridge NSView*)ns_view;
    [view setLayer:swap->layer];
    [view setWantsLayer:YES];

    // Initial size
    swap->size = view.bounds.size;
    swap->layer.drawableSize = [view convertSizeToBacking:swap->size];
    
    swap->hdr_enabled = hdr;
    swap->pixel_format = swap->layer.pixelFormat;
    
    // Initialize present mode and timing
    swap->present_mode = METAL_PRESENT_MODE_VSYNC;
    swap->drawable_timeout = 1.0; // 1 second default timeout
    
    // Initialize statistics
    memset(&swap->stats, 0, sizeof(metal_swapchain_stats_t));
    swap->frame_start_time = 0;
    
    printf("Metal Swapchain Created:\n");
    printf("  - Size: %.0f x %.0f\n", swap->size.width, swap->size.height);
    printf("  - HDR: %s\n", hdr ? "YES" : "NO");
    printf("  - Pixel Format: %lu\n", (unsigned long)swap->pixel_format);
    printf("  - Present Mode: VSYNC\n");
    printf("  - Drawable Timeout: %.2f seconds\n", swap->drawable_timeout);

    return swap;
}

void metal_swapchain_destroy(metal_swapchain_t* swap) {
    if (!swap) return;

    if (swap->layer) {
        swap->layer.device = nil;
        swap->layer = nil;
    }
    
    swap->current_drawable = nil;
    
    free(swap);
}

void metal_swapchain_resize(metal_swapchain_t* swap, uint32_t width, uint32_t height) {
    if (!swap || !swap->layer) return;

    // width/height passed here might be usually points or pixels depending on caller.
    // Assuming the engine handles window events and passes points, but for exact pixel control we set drawableSize.
    // If width/height are pixels (framebuffer size), we set drawableSize directly.
    
    CGSize new_size = CGSizeMake((CGFloat)width, (CGFloat)height);
    swap->layer.drawableSize = new_size;
    swap->size = new_size; // Store pixel size or point size? struct says just CGSize.
}

id<MTLTexture> metal_swapchain_get_texture(metal_swapchain_t* swap) {
    if (!swap || !swap->layer) return nil;

    uint64_t acquire_start = get_current_mach_time();
    
    swap->current_drawable = [swap->layer nextDrawable];
    
    uint64_t acquire_end = get_current_mach_time();
    swap->stats.drawable_acquire_time_ms = mach_time_to_ms(acquire_end - acquire_start);
    
    if (!swap->current_drawable) {
        swap->stats.dropped_frames++;
        return nil;
    }
    
    return swap->current_drawable.texture;
}

void metal_swapchain_present(metal_swapchain_t* swap, id<MTLCommandBuffer> cmd) {
    if (!swap || !swap->current_drawable || !cmd) return;

    [cmd presentDrawable:swap->current_drawable];
    // Release drawable reference after command buffer consumes it? 
    // ARC keeps it alive until command buffer is committed and scheduled.
    // We clear our strong reference to it for the next frame.
    swap->current_drawable = nil;
    
    swap->stats.total_frames++;
}

/* ============================================================================
 * PRESENT MODE AND CONFIGURATION
 * ============================================================================ */

void metal_swapchain_set_present_mode(metal_swapchain_t* swap, metal_present_mode_t mode) {
    if (!swap || !swap->layer) return;
    
    swap->present_mode = mode;
    
    switch (mode) {
        case METAL_PRESENT_MODE_VSYNC:
            swap->layer.displaySyncEnabled = YES;
            printf("Swapchain: Present mode set to VSYNC\n");
            break;
        case METAL_PRESENT_MODE_IMMEDIATE:
            swap->layer.displaySyncEnabled = NO;
            printf("Swapchain: Present mode set to IMMEDIATE\n");
            break;
        case METAL_PRESENT_MODE_ADAPTIVE:
            // Adaptive mode - application needs to manage this based on frame timing
            swap->layer.displaySyncEnabled = YES; // Start with vsync, adapt later
            printf("Swapchain: Present mode set to ADAPTIVE\n");
            break;
    }
}

metal_present_mode_t metal_swapchain_get_present_mode(const metal_swapchain_t* swap) {
    return swap ? swap->present_mode : METAL_PRESENT_MODE_VSYNC;
}

void metal_swapchain_set_drawable_timeout(metal_swapchain_t* swap, double timeout_seconds) {
    if (!swap) return;
    swap->drawable_timeout = timeout_seconds;
}

/* ============================================================================
 * STATISTICS AND TIMING
 * ============================================================================ */

void metal_swapchain_get_statistics(const metal_swapchain_t* swap, metal_swapchain_stats_t* stats) {
    if (!swap || !stats) return;
    *stats = swap->stats;
}

void metal_swapchain_reset_statistics(metal_swapchain_t* swap) {
    if (!swap) return;
    memset(&swap->stats, 0, sizeof(metal_swapchain_stats_t));
}

void metal_swapchain_begin_frame(metal_swapchain_t* swap) {
    if (!swap) return;
    swap->frame_start_time = get_current_mach_time();
}

void metal_swapchain_end_frame(metal_swapchain_t* swap) {
    if (!swap) return;
    
    if (swap->frame_start_time == 0) return;
    
    uint64_t frame_end = get_current_mach_time();
    double frame_time_ms = mach_time_to_ms(frame_end - swap->frame_start_time);
    
    // Update statistics
    swap->stats.last_frame_time_ms = frame_time_ms;
    
    // Calculate rolling average
    if (swap->stats.total_frames == 0) {
        swap->stats.avg_frame_time_ms = frame_time_ms;
    } else {
        // Exponential moving average (alpha = 0.1)
        swap->stats.avg_frame_time_ms = 0.9 * swap->stats.avg_frame_time_ms + 0.1 * frame_time_ms;
    }
    
    swap->frame_start_time = 0;
}

/* ============================================================================
 * ADVANCED LAYER CONFIGURATION
 * ============================================================================ */

void metal_swapchain_set_max_drawable_count(metal_swapchain_t* swap, uint32_t count) {
    if (!swap || !swap->layer) return;
    
    if (@available(macOS 10.13.2, *)) {
        swap->layer.maximumDrawableCount = count;
        printf("Swapchain: Set maximum drawable count to %u\n", count);
    }
}

uint32_t metal_swapchain_get_max_drawable_count(const metal_swapchain_t* swap) {
    if (!swap || !swap->layer) return 0;
    
    if (@available(macOS 10.13.2, *)) {
        return (uint32_t)swap->layer.maximumDrawableCount;
    }
    return 3; // Default
}

void metal_swapchain_set_framebuffer_only(metal_swapchain_t* swap, bool framebuffer_only) {
    if (!swap || !swap->layer) return;
    swap->layer.framebufferOnly = framebuffer_only ? YES : NO;
}

float metal_swapchain_get_content_scale(const metal_swapchain_t* swap) {
    if (!swap || !swap->layer) return 1.0f;
    return (float)swap->layer.contentsScale;
}

void metal_swapchain_set_debug_label(metal_swapchain_t* swap, const char* label) {
    if (!swap || !swap->layer || !label) return;
    swap->layer.name = [NSString stringWithUTF8String:label];
}

void metal_swapchain_set_opaque(metal_swapchain_t* swap, bool opaque) {
    if (!swap || !swap->layer) return;
    swap->layer.opaque = opaque ? YES : NO;
}

bool metal_swapchain_validate(const metal_swapchain_t* swap) {
    if (!swap) return false;
    if (!swap->layer) return false;
    if (!swap->layer.device) return false;
    if (swap->layer.drawableSize.width <= 0 || swap->layer.drawableSize.height <= 0) return false;
    return true;
}

void metal_swapchain_get_bounds(const metal_swapchain_t* swap, float* width, float* height) {
    if (!swap || !swap->layer || !width || !height) return;
    CGRect bounds = swap->layer.bounds;
    *width = (float)bounds.size.width;
    *height = (float)bounds.size.height;
}

void metal_swapchain_get_drawable_size(const metal_swapchain_t* swap, uint32_t* width, uint32_t* height) {
    if (!swap || !swap->layer || !width || !height) return;
    CGSize size = swap->layer.drawableSize;
    *width = (uint32_t)size.width;
    *height = (uint32_t)size.height;
}

/* ============================================================================
 * ENHANCED DRAWABLE MANAGEMENT
 * ============================================================================ */

uint32_t metal_swapchain_get_available_drawable_count(const metal_swapchain_t* swap) {
    if (!swap || !swap->layer) return 0;
    
    // Note: Metal doesn't expose this directly, so we estimate based on max count
    // and whether we currently have a drawable
    uint32_t max_count = metal_swapchain_get_max_drawable_count(swap);
    return swap->current_drawable ? max_count - 1 : max_count;
}

bool metal_swapchain_has_available_drawable(const metal_swapchain_t* swap) {
    return metal_swapchain_get_available_drawable_count(swap) > 0;
}

void metal_swapchain_present_at_time(metal_swapchain_t* swap, id<MTLCommandBuffer> cmd, double present_at_time) {
    if (!swap || !swap->current_drawable || !cmd) return;
    
    if (@available(macOS 10.15.4, *)) {
        if (present_at_time > 0.0) {
            [cmd presentDrawable:swap->current_drawable atTime:present_at_time];
        } else {
            [cmd presentDrawable:swap->current_drawable];
        }
    } else {
        [cmd presentDrawable:swap->current_drawable];
    }
    
    swap->current_drawable = nil;
    swap->stats.total_frames++;
}

void metal_swapchain_present_after_minimum_duration(metal_swapchain_t* swap, id<MTLCommandBuffer> cmd, double duration) {
    if (!swap || !swap->current_drawable || !cmd) return;
    
    if (@available(macOS 10.15.4, *)) {
        [cmd presentDrawable:swap->current_drawable afterMinimumDuration:duration];
    } else {
        [cmd presentDrawable:swap->current_drawable];
    }
    
    swap->current_drawable = nil;
    swap->stats.total_frames++;
}

void metal_swapchain_set_present_callback(metal_swapchain_t* swap, metal_drawable_presented_callback_t callback, void* user_data) {
    if (!swap) return;
    // Note: Stored in extended swapchain struct (not shown in base implementation)
    // For now, we'll implement this via command buffer completion handlers in the present functions
    printf("Present callback registration not yet fully implemented\n");
}

double metal_swapchain_get_last_presented_time(const metal_swapchain_t* swap) {
    if (!swap) return 0.0;
    // Returns the last present time - would need to track this in command buffer handlers
    return 0.0; // Placeholder
}

void metal_swapchain_flush(metal_swapchain_t* swap) {
    if (!swap) return;
    // Wait for all pending drawable presentations to complete
    // This would typically involve waiting on completion handlers
    printf("Swapchain flush requested\n");
}

/* ============================================================================
 * VARIABLE REFRESH RATE (VRR) / PROMOTION SUPPORT
 * ============================================================================ */

void metal_swapchain_set_target_framerate(metal_swapchain_t* swap, uint32_t target_fps) {
    if (!swap) return;
    // Store target FPS for adaptive sync
    printf("Target framerate set to %u FPS\n", target_fps);
}

uint32_t metal_swapchain_get_target_framerate(const metal_swapchain_t* swap) {
    if (!swap) return 60; // Default to 60 FPS
    return 60; // Would be stored in extended struct
}

void metal_swapchain_set_low_latency_mode(metal_swapchain_t* swap, bool enable) {
    if (!swap || !swap->layer) return;
    
    if (enable) {
        // Reduce buffering for lower latency
        metal_swapchain_set_max_drawable_count(swap, 2); // Double buffering
        swap->layer.displaySyncEnabled = NO; // Disable vsync for lowest latency
        printf("Low latency mode enabled\n");
    } else {
        // Restore normal buffering
        metal_swapchain_set_max_drawable_count(swap, 3); // Triple buffering
        swap->layer.displaySyncEnabled = YES;
        printf("Low latency mode disabled\n");
    }
}

double metal_swapchain_get_display_refresh_rate(const metal_swapchain_t* swap) {
    if (!swap) return 60.0;
    
    // Query display info to get refresh rate
    metal_display_info_t* info = metal_display_get_info(NULL);
    if (info) {
        double refresh_rate = metal_display_get_refresh_rate(info);
        metal_display_info_destroy(info);
        return refresh_rate;
    }
    
    return 60.0;
}

bool metal_swapchain_supports_vrr(const metal_swapchain_t* swap) {
    if (!swap) return false;
    
    metal_display_info_t* info = metal_display_get_info(NULL);
    if (info) {
        bool vrr = metal_display_supports_vrr(info);
        metal_display_info_destroy(info);
        return vrr;
    }
    
    return false;
}

/* ============================================================================
 * PERFORMANCE MONITORING
 * ============================================================================ */

void metal_swapchain_get_performance(const metal_swapchain_t* swap, metal_swapchain_performance_t* out_metrics) {
    if (!swap || !out_metrics) return;
    
    // Calculate FPS from average frame time
    if (swap->stats.avg_frame_time_ms > 0.0) {
        out_metrics->fps = 1000.0 / swap->stats.avg_frame_time_ms;
    } else {
        out_metrics->fps = 0.0;
    }
    
    // Fill in metrics from stats
    out_metrics->frame_time_min_ms = 0.0; // Would need rolling window tracking
    out_metrics->frame_time_max_ms = 0.0;
    out_metrics->frame_time_std_dev_ms = 0.0;
    out_metrics->total_presents = swap->stats.total_frames;
    out_metrics->dropped_drawables = swap->stats.dropped_frames;
    out_metrics->avg_drawable_wait_ms = swap->stats.drawable_acquire_time_ms;
}

bool metal_swapchain_capture_screenshot(metal_swapchain_t* swap, const char* output_path) {
    if (!swap || !swap->current_drawable || !output_path) return false;
    
    // Get the drawable's texture
    id<MTLTexture> texture = swap->current_drawable.texture;
    if (!texture) return false;
    
    printf("Screenshot capture to %s (not yet implemented)\n", output_path);
    // Would need to:
    // 1. Create a blit command to copy texture to CPU-readable buffer
    // 2. Read pixel data
    // 3. Encode as PNG
    // 4. Write to file
    
    return false; // Not yet fully implemented
}

#endif /* __OBJC__ */
