/*
 * forward_transparency.c
 * Forward transparency pass
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "rendering/forward/forward_transparency.h"
#include "rendering/forward/forward_lighting.h"
#include "geometry/mesh/static_mesh_draw.h"
#include "geometry/mesh/mesh_sorting.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <simd/simd.h>
#endif

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
    
#ifdef __OBJC__
    id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)cmd_buffer;
    forward_renderer_t* fr = rendering_forward_lighting_get_renderer();

    if (encoder && fr && fr->transparent_pipeline) {
        [encoder setRenderPipelineState:fr->transparent_pipeline];
        [encoder setDepthStencilState:fr->depth_state_transparent]; // Write off, test on
        [encoder setCullMode:MTLCullModeNone]; // Render both sides usually for glass etc.

        /* Bind Lighting Buffers */
        if (fr->light_grid_buffer) {
            [encoder setFragmentBuffer:fr->light_grid_buffer offset:0 atIndex:10];
        }
        if (fr->light_data_buffer) {
            [encoder setFragmentBuffer:fr->light_data_buffer offset:0 atIndex:11];
        }
        
        /* Draw Items */
        for (uint32_t i = 0; i < g_transf_ctx.data.draw_item_count; i++) {
            // Mocking member access as per opaque pass logic
            // static_mesh_draw_info_t* item = &g_transf_ctx.data.draw_items[i];
            
            // [encoder setVertexBytes:&item->transform ...];
            // [encoder setVertexBuffer:...];
            // [encoder setFragmentTexture:...];
            
            // [encoder drawIndexedPrimitives:...];
        }
    }
#endif
    
    /* Clear for next frame */
    g_transf_ctx.data.draw_item_count = 0;
}
