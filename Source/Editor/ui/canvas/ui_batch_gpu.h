/*
 * ui_batch_gpu.h
 * GPU buffer management and rendering integration
 *
 * Part of the UI Rendering subsystem
 * Advanced 3D Rendering Engine
 *
 * Handles GPU upload of batched geometry, buffer allocation,
 * and rendering command submission.
 */

#ifndef UI_RENDERING_UI_BATCH_GPU_H
#define UI_RENDERING_UI_BATCH_GPU_H

#include "editor/ui/canvas/ui_batch.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* GPU buffer handle */
typedef struct ui_batch_gpu_buffer_handle {
    uint32_t id;
} ui_batch_gpu_buffer_handle_t;

/* GPU buffer descriptor */
typedef struct ui_batch_gpu_buffer_desc {
    uint32_t buffer_type;   /* Vertex, Index, etc. */
    size_t size;
    uint32_t usage_flags;
    void* initial_data;
} ui_batch_gpu_buffer_desc_t;

/* GPU render context */
typedef struct ui_batch_gpu_render_context {
    void* command_buffer;
    void* render_pass;
    void* pipeline_layout;
} ui_batch_gpu_render_context_t;

/* ============================================================================
 * API - INITIALIZATION
 * ============================================================================ */

int ui_batch_gpu_init(void);
void ui_batch_gpu_shutdown(void);

/* ============================================================================
 * API - BUFFER MANAGEMENT
 * ============================================================================ */

/* Create GPU buffer */
int ui_batch_gpu_create_buffer(ui_batch_gpu_buffer_handle_t* out_handle,
                                const ui_batch_gpu_buffer_desc_t* desc);

/* Destroy GPU buffer */
void ui_batch_gpu_destroy_buffer(ui_batch_gpu_buffer_handle_t handle);

/* Upload data to GPU buffer */
int ui_batch_gpu_upload_buffer(ui_batch_gpu_buffer_handle_t handle,
                                const void* data, size_t size, size_t offset);

/* Update buffer with new data (device memory only) */
int ui_batch_gpu_update_buffer(ui_batch_gpu_buffer_handle_t handle,
                                const void* data, size_t size, size_t offset);

/* ============================================================================
 * API - BATCH GPU OPERATIONS
 * ============================================================================ */

/* Upload batch geometry to GPU */
int ui_batch_gpu_upload_batch(ui_rendering_ui_batch_handle_t batch_handle);

/* Bind batch buffers for rendering */
int ui_batch_gpu_bind_batch(ui_rendering_ui_batch_handle_t batch_handle);

/* Draw batch with its commands */
int ui_batch_gpu_draw_batch(ui_rendering_ui_batch_handle_t batch_handle,
                             const ui_batch_gpu_render_context_t* ctx);

/* ============================================================================
 * API - RESOURCE POOLING
 * ============================================================================ */

/* Get available GPU memory */
size_t ui_batch_gpu_get_available_memory(void);

/* Request GPU memory allocation */
int ui_batch_gpu_allocate_memory(size_t size, void** out_gpu_memory);

/* Free GPU memory allocation */
void ui_batch_gpu_free_memory(void* gpu_memory, size_t size);

/* ============================================================================
 * API - STATISTICS
 * ============================================================================ */

size_t ui_batch_gpu_get_memory_usage(void);
uint32_t ui_batch_gpu_get_buffer_count(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_RENDERING_UI_BATCH_GPU_H */
