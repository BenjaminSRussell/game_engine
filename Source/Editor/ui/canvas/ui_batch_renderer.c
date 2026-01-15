/*
 * ui_batch_renderer.c
 * UI batch rendering pipeline integration
 *
 * Part of the UI Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "editor/ui/canvas/ui_batch_renderer.h"
#include "editor/ui/canvas/ui_batch.h"
#include "editor/ui/canvas/ui_batch_gpu.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define UI_BATCH_RENDERER_MAX_SHADERS 32
#define UI_BATCH_RENDERER_MAX_TEXTURES 256

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct ui_batch_shader {
    uint32_t id;
    void* program;
    bool valid;
} ui_batch_shader_t;

typedef struct ui_batch_texture_binding {
    uint32_t texture_id;
    uint32_t slot;
    bool valid;
} ui_batch_texture_binding_t;

typedef struct ui_batch_renderer_context {
    ui_batch_shader_t shaders[UI_BATCH_RENDERER_MAX_SHADERS];
    uint32_t shader_count;

    ui_batch_texture_binding_t textures[UI_BATCH_RENDERER_MAX_TEXTURES];
    uint32_t texture_count;

    ui_batch_render_context_t render_ctx;

    float clear_color[4];
    uint32_t viewport_width;
    uint32_t viewport_height;

    bool scissor_enabled;
    int32_t scissor_x, scissor_y;
    uint32_t scissor_width, scissor_height;

    uint32_t current_blend_mode;

    ui_batch_render_stats_t stats;

    bool initialized;
} ui_batch_renderer_context_t;

static ui_batch_renderer_context_t g_renderer_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void* ui_batch_renderer_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

static void ui_batch_renderer_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

/* Submit draw command to GPU */
static int ui_batch_renderer_draw(const ui_rendering_draw_command_t* cmd,
                                   const ui_rendering_vertex_t* vertices,
                                   const uint32_t* indices) {
    if (!cmd || !vertices || !indices) {
        return -1;
    }

    /* In a real implementation, this would submit to the actual graphics API
     * For now, we record the draw call for statistics */
    g_renderer_ctx.stats.draw_calls++;
    g_renderer_ctx.stats.vertices_rendered += cmd->vertex_count;

    /* Calculate triangles from indices */
    if (cmd->index_count >= 3) {
        g_renderer_ctx.stats.triangles_rendered += cmd->index_count / 3;
    }

    return 0;
}

/* ============================================================================
 * PUBLIC API - INITIALIZATION
 * ============================================================================ */

int ui_batch_renderer_init(void) {
    if (g_renderer_ctx.initialized) {
        return 0;
    }

    memset(&g_renderer_ctx, 0, sizeof(g_renderer_ctx));

    g_renderer_ctx.clear_color[0] = 0.0f;
    g_renderer_ctx.clear_color[1] = 0.0f;
    g_renderer_ctx.clear_color[2] = 0.0f;
    g_renderer_ctx.clear_color[3] = 1.0f;

    g_renderer_ctx.viewport_width = 1920;
    g_renderer_ctx.viewport_height = 1080;

    g_renderer_ctx.scissor_enabled = false;

    g_renderer_ctx.initialized = true;

    return 0;
}

void ui_batch_renderer_shutdown(void) {
    if (!g_renderer_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_renderer_ctx.shader_count; i++) {
        if (g_renderer_ctx.shaders[i].valid && g_renderer_ctx.shaders[i].program) {
            ui_batch_renderer_free(g_renderer_ctx.shaders[i].program);
        }
    }

    memset(&g_renderer_ctx, 0, sizeof(g_renderer_ctx));
}

/* ============================================================================
 * PUBLIC API - SHADER MANAGEMENT
 * ============================================================================ */

int ui_batch_renderer_create_shaders(const ui_batch_shader_desc_t* desc) {
    if (!desc) {
        return -1;
    }

    if (!g_renderer_ctx.initialized) {
        return -2;
    }

    if (g_renderer_ctx.shader_count >= UI_BATCH_RENDERER_MAX_SHADERS) {
        return -3;
    }

    uint32_t index = g_renderer_ctx.shader_count++;
    ui_batch_shader_t* shader = &g_renderer_ctx.shaders[index];

    shader->id = index;
    shader->valid = true;

    /* In a real implementation, this would compile the shader
     * For now, we just store the descriptor */
    if (desc->vertex_source) {
        shader->program = ui_batch_renderer_malloc(desc->vertex_size + desc->fragment_size);
        if (!shader->program) {
            g_renderer_ctx.shader_count--;
            return -4;
        }
    }

    return 0;
}

int ui_batch_renderer_bind_shader(uint32_t shader_id) {
    if (shader_id >= g_renderer_ctx.shader_count) {
        return -1;
    }

    ui_batch_shader_t* shader = &g_renderer_ctx.shaders[shader_id];
    if (!shader->valid) {
        return -2;
    }

    /* In a real implementation, this would bind the shader to the GPU */

    return 0;
}

int ui_batch_renderer_bind_texture(uint32_t texture_id, uint32_t slot) {
    if (slot >= UI_BATCH_RENDERER_MAX_TEXTURES) {
        return -1;
    }

    ui_batch_texture_binding_t* binding = &g_renderer_ctx.textures[slot];
    binding->texture_id = texture_id;
    binding->slot = slot;
    binding->valid = true;

    if (slot >= g_renderer_ctx.texture_count) {
        g_renderer_ctx.texture_count = slot + 1;
    }

    /* In a real implementation, this would bind the texture to the GPU */

    return 0;
}

/* ============================================================================
 * PUBLIC API - RENDERING
 * ============================================================================ */

int ui_batch_renderer_begin_frame(const ui_batch_render_context_t* ctx) {
    if (!ctx) {
        return -1;
    }

    if (!g_renderer_ctx.initialized) {
        return -2;
    }

    memcpy(&g_renderer_ctx.render_ctx, ctx, sizeof(*ctx));

    /* Clear screen with clear color */
    g_renderer_ctx.stats.draw_calls = 0;
    g_renderer_ctx.stats.vertices_rendered = 0;
    g_renderer_ctx.stats.triangles_rendered = 0;
    g_renderer_ctx.stats.gpu_time_ms = 0.0f;
    g_renderer_ctx.stats.cpu_time_ms = 0.0f;

    /* In a real implementation, this would:
     * - Clear framebuffer with clear color
     * - Begin render pass
     * - Set up viewport and scissor */

    return 0;
}

int ui_batch_renderer_end_frame(void) {
    if (!g_renderer_ctx.initialized) {
        return -1;
    }

    /* In a real implementation, this would:
     * - End render pass
     * - Submit command buffer
     * - Present framebuffer */

    return 0;
}

int ui_batch_renderer_submit_batch(ui_rendering_ui_batch_handle_t batch_handle,
                                    const ui_batch_render_context_t* ctx) {
    if (!ui_rendering_ui_batch_is_valid(batch_handle)) {
        return -1;
    }

    if (!ctx) {
        return -2;
    }

    /* Get batch geometry */
    const ui_rendering_vertex_t* vertices = NULL;
    uint32_t vertex_count = 0;
    if (ui_rendering_ui_batch_get_vertex_buffer(batch_handle, &vertices, &vertex_count) != 0) {
        return -3;
    }

    const uint32_t* indices = NULL;
    uint32_t index_count = 0;
    if (ui_rendering_ui_batch_get_index_buffer(batch_handle, &indices, &index_count) != 0) {
        return -4;
    }

    /* Get draw commands */
    ui_rendering_draw_command_t* commands = NULL;
    uint32_t command_count = 0;
    if (ui_rendering_ui_batch_get_draw_commands(batch_handle, &commands, &command_count) != 0) {
        return -5;
    }

    /* Submit each draw command */
    for (uint32_t i = 0; i < command_count; i++) {
        ui_rendering_draw_command_t* cmd = &commands[i];

        /* Bind material and texture */
        ui_batch_renderer_bind_texture(cmd->texture_id, 0);
        ui_batch_renderer_set_blend_mode(cmd->blend_mode);

        /* Submit draw */
        ui_batch_renderer_draw(cmd, vertices, indices);
    }

    return command_count;
}

int ui_batch_renderer_render_all(const ui_batch_render_context_t* ctx) {
    if (!ctx) {
        return -1;
    }

    if (!g_renderer_ctx.initialized) {
        return -2;
    }

    int total_batches = 0;
    uint32_t batch_count = ui_rendering_ui_batch_get_count();

    for (uint32_t i = 0; i < batch_count; i++) {
        ui_rendering_ui_batch_handle_t handle = { i };

        if (ui_rendering_ui_batch_is_valid(handle)) {
            int result = ui_batch_renderer_submit_batch(handle, ctx);
            if (result > 0) {
                total_batches++;
            }
        }
    }

    return total_batches;
}

/* ============================================================================
 * PUBLIC API - VISUAL EFFECTS
 * ============================================================================ */

int ui_batch_renderer_set_viewport(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        return -1;
    }

    g_renderer_ctx.viewport_width = width;
    g_renderer_ctx.viewport_height = height;

    /* In a real implementation, this would set GPU viewport */

    return 0;
}

int ui_batch_renderer_set_clear_color(float r, float g, float b, float a) {
    if (r < 0 || r > 1 || g < 0 || g > 1 || b < 0 || b > 1 || a < 0 || a > 1) {
        return -1;
    }

    g_renderer_ctx.clear_color[0] = r;
    g_renderer_ctx.clear_color[1] = g;
    g_renderer_ctx.clear_color[2] = b;
    g_renderer_ctx.clear_color[3] = a;

    return 0;
}

int ui_batch_renderer_set_scissor(bool enabled, int32_t x, int32_t y,
                                   uint32_t width, uint32_t height) {
    g_renderer_ctx.scissor_enabled = enabled;
    g_renderer_ctx.scissor_x = x;
    g_renderer_ctx.scissor_y = y;
    g_renderer_ctx.scissor_width = width;
    g_renderer_ctx.scissor_height = height;

    /* In a real implementation, this would set GPU scissor rect */

    return 0;
}

int ui_batch_renderer_set_blend_mode(uint32_t blend_mode) {
    g_renderer_ctx.current_blend_mode = blend_mode;

    /* In a real implementation, this would set GPU blend state */

    return 0;
}

/* ============================================================================
 * PUBLIC API - STATISTICS & PROFILING
 * ============================================================================ */

int ui_batch_renderer_get_stats(ui_batch_render_stats_t* out_stats) {
    if (!out_stats) {
        return -1;
    }

    memcpy(out_stats, &g_renderer_ctx.stats, sizeof(*out_stats));

    return 0;
}

void ui_batch_renderer_reset_stats(void) {
    memset(&g_renderer_ctx.stats, 0, sizeof(g_renderer_ctx.stats));
}

/* End of ui_batch_renderer.c */
