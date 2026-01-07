/*
 * depth_prepass.c
 * Depth prepass implementation
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "depth_prepass.h"
#include "../../geometry/mesh/static_mesh_draw.h" // Reuse draw infrastructure
#include "../../geometry/mesh/mesh_sorting.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

typedef struct depth_pass_data {
    static_mesh_draw_info_t* draw_items;
    uint32_t draw_item_count;
    uint32_t draw_item_capacity;
} depth_pass_data_t;

static struct {
    depth_pass_data_t data;
    bool initialized;
} g_depth_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int rendering_depth_prepass_init(void) {
    if (g_depth_ctx.initialized) return 0;
    
    g_depth_ctx.data.draw_item_capacity = 4096;
    g_depth_ctx.data.draw_items = (static_mesh_draw_info_t*)malloc(
        g_depth_ctx.data.draw_item_capacity * sizeof(static_mesh_draw_info_t)
    );
    
    if (!g_depth_ctx.data.draw_items) return -1;
    
    g_depth_ctx.initialized = true;
    return 0;
}

void rendering_depth_prepass_shutdown(void) {
    if (!g_depth_ctx.initialized) return;
    
    if (g_depth_ctx.data.draw_items) {
        free(g_depth_ctx.data.draw_items);
        g_depth_ctx.data.draw_items = NULL;
    }
    
    g_depth_ctx.initialized = false;
}

void rendering_depth_prepass_submit(const static_mesh_draw_info_t* info) {
    if (!g_depth_ctx.initialized || !info) return;
    
    /* Resize if needed */
    if (g_depth_ctx.data.draw_item_count >= g_depth_ctx.data.draw_item_capacity) {
        uint32_t new_cap = g_depth_ctx.data.draw_item_capacity * 2;
        static_mesh_draw_info_t* new_arr = (static_mesh_draw_info_t*)realloc(
            g_depth_ctx.data.draw_items, new_cap * sizeof(static_mesh_draw_info_t)
        );
        if (new_arr) {
            g_depth_ctx.data.draw_items = new_arr;
            g_depth_ctx.data.draw_item_capacity = new_cap;
        } else {
            return; /* Drop */
        }
    }
    
    g_depth_ctx.data.draw_items[g_depth_ctx.data.draw_item_count++] = *info;
}

void rendering_depth_prepass_execute(void* cmd_buffer, const float camera_pos[3]) {
    if (!g_depth_ctx.initialized) return;
    
    /* 
     * Even for depth prepass, sorting front-to-back is beneficial 
     * as it maximizes Hierarchical-Z culling efficiency on modern GPUs
     */
    mesh_sort_items(
        g_depth_ctx.data.draw_items, 
        g_depth_ctx.data.draw_item_count, 
        MESH_SORT_MODE_FRONT_TO_BACK, 
        camera_pos
    );
    
    /* 
     * Bind Depth-only pipeline
     * - Color writes disabled
     * - Depth write/test enabled
     * - No fragment shader (or simple one for alpha test)
     */
     
    // command_buffer_bind_pipeline(cmd_buffer, PIPELINE_DEPTH_ONLY);
    
    static_mesh_draw_begin((command_buffer_t*)cmd_buffer);
    
    for (uint32_t i = 0; i < g_depth_ctx.data.draw_item_count; i++) {
        static_mesh_draw_submit(&g_depth_ctx.data.draw_items[i]);
    }
    
    static_mesh_draw_end();
    
    /* Clear list */
    g_depth_ctx.data.draw_item_count = 0;
}
