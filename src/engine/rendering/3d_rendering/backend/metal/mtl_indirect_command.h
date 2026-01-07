/*
 * mtl_indirect_command.h
 * Metal indirect command buffer interface for GPU-driven rendering
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_INDIRECT_COMMAND_H
#define PLATFORM_MTL_INDIRECT_COMMAND_H

#include "mtl_device.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef void* mtl_indirect_command_buffer_t;

typedef enum mtl_indirect_command_type {
    MTL_INDIRECT_COMMAND_TYPE_DRAW = (1 << 0),
    MTL_INDIRECT_COMMAND_TYPE_DRAW_INDEXED = (1 << 1),
    MTL_INDIRECT_COMMAND_TYPE_DRAW_PATCHES = (1 << 2),
    MTL_INDIRECT_COMMAND_TYPE_DRAW_INDEXED_PATCHES = (1 << 3),
    MTL_INDIRECT_COMMAND_TYPE_CONCURRENT_DISPATCH = (1 << 5),
    MTL_INDIRECT_COMMAND_TYPE_CONCURRENT_DISPATCH_THREADS = (1 << 6)
} mtl_indirect_command_type_t;

typedef struct mtl_indirect_command_buffer_descriptor {
    unsigned int command_types;  // Bitfield of mtl_indirect_command_type_t
    bool inherit_buffers;
    bool inherit_pipeline_state;
    unsigned long max_vertex_buffer_bind_count;
    unsigned long max_fragment_buffer_bind_count;
    unsigned long max_kernel_buffer_bind_count;
} mtl_indirect_command_buffer_descriptor_t;

/* ============================================================================
 * INDIRECT COMMAND BUFFER API
 * ============================================================================ */

/**
 * Creates an indirect command buffer for GPU-driven rendering.
 * @param device The Metal device.
 * @param descriptor The descriptor specifying ICB capabilities.
 * @param max_command_count Maximum number of commands the ICB can hold.
 * @return The indirect command buffer.
 */
mtl_indirect_command_buffer_t metal_indirect_command_buffer_create(
    metal_device_t* device, 
    mtl_indirect_command_buffer_descriptor_t* descriptor, 
    unsigned long max_command_count);

/**
 * Destroys an indirect command buffer.
 * @param icb The indirect command buffer to destroy.
 */
void metal_indirect_command_buffer_destroy(mtl_indirect_command_buffer_t icb);

/**
 * Resets a range of commands in the ICB.
 * @param icb The indirect command buffer.
 * @param start_index The starting command index.
 * @param count The number of commands to reset.
 */
void metal_indirect_command_buffer_reset_range(mtl_indirect_command_buffer_t icb, unsigned long start_index, unsigned long count);

/**
 * Gets the size (in bytes) of the indirect command buffer.
 * @param icb The indirect command buffer.
 * @return The size in bytes.
 */
unsigned long metal_indirect_command_buffer_get_size(mtl_indirect_command_buffer_t icb);

/**
 * Gets the GPU resource ID for the ICB (for use in compute shaders).
 * @param icb The indirect command buffer.
 * @return The GPU resource ID.
 */
uint64_t metal_indirect_command_buffer_get_gpu_resource_id(mtl_indirect_command_buffer_t icb);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_INDIRECT_COMMAND_H */
