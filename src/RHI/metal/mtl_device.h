/*
 * mtl_device.h
 * Metal device interface
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_DEVICE_H
#define PLATFORM_MTL_DEVICE_H

#include "mtl_device_caps.h"
#include "mtl_memory_heap.h"

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
 * Creates or retrieves the system default Metal device (Singleton).
 * Initializes the device, capabilities, and primary command queues on first call.
 * @return Pointer to the shared device instance, or NULL on failure.
 */
metal_device_t* metal_device_create_system_default(void);

/**
 * Retrieves the existing system default Metal device.
 * @return Pointer to the shared device instance, or NULL if not created.
 */
metal_device_t* metal_device_get_default(void);

/**
 * Retrieves the underlying MTLDevice (id<MTLDevice> cast to void*).
 * @param dev Pointer to the device wrapper.
 * @return Raw MTLDevice pointer or NULL.
 */
void* metal_get_device(metal_device_t* dev);


/**
 * Destroys the metal device and releases resources.
 * For the singleton, this should be called at application shutdown.
 * @param dev Pointer to the device to destroy.
 */
void metal_device_destroy(metal_device_t* dev);

/**
 * Creates a new command buffer from the device's primary command queue.
 * @param dev Pointer to the device.
 * @return A new autoreleased command buffer (cast to void* for C).
 */
mtl_command_buffer_t metal_create_command_buffer(metal_device_t* dev);

/**
 * Retrieves the capabilities of the device.
 * @param dev Pointer to the device.
 * @return Pointer to the capabilities structure.
 */
const mtl_device_caps_t* metal_device_get_caps(metal_device_t* dev);

/**
 * Retrieves the current memory statistics of the device.
 * @param dev Pointer to the device.
 * @param out_info Pointer to the structure to fill.
 */
void metal_device_get_memory_info(metal_device_t* dev, mtl_memory_info_t* out_info);

/**
 * Checks if the device supports ray tracing.
 */
bool metal_device_supports_raytracing(metal_device_t* dev);



#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_DEVICE_H */
