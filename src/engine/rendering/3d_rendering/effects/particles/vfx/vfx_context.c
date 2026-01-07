/*
 * vfx_context.c
 * VFX Graph Runtime Context
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#include "vfx_context.h"
#include <stdlib.h>
#include <string.h>

typedef struct vfx_graph_internal {
    uint32_t id;
    bool active;
    // Nodes list, properties, etc. (To be implemented)
} vfx_graph_internal_t;

typedef struct vfx_context {
    vfx_graph_internal_t* graphs;
    uint32_t graph_count;
    uint32_t max_graphs;
    bool initialized;
} vfx_context_t;

static vfx_context_t g_vfx_ctx = {0};

/* ============================================================================
 * API
 * ============================================================================ */

int vfx_context_init(const vfx_context_desc_t* desc) {
    if (g_vfx_ctx.initialized) return 0;

    g_vfx_ctx.max_graphs = desc ? desc->max_graphs : 128;
    g_vfx_ctx.graphs = calloc(g_vfx_ctx.max_graphs, sizeof(vfx_graph_internal_t));
    if (!g_vfx_ctx.graphs) return -1;

    g_vfx_ctx.graph_count = 0;
    g_vfx_ctx.initialized = true;
    return 0;
}

void vfx_context_shutdown(void) {
    if (!g_vfx_ctx.initialized) return;

    if (g_vfx_ctx.graphs) {
        free(g_vfx_ctx.graphs);
        g_vfx_ctx.graphs = NULL;
    }
    g_vfx_ctx.initialized = false;
}

int vfx_context_create_graph(vfx_graph_handle_t* out_handle) {
    if (!out_handle || !g_vfx_ctx.initialized) return -1;
    
    // Find free slot
    for (uint32_t i = 0; i < g_vfx_ctx.max_graphs; i++) {
        if (!g_vfx_ctx.graphs[i].active) {
            g_vfx_ctx.graphs[i].active = true;
            g_vfx_ctx.graphs[i].id = i;
            out_handle->id = i;
            return 0;
        }
    }
    return -2; // Full
}

void vfx_context_destroy_graph(vfx_graph_handle_t handle) {
    if (handle.id < g_vfx_ctx.max_graphs) {
        g_vfx_ctx.graphs[handle.id].active = false;
    }
}

int vfx_context_update(float dt) {
    if (!g_vfx_ctx.initialized) return -1;

    for (uint32_t i = 0; i < g_vfx_ctx.max_graphs; i++) {
        if (g_vfx_ctx.graphs[i].active) {
            // Traverse nodes and execute
        }
    }
    return 0;
}
