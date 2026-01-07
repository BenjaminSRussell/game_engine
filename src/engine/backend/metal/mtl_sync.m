#import "mtl_sync.h"
#import "mtl_command.h"
#import <Metal/Metal.h>
#import <dispatch/dispatch.h>
#import <mach/mach_time.h>
#import <math.h>
#import <stdlib.h>
#import <string.h>

// ============================================================================
// Frame Synchronization Implementation
// ============================================================================

metal_frame_sync_t *metal_frame_sync_create(id<MTLDevice> device,
                                            uint32_t max_frames_in_flight) {
  if (!device || max_frames_in_flight == 0) {
    return NULL;
  }

  metal_frame_sync_t *sync =
      (metal_frame_sync_t *)calloc(1, sizeof(metal_frame_sync_t));
  if (!sync) {
    return NULL;
  }

  // Create dispatch semaphore for CPU-side throttling
  // This prevents the CPU from getting too far ahead of the GPU
  sync->frame_semaphore = dispatch_semaphore_create(max_frames_in_flight);
  if (!sync->frame_semaphore) {
    free(sync);
    return NULL;
  }

  // Create shared event for GPU completion tracking
  sync->shared_event = [device newSharedEvent];
  if (!sync->shared_event) {
    dispatch_release(sync->frame_semaphore);
    free(sync);
    return NULL;
  }

  sync->frame_index = 0;
  sync->max_frames_in_flight = max_frames_in_flight;

  // Initialize timing and statistics
  sync->frame_start_time = 0;
  sync->timeout_ns = 5000000000ULL; // 5 second default timeout
  sync->stats.min_frame_time_ms = INFINITY;
  sync->stats.max_frame_time_ms = 0.0;
  sync->stats.avg_frame_time_ms = 0.0;
  sync->stats.target_frame_time_ms = 16.67; // 60 FPS default
  sync->stats.total_frames = 0;
  sync->stats.dropped_frames = 0;
  sync->pacing_mode = METAL_FRAME_PACING_FIXED;

  // Initialize deadlock detection
  sync->last_completed_frame = 0;
  sync->deadlock_threshold = 100; // Consider deadlock if 100 frames behind

  return sync;
}

void metal_frame_begin(metal_frame_sync_t *sync) {
  if (!sync) {
    return;
  }

  // Wait for a frame slot to become available
  // This blocks if we already have max_frames_in_flight frames in flight
  dispatch_semaphore_wait(sync->frame_semaphore, DISPATCH_TIME_FOREVER);
}

void metal_frame_end(metal_frame_sync_t *sync,
                     metal_command_buffer_t *cmd_buffer) {
  if (!sync || !cmd_buffer || !cmd_buffer->buffer) {
    return;
  }

  // Increment frame counter and signal this value when GPU completes
  uint64_t signal_value = ++sync->frame_index;
  [cmd_buffer->buffer encodeSignalEvent:sync->shared_event value:signal_value];

  // Add completion handler to release the frame slot when GPU is done
  // We need to capture the semaphore in the block
  __block dispatch_semaphore_t sem = sync->frame_semaphore;
  [cmd_buffer->buffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
    // Signal the semaphore to release the frame slot
    dispatch_semaphore_signal(sem);
  }];
}

void metal_frame_sync_destroy(metal_frame_sync_t *sync) {
  if (!sync) {
    return;
  }

  if (sync->shared_event) {
    [sync->shared_event release];
    sync->shared_event = nil;
  }

  if (sync->frame_semaphore) {
    dispatch_release(sync->frame_semaphore);
    sync->frame_semaphore = NULL;
  }

  free(sync);
}

// ============================================================================
// Enhanced Frame Synchronization Implementation
// ============================================================================

// Helper function to convert mach time to milliseconds
static double mach_time_to_ms(uint64_t mach_time) {
  static mach_timebase_info_data_t timebase_info;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    mach_timebase_info(&timebase_info);
  });

  uint64_t nanos = mach_time * timebase_info.numer / timebase_info.denom;
  return (double)nanos / 1000000.0;
}

bool metal_frame_begin_with_timeout(metal_frame_sync_t *sync,
                                    uint64_t timeout_ns) {
  if (!sync) {
    return false;
  }

  uint64_t timeout = timeout_ns > 0 ? timeout_ns : sync->timeout_ns;
  dispatch_time_t timeout_time = timeout > 0
                                     ? dispatch_time(DISPATCH_TIME_NOW, timeout)
                                     : DISPATCH_TIME_FOREVER;

  long result = dispatch_semaphore_wait(sync->frame_semaphore, timeout_time);

  if (result == 0) {
    // Successfully acquired frame slot, record start time
    sync->frame_start_time = mach_absolute_time();
    return true;
  }

  // Timeout occurred
  return false;
}

bool metal_frame_wait_for_completion(metal_frame_sync_t *sync,
                                     uint64_t frame_index,
                                     uint64_t timeout_ns) {
  if (!sync || !sync->shared_event) {
    return false;
  }

  // Check if frame has already completed
  if (sync->shared_event.signaledValue >= frame_index) {
    return true;
  }

  // Create listener for the frame
  MTLSharedEventListener *listener = [[MTLSharedEventListener alloc] init];
  if (!listener) {
    return false;
  }

  __block bool completed = false;
  dispatch_semaphore_t wait_sem = dispatch_semaphore_create(0);

  [sync->shared_event
      notifyListener:listener
             atValue:frame_index
               block:^(id<MTLSharedEvent> event, uint64_t value) {
                 completed = true;
                 dispatch_semaphore_signal(wait_sem);
               }];

  dispatch_time_t timeout = timeout_ns > 0
                                ? dispatch_time(DISPATCH_TIME_NOW, timeout_ns)
                                : DISPATCH_TIME_FOREVER;

  long result = dispatch_semaphore_wait(wait_sem, timeout);

  dispatch_release(wait_sem);
  [listener release];

  return (result == 0) && completed;
}

uint64_t metal_frame_get_current_index(metal_frame_sync_t *sync) {
  return sync ? sync->frame_index : 0;
}

void metal_frame_get_stats(metal_frame_sync_t *sync,
                           metal_frame_stats_t *stats) {
  if (!sync || !stats) {
    return;
  }

  *stats = sync->stats;
}

void metal_frame_reset_stats(metal_frame_sync_t *sync) {
  if (!sync) {
    return;
  }

  sync->stats.min_frame_time_ms = INFINITY;
  sync->stats.max_frame_time_ms = 0.0;
  sync->stats.avg_frame_time_ms = 0.0;
  sync->stats.total_frames = 0;
  sync->stats.dropped_frames = 0;
}

void metal_frame_set_pacing_mode(metal_frame_sync_t *sync,
                                 metal_frame_pacing_mode_t mode) {
  if (!sync) {
    return;
  }

  sync->pacing_mode = mode;
}

void metal_frame_set_target_fps(metal_frame_sync_t *sync, uint32_t target_fps) {
  if (!sync || target_fps == 0) {
    return;
  }

  sync->stats.target_frame_time_ms = 1000.0 / (double)target_fps;
}

bool metal_frame_is_within_budget(metal_frame_sync_t *sync) {
  if (!sync || sync->frame_start_time == 0) {
    return true; // No timing data yet
  }

  uint64_t current_time = mach_absolute_time();
  double elapsed_ms = mach_time_to_ms(current_time - sync->frame_start_time);

  return elapsed_ms < sync->stats.target_frame_time_ms;
}

// ============================================================================
// GPU-GPU Synchronization (Fences) Implementation
// ============================================================================

metal_fence_t *metal_fence_create(id<MTLDevice> device) {
  if (!device) {
    return NULL;
  }

  metal_fence_t *fence = (metal_fence_t *)calloc(1, sizeof(metal_fence_t));
  if (!fence) {
    return NULL;
  }

  fence->fence = [device newFence];
  if (!fence->fence) {
    free(fence);
    return NULL;
  }

  return fence;
}

void metal_fence_encode_wait(metal_fence_t *fence,
                             metal_command_buffer_t *cmd_buffer,
                             uint32_t stage) {
  if (!fence || !fence->fence || !cmd_buffer || !cmd_buffer->buffer) {
    return;
  }

  // Convert stage flags to MTLRenderStages
  MTLRenderStages mtl_stages = 0;
  if (stage & METAL_STAGE_VERTEX) {
    mtl_stages |= MTLRenderStageVertex;
  }
  if (stage & METAL_STAGE_FRAGMENT) {
    mtl_stages |= MTLRenderStageFragment;
  }

  // For render encoders, we need to wait before the specified stages
  // Note: This is typically called within a render pass encoder context
  // For now, we'll encode a wait at the command buffer level
  [cmd_buffer->buffer encodeWaitForEvent:(id<MTLEvent>)fence->fence value:1];
}

void metal_fence_encode_signal(metal_fence_t *fence,
                               metal_command_buffer_t *cmd_buffer,
                               uint32_t stage) {
  if (!fence || !fence->fence || !cmd_buffer || !cmd_buffer->buffer) {
    return;
  }

  // Convert stage flags to MTLRenderStages
  MTLRenderStages mtl_stages = 0;
  if (stage & METAL_STAGE_VERTEX) {
    mtl_stages |= MTLRenderStageVertex;
  }
  if (stage & METAL_STAGE_FRAGMENT) {
    mtl_stages |= MTLRenderStageFragment;
  }

  // Signal the fence after the specified stages
  // Note: This is typically called within a render pass encoder context
  [cmd_buffer->buffer encodeSignalEvent:(id<MTLEvent>)fence->fence value:1];
}

void metal_fence_destroy(metal_fence_t *fence) {
  if (!fence) {
    return;
  }

  if (fence->fence) {
    [fence->fence release];
    fence->fence = nil;
  }

  free(fence);
}

// ============================================================================
// CPU-GPU Synchronization (Events) Implementation
// ============================================================================

metal_event_t *metal_event_create(id<MTLDevice> device,
                                  uint64_t initial_value) {
  if (!device) {
    return NULL;
  }

  metal_event_t *event = (metal_event_t *)calloc(1, sizeof(metal_event_t));
  if (!event) {
    return NULL;
  }

  event->event = [device newSharedEvent];
  if (!event->event) {
    free(event);
    return NULL;
  }

  event->current_value = initial_value;
  event->event.signaledValue = initial_value;

  return event;
}

void metal_event_encode_signal(metal_event_t *event,
                               metal_command_buffer_t *cmd_buffer,
                               uint64_t value) {
  if (!event || !event->event || !cmd_buffer || !cmd_buffer->buffer) {
    return;
  }

  // Encode GPU signal operation
  [cmd_buffer->buffer encodeSignalEvent:event->event value:value];

  // Update tracked value
  if (value > event->current_value) {
    event->current_value = value;
  }
}

void metal_event_encode_wait(metal_event_t *event,
                             metal_command_buffer_t *cmd_buffer,
                             uint64_t value) {
  if (!event || !event->event || !cmd_buffer || !cmd_buffer->buffer) {
    return;
  }

  // Encode GPU wait operation
  [cmd_buffer->buffer encodeWaitForEvent:event->event value:value];
}

bool metal_event_wait(metal_event_t *event, uint64_t value,
                      uint64_t timeout_ns) {
  if (!event || !event->event) {
    return false;
  }

  // Create a listener for the event
  MTLSharedEventListener *listener = [[MTLSharedEventListener alloc] init];
  if (!listener) {
    return false;
  }

  __block bool signaled = false;
  dispatch_semaphore_t wait_semaphore = dispatch_semaphore_create(0);

  // Register notification handler
  [event->event notifyListener:listener
                       atValue:value
                         block:^(id<MTLSharedEvent> sharedEvent, uint64_t val) {
                           signaled = true;
                           dispatch_semaphore_signal(wait_semaphore);
                         }];

  // Wait with timeout
  dispatch_time_t timeout;
  if (timeout_ns == 0) {
    timeout = DISPATCH_TIME_FOREVER;
  } else {
    timeout = dispatch_time(DISPATCH_TIME_NOW, timeout_ns);
  }

  long result = dispatch_semaphore_wait(wait_semaphore, timeout);

  dispatch_release(wait_semaphore);
  [listener release];

  return (result == 0) && signaled;
}

void metal_event_signal(metal_event_t *event, uint64_t value) {
  if (!event || !event->event) {
    return;
  }

  // CPU signals the event
  event->event.signaledValue = value;
  event->current_value = value;
}

uint64_t metal_event_get_value(metal_event_t *event) {
  if (!event || !event->event) {
    return 0;
  }

  return event->event.signaledValue;
}

void metal_event_destroy(metal_event_t *event) {
  if (!event) {
    return;
  }

  if (event->event) {
    [event->event release];
    event->event = nil;
  }

  free(event);
}

// ============================================================================
// Extended Event APIs Implementation
// ============================================================================

bool metal_event_add_listener(metal_event_t *event, uint64_t value,
                              metal_event_callback_t callback,
                              void *user_data) {
  if (!event || !event->event || !callback) {
    return false;
  }

  MTLSharedEventListener *listener = [[MTLSharedEventListener alloc] init];
  if (!listener) {
    return false;
  }

  // Capture event and callback in the block - Metal retains the listener during callback
  [event->event notifyListener:listener
                       atValue:value
                         block:^(id<MTLSharedEvent> sharedEvent, uint64_t val) {
                           if (event && callback) {
                             callback(event, val, user_data);
                           }
                         }];

  // IMPORTANT: Keep listener alive - MTLSharedEvent retains it during the callback
  // The listener will be released by MTLSharedEvent automatically after the callback
  // DO NOT release it here as it causes EXC_BAD_ACCESS when the callback fires

  return true;
}

bool metal_event_wait_multiple(metal_event_t **events, uint64_t *values,
                               uint32_t count, uint64_t timeout_ns) {
  if (!events || !values || count == 0) {
    return false;
  }

  // Wait for all events in sequence
  for (uint32_t i = 0; i < count; i++) {
    if (!metal_event_wait(events[i], values[i], timeout_ns)) {
      return false;
    }
  }

  return true;
}

void metal_event_encode_wait_multiple(metal_event_t **events, uint64_t *values,
                                      uint32_t count,
                                      metal_command_buffer_t *cmd_buffer) {
  if (!events || !values || count == 0 || !cmd_buffer) {
    return;
  }

  // Encode waits for all events
  for (uint32_t i = 0; i < count; i++) {
    metal_event_encode_wait(events[i], cmd_buffer, values[i]);
  }
}

metal_event_t *metal_create_sync_point(id<MTLDevice> device) {
  metal_event_t *event = metal_event_create(device, 0);
  if (event) {
    metal_event_signal(event, 1);
  }
  return event;
}

// ============================================================================
// Resource Hazard Tracking Implementation
// ============================================================================

bool metal_check_resource_hazard(metal_resource_tracker_t *tracker,
                                 void *resource, metal_resource_access_t access,
                                 metal_command_buffer_t *cmd_buffer,
                                 uint64_t frame_index) {
  if (!tracker || !resource || !cmd_buffer) {
    return false;
  }

  bool needs_fence = false;

  // Check if this is a new resource or same frame (no hazard)
  if (tracker->resource != resource || tracker->last_frame == frame_index) {
    // Update tracker for new resource or same frame
    tracker->resource = resource;
    tracker->last_access = access;
    tracker->last_frame = frame_index;
    return false;
  }

  // Detect hazards:
  // WAR (Write-After-Read): Previous read, current write
  // RAW (Read-After-Write): Previous write, current read
  // WAW (Write-After-Write): Previous write, current write

  if (tracker->last_access == METAL_RESOURCE_ACCESS_WRITE ||
      access == METAL_RESOURCE_ACCESS_WRITE ||
      access == METAL_RESOURCE_ACCESS_READ_WRITE ||
      tracker->last_access == METAL_RESOURCE_ACCESS_READ_WRITE) {
    needs_fence = true;
  }

  // Insert fence if needed
  if (needs_fence && tracker->last_fence) {
    // Wait on the fence from the last access
    metal_fence_encode_wait(tracker->last_fence, cmd_buffer,
                            METAL_STAGE_VERTEX | METAL_STAGE_FRAGMENT |
                                METAL_STAGE_COMPUTE);

    // FIXME: Create and signal a new fence for this access
    // TODO: Refactor to pass device explicitly for fence creation
    // For now, just keep using the existing fence with addCompletedHandler
  }

  // Update tracker
  tracker->resource = resource;
  tracker->last_access = access;
  tracker->last_frame = frame_index;

  return needs_fence;
}

// ============================================================================
// Enhanced Resource Hazard Tracking Implementation  
// ============================================================================

bool metal_analyze_resource_hazard(metal_resource_tracker_t *tracker,
                                   void *resource,
                                   metal_resource_access_t access,
                                   metal_hazard_info_t *info) {
  if (!tracker || !resource || !info) {
    return false;
  }
  
  // Initialize hazard info
  memset(info, 0, sizeof(metal_hazard_info_t));
  
  // Check if this is a new resource
  if (tracker->resource != resource) {
    info->has_hazard = false;
    return false;
  }
  
  // Analyze hazard type
  if (tracker->last_access == METAL_RESOURCE_ACCESS_WRITE) {
    if (access == METAL_RESOURCE_ACCESS_READ || access == METAL_RESOURCE_ACCESS_READ_WRITE) {
      info->is_raw = true;
      info->has_hazard = true;
    } else if (access == METAL_RESOURCE_ACCESS_WRITE) {
      info->is_waw = true;
      info->has_hazard = true;
    }
  } else if (tracker->last_access == METAL_RESOURCE_ACCESS_READ) {
    if (access == METAL_RESOURCE_ACCESS_WRITE || access == METAL_RESOURCE_ACCESS_READ_WRITE) {
      info->is_war = true;
      info->has_hazard = true;
    }
  } else if (tracker->last_access == METAL_RESOURCE_ACCESS_READ_WRITE) {
    info->has_hazard = true;
    info->is_raw = (access == METAL_RESOURCE_ACCESS_READ);
    info->is_war = (access == METAL_RESOURCE_ACCESS_WRITE);
    info->is_waw = (access == METAL_RESOURCE_ACCESS_WRITE);
  }
  
  if (info->has_hazard) {
    info->needs_barrier = true;
    info->recommended_stages = METAL_STAGE_VERTEX | METAL_STAGE_FRAGMENT | METAL_STAGE_COMPUTE;
  }
  
  return info->has_hazard;
}

void metal_insert_texture_barrier(metal_command_buffer_t *cmd_buffer,
                                  void *texture,
                                  uint32_t stages) {
  if (!cmd_buffer || !texture) {
    return;
  }
  
  // Note: Metal doesn't have explicit texture barriers like Vulkan
  // Instead, we use fences at the command encoder level
}

void metal_insert_buffer_barrier(metal_command_buffer_t *cmd_buffer,
                                 void *buffer,
                                 uint32_t stages) {
  if (!cmd_buffer || !buffer) {
    return;
  }
  
  // Note: Similar to texture barriers, Metal uses fences
}

void metal_resource_tracker_init(metal_resource_tracker_t *tracker,
                                metal_resource_type_t type) {
  if (!tracker) {
    return;
  }
  
  memset(tracker, 0, sizeof(metal_resource_tracker_t));
  tracker->type = type;
}

void metal_resource_tracker_reset(metal_resource_tracker_t *tracker) {
  if (!tracker) {
    return;
  }
  
  if (tracker->last_fence) {
    metal_fence_destroy(tracker->last_fence);
  }
  
  memset(tracker, 0, sizeof(metal_resource_tracker_t));
}
