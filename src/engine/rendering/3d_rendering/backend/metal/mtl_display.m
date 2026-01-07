/*
 * mtl_display.m
 * Metal display capability detection implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_display.h"
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================
 */

static double get_screen_refresh_rate(NSScreen *screen) {
  if (!screen)
    return 60.0;

  // Get display link for this screen
  CGDirectDisplayID displayID = [[[screen deviceDescription]
      objectForKey:@"NSScreenNumber"] unsignedIntValue];

  // Query refresh rate from display mode
  CGDisplayModeRef mode = CGDisplayCopyDisplayMode(displayID);
  if (!mode)
    return 60.0;

  double refresh_rate = CGDisplayModeGetRefreshRate(mode);
  CGDisplayModeRelease(mode);

  // If refresh rate is 0, assume 60Hz (common for built-in displays)
  return (refresh_rate > 0.0) ? refresh_rate : 60.0;
}

static double get_screen_max_refresh_rate(NSScreen *screen) {
  if (!screen)
    return 60.0;

  // Check for ProMotion support (macOS 12.0+)
  if (@available(macOS 12.0, *)) {
    NSNumber *maxFPS = [screen valueForKey:@"maximumFramesPerSecond"];
    if (maxFPS) {
      return [maxFPS doubleValue];
    }
  }

  // Fallback to current refresh rate
  return get_screen_refresh_rate(screen);
}

static float get_screen_edr_headroom(NSScreen *screen) {
  if (!screen)
    return 1.0f;

  // Query EDR headroom (macOS 10.15+)
  if (@available(macOS 10.15, *)) {
    CGFloat headroom =
        [screen maximumPotentialExtendedDynamicRangeColorComponentValue];
    return (float)headroom;
  }

  return 1.0f; // SDR fallback
}

static CGColorSpaceRef get_screen_color_space(NSScreen *screen) {
  if (!screen) {
    return CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
  }

  NSColorSpace *colorSpace = [screen colorSpace];
  if (colorSpace) {
    return CGColorSpaceRetain([colorSpace CGColorSpace]);
  }

  return CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
}

static int32_t calculate_ppi(NSScreen *screen) {
  if (!screen)
    return 72;

  NSDictionary *description = [screen deviceDescription];
  NSSize displayPixelSize = [[description objectForKey:NSDeviceSize] sizeValue];
  CGSize displayPhysicalSize = CGDisplayScreenSize(
      [[description objectForKey:@"NSScreenNumber"] unsignedIntValue]);

  // Calculate PPI from physical size in mm
  if (displayPhysicalSize.width > 0) {
    double widthInches = displayPhysicalSize.width / 25.4; // mm to inches
    return (int32_t)(displayPixelSize.width / widthInches);
  }

  return 72; // Default fallback
}

/* ============================================================================
 * API IMPLEMENTATION
 * ============================================================================
 */

metal_display_info_t *metal_display_get_info(void *ns_screen) {
  NSScreen *screen = (__bridge NSScreen *)ns_screen;

  // Use main screen if none specified
  if (!screen) {
    screen = [NSScreen mainScreen];
  }

  if (!screen) {
    fprintf(stderr, "metal_display_get_info: Failed to get screen\n");
    return NULL;
  }

  metal_display_info_t *info =
      (metal_display_info_t *)calloc(1, sizeof(metal_display_info_t));
  if (!info) {
    fprintf(stderr,
            "metal_display_get_info: Failed to allocate display info\n");
    return NULL;
  }

  // Query all display properties
  info->nominal_refresh_rate = get_screen_refresh_rate(screen);
  info->max_refresh_rate = get_screen_max_refresh_rate(screen);
  info->edr_max_headroom = get_screen_edr_headroom(screen);
  info->native_color_space = get_screen_color_space(screen);

  // Determine capabilities
  info->supports_hdr = (info->edr_max_headroom > 1.0f);
  info->supports_vrr = (info->max_refresh_rate > info->nominal_refresh_rate);

  // Get physical size
  NSDictionary *description = [screen deviceDescription];
  CGDirectDisplayID displayID =
      [[description objectForKey:@"NSScreenNumber"] unsignedIntValue];
  CGSize physicalSize = CGDisplayScreenSize(displayID);
  info->physical_size_mm = physicalSize;

  // Calculate PPI
  info->ppi = calculate_ppi(screen);

  // Log display info
  printf("Metal Display Info:\n");
  printf("  - Refresh Rate: %.2f Hz\n", info->nominal_refresh_rate);
  printf("  - Max Refresh Rate: %.2f Hz\n", info->max_refresh_rate);
  printf("  - HDR Support: %s (Headroom: %.2f)\n",
         info->supports_hdr ? "YES" : "NO", info->edr_max_headroom);
  printf("  - VRR Support: %s\n", info->supports_vrr ? "YES" : "NO");
  printf("  - PPI: %d\n", info->ppi);

  return info;
}

bool metal_display_supports_hdr(const metal_display_info_t *info) {
  return info ? info->supports_hdr : false;
}

bool metal_display_supports_vrr(const metal_display_info_t *info) {
  return info ? info->supports_vrr : false;
}

double metal_display_get_refresh_rate(const metal_display_info_t *info) {
  return info ? info->nominal_refresh_rate : 60.0;
}

double metal_display_get_max_refresh_rate(const metal_display_info_t *info) {
  return info ? info->max_refresh_rate : 60.0;
}

float metal_display_get_edr_headroom(const metal_display_info_t *info) {
  return info ? info->edr_max_headroom : 1.0f;
}

CGColorSpaceRef
metal_display_get_color_space(const metal_display_info_t *info) {
  return info ? info->native_color_space : NULL;
}

void metal_display_info_destroy(metal_display_info_t *info) {
  if (!info)
    return;

  if (info->native_color_space) {
    CGColorSpaceRelease(info->native_color_space);
    info->native_color_space = NULL;
  }

  free(info);
}
