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
 * TYPES
 * ============================================================================ */

typedef struct rendering_draw_command_gen_handle {
    uint32_t id;
} rendering_draw_command_gen_handle_t;

typedef struct rendering_draw_command_gen_desc {
    uint32_t flags;
    void* user_data;
} rendering_draw_command_gen_desc_t;

typedef struct rendering_draw_command_gen_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
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
int rendering_draw_command_gen_process_pending(void);

/* Statistics */
uint32_t rendering_draw_command_gen_get_count(void);
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
