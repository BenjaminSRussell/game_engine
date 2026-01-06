#ifndef MTL_SYNC_H
#define MTL_SYNC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <dispatch/dispatch.h>
#else
typedef void* id;
typedef void* dispatch_semaphore_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct metal_device metal_device_t;
typedef struct metal_command_buffer metal_command_buffer_t;

// ============================================================================
// Frame Synchronization (Triple Buffering)
// ============================================================================

/**
 * Frame synchronization structure for triple buffering.
 * Manages CPU-GPU synchronization to prevent frame overruns.
 */
typedef struct metal_frame_sync {
#ifdef __OBJC__
    dispatch_semaphore_t frame_semaphore;  // CPU-side throttling
    id<MTLSharedEvent> shared_event;       // GPU completion tracking
#else
    dispatch_semaphore_t frame_semaphore;
    id shared_event;
#endif
    uint64_t frame_index;                  // Current frame counter
    uint32_t max_frames_in_flight;         // Maximum concurrent frames (typically 3)
} metal_frame_sync_t;

/**
 * Create a frame synchronization object for triple buffering.
 * 
 * @param device Metal device (id<MTLDevice>)
 * @param max_frames_in_flight Maximum number of frames that can be in flight (typically 2 or 3)
 * @return Frame synchronization object, or NULL on failure
 */
#ifdef __OBJC__
metal_frame_sync_t* metal_frame_sync_create(id<MTLDevice> device, uint32_t max_frames_in_flight);
#else
metal_frame_sync_t* metal_frame_sync_create(id device, uint32_t max_frames_in_flight);
#endif

/**
 * Begin a new frame. Blocks if max frames are already in flight.
 * Call this at the start of each frame before recording commands.
 * 
 * @param sync Frame synchronization object
 */
void metal_frame_begin(metal_frame_sync_t* sync);

/**
 * End the current frame. Signals GPU completion and releases frame slot.
 * Call this after submitting the command buffer for the frame.
 * 
 * @param sync Frame synchronization object
 * @param cmd_buffer Command buffer to track for completion
 */
void metal_frame_end(metal_frame_sync_t* sync, metal_command_buffer_t* cmd_buffer);

/**
 * Destroy a frame synchronization object.
 * 
 * @param sync Frame synchronization object to destroy
 */
void metal_frame_sync_destroy(metal_frame_sync_t* sync);

// ============================================================================
// GPU-GPU Synchronization (Fences)
// ============================================================================

/**
 * Metal fence wrapper for GPU-GPU synchronization.
 * Used to create dependencies between render/compute passes.
 */
typedef struct metal_fence {
#ifdef __OBJC__
    id<MTLFence> fence;
#else
    id fence;
#endif
} metal_fence_t;

/**
 * Create a Metal fence for GPU-GPU synchronization.
 * 
 * @param device Metal device (id<MTLDevice>)
 * @return Fence object, or NULL on failure
 */
#ifdef __OBJC__
metal_fence_t* metal_fence_create(id<MTLDevice> device);
#else
metal_fence_t* metal_fence_create(id device);
#endif

/**
 * Encode a wait operation on a fence in a command buffer.
 * The GPU will wait until the fence is signaled before proceeding.
 * 
 * @param fence Fence to wait on
 * @param cmd_buffer Command buffer to encode wait into
 * @param stage Pipeline stage to wait at (before vertex, fragment, etc.)
 */
void metal_fence_encode_wait(metal_fence_t* fence, metal_command_buffer_t* cmd_buffer, uint32_t stage);

/**
 * Encode a signal operation on a fence in a command buffer.
 * The GPU will signal the fence when it reaches this point.
 * 
 * @param fence Fence to signal
 * @param cmd_buffer Command buffer to encode signal into
 * @param stage Pipeline stage to signal at (after vertex, fragment, etc.)
 */
void metal_fence_encode_signal(metal_fence_t* fence, metal_command_buffer_t* cmd_buffer, uint32_t stage);

/**
 * Destroy a Metal fence.
 * 
 * @param fence Fence to destroy
 */
void metal_fence_destroy(metal_fence_t* fence);

// ============================================================================
// CPU-GPU Synchronization (Events)
// ============================================================================

/**
 * Metal event wrapper for CPU-GPU timeline synchronization.
 * Allows CPU to wait for GPU work or GPU to wait for CPU signals.
 */
typedef struct metal_event {
#ifdef __OBJC__
    id<MTLSharedEvent> event;
#else
    id event;
#endif
    uint64_t current_value;
} metal_event_t;

/**
 * Create a Metal shared event for CPU-GPU synchronization.
 * 
 * @param device Metal device (id<MTLDevice>)
 * @param initial_value Initial value of the event
 * @return Event object, or NULL on failure
 */
#ifdef __OBJC__
metal_event_t* metal_event_create(id<MTLDevice> device, uint64_t initial_value);
#else
metal_event_t* metal_event_create(id device, uint64_t initial_value);
#endif

/**
 * Encode a GPU signal operation on an event.
 * The GPU will set the event to the specified value when it reaches this point.
 * 
 * @param event Event to signal
 * @param cmd_buffer Command buffer to encode signal into
 * @param value Value to signal
 */
void metal_event_encode_signal(metal_event_t* event, metal_command_buffer_t* cmd_buffer, uint64_t value);

/**
 * Encode a GPU wait operation on an event.
 * The GPU will wait until the event reaches the specified value.
 * 
 * @param event Event to wait on
 * @param cmd_buffer Command buffer to encode wait into
 * @param value Value to wait for
 */
void metal_event_encode_wait(metal_event_t* event, metal_command_buffer_t* cmd_buffer, uint64_t value);

/**
 * CPU wait for an event to reach a specific value.
 * Blocks the calling thread until the event reaches the specified value or timeout.
 * 
 * @param event Event to wait on
 * @param value Value to wait for
 * @param timeout_ns Timeout in nanoseconds (0 = no timeout)
 * @return true if event reached value, false if timeout
 */
bool metal_event_wait(metal_event_t* event, uint64_t value, uint64_t timeout_ns);

/**
 * CPU signal an event to a specific value.
 * 
 * @param event Event to signal
 * @param value Value to set
 */
void metal_event_signal(metal_event_t* event, uint64_t value);

/**
 * Get the current value of an event.
 * 
 * @param event Event to query
 * @return Current event value
 */
uint64_t metal_event_get_value(metal_event_t* event);

/**
 * Destroy a Metal event.
 * 
 * @param event Event to destroy
 */
void metal_event_destroy(metal_event_t* event);

// ============================================================================
// Resource Hazard Tracking
// ============================================================================

/**
 * Resource access type for hazard detection.
 */
typedef enum metal_resource_access {
    METAL_RESOURCE_ACCESS_READ,
    METAL_RESOURCE_ACCESS_WRITE,
    METAL_RESOURCE_ACCESS_READ_WRITE
} metal_resource_access_t;

/**
 * Resource dependency tracker for automatic fence insertion.
 */
typedef struct metal_resource_tracker {
    void* resource;                     // Resource pointer (buffer or texture)
    metal_resource_access_t last_access; // Last access type
    metal_fence_t* last_fence;          // Fence from last access
    uint64_t last_frame;                // Frame index of last access
} metal_resource_tracker_t;

/**
 * Check if a resource access creates a hazard and insert fence if needed.
 * 
 * @param tracker Resource tracker
 * @param resource Resource being accessed
 * @param access Access type
 * @param cmd_buffer Command buffer for fence insertion
 * @param frame_index Current frame index
 * @return true if fence was inserted, false otherwise
 */
bool metal_check_resource_hazard(
    metal_resource_tracker_t* tracker,
    void* resource,
    metal_resource_access_t access,
    metal_command_buffer_t* cmd_buffer,
    uint64_t frame_index
);

// ============================================================================
// Pipeline Stage Flags (for fence wait/signal)
// ============================================================================

#define METAL_STAGE_VERTEX      (1 << 0)
#define METAL_STAGE_FRAGMENT    (1 << 1)
#define METAL_STAGE_COMPUTE     (1 << 2)
#define METAL_STAGE_BLIT        (1 << 3)

#ifdef __cplusplus
}
#endif

#endif // MTL_SYNC_H
