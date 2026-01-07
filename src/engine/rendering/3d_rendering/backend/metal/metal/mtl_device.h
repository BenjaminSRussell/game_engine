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
typedef void* mtl_command_queue_t;  /* Maps to id<MTLCommandQueue> */

/* Error Codes */
typedef enum mtl_error_code {
    MTL_SUCCESS = 0,
    MTL_ERROR_DEVICE_NOT_FOUND = -1,
    MTL_ERROR_DEVICE_CREATION_FAILED = -2,
    MTL_ERROR_QUEUE_CREATION_FAILED = -3,
    MTL_ERROR_INVALID_PARAMETER = -4,
    MTL_ERROR_OUT_OF_MEMORY = -5,
    MTL_ERROR_DEVICE_LOST = -6,
    MTL_ERROR_NOT_SUPPORTED = -7,
    MTL_ERROR_VALIDATION_FAILED = -8,
} mtl_error_code_t;

/* Queue Types */
typedef enum mtl_queue_type {
    MTL_QUEUE_GRAPHICS = 0,   /* General purpose graphics + compute */
    MTL_QUEUE_COMPUTE = 1,    /* Compute-focused queue */
    MTL_QUEUE_TRANSFER = 2,   /* Transfer/blit operations */
} mtl_queue_type_t;

/* Queue Priority */
typedef enum mtl_queue_priority {
    MTL_QUEUE_PRIORITY_DEFAULT = 0,
    MTL_QUEUE_PRIORITY_HIGH = 1,
    MTL_QUEUE_PRIORITY_LOW = 2,
} mtl_queue_priority_t;

/* Device Statistics */
typedef struct mtl_device_stats {
    uint64_t frames_rendered;
    uint64_t commands_submitted;
    uint64_t total_memory_allocated;
    uint64_t peak_memory_usage;
    uint32_t active_command_queues;
    uint32_t active_heaps;
} mtl_device_stats_t;

/* Device Creation Options */
typedef struct mtl_device_options {
    bool enable_debug_layer;
    bool enable_gpu_validation;
    bool prefer_low_power;
    uint32_t reserved[8];
} mtl_device_options_t;

/* ============================================================================
 * API - Device Lifecycle
 * ============================================================================ */

/**
 * Creates or retrieves the system default Metal device (Singleton).
 * Initializes the device, capabilities, and primary command queues on first call.
 * @return Pointer to the shared device instance, or NULL on failure.
 */
metal_device_t* metal_device_create_system_default(void);

/**
 * Creates a Metal device with custom options.
 * @param options Device creation options, or NULL for defaults.
 * @param out_error Optional error code output.
 * @return Pointer to the device instance, or NULL on failure.
 */
metal_device_t* metal_device_create_with_options(const mtl_device_options_t* options, mtl_error_code_t* out_error);

/**
 * Retrieves the existing system default Metal device.
 * @return Pointer to the shared device instance, or NULL if not created.
 */
metal_device_t* metal_device_get_default(void);

/**
 * Increments the reference count of the device.
 * @param dev Pointer to the device.
 */
void metal_device_retain(metal_device_t* dev);

/**
 * Decrements the reference count and destroys if zero.
 * @param dev Pointer to the device.
 */
void metal_device_release(metal_device_t* dev);

/**
 * Destroys the metal device and releases resources.
 * For the singleton, this should be called at application shutdown.
 * @param dev Pointer to the device to destroy.
 */
void metal_device_destroy(metal_device_t* dev);

/**
 * Validates the device is still valid and functional.
 * @param dev Pointer to the device.
 * @return true if valid, false otherwise.
 */
bool metal_device_validate(metal_device_t* dev);

/**
 * Gets the device name string.
 * @param dev Pointer to the device.
 * @return Device name, or NULL if invalid.
 */
const char* metal_device_get_name(metal_device_t* dev);

/**
 * Gets the device registry ID.
 * @param dev Pointer to the device.
 * @return Registry ID, or 0 if invalid.
 */
uint64_t metal_device_get_registry_id(metal_device_t* dev);

/* ============================================================================
 * API - Command Queue Management
 * ============================================================================ */

/**
 * Creates a command queue with specified type and priority.
 * @param dev Pointer to the device.
 * @param type Queue type.
 * @param priority Queue priority.
 * @param label Optional debug label (can be NULL).
 * @param out_error Optional error code output.
 * @return Queue handle, or NULL on failure.
 */
mtl_command_queue_t metal_device_create_queue(metal_device_t* dev, 
                                               mtl_queue_type_t type,
                                               mtl_queue_priority_t priority,
                                               const char* label,
                                               mtl_error_code_t* out_error);

/**
 * Gets the primary graphics queue.
 * @param dev Pointer to the device.
 * @return Queue handle, or NULL if not created.
 */
mtl_command_queue_t metal_device_get_graphics_queue(metal_device_t* dev);

/**
 * Gets a dedicated compute queue (creates if not exists).
 * @param dev Pointer to the device.
 * @return Queue handle, or NULL on failure.
 */
mtl_command_queue_t metal_device_get_compute_queue(metal_device_t* dev);

/**
 * Gets a dedicated transfer queue (creates if not exists).
 * @param dev Pointer to the device.
 * @return Queue handle, or NULL on failure.
 */
mtl_command_queue_t metal_device_get_transfer_queue(metal_device_t* dev);

/**
 * Destroys a command queue.
 * @param dev Pointer to the device.
 * @param queue Queue handle to destroy.
 */
void metal_device_destroy_queue(metal_device_t* dev, mtl_command_queue_t queue);

/**
 * Creates a new command buffer from the device's primary command queue.
 * @param dev Pointer to the device.
 * @return A new autoreleased command buffer (cast to void* for C).
 */
mtl_command_buffer_t metal_create_command_buffer(metal_device_t* dev);

/**
 * Creates a command buffer from a specific queue.
 * @param queue The queue to create the buffer from.
 * @return A new autoreleased command buffer.
 */
mtl_command_buffer_t metal_create_command_buffer_from_queue(mtl_command_queue_t queue);

/* ============================================================================
 * API - Capabilities & Information
 * ============================================================================ */

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
 * Retrieves device statistics.
 * @param dev Pointer to the device.
 * @param out_stats Pointer to the structure to fill.
 */
void metal_device_get_stats(metal_device_t* dev, mtl_device_stats_t* out_stats);

/**
 * Checks if the device supports ray tracing.
 */
bool metal_device_supports_raytracing(metal_device_t* dev);

/**
 * Exports device capabilities to a human-readable string.
 * @param dev Pointer to the device.
 * @param buffer Output buffer.
 * @param buffer_size Size of the buffer.
 * @return Number of bytes written (excluding null terminator).
 */
size_t metal_device_export_capabilities(metal_device_t* dev, char* buffer, size_t buffer_size);

/* ============================================================================
 * API - Debugging & Profiling
 * ============================================================================ */

/**
 * Enables GPU capture for debugging in Xcode.
 * @param dev Pointer to the device.
 * @return true if enabled successfully.
 */
bool metal_device_enable_gpu_capture(metal_device_t* dev);

/**
 * Begins a GPU capture frame.
 * @param dev Pointer to the device.
 */
void metal_device_begin_capture(metal_device_t* dev);

/**
 * Ends a GPU capture frame.
 * @param dev Pointer to the device.
 */
void metal_device_end_capture(metal_device_t* dev);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_DEVICE_H */
