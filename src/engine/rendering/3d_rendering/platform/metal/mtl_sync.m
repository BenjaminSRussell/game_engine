#import "mtl_sync.h"
#import "mtl_command.h"
#import <Metal/Metal.h>
#import <dispatch/dispatch.h>
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

    // Create and signal a new fence for this access
    // Note: We can't create a new fence here without a device reference
    // This would need to be refactored to pass device explicitly
    metal_fence_t *new_fence = NULL;
    if (new_fence) {
      metal_fence_encode_signal(new_fence, cmd_buffer,
                                METAL_STAGE_VERTEX | METAL_STAGE_FRAGMENT |
                                    METAL_STAGE_COMPUTE);

      // Clean up old fence
      metal_fence_destroy(tracker->last_fence);
      tracker->last_fence = new_fence;
    }
  } else if (needs_fence && !tracker->last_fence) {
    // Create initial fence
    // Note: We can't create a fence here without a device reference
    // This would need to be refactored to pass device explicitly
    tracker->last_fence = NULL;
    if (tracker->last_fence) {
      metal_fence_encode_signal(tracker->last_fence, cmd_buffer,
                                METAL_STAGE_VERTEX | METAL_STAGE_FRAGMENT |
                                    METAL_STAGE_COMPUTE);
    }
  }

  // Update tracker
  tracker->resource = resource;
  tracker->last_access = access;
  tracker->last_frame = frame_index;

  return needs_fence;
}
