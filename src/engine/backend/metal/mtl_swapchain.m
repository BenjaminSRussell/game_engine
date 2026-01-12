/*
 * mtl_swapchain.c
 * Metal swapchain (CAMetalLayer) implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_swapchain.h"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <mach/mach_time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __OBJC__

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================
 */

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
 * ============================================================================
 */

static double mach_time_to_ms(uint64_t mach_time) {
  static mach_timebase_info_data_t timebase = {0, 0};
  if (timebase.denom == 0) {
    mach_timebase_info(&timebase);
  }
  return (double)mach_time * timebase.numer / timebase.denom / 1000000.0;
}

static uint64_t get_current_mach_time(void) { return mach_absolute_time(); }

/* ============================================================================
 * API IMPLEMENTATION
 * ============================================================================
 */

metal_swapchain_t *metal_swapchain_create(metal_device_t *dev, void *ns_view,
                                          bool hdr) {
  if (!dev || !ns_view)
    return NULL;

  metal_swapchain_t *swap =
      (metal_swapchain_t *)calloc(1, sizeof(metal_swapchain_t));
  if (!swap)
    return NULL;

  struct metal_device *internal_dev = (struct metal_device *)dev;
  id<MTLDevice> mtl_device = internal_dev->device;

  if (!mtl_device) {
    free(swap);
    return NULL;
  }

  // Configure Layer
  swap->layer = [CAMetalLayer layer];
  swap->layer.device = mtl_device;
  swap->layer.pixelFormat =
      hdr ? MTLPixelFormatRGBA16Float : MTLPixelFormatBGRA8Unorm;
  swap->layer.framebufferOnly =
      YES; // Set to NO if you need to read back or use as compute target
  swap->layer.displaySyncEnabled = YES; // V-Sync

  // HDR Configuration
  if (hdr) {
    if (@available(macOS 10.11, *)) {
      swap->layer.wantsExtendedDynamicRangeContent = YES;
      swap->layer.colorspace =
          CGColorSpaceCreateWithName(kCGColorSpaceExtendedLinearDisplayP3);
    } else {
      // Fallback for older macOS if necessary (though engine likely
      // targets 10.15+)
      swap->layer.colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    }
  } else {
    swap->layer.colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
  }

  // Attach to NSView
  NSView *view = (__bridge NSView *)ns_view;
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

void metal_swapchain_destroy(metal_swapchain_t *swap) {
  if (!swap)
    return;

  if (swap->layer) {
    swap->layer.device = nil;
    swap->layer = nil;
  }

  swap->current_drawable = nil;

  free(swap);
}

void metal_swapchain_resize(metal_swapchain_t *swap, uint32_t width,
                            uint32_t height) {
  if (!swap || !swap->layer)
    return;

  // width/height passed here might be usually points or pixels depending on
  // caller. Assuming the engine handles window events and passes points, but
  // for exact pixel control we set drawableSize. If width/height are pixels
  // (framebuffer size), we set drawableSize directly.

  CGSize new_size = CGSizeMake((CGFloat)width, (CGFloat)height);
  swap->layer.drawableSize = new_size;
  swap->size =
      new_size; // Store pixel size or point size? struct says just CGSize.
}

id<MTLTexture> metal_swapchain_get_texture(metal_swapchain_t *swap) {
  if (!swap || !swap->layer)
    return nil;

  uint64_t acquire_start = get_current_mach_time();

  swap->current_drawable = [swap->layer nextDrawable];

  uint64_t acquire_end = get_current_mach_time();
  swap->stats.drawable_acquire_time_ms =
      mach_time_to_ms(acquire_end - acquire_start);

  if (!swap->current_drawable) {
    swap->stats.dropped_frames++;
    return nil;
  }

  return swap->current_drawable.texture;
}

void metal_swapchain_present(metal_swapchain_t *swap,
                             id<MTLCommandBuffer> cmd) {
  if (!swap || !swap->current_drawable || !cmd)
    return;

  [cmd presentDrawable:swap->current_drawable];
  // Release drawable reference after command buffer consumes it?
  // ARC keeps it alive until command buffer is committed and scheduled.
  // We clear our strong reference to it for the next frame.
  swap->current_drawable = nil;

  swap->stats.total_frames++;
}

/* ============================================================================
 * PRESENT MODE AND CONFIGURATION
 * ============================================================================
 */

void metal_swapchain_set_present_mode(metal_swapchain_t *swap,
                                      metal_present_mode_t mode) {
  if (!swap || !swap->layer)
    return;

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

metal_present_mode_t
metal_swapchain_get_present_mode(const metal_swapchain_t *swap) {
  return swap ? swap->present_mode : METAL_PRESENT_MODE_VSYNC;
}

void metal_swapchain_set_drawable_timeout(metal_swapchain_t *swap,
                                          double timeout_seconds) {
  if (!swap)
    return;
  swap->drawable_timeout = timeout_seconds;
}

/* ============================================================================
 * STATISTICS AND TIMING
 * ============================================================================
 */

void metal_swapchain_get_statistics(const metal_swapchain_t *swap,
                                    metal_swapchain_stats_t *stats) {
  if (!swap || !stats)
    return;
  *stats = swap->stats;
}

void metal_swapchain_reset_statistics(metal_swapchain_t *swap) {
  if (!swap)
    return;
  memset(&swap->stats, 0, sizeof(metal_swapchain_stats_t));
}

void metal_swapchain_begin_frame(metal_swapchain_t *swap) {
  if (!swap)
    return;
  swap->frame_start_time = get_current_mach_time();
}

void metal_swapchain_end_frame(metal_swapchain_t *swap) {
  if (!swap)
    return;

  if (swap->frame_start_time == 0)
    return;

  uint64_t frame_end = get_current_mach_time();
  double frame_time_ms = mach_time_to_ms(frame_end - swap->frame_start_time);

  // Update statistics
  swap->stats.last_frame_time_ms = frame_time_ms;

  // Calculate rolling average
  if (swap->stats.total_frames == 0) {
    swap->stats.avg_frame_time_ms = frame_time_ms;
  } else {
    // Exponential moving average (alpha = 0.1)
    swap->stats.avg_frame_time_ms =
        0.9 * swap->stats.avg_frame_time_ms + 0.1 * frame_time_ms;
  }

  swap->frame_start_time = 0;
}

#endif /* __OBJC__ */
