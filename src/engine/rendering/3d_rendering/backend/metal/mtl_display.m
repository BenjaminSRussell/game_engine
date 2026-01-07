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

/* ============================================================================
 * MULTI-MONITOR SUPPORT
 * ============================================================================
 */

void **metal_display_get_all_screens(uint32_t *out_count) {
  if (!out_count)
    return NULL;

  NSArray<NSScreen *> *screens = [NSScreen screens];
  *out_count = (uint32_t)[screens count];

  if (*out_count == 0)
    return NULL;

  void **screen_array = (void **)malloc(sizeof(void *) * (*out_count));
  if (!screen_array) {
    *out_count = 0;
    return NULL;
  }

  for (uint32_t i = 0; i < *out_count; i++) {
    screen_array[i] = (__bridge void *)[screens objectAtIndex:i];
  }

  return screen_array;
}

void metal_display_free_screens(void **screens) {
  if (screens) {
    free(screens);
  }
}

metal_display_info_t *metal_display_get_info_by_index(uint32_t screen_index) {
  NSArray<NSScreen *> *screens = [NSScreen screens];

  if (screen_index >= [screens count]) {
    fprintf(stderr,
            "metal_display_get_info_by_index: Invalid screen index %u\n",
            screen_index);
    return NULL;
  }

  NSScreen *screen = [screens objectAtIndex:screen_index];
  return metal_display_get_info((__bridge void *)screen);
}

bool metal_display_is_main(void *ns_screen) {
  if (!ns_screen)
    return false;

  NSScreen *screen = (__bridge NSScreen *)ns_screen;
  return (screen == [NSScreen mainScreen]);
}

bool metal_display_is_builtin(void *ns_screen) {
  if (!ns_screen)
    return false;

  NSScreen *screen = (__bridge NSScreen *)ns_screen;
  NSDictionary *description = [screen deviceDescription];
  CGDirectDisplayID displayID =
      [[description objectForKey:@"NSScreenNumber"] unsignedIntValue];

  return CGDisplayIsBuiltin(displayID);
}

/* ============================================================================
 * SCREEN CHANGE NOTIFICATIONS
 * ============================================================================
 */

static metal_display_change_callback_t g_display_change_callback = NULL;
static void *g_display_change_user_data = NULL;
static id g_display_change_observer = nil;

void metal_display_register_change_callback(
    metal_display_change_callback_t callback, void *user_data) {
  g_display_change_callback = callback;
  g_display_change_user_data = user_data;

  if (g_display_change_observer) {
    [[NSNotificationCenter defaultCenter]
        removeObserver:g_display_change_observer];
  }

  g_display_change_observer = [[NSNotificationCenter defaultCenter]
      addObserverForName:NSApplicationDidChangeScreenParametersNotification
                  object:nil
                   queue:[NSOperationQueue mainQueue]
              usingBlock:^(NSNotification *note) {
                if (g_display_change_callback) {
                  g_display_change_callback(g_display_change_user_data);
                }
              }];

  printf("Display change callback registered\n");
}

void metal_display_unregister_change_callback(void) {
  if (g_display_change_observer) {
    [[NSNotificationCenter defaultCenter]
        removeObserver:g_display_change_observer];
    g_display_change_observer = nil;
  }

  g_display_change_callback = NULL;
  g_display_change_user_data = NULL;

  printf("Display change callback unregistered\n");
}

/* ============================================================================
 * WINDOW INTEGRATION
 * ============================================================================
 */

void *metal_display_get_window_screen(void *ns_window) {
  if (!ns_window)
    return NULL;

  NSWindow *window = (__bridge NSWindow *)ns_window;
  return (__bridge void *)[window screen];
}

bool metal_display_is_window_fullscreen(void *ns_window) {
  if (!ns_window)
    return false;

  NSWindow *window = (__bridge NSWindow *)ns_window;
  return ([window styleMask] & NSWindowStyleMaskFullScreen) != 0;
}

bool metal_display_is_window_visible(void *ns_window) {
  if (!ns_window)
    return false;

  NSWindow *window = (__bridge NSWindow *)ns_window;

  if (![window isVisible])
    return false;

  if (@available(macOS 10.9, *)) {
    return [window occlusionState] & NSWindowOcclusionStateVisible;
  }

  return [window isVisible];
}

float metal_display_get_window_scale(void *ns_window) {
  if (!ns_window)
    return 1.0f;

  NSWindow *window = (__bridge NSWindow *)ns_window;
  return (float)[window backingScaleFactor];
}

/* ============================================================================
 * HDR/EDR REAL-TIME MONITORING
 * ============================================================================
 */

static metal_display_edr_callback_t g_edr_callback = NULL;
static void *g_edr_user_data = NULL;
static id g_edr_observer = nil;
static NSScreen *g_monitored_screen = nil;
static float g_last_edr_headroom = 1.0f;

// Timer to poll EDR changes (NSScreen doesn't have direct notifications)
static NSTimer *g_edr_poll_timer = nil;

static void poll_edr_changes(NSTimer *timer) {
  NSScreen *screen =
      g_monitored_screen ? g_monitored_screen : [NSScreen mainScreen];
  if (!screen)
    return;

  float current_headroom = get_screen_edr_headroom(screen);

  if (fabs(current_headroom - g_last_edr_headroom) > 0.01f) {
    g_last_edr_headroom = current_headroom;
    if (g_edr_callback) {
      g_edr_callback(current_headroom, g_edr_user_data);
    }
  }
}

void metal_display_register_edr_callback(void *ns_screen,
                                         metal_display_edr_callback_t callback,
                                         void *user_data) {
  g_edr_callback = callback;
  g_edr_user_data = user_data;
  g_monitored_screen = ns_screen ? (__bridge NSScreen *)ns_screen : nil;

  NSScreen *screen =
      g_monitored_screen ? g_monitored_screen : [NSScreen mainScreen];
  g_last_edr_headroom = get_screen_edr_headroom(screen);

  // Poll EDR headroom every 0.5 seconds
  if (g_edr_poll_timer) {
    [g_edr_poll_timer invalidate];
  }

  g_edr_poll_timer =
      [NSTimer scheduledTimerWithTimeInterval:0.5
                                       target:[NSBlockOperation
                                                  blockOperationWithBlock:^{
                                                    poll_edr_changes(nil);
                                                  }]
                                     selector:@selector(main)
                                     userInfo:nil
                                      repeats:YES];

  printf("EDR monitoring started (headroom: %.2f)\n", g_last_edr_headroom);
}

void metal_display_unregister_edr_callback(void) {
  if (g_edr_poll_timer) {
    [g_edr_poll_timer invalidate];
    g_edr_poll_timer = nil;
  }

  g_edr_callback = NULL;
  g_edr_user_data = NULL;
  g_monitored_screen = nil;

  printf("EDR monitoring stopped\n");
}

float metal_display_get_current_edr_headroom(void *ns_screen) {
  NSScreen *screen =
      ns_screen ? (__bridge NSScreen *)ns_screen : [NSScreen mainScreen];
  return get_screen_edr_headroom(screen);
}
