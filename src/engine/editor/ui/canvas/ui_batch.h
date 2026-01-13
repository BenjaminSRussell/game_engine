/*
 * ui_batch.h
 * UI draw batching and geometry rendering
 *
 * Part of the UI Rendering subsystem
 * Advanced 3D Rendering Engine
 *
 * Handles batching of UI geometry, text, and effects for optimized rendering.
 * Combines multiple UI draw commands into single GPU batches, reducing state changes.
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

/* Batch handle for identification */
typedef struct ui_rendering_ui_batch_handle {
    uint32_t id;
} ui_rendering_ui_batch_handle_t;

/* Vertex data structure for UI geometry */
typedef struct ui_rendering_vertex {
    float x, y, z;              /* Position */
    float u, v;                 /* Texture coordinates */
    uint32_t color;             /* RGBA color */
    float nx, ny;               /* Normal (for effects) */
} ui_rendering_vertex_t;

/* Batch descriptor for creation */
typedef struct ui_rendering_ui_batch_desc {
    uint32_t flags;
    uint32_t max_vertices;
    uint32_t max_indices;
    void* user_data;
} ui_rendering_ui_batch_desc_t;

/* Batch info structure */
typedef struct ui_rendering_ui_batch_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t vertex_count;
    uint32_t index_count;
    size_t memory_usage;
} ui_rendering_ui_batch_info_t;

/* Draw command for batching */
typedef struct ui_rendering_draw_command {
    uint32_t vertex_offset;
    uint32_t vertex_count;
    uint32_t index_offset;
    uint32_t index_count;
    uint32_t material_id;
    uint32_t texture_id;
    uint32_t blend_mode;
    float z_order;              /* For sorting */
} ui_rendering_draw_command_t;

/* Batch statistics */
typedef struct ui_rendering_batch_stats {
    uint32_t total_batches;
    uint32_t total_vertices;
    uint32_t total_indices;
    uint32_t total_draw_calls;
    uint32_t merged_draw_calls;
    size_t total_memory;
    uint32_t frames_since_update;
} ui_rendering_batch_stats_t;

/* ============================================================================
 * API - INITIALIZATION
 * ============================================================================ */

int ui_rendering_ui_batch_init(void);
void ui_rendering_ui_batch_shutdown(void);

/* ============================================================================
 * API - LIFECYCLE
 * ============================================================================ */

int ui_rendering_ui_batch_create(ui_rendering_ui_batch_handle_t* out_handle,
                                  const ui_rendering_ui_batch_desc_t* desc);
void ui_rendering_ui_batch_destroy(ui_rendering_ui_batch_handle_t handle);

/* ============================================================================
 * API - GEOMETRY OPERATIONS
 * ============================================================================ */

/* Add vertices and indices to batch */
int ui_rendering_ui_batch_add_geometry(ui_rendering_ui_batch_handle_t handle,
                                        const ui_rendering_vertex_t* vertices,
                                        uint32_t vertex_count,
                                        const uint32_t* indices,
                                        uint32_t index_count);

/* Add a draw command to the batch */
int ui_rendering_ui_batch_add_draw_command(ui_rendering_ui_batch_handle_t handle,
                                            const ui_rendering_draw_command_t* cmd);

/* Clear all geometry from batch */
int ui_rendering_ui_batch_clear(ui_rendering_ui_batch_handle_t handle);

/* ============================================================================
 * API - BATCH OPERATIONS
 * ============================================================================ */

int ui_rendering_ui_batch_update(ui_rendering_ui_batch_handle_t handle, const void* data, size_t size);
bool ui_rendering_ui_batch_is_valid(ui_rendering_ui_batch_handle_t handle);
int ui_rendering_ui_batch_get_info(ui_rendering_ui_batch_handle_t handle, ui_rendering_ui_batch_info_t* out_info);
void ui_rendering_ui_batch_mark_dirty(ui_rendering_ui_batch_handle_t handle);

/* ============================================================================
 * API - BATCH PROCESSING & RENDERING
 * ============================================================================ */

/* Process pending batches and prepare for rendering */
int ui_rendering_ui_batch_process_pending(void);

/* Get draw commands for this batch (sorted and optimized) */
int ui_rendering_ui_batch_get_draw_commands(ui_rendering_ui_batch_handle_t handle,
                                             ui_rendering_draw_command_t** out_commands,
                                             uint32_t* out_count);

/* Merge compatible draw commands to reduce draw calls */
int ui_rendering_ui_batch_optimize_draw_calls(ui_rendering_ui_batch_handle_t handle);

/* Sort geometry by z-order and material for optimal batching */
int ui_rendering_ui_batch_sort(ui_rendering_ui_batch_handle_t handle);

/* ============================================================================
 * API - GPU UPLOAD
 * ============================================================================ */

/* Get vertex buffer data for GPU upload */
int ui_rendering_ui_batch_get_vertex_buffer(ui_rendering_ui_batch_handle_t handle,
                                             const ui_rendering_vertex_t** out_vertices,
                                             uint32_t* out_count);

/* Get index buffer data for GPU upload */
int ui_rendering_ui_batch_get_index_buffer(ui_rendering_ui_batch_handle_t handle,
                                            const uint32_t** out_indices,
                                            uint32_t* out_count);

/* Mark buffers as uploaded to GPU (for dirty tracking) */
int ui_rendering_ui_batch_mark_gpu_uploaded(ui_rendering_ui_batch_handle_t handle);

/* ============================================================================
 * API - STATISTICS & DEBUG
 * ============================================================================ */

uint32_t ui_rendering_ui_batch_get_count(void);
size_t ui_rendering_ui_batch_get_memory_usage(void);
int ui_rendering_ui_batch_get_stats(ui_rendering_batch_stats_t* out_stats);
void ui_rendering_ui_batch_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_RENDERING_UI_BATCH_H */
