/*
 * ui_batch_renderer.h
 * UI batch rendering pipeline integration
 *
 * Part of the UI Rendering subsystem
 * Advanced 3D Rendering Engine
 *
 * Connects batched geometry to GPU rendering pipeline.
 * Handles shader binding, draw submission, and visual output.
 */

#ifndef UI_RENDERING_UI_BATCH_RENDERER_H
#define UI_RENDERING_UI_BATCH_RENDERER_H

#include "editor/ui/canvas/ui_batch.h"
#include "editor/ui/canvas/ui_batch_gpu.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Render context for GPU submission */
typedef struct ui_batch_render_context {
    void* command_buffer;
    void* render_pass;
    void* framebuffer;
    void* pipeline;
    void* descriptor_set;

    uint32_t viewport_width;
    uint32_t viewport_height;
} ui_batch_render_context_t;

/* Shader descriptor */
typedef struct ui_batch_shader_desc {
    const char* vertex_source;
    const char* fragment_source;
    const char* compute_source;
    size_t vertex_size;
    size_t fragment_size;
    size_t compute_size;
} ui_batch_shader_desc_t;

/* Render statistics */
typedef struct ui_batch_render_stats {
    uint32_t draw_calls;
    uint32_t vertices_rendered;
    uint32_t triangles_rendered;
    float gpu_time_ms;
    float cpu_time_ms;
} ui_batch_render_stats_t;

/* ============================================================================
 * API - INITIALIZATION
 * ============================================================================ */

int ui_batch_renderer_init(void);
void ui_batch_renderer_shutdown(void);

/* ============================================================================
 * API - SHADER MANAGEMENT
 * ============================================================================ */

/* Create and compile shaders */
int ui_batch_renderer_create_shaders(const ui_batch_shader_desc_t* desc);

/* Bind shader program */
int ui_batch_renderer_bind_shader(uint32_t shader_id);

/* Bind texture for rendering */
int ui_batch_renderer_bind_texture(uint32_t texture_id, uint32_t slot);

/* ============================================================================
 * API - RENDERING
 * ============================================================================ */

/* Begin frame rendering */
int ui_batch_renderer_begin_frame(const ui_batch_render_context_t* ctx);

/* End frame rendering */
int ui_batch_renderer_end_frame(void);

/* Submit batch for rendering */
int ui_batch_renderer_submit_batch(ui_rendering_ui_batch_handle_t batch_handle,
                                    const ui_batch_render_context_t* ctx);

/* Render all pending batches */
int ui_batch_renderer_render_all(const ui_batch_render_context_t* ctx);

/* ============================================================================
 * API - VISUAL EFFECTS
 * ============================================================================ */

/* Set viewport dimensions */
int ui_batch_renderer_set_viewport(uint32_t width, uint32_t height);

/* Set clear color */
int ui_batch_renderer_set_clear_color(float r, float g, float b, float a);

/* Enable/disable scissor testing */
int ui_batch_renderer_set_scissor(bool enabled, int32_t x, int32_t y,
                                   uint32_t width, uint32_t height);

/* Set blend mode */
int ui_batch_renderer_set_blend_mode(uint32_t blend_mode);

/* ============================================================================
 * API - STATISTICS & PROFILING
 * ============================================================================ */

int ui_batch_renderer_get_stats(ui_batch_render_stats_t* out_stats);
void ui_batch_renderer_reset_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_RENDERING_UI_BATCH_RENDERER_H */
