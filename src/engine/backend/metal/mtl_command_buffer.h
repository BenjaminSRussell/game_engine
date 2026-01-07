/*
 * mtl_command_buffer.h
 * Metal command buffer lifecycle
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_COMMAND_BUFFER_H
#define PLATFORM_MTL_COMMAND_BUFFER_H

#include "backend/metal/mtl_device.h"
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

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_COMMAND_BUFFER_H */
