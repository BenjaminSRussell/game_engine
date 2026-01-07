/*
 * forward_batching.c
 * Forward Renderer Batching Implementation
 */

#include "forward_batching.h"
#include <core/logger.h>
#include <core/memory.h>
#include <stdlib.h>

/* ============================================================================
 * INTERNAL STATE
 * ============================================================================ */

#define MAX_DRAW_COMMANDS 10000

static struct {
    ForwardDrawCommand commands[MAX_DRAW_COMMANDS];
    u32 count;
    bool initialized;
} g_batcher;

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static int compare_opaque(const void* a, const void* b) {
    const ForwardDrawCommand* cmdA = (const ForwardDrawCommand*)a;
    const ForwardDrawCommand* cmdB = (const ForwardDrawCommand*)b;
    
    // Sort by Shader State first (minimize pipeline switches)
    if (cmdA->shader_key != cmdB->shader_key) {
        return (cmdA->shader_key < cmdB->shader_key) ? -1 : 1;
    }
    
    // Sort by Material second
    if (cmdA->material_id != cmdB->material_id) {
        return (cmdA->material_id < cmdB->material_id) ? -1 : 1;
    }
    
    // Finally Front-to-Back for early-Z optimization (approximate)
    // Smaller distance first
    if (cmdA->distance_to_camera < cmdB->distance_to_camera) return -1;
    if (cmdA->distance_to_camera > cmdB->distance_to_camera) return 1;
    
    return 0;
}

static int compare_transparent(const void* a, const void* b) {
    const ForwardDrawCommand* cmdA = (const ForwardDrawCommand*)a;
    const ForwardDrawCommand* cmdB = (const ForwardDrawCommand*)b;
    
    // STRICT Back-to-Front sorting is required for transparency
    // Larger distance first
    if (cmdA->distance_to_camera > cmdB->distance_to_camera) return -1;
    if (cmdA->distance_to_camera < cmdB->distance_to_camera) return 1;
    
    return 0;
}

/* ============================================================================
 * IMPLEMENTATION
 * ============================================================================ */

void forward_batching_init(void) {
    g_batcher.count = 0;
    g_batcher.initialized = true;
    LOG_INFO("ForwardBatching: Initialized.");
}

void forward_batching_begin_frame(void) {
    g_batcher.count = 0;
}

void forward_batching_submit(const ForwardDrawCommand* cmd) {
    if (!g_batcher.initialized) return;
    
    if (g_batcher.count >= MAX_DRAW_COMMANDS) {
        LOG_WARN("ForwardBatching: Command buffer full!");
        return;
    }
    
    g_batcher.commands[g_batcher.count++] = *cmd;
}

void forward_batching_sort(bool is_transparent) {
    if (g_batcher.count == 0) return;
    
    if (is_transparent) {
        qsort(g_batcher.commands, g_batcher.count, sizeof(ForwardDrawCommand), compare_transparent);
    } else {
        qsort(g_batcher.commands, g_batcher.count, sizeof(ForwardDrawCommand), compare_opaque);
    }
}

void forward_batching_flush(void* renderer_context) {
    if (g_batcher.count == 0 || !renderer_context) return;
    
    // State Tracking
    ShaderVariantKey current_shader = 0xFFFFFFFF;
    u32 current_material = 0xFFFFFFFF;
    u32 current_mesh = 0xFFFFFFFF;
    
    for (u32 i = 0; i < g_batcher.count; i++) {
        ForwardDrawCommand* cmd = &g_batcher.commands[i];
        
        // 1. Pipeline / Shader Switch
        if (cmd->shader_key != current_shader) {
            // void* pipeline = shader_variant_get(cmd->shader_key);
            // rhi_bind_pipeline(renderer_context, pipeline);
            current_shader = cmd->shader_key;
        }
        
        // 2. Resource Bindings (Material/Textures)
        if (cmd->material_id != current_material) {
            // rhi_bind_material_descriptor(renderer_context, cmd->material_id);
            current_material = cmd->material_id;
        }
        
        // 3. Geometry Binding
        if (cmd->mesh_id != current_mesh) {
            // rhi_bind_mesh(renderer_context, cmd->mesh_id);
            current_mesh = cmd->mesh_id;
        }
        
        // 4. Draw
        // rhi_draw_indexed(renderer_context, cmd->mesh_id);
    }
    
    // Reset count for next batch
    g_batcher.count = 0;
}
