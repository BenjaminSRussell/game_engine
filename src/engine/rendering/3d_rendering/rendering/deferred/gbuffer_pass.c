/*
 * gbuffer_pass.c
 * G-buffer geometry pass
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "gbuffer_pass.h"
#include "gbuffer_layout.h"
#include "../../static_mesh_rendering/static_mesh_draw.h"
#include "../../static_mesh_rendering/mesh_sorting.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

typedef struct gbuffer_pass_data {
    static_mesh_draw_info_t* draw_items;
    uint32_t draw_item_count;
    uint32_t draw_item_capacity;
} gbuffer_pass_data_t;

static struct {
    gbuffer_pass_data_t data;
    bool initialized;
} g_gbuffer_pass = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_gbuffer_pass_init(void) {
    if (g_gbuffer_pass.initialized) return 0;
    
    g_gbuffer_pass.data.draw_item_capacity = 4096;
    g_gbuffer_pass.data.draw_items = (static_mesh_draw_info_t*)malloc(
        g_gbuffer_pass.data.draw_item_capacity * sizeof(static_mesh_draw_info_t)
    );
    
    if (!g_gbuffer_pass.data.draw_items) return -1;
    
    g_gbuffer_pass.initialized = true;
    return 0;
}

void rendering_gbuffer_pass_shutdown(void) {
    if (!g_gbuffer_pass.initialized) return;
    
    if (g_gbuffer_pass.data.draw_items) {
        free(g_gbuffer_pass.data.draw_items);
        g_gbuffer_pass.data.draw_items = NULL;
    }
    
    g_gbuffer_pass.initialized = false;
}

void rendering_gbuffer_pass_submit(const static_mesh_draw_info_t* info) {
    if (!g_gbuffer_pass.initialized || !info) return;
    
    if (g_gbuffer_pass.data.draw_item_count >= g_gbuffer_pass.data.draw_item_capacity) {
        uint32_t new_cap = g_gbuffer_pass.data.draw_item_capacity * 2;
        static_mesh_draw_info_t* new_arr = (static_mesh_draw_info_t*)realloc(
            g_gbuffer_pass.data.draw_items, new_cap * sizeof(static_mesh_draw_info_t)
        );
        if (new_arr) {
            g_gbuffer_pass.data.draw_items = new_arr;
            g_gbuffer_pass.data.draw_item_capacity = new_cap;
        } else {
            return;
        }
    }
    
    g_gbuffer_pass.data.draw_items[g_gbuffer_pass.data.draw_item_count++] = *info;
}

void rendering_gbuffer_pass_execute(void* cmd_buffer, const float camera_pos[3]) {
    if (!g_gbuffer_pass.initialized) return;
    
    /* 1. Sort opaque items Front-to-Back for early-Z / G-buffer write optimization */
    mesh_sort_items(
        g_gbuffer_pass.data.draw_items, 
        g_gbuffer_pass.data.draw_item_count, 
        MESH_SORT_MODE_FRONT_TO_BACK, 
        camera_pos
    );
    
    /* 2. Begin G-buffer rendering pass */
     void* framebuffer = rendering_gbuffer_get_framebuffer();
     // command_buffer_begin_render_pass(cmd_buffer, framebuffer, ...);
    
    /* 3. Draw sorted items */
    static_mesh_draw_begin((command_buffer_t*)cmd_buffer);
    
    for (uint32_t i = 0; i < g_gbuffer_pass.data.draw_item_count; i++) {
        static_mesh_draw_submit(&g_gbuffer_pass.data.draw_items[i]);
    }
    
    static_mesh_draw_end();
    
    /* 4. End pass */
    // command_buffer_end_render_pass(cmd_buffer);
    
    /* Clear for next frame */
    g_gbuffer_pass.data.draw_item_count = 0;
}

