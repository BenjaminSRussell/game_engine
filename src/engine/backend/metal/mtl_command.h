/*
 * mtl_command.h
 * Metal command encoding interface
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_COMMAND_H
#define PLATFORM_MTL_COMMAND_H

#include "mtl_device.h"
#include "mtl_command_buffer.h"
#include "mtl_encoder.h"
#include "mtl_parallel_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Legacy/Convenience Header
 * All functionality has been moved to:
 * - mtl_command_buffer.h: Lifecycle, Status, Commit
 * - mtl_encoder.h: Render, Compute, and Blit Encoders
 * - mtl_parallel_encoder.h: Parallel Encoding
 */

/* Opaque types for C compatibility */
typedef void* mtl_render_command_encoder_t; /* Maps to id<MTLRenderCommandEncoder> */
typedef void* mtl_compute_command_encoder_t; /* Maps to id<MTLComputeCommandEncoder> */
typedef void* mtl_blit_command_encoder_t;    /* Maps to id<MTLBlitCommandEncoder> */

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Commits a command buffer for execution.
 * @param buffer The command buffer to commit.
 */
void metal_command_buffer_commit(mtl_command_buffer_t buffer);

/**
 * Waits for the command buffer to complete execution.
 * @param buffer The command buffer to wait for.
 */
void metal_command_buffer_wait_until_completed(mtl_command_buffer_t buffer);

/**
 * Enqueues a command buffer.
 * @param buffer The command buffer to enqueue.
 */
void metal_command_buffer_enqueue(mtl_command_buffer_t buffer);

/**
 * Creates a render command encoder.
 * Note: Real engine usage would require a descriptor. This is a basic helper.
 * @param buffer The command buffer to encode into.
 * @param pass_descriptor Pointer to MTLRenderPassDescriptor (void*).
 * @return The render encoder or NULL.
 */
mtl_render_command_encoder_t metal_command_encoder_render_create(mtl_command_buffer_t buffer, void* pass_descriptor);

/**
 * Creates a compute command encoder.
 * @param buffer The command buffer to encode into.
 * @return The compute encoder.
 */
mtl_compute_command_encoder_t metal_command_encoder_compute_create(mtl_command_buffer_t buffer);

/**
 * Creates a blit command encoder.
 * @param buffer The command buffer to encode into.
 * @return The blit encoder.
 */
mtl_blit_command_encoder_t metal_command_encoder_blit_create(mtl_command_buffer_t buffer);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_COMMAND_H */
