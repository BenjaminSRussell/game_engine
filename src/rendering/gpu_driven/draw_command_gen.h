/*
 * draw_command_gen.h
 * Draw command generation
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_DRAW_COMMAND_GEN_H
#define RENDERING_DRAW_COMMAND_GEN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

typedef struct metal_device metal_device_t;
typedef struct metal_buffer metal_buffer_t;

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Batch information for grouped draw commands */
typedef struct draw_batch {
    uint32_t mesh_id;              // Mesh identifier
    uint32_t material_id;          // Material identifier
    uint32_t instance_count;       // Number of instances in batch
    uint32_t first_instance;       // First instance index
    uint32_t index_count;          // Indices per instance
    uint32_t index_offset;         // Offset into index buffer
} draw_batch_t;

/* GPU counter structure for atomic operations */
typedef struct gpu_counter {
    uint32_t value;
} gpu_counter_t;

typedef struct rendering_draw_command_gen_handle {
    uint32_t id;
} rendering_draw_command_gen_handle_t;

typedef struct rendering_draw_command_gen_desc {
    metal_device_t* device;        // Metal device
    uint32_t max_draw_commands;    // Maximum draw commands
    uint32_t flags;
    void* user_data;
} rendering_draw_command_gen_desc_t;

typedef struct rendering_draw_command_gen_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t pending_commands;
    uint32_t max_capacity;
    size_t memory_used;
} rendering_draw_command_gen_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_draw_command_gen_init(void);
void rendering_draw_command_gen_shutdown(void);

/* Lifecycle */
int rendering_draw_command_gen_create(rendering_draw_command_gen_handle_t* out_handle, const rendering_draw_command_gen_desc_t* desc);
void rendering_draw_command_gen_destroy(rendering_draw_command_gen_handle_t handle);

/* Operations */
int rendering_draw_command_gen_update(rendering_draw_command_gen_handle_t handle, const void* data, size_t size);
bool rendering_draw_command_gen_is_valid(rendering_draw_command_gen_handle_t handle);
int rendering_draw_command_gen_get_info(rendering_draw_command_gen_handle_t handle, rendering_draw_command_gen_info_t* out_info);
void rendering_draw_command_gen_mark_dirty(rendering_draw_command_gen_handle_t handle);

/**
 * Process pending draw commands and upload to GPU
 * @param device Metal device (for buffer operations)
 * @return Number of commands processed
 */
int rendering_draw_command_gen_process_pending(metal_device_t* device);

/**
 * Get Metal buffer for indirect draw commands
 * @return Metal buffer pointer or NULL
 */
metal_buffer_t* rendering_draw_command_gen_get_command_buffer(rendering_draw_command_gen_handle_t handle);

/**
 * Get Metal buffer for visible instance IDs
 * @return Metal buffer pointer or NULL
 */
metal_buffer_t* rendering_draw_command_gen_get_visible_buffer(rendering_draw_command_gen_handle_t handle);

/**
 * Get Metal buffer for command counters
 * @return Metal buffer pointer or NULL
 */
metal_buffer_t* rendering_draw_command_gen_get_counter_buffer(rendering_draw_command_gen_handle_t handle);

/**
 * Add a batch to the pending queue
 * @param handle Generator handle
 * @param batch Batch information
 * @return Batch index or negative on error
 */
int rendering_draw_command_gen_add_batch(rendering_draw_command_gen_handle_t handle,
                                        const draw_batch_t* batch);

/**
 * Clear all pending batches
 */
void rendering_draw_command_gen_clear_batches(rendering_draw_command_gen_handle_t handle);

/**
 * Get current batch count
 */
uint32_t rendering_draw_command_gen_get_batch_count(rendering_draw_command_gen_handle_t handle);

/* Statistics */
uint32_t rendering_draw_command_gen_get_count(void);
uint32_t rendering_draw_command_gen_get_total_draw_commands(void);
size_t rendering_draw_command_gen_get_memory_usage(void);
void rendering_draw_command_gen_debug_print(void);

/* GPU-Driven Rendering Support */
#include "../core/gpu_types.h"

/* Generate indirect draw arguments from culling results */
int rendering_draw_command_gen_generate_from_culling(
    rendering_draw_command_gen_handle_t handle,
    const CullingResult* culling_results,
    uint32_t result_count,
    IndirectDrawArgs* out_args,
    uint32_t* out_arg_count);

/* Populate Metal's indirect command buffer structure */
int rendering_draw_command_gen_populate_icb(
    rendering_draw_command_gen_handle_t handle,
    const IndirectDrawArgs* args,
    uint32_t arg_count,
    void* icb_buffer);

/* Get indirect argument buffer for GPU upload */
IndirectDrawArgs* rendering_draw_command_gen_get_indirect_buffer(
    rendering_draw_command_gen_handle_t handle,
    uint32_t* out_count);

/* Get draw count for multi-draw indirect */
uint32_t rendering_draw_command_gen_get_draw_count(rendering_draw_command_gen_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_DRAW_COMMAND_GEN_H */
