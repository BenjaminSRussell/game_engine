/*
 * forward_transparency.c
 * Forward transparency pass
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "forward_transparency.h"
#include "../../static_mesh_rendering/static_mesh_draw.h"
#include "../../static_mesh_rendering/mesh_sorting.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

typedef struct transparency_pass_data {
    static_mesh_draw_info_t* draw_items;
    uint32_t draw_item_count;
    uint32_t draw_item_capacity;
} transparency_pass_data_t;

static struct {
    transparency_pass_data_t data;
    bool initialized;
} g_transf_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_forward_transparency_init(void) {
    if (g_transf_ctx.initialized) return 0;
    
    g_transf_ctx.data.draw_item_capacity = 1024;
    g_transf_ctx.data.draw_items = (static_mesh_draw_info_t*)malloc(
        g_transf_ctx.data.draw_item_capacity * sizeof(static_mesh_draw_info_t)
    );
    
    if (!g_transf_ctx.data.draw_items) return -1;
    
    g_transf_ctx.initialized = true;
    return 0;
}

void rendering_forward_transparency_shutdown(void) {
    if (!g_transf_ctx.initialized) return;
    
    if (g_transf_ctx.data.draw_items) {
        free(g_transf_ctx.data.draw_items);
        g_transf_ctx.data.draw_items = NULL;
    }
    
    g_transf_ctx.initialized = false;
}

void rendering_forward_transparency_submit(const static_mesh_draw_info_t* info) {
    if (!g_transf_ctx.initialized || !info) return;
    
    if (g_transf_ctx.data.draw_item_count >= g_transf_ctx.data.draw_item_capacity) {
        uint32_t new_cap = g_transf_ctx.data.draw_item_capacity * 2;
        static_mesh_draw_info_t* new_arr = (static_mesh_draw_info_t*)realloc(
            g_transf_ctx.data.draw_items, new_cap * sizeof(static_mesh_draw_info_t)
        );
        if (new_arr) {
            g_transf_ctx.data.draw_items = new_arr;
            g_transf_ctx.data.draw_item_capacity = new_cap;
        } else {
            return;
        }
    }
    
    g_transf_ctx.data.draw_items[g_transf_ctx.data.draw_item_count++] = *info;
}

void rendering_forward_transparency_execute(void* cmd_buffer, const float camera_pos[3]) {
    if (!g_transf_ctx.initialized) return;
    
    /* 1. Sort transparent items Back-to-Front for correct blending */
    mesh_sort_items(
        g_transf_ctx.data.draw_items, 
        g_transf_ctx.data.draw_item_count, 
        MESH_SORT_MODE_BACK_TO_FRONT, 
        camera_pos
    );
    
    /* 2. Setup render state for transparency */
    /* 
     * - Enable blend (SrcAlpha, OneMinusSrcAlpha)
     * - Enable Depth Test
     * - Disable Depth Write (usually)
     */
    
    /* 3. Draw items */
    static_mesh_draw_begin((command_buffer_t*)cmd_buffer);
    
    for (uint32_t i = 0; i < g_transf_ctx.data.draw_item_count; i++) {
        static_mesh_draw_submit(&g_transf_ctx.data.draw_items[i]);
    }
    
    static_mesh_draw_end();
    
    /* Clear for next frame */
    g_transf_ctx.data.draw_item_count = 0;
}
