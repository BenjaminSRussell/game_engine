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

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_DRAW_COMMAND_GEN_H */
