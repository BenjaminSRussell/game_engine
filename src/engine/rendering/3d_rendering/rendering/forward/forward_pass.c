/*
 * forward_pass.c
 * Forward rendering pass
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "forward_pass.h"
#include "../../static_mesh_rendering/static_mesh_draw.h" // We need these
#include "../../static_mesh_rendering/mesh_sorting.h"
#include "../../skeletal_mesh_rendering/skeletal_mesh_draw.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

typedef struct forward_pass_data {
    uint32_t width;
    uint32_t height;
    
    /* Framebuffer attachments would go here */
    void* color_target;
    void* depth_target;
    
    /* Internal lists for sorting */
    static_mesh_draw_info_t* draw_items;
    uint32_t draw_item_count;
    uint32_t draw_item_capacity;
    
} forward_pass_data_t;

/* Global context (simplification) */
static struct {
    forward_pass_data_t data;
    bool initialized;
} g_forward_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_forward_pass_init(void) {
    if (g_forward_ctx.initialized) return 0;
    
    g_forward_ctx.data.draw_item_capacity = 4096;
    g_forward_ctx.data.draw_items = (static_mesh_draw_info_t*)malloc(
        g_forward_ctx.data.draw_item_capacity * sizeof(static_mesh_draw_info_t)
    );
    
    if (!g_forward_ctx.data.draw_items) return -1;
    
    g_forward_ctx.initialized = true;
    return 0;
}

void rendering_forward_pass_shutdown(void) {
    if (!g_forward_ctx.initialized) return;
    
    if (g_forward_ctx.data.draw_items) {
        free(g_forward_ctx.data.draw_items);
        g_forward_ctx.data.draw_items = NULL;
    }
    
    g_forward_ctx.initialized = false;
}

void rendering_forward_pass_submit_mesh(const static_mesh_draw_info_t* info) {
    if (!g_forward_ctx.initialized || !info) return;
    
    /* Resize if needed */
    if (g_forward_ctx.data.draw_item_count >= g_forward_ctx.data.draw_item_capacity) {
        uint32_t new_cap = g_forward_ctx.data.draw_item_capacity * 2;
        static_mesh_draw_info_t* new_arr = (static_mesh_draw_info_t*)realloc(
            g_forward_ctx.data.draw_items, new_cap * sizeof(static_mesh_draw_info_t)
        );
        if (new_arr) {
            g_forward_ctx.data.draw_items = new_arr;
            g_forward_ctx.data.draw_item_capacity = new_cap;
        } else {
            return; /* Out of memory, drop item */
        }
    }
    
    g_forward_ctx.data.draw_items[g_forward_ctx.data.draw_item_count++] = *info;
}

void rendering_forward_pass_execute(void* cmd_buffer, const float camera_pos[3]) {
    if (!g_forward_ctx.initialized) return;
    
    /* 1. Sort opaque items Front-to-Back to minimize overdraw */
    mesh_sort_items(
        g_forward_ctx.data.draw_items, 
        g_forward_ctx.data.draw_item_count, 
        MESH_SORT_MODE_FRONT_TO_BACK, 
        camera_pos
    );
    
    /* 2. Begin render pass setup (viewport, scissor, clear) */
    /* mock: command_buffer_begin_render_pass(cmd_buffer, ...); */
    
    /* 3. Draw sorted items using static mesh system */
    static_mesh_draw_begin((command_buffer_t*)cmd_buffer);
    
    for (uint32_t i = 0; i < g_forward_ctx.data.draw_item_count; i++) {
        static_mesh_draw_submit(&g_forward_ctx.data.draw_items[i]);
    }
    
    static_mesh_draw_end();
    
    /* 4. End pass */
    /* mock: command_buffer_end_render_pass(cmd_buffer); */
    
    /* Clear list for next frame */
    g_forward_ctx.data.draw_item_count = 0;
}

