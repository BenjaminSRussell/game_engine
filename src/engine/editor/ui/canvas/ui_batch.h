/*
 * ui_batch.h
 * UI draw batching
 *
 * Part of the Ui Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_RENDERING_UI_BATCH_H
#define UI_RENDERING_UI_BATCH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct ui_rendering_ui_batch_handle {
    uint32_t id;
} ui_rendering_ui_batch_handle_t;

typedef struct ui_rendering_ui_batch_desc {
    uint32_t flags;
    void* user_data;
} ui_rendering_ui_batch_desc_t;

typedef struct ui_rendering_ui_batch_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} ui_rendering_ui_batch_info_t;

/* Positional order matches the canvas sources: x, y, z, u, v, color, flags, depth. */
typedef struct ui_rendering_vertex {
    float x;
    float y;
    float z;
    float u;
    float v;
    uint32_t color;
    uint32_t flags;
    float depth;
} ui_rendering_vertex_t;

typedef struct ui_rendering_draw_command {
    uint32_t vertex_offset;
    uint32_t vertex_count;
    uint32_t index_offset;
    uint32_t index_count;
    uint32_t material_id;
    uint32_t texture_id;
    uint32_t blend_mode;
    float z_order;
} ui_rendering_draw_command_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int ui_rendering_ui_batch_init(void);
void ui_rendering_ui_batch_shutdown(void);

/* Lifecycle */
int ui_rendering_ui_batch_create(ui_rendering_ui_batch_handle_t* out_handle, const ui_rendering_ui_batch_desc_t* desc);
void ui_rendering_ui_batch_destroy(ui_rendering_ui_batch_handle_t handle);

/* Operations */
int ui_rendering_ui_batch_update(ui_rendering_ui_batch_handle_t handle, const void* data, size_t size);
bool ui_rendering_ui_batch_is_valid(ui_rendering_ui_batch_handle_t handle);
int ui_rendering_ui_batch_get_info(ui_rendering_ui_batch_handle_t handle, ui_rendering_ui_batch_info_t* out_info);
void ui_rendering_ui_batch_mark_dirty(ui_rendering_ui_batch_handle_t handle);
int ui_rendering_ui_batch_process_pending(void);

/* Geometry stored by the batch. Indices are copied and offset to the batch's vertex base. */
int ui_rendering_ui_batch_add_geometry(ui_rendering_ui_batch_handle_t handle,
                                       const ui_rendering_vertex_t* vertices,
                                       uint32_t vertex_count,
                                       const uint32_t* indices,
                                       uint32_t index_count);
int ui_rendering_ui_batch_get_vertex_buffer(ui_rendering_ui_batch_handle_t handle,
                                            const ui_rendering_vertex_t** out_vertices,
                                            uint32_t* out_count);
int ui_rendering_ui_batch_get_index_buffer(ui_rendering_ui_batch_handle_t handle,
                                           const uint32_t** out_indices,
                                           uint32_t* out_count);
int ui_rendering_ui_batch_get_draw_commands(ui_rendering_ui_batch_handle_t handle,
                                            ui_rendering_draw_command_t** out_commands,
                                            uint32_t* out_count);
void ui_rendering_ui_batch_mark_gpu_uploaded(ui_rendering_ui_batch_handle_t handle);

/* Statistics */
uint32_t ui_rendering_ui_batch_get_count(void);
size_t ui_rendering_ui_batch_get_memory_usage(void);
void ui_rendering_ui_batch_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_RENDERING_UI_BATCH_H */
