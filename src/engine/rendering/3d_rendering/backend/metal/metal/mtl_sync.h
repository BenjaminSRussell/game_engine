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
 * Frame timing statistics for performance monitoring.
 */
typedef struct metal_frame_stats {
    double min_frame_time_ms;              // Minimum frame time recorded
    double max_frame_time_ms;              // Maximum frame time recorded
    double avg_frame_time_ms;              // Rolling average frame time
    double target_frame_time_ms;           // Target frame time (e.g., 16.67ms for 60 FPS)
    uint64_t total_frames;                 // Total number of frames rendered
    uint64_t dropped_frames;               // Number of frames that exceeded budget
} metal_frame_stats_t;

/**
 * Frame pacing mode.
 */
typedef enum metal_frame_pacing_mode {
    METAL_FRAME_PACING_FIXED,              // Fixed frame rate
    METAL_FRAME_PACING_VARIABLE,           // Variable frame rate
    METAL_FRAME_PACING_ADAPTIVE            // Adaptive based on GPU load
} metal_frame_pacing_mode_t;

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
    
    // Enhanced timing and statistics
    uint64_t frame_start_time;             // Start time of current frame (mach_absolute_time)
    uint64_t timeout_ns;                   // Timeout for frame wait in nanoseconds (0 = infinite)
    metal_frame_stats_t stats;             // Frame statistics
    metal_frame_pacing_mode_t pacing_mode; // Frame pacing mode
    
    // Deadlock detection
    uint64_t last_completed_frame;         // Last frame that completed successfully
    uint32_t deadlock_threshold;           // Frames to wait before detecting deadlock
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
 * Begin a new frame with timeout. Blocks if max frames are already in flight.
 * Returns false if timeout is exceeded.
 * 
 * @param sync Frame synchronization object
 * @param timeout_ns Timeout in nanoseconds (0 = use default from sync object)
 * @return true if frame started successfully, false if timeout
 */
bool metal_frame_begin_with_timeout(metal_frame_sync_t* sync, uint64_t timeout_ns);

/**
 * Wait for a specific frame to complete.
 * 
 * @param sync Frame synchronization object
 * @param frame_index Frame index to wait for
 * @param timeout_ns Timeout in nanoseconds (0 = infinite)
 * @return true if frame completed, false if timeout
 */
bool metal_frame_wait_for_completion(metal_frame_sync_t* sync, uint64_t frame_index, uint64_t timeout_ns);

/**
 * Get the current frame index.
 * 
 * @param sync Frame synchronization object
 * @return Current frame index
 */
uint64_t metal_frame_get_current_index(metal_frame_sync_t* sync);

/**
 * Get frame statistics.
 * 
 * @param sync Frame synchronization object
 * @param stats Output statistics structure
 */
void metal_frame_get_stats(metal_frame_sync_t* sync, metal_frame_stats_t* stats);

/**
 * Reset frame statistics.
 * 
 * @param sync Frame synchronization object
 */
void metal_frame_reset_stats(metal_frame_sync_t* sync);

/**
 * Set frame pacing mode.
 * 
 * @param sync Frame synchronization object
 * @param mode Pacing mode to set
 */
void metal_frame_set_pacing_mode(metal_frame_sync_t* sync, metal_frame_pacing_mode_t mode);

/**
 * Set target frame time for frame budget enforcement.
 * 
 * @param sync Frame synchronization object
 * @param target_fps Target frames per second (e.g., 60, 144)
 */
void metal_frame_set_target_fps(metal_frame_sync_t* sync, uint32_t target_fps);

/**
 * Check if current frame is within budget.
 * 
 * @param sync Frame synchronization object
 * @return true if within budget, false if exceeded
 */
bool metal_frame_is_within_budget(metal_frame_sync_t* sync);

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

void metal_event_destroy(metal_event_t* event);

// ============================================================================
// Extended Event APIs
// ============================================================================

/**
 * Event listener callback function type.
 * Called when an event reaches a specific value.
 * 
 * @param event Event that was signaled
 * @param value Value that was reached
 * @param user_data User-provided data
 */
typedef void (*metal_event_callback_t)(metal_event_t* event, uint64_t value, void* user_data);

/**
 * Register a listener callback for an event.
 * The callback will be invoked when the event reaches the specified value.
 * 
 * @param event Event to listen to
 * @param value Value to wait for
 * @param callback Callback function to invoke
 * @param user_data User data to pass to callback
 * @return true if listener registered successfully, false otherwise
 */
bool metal_event_add_listener(metal_event_t* event, uint64_t value, metal_event_callback_t callback, void* user_data);

/**
 * Wait for multiple events to reach specific values.
 * Blocks until all events reach their target values or timeout.
 * 
 * @param events Array of events to wait for
 * @param values Array of values to wait for (parallel to events)
 * @param count Number of events in the arrays
 * @param timeout_ns Timeout in nanoseconds (0 = infinite)
 * @return true if all events reached values, false if timeout
 */
bool metal_event_wait_multiple(metal_event_t** events, uint64_t* values, uint32_t count, uint64_t timeout_ns);

/**
 * Encode GPU waits for multiple events in a command buffer.
 * 
 * @param events Array of events to wait for
 * @param values Array of values to wait for (parallel to events)
 * @param count Number of events in the arrays
 * @param cmd_buffer Command buffer to encode waits into
 */
void metal_event_encode_wait_multiple(metal_event_t** events, uint64_t* values, uint32_t count, metal_command_buffer_t* cmd_buffer);

/**
 * Create a synchronization point for CPU-GPU coordination.
 * Helper that creates an event and signals it immediately.
 * 
 * @param device Metal device
 * @return Event at value 1, or NULL on failure
 */
#ifdef __OBJC__
metal_event_t* metal_create_sync_point(id<MTLDevice> device);
#else
metal_event_t* metal_create_sync_point(id device);
#endif

// ============================================================================
// Resource Hazard Tracking
// ============================================================================

/**
 * Resource type for hazard tracking.
 */
typedef enum metal_resource_type {
    METAL_RESOURCE_TYPE_BUFFER,
    METAL_RESOURCE_TYPE_TEXTURE
} metal_resource_type_t;

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
    metal_resource_type_t type;         // Resource type
    metal_resource_access_t last_access; // Last access type
    metal_fence_t* last_fence;          // Fence from last access
    uint64_t last_frame;                // Frame index of last access
    bool requires_cache_flush;          // Cache coherence flag
} metal_resource_tracker_t;

/**
 * Hazard analysis result.
 */
typedef struct metal_hazard_info {
    bool has_hazard;                    // True if hazard detected
    bool is_raw;                        // Read-after-write hazard
    bool is_war;                        // Write-after-read hazard
    bool is_waw;                        // Write-after-write hazard
    bool needs_barrier;                 // Needs memory barrier
    uint32_t recommended_stages;        // Recommended pipeline stages for barrier
} metal_hazard_info_t;

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

/**
 * Analyze a resource access for hazards without inserting barriers.
 * 
 * @param tracker Resource tracker
 * @param resource Resource being accessed
 * @param access Access type
 * @param info Output hazard information
 * @return true if hazard detected, false otherwise
 */
bool metal_analyze_resource_hazard(
    metal_resource_tracker_t* tracker,
    void* resource,
    metal_resource_access_t access,
    metal_hazard_info_t* info
);

/**
 * Insert a memory barrier for a texture resource.
 * 
 * @param cmd_buffer Command buffer to encode barrier into
 * @param texture Texture resource
 * @param stages Pipeline stages to apply barrier
 */
void metal_insert_texture_barrier(
    metal_command_buffer_t* cmd_buffer,
    void* texture,
    uint32_t stages
);

/**
 * Insert a memory barrier for a buffer resource.
 * 
 * @param cmd_buffer Command buffer to encode barrier into
 * @param buffer Buffer resource
 * @param stages Pipeline stages to apply barrier
 */
void metal_insert_buffer_barrier(
    metal_command_buffer_t* cmd_buffer,
    void* buffer,
    uint32_t stages
);

/**
 * Initialize a resource tracker.
 * 
 * @param tracker Resource tracker to initialize
 * @param type Resource type
 */
void metal_resource_tracker_init(metal_resource_tracker_t* tracker, metal_resource_type_t type);

/**
 * Reset a resource tracker.
 * 
 * @param tracker Resource tracker to reset
 */
void metal_resource_tracker_reset(metal_resource_tracker_t* tracker);

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
