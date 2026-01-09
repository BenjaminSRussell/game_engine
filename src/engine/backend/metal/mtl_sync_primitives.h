/*
 * mtl_sync_primitives.h
 * Metal synchronization primitives (fences and events)
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_SYNC_PRIMITIVES_H
#define PLATFORM_MTL_SYNC_PRIMITIVES_H

#include "mtl_device.h"
#include "backend/metal/mtl_encoder.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef void* mtl_fence_t;
typedef void* mtl_event_t;

/* ============================================================================
 * FENCE API
 * ============================================================================ */

/**
 * Creates a fence for synchronization between encoders.
 * @param device The Metal device.
 * @return The fence object.
 */
mtl_fence_t metal_fence_create(metal_device_t* device);

/**
 * Destroys a fence.
 * @param fence The fence to destroy.
 */
void metal_fence_destroy(mtl_fence_t fence);

/* Render Encoder Fence Operations */
void metal_render_encoder_update_fence(mtl_render_command_encoder_t encoder, mtl_fence_t fence);
void metal_render_encoder_wait_for_fence(mtl_render_command_encoder_t encoder, mtl_fence_t fence);

/* Compute Encoder Fence Operations */
void metal_compute_encoder_update_fence(mtl_compute_command_encoder_t encoder, mtl_fence_t fence);
void metal_compute_encoder_wait_for_fence(mtl_compute_command_encoder_t encoder, mtl_fence_t fence);

/* Blit Encoder Fence Operations */
void metal_blit_encoder_update_fence(mtl_blit_command_encoder_t encoder, mtl_fence_t fence);
void metal_blit_encoder_wait_for_fence(mtl_blit_command_encoder_t encoder, mtl_fence_t fence);

/* ============================================================================
 * EVENT API (for fine-grained cross-command-buffer sync)
 * ============================================================================ */

/**
 * Creates an event for cross-command-buffer synchronization.
 * @param device The Metal device.
 * @return The event object.
 */
mtl_event_t metal_event_create(metal_device_t* device);

/**
 * Destroys an event.
 * @param event The event to destroy.
 */
void metal_event_destroy(mtl_event_t event);

/**
 * Encodes a signal event command (signals when this point is reached).
 * @param encoder The encoder (render/compute/blit).
 * @param event The event to signal.
 * @param value The value to signal.
 */
void metal_render_encoder_signal_event(mtl_render_command_encoder_t encoder, mtl_event_t event, uint64_t value);
void metal_compute_encoder_signal_event(mtl_compute_command_encoder_t encoder, mtl_event_t event, uint64_t value);
void metal_blit_encoder_signal_event(mtl_blit_command_encoder_t encoder, mtl_event_t event, uint64_t value);

/**
 * Encodes a wait for event command (waits until event reaches value).
 * @param encoder The encoder (render/compute/blit).
 * @param event The event to wait for.
 * @param value The value to wait for.
 */
void metal_render_encoder_wait_for_event(mtl_render_command_encoder_t encoder, mtl_event_t event, uint64_t value);
void metal_compute_encoder_wait_for_event(mtl_compute_command_encoder_t encoder, mtl_event_t event, uint64_t value);
void metal_blit_encoder_wait_for_event(mtl_blit_command_encoder_t encoder, mtl_event_t event, uint64_t value);

/**
 * Gets the current signaled value of an event.
 * @param event The event.
 * @return The current value.
 */
uint64_t metal_event_get_signaled_value(mtl_event_t event);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_SYNC_PRIMITIVES_H */
