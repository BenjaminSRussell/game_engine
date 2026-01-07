/*
 * mtl_vsync.m
 * VSync integration using CVDisplayLink implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#import "mtl_vsync.h"
#import <CoreVideo/CoreVideo.h>
#import <dispatch/dispatch.h>
#import <mach/mach_time.h>
#import <math.h>
#import <stdlib.h>
#import <string.h>

// ============================================================================
// Internal Structures
// ============================================================================

struct metal_vsync_manager {
  CVDisplayLinkRef display_link;
  metal_vsync_config_t config;

  // Callback
  metal_vsync_callback_t callback;
  void *user_data;

  // State
  bool running;
  uint64_t last_timestamp;

  // Statistics
  metal_vsync_stats_t stats;
  double *frame_times; // Rolling window for jitter calculation
  uint32_t frame_time_index;
  uint32_t frame_time_capacity;

  // Synchronization
  dispatch_semaphore_t vblank_semaphore;
};

// ============================================================================
// Helper Functions
// ============================================================================

static double cv_time_to_seconds(const CVTimeStamp *timestamp) {
  if (!timestamp || timestamp->videoTime == 0) {
    return 0.0;
  }

  return (double)timestamp->videoTime / (double)timestamp->videoTimeScale;
}

static double mach_time_to_seconds(uint64_t mach_time) {
  static mach_timebase_info_data_t timebase_info;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    mach_timebase_info(&timebase_info);
  });

  uint64_t nanos = mach_time * timebase_info.numer / timebase_info.denom;
  return (double)nanos / 1000000000.0;
}

static void update_jitter_stats(metal_vsync_manager_t *manager,
                                double frame_time_ms) {
  if (!manager || !manager->frame_times)
    return;

  // Add to rolling window
  manager->frame_times[manager->frame_time_index] = frame_time_ms;
  manager->frame_time_index =
      (manager->frame_time_index + 1) % manager->frame_time_capacity;

  // Calculate standard deviation (jitter)
  double sum = 0.0;
  double sum_sq = 0.0;
  uint32_t count = 0;

  for (uint32_t i = 0; i < manager->frame_time_capacity; i++) {
    if (manager->frame_times[i] > 0.0) {
      sum += manager->frame_times[i];
      sum_sq += manager->frame_times[i] * manager->frame_times[i];
      count++;
    }
  }

  if (count > 1) {
    double mean = sum / count;
    double variance = (sum_sq / count) - (mean * mean);
    manager->stats.jitter_ms = sqrt(fmax(variance, 0.0));
  }
}

// ============================================================================
// DisplayLink Callback
// ============================================================================

static CVReturn
display_link_callback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow,
                      const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn,
                      CVOptionFlags *flagsOut, void *displayLinkContext) {
  metal_vsync_manager_t *manager = (metal_vsync_manager_t *)displayLinkContext;
  if (!manager)
    return kCVReturnSuccess;

  double timestamp = cv_time_to_seconds(inOutputTime);
  double frame_duration = 0.0;

  if (manager->last_timestamp > 0.0) {
    frame_duration = timestamp - manager->last_timestamp;
    double frame_time_ms = frame_duration * 1000.0;

    manager->stats.actual_frame_time_ms = frame_time_ms;
    update_jitter_stats(manager, frame_time_ms);
  }

  manager->last_timestamp = timestamp;
  manager->stats.total_vblanks++;

  // Signal waiting threads
  if (manager->vblank_semaphore) {
    dispatch_semaphore_signal(manager->vblank_semaphore);
  }

  // Call user callback
  if (manager->callback) {
    manager->callback(timestamp, frame_duration, manager->user_data);
  }

  return kCVReturnSuccess;
}

// ============================================================================
// VSync Manager Implementation
// ============================================================================

metal_vsync_manager_t *
metal_vsync_manager_create(const metal_vsync_config_t *config) {
  if (!config)
    return NULL;

  metal_vsync_manager_t *manager =
      (metal_vsync_manager_t *)calloc(1, sizeof(metal_vsync_manager_t));
  if (!manager)
    return NULL;

  manager->config = *config;

  // Allocate frame time buffer for jitter calculation
  manager->frame_time_capacity = 60; // Track last 60 frames
  manager->frame_times =
      (double *)calloc(manager->frame_time_capacity, sizeof(double));
  if (!manager->frame_times) {
    free(manager);
    return NULL;
  }

  // Create VBlank semaphore
  manager->vblank_semaphore = dispatch_semaphore_create(0);
  if (!manager->vblank_semaphore) {
    free(manager->frame_times);
    free(manager);
    return NULL;
  }

  // Create display link
  CGDirectDisplayID display_id = config->display_id;
  if (display_id == 0) {
    display_id = CGMainDisplayID();
  }

  CVReturn status =
      CVDisplayLinkCreateWithCGDisplay(display_id, &manager->display_link);
  if (status != kCVReturnSuccess || !manager->display_link) {
    dispatch_release(manager->vblank_semaphore);
    free(manager->frame_times);
    free(manager);
    return NULL;
  }

  // Set callback
  CVDisplayLinkSetOutputCallback(manager->display_link, display_link_callback,
                                 manager);

  // Get refresh rate
  CVTime refresh_period =
      CVDisplayLinkGetNominalOutputVideoRefreshPeriod(manager->display_link);
  if (refresh_period.timeValue > 0 &&
      (refresh_period.flags & kCVTimeIsIndefinite) == 0) {
    manager->stats.refresh_rate_hz =
        (double)refresh_period.timeScale / (double)refresh_period.timeValue;
  } else {
    manager->stats.refresh_rate_hz = 60.0; // Default fallback
  }

  return manager;
}

void metal_vsync_manager_set_callback(metal_vsync_manager_t *manager,
                                      metal_vsync_callback_t callback,
                                      void *user_data) {
  if (!manager)
    return;

  manager->callback = callback;
  manager->user_data = user_data;
}

bool metal_vsync_manager_start(metal_vsync_manager_t *manager) {
  if (!manager || !manager->display_link)
    return false;

  if (manager->running)
    return true;

  CVReturn status = CVDisplayLinkStart(manager->display_link);
  if (status != kCVReturnSuccess) {
    return false;
  }

  manager->running = true;
  return true;
}

void metal_vsync_manager_stop(metal_vsync_manager_t *manager) {
  if (!manager || !manager->display_link || !manager->running)
    return;

  CVDisplayLinkStop(manager->display_link);
  manager->running = false;
}

void metal_vsync_manager_get_stats(metal_vsync_manager_t *manager,
                                   metal_vsync_stats_t *stats) {
  if (!manager || !stats)
    return;

  *stats = manager->stats;
}

void metal_vsync_manager_reset_stats(metal_vsync_manager_t *manager) {
  if (!manager)
    return;

  manager->stats.total_vblanks = 0;
  manager->stats.missed_vblanks = 0;
  manager->stats.jitter_ms = 0.0;
  manager->frame_time_index = 0;

  memset(manager->frame_times, 0,
         manager->frame_time_capacity * sizeof(double));
}

double metal_vsync_manager_get_refresh_rate(metal_vsync_manager_t *manager) {
  return manager ? manager->stats.refresh_rate_hz : 60.0;
}

bool metal_vsync_manager_wait_for_vblank(metal_vsync_manager_t *manager,
                                         uint32_t timeout_ms) {
  if (!manager || !manager->vblank_semaphore)
    return false;

  dispatch_time_t timeout;
  if (timeout_ms == 0) {
    timeout = DISPATCH_TIME_FOREVER;
  } else {
    timeout = dispatch_time(DISPATCH_TIME_NOW, timeout_ms * 1000000ULL);
  }

  long result = dispatch_semaphore_wait(manager->vblank_semaphore, timeout);
  return (result == 0);
}

void metal_vsync_manager_destroy(metal_vsync_manager_t *manager) {
  if (!manager)
    return;

  if (manager->running && manager->display_link) {
    CVDisplayLinkStop(manager->display_link);
  }

  if (manager->display_link) {
    CVDisplayLinkRelease(manager->display_link);
  }

  if (manager->vblank_semaphore) {
    dispatch_release(manager->vblank_semaphore);
  }

  if (manager->frame_times) {
    free(manager->frame_times);
  }

  free(manager);
}
