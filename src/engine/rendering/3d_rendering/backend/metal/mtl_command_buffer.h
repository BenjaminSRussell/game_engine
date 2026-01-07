/*
 * mtl_command_buffer.h
 * Metal command buffer lifecycle
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_COMMAND_BUFFER_H
#define PLATFORM_MTL_COMMAND_BUFFER_H

#include "mtl_device.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef void (*mtl_command_buffer_handler_t)(mtl_command_buffer_t buffer);

typedef enum mtl_command_buffer_status {
    MTL_COMMAND_BUFFER_STATUS_NOT_ENQUEUED = 0,
    MTL_COMMAND_BUFFER_STATUS_ENQUEUED = 1,
    MTL_COMMAND_BUFFER_STATUS_COMMITTED = 2,
    MTL_COMMAND_BUFFER_STATUS_SCHEDULED = 3,
    MTL_COMMAND_BUFFER_STATUS_COMPLETED = 4,
    MTL_COMMAND_BUFFER_STATUS_ERROR = 5
} mtl_command_buffer_status_t;

typedef enum mtl_command_buffer_error {
    MTL_COMMAND_BUFFER_ERROR_NONE = 0,
    MTL_COMMAND_BUFFER_ERROR_INTERNAL = 1,
    MTL_COMMAND_BUFFER_ERROR_TIMEOUT = 2,
    MTL_COMMAND_BUFFER_ERROR_PAGE_FAULT = 3,
    MTL_COMMAND_BUFFER_ERROR_ACCESS_REVOKED = 4,
    MTL_COMMAND_BUFFER_ERROR_OUT_OF_MEMORY = 5,
    MTL_COMMAND_BUFFER_ERROR_INVALID_RESOURCE = 6
} mtl_command_buffer_error_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Lifecycle */
mtl_command_buffer_t metal_command_buffer_create(metal_device_t* device);
void metal_command_buffer_set_label(mtl_command_buffer_t buffer, const char* label);
void metal_command_buffer_enqueue(mtl_command_buffer_t buffer);
void metal_command_buffer_commit(mtl_command_buffer_t buffer);
void metal_command_buffer_wait_until_scheduled(mtl_command_buffer_t buffer);
void metal_command_buffer_wait_until_completed(mtl_command_buffer_t buffer);

/* Debugging */
void metal_command_buffer_push_debug_group(mtl_command_buffer_t buffer, const char* label);
void metal_command_buffer_pop_debug_group(mtl_command_buffer_t buffer);

/* Status & Errors */
mtl_command_buffer_status_t metal_command_buffer_get_status(mtl_command_buffer_t buffer);
mtl_command_buffer_error_t metal_command_buffer_get_error(mtl_command_buffer_t buffer);
const char* metal_command_buffer_get_error_string(mtl_command_buffer_t buffer);

/* Calbacks */
void metal_command_buffer_add_scheduled_handler(mtl_command_buffer_t buffer, mtl_command_buffer_handler_t handler);
void metal_command_buffer_add_completed_handler(mtl_command_buffer_t buffer, mtl_command_buffer_handler_t handler);

/* Timing */
double metal_command_buffer_get_gpu_start_time(mtl_command_buffer_t buffer);
double metal_command_buffer_get_gpu_end_time(mtl_command_buffer_t buffer);
double metal_command_buffer_get_kernel_start_time(mtl_command_buffer_t buffer);
double metal_command_buffer_get_kernel_end_time(mtl_command_buffer_t buffer);

/* ============================================================================
 * COMMAND BUFFER POOLING
 * ============================================================================ */

typedef struct mtl_command_buffer_pool* mtl_command_buffer_pool_t;

/**
 * Creates a command buffer pool for efficient reuse.
 * @param device The Metal device.
 * @param max_count Maximum number of pooled buffers (0 = unlimited).
 * @return The command buffer pool.
 */
mtl_command_buffer_pool_t metal_command_buffer_pool_create(metal_device_t* device, unsigned long max_count);

/**
 * Destroys a command buffer pool.
 * @param pool The pool to destroy.
 */
void metal_command_buffer_pool_destroy(mtl_command_buffer_pool_t pool);

/**
 * Acquires a command buffer from the pool (reuses completed buffers).
 * @param pool The command buffer pool.
 * @return A command buffer ready for encoding.
 */
mtl_command_buffer_t metal_command_buffer_pool_acquire(mtl_command_buffer_pool_t pool);

/**
 * Returns a completed command buffer to the pool for reuse.
 * @param pool The command buffer pool.
 * @param buffer The command buffer to release.
 */
void metal_command_buffer_pool_release(mtl_command_buffer_pool_t pool, mtl_command_buffer_t buffer);

/* ============================================================================
 * TRIPLE BUFFERING & FRAME SYNCHRONIZATION
 * ============================================================================ */

typedef struct mtl_frame_sync* mtl_frame_sync_t;

/**
 * Creates a triple buffering synchronization system.
 * @param device The Metal device.
 * @return The frame sync object.
 */
mtl_frame_sync_t metal_frame_sync_create(metal_device_t* device);

/**
 * Destroys a frame sync object.
 * @param sync The frame sync object to destroy.
 */
void metal_frame_sync_destroy(mtl_frame_sync_t sync);

/**
 * Waits for the next frame buffer to become available (limits in-flight frames to 3).
 * @param sync The frame sync object.
 */
void metal_frame_sync_wait_for_next_frame(mtl_frame_sync_t sync);

/**
 * Signals that the current frame has been submitted.
 * @param sync The frame sync object.
 */
void metal_frame_sync_signal_frame_complete(mtl_frame_sync_t sync);

/**
 * Gets the current frame index (0-2 for triple buffering).
 * @param sync The frame sync object.
 * @return The current frame index.
 */
unsigned int metal_frame_sync_get_current_frame_index(mtl_frame_sync_t sync);

/* ============================================================================
 * PRESENTATION TIMING
 * ============================================================================ */

/**
 * Presents a drawable when the command buffer completes.
 * @param buffer The command buffer.
 * @param drawable The CAMetalDrawable to present (void*).
 */
void metal_command_buffer_present_drawable(mtl_command_buffer_t buffer, void* drawable);

/**
 * Presents a drawable at a specific host time.
 * @param buffer The command buffer.
 * @param drawable The CAMetalDrawable to present.
 * @param presentation_time The absolute host time to present at (CFAbsoluteTime).
 */
void metal_command_buffer_present_drawable_at_time(mtl_command_buffer_t buffer, void* drawable, double presentation_time);

/**
 * Presents a drawable after a minimum duration from now.
 * @param buffer The command buffer.
 * @param drawable The CAMetalDrawable to present.
 * @param minimum_duration Minimum time to wait before presenting (seconds).
 */
void metal_command_buffer_present_drawable_after_minimum_duration(mtl_command_buffer_t buffer, void* drawable, double minimum_duration);

/* ============================================================================
 * VALIDATION & RETRY
 * ============================================================================ */

/**
 * Validates command buffer state before commit.
 * @param buffer The command buffer to validate.
 * @return true if valid, false otherwise.
 */
bool metal_command_buffer_validate(mtl_command_buffer_t buffer);

/**
 * Attempts to retry a failed command buffer.
 * Creates a new buffer and re-executes if possible.
 * @param buffer The failed command buffer.
 * @param max_retries Maximum number of retry attempts.
 * @return New command buffer on success, NULL on failure.
 */
mtl_command_buffer_t metal_command_buffer_retry_on_error(mtl_command_buffer_t buffer, unsigned int max_retries);


#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_COMMAND_BUFFER_H */
