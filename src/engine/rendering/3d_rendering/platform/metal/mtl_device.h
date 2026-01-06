/*
 * mtl_device.h
 * Metal device interface
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_DEVICE_H
#define PLATFORM_MTL_DEVICE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Opaque types for C compatibility */
typedef struct metal_device metal_device_t;
typedef void* mtl_command_buffer_t; /* Maps to id<MTLCommandBuffer> */

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Creates a Metal device instance.
 * Initializes the system default device and standard command queue.
 * @return Pointer to the new device, or NULL on failure.
 */
metal_device_t* metal_device_create(void);

/**
 * Destroys a Metal device instance.
 * Releases all resources associated with the device.
 * @param dev Pointer to the device to destroy.
 */
void metal_device_destroy(metal_device_t* dev);

/**
 * Creates a new command buffer from the device's command queue.
 * @param dev Pointer to the device.
 * @return A new autoreleased command buffer (cast to void* for C).
 */
mtl_command_buffer_t metal_create_command_buffer(metal_device_t* dev);

/**
 * Returns the maximum number of threads per threadgroup.
 */
uint32_t metal_device_get_max_threads_per_group(metal_device_t* dev);

/**
 * Checks if the device supports ray tracing.
 */
bool metal_device_supports_raytracing(metal_device_t* dev);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_DEVICE_H */
