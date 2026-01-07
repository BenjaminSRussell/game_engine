/*
 * hzb_builder.c
 * Hierarchical Z-Buffer Construction (Downsampling)
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#include "geometry/culling/occlusion/hzb_builder.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <include/math/math.h>

/* ============================================================================
 * TYPES & GLOBALS
 * ============================================================================ */

typedef struct hzb_builder_context {
    bool initialized;
    uint32_t width;
    uint32_t height;
    uint32_t mip_levels;
    
    void* hzb_texture; // GPU handle
    void* sampler_min; // Reduction sampler
} hzb_builder_context_t;

static hzb_builder_context_t g_hzb_ctx = {0};

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

static uint32_t calculate_mips(uint32_t width, uint32_t height) {
    uint32_t levels = 1;
    while (width > 1 || height > 1) {
        width = (width > 1) ? width >> 1 : 1;
        height = (height > 1) ? height >> 1 : 1;
        levels++;
    }
    return levels;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int hzb_builder_init(uint32_t width, uint32_t height) {
    if (g_hzb_ctx.initialized) return 0;
    
    g_hzb_ctx.width = width;
    g_hzb_ctx.height = height;
    g_hzb_ctx.mip_levels = calculate_mips(width, height);
    
    // TODO: Create HZB Texture (R32F)
    // RenderAPI_CreateTexture(&desc);
    
    // TODO: Create Compute Pipelines for downsampling
    
    g_hzb_ctx.initialized = true;
    return 0;
}

void hzb_builder_shutdown(void) {
    if (!g_hzb_ctx.initialized) return;
    
    // TODO: Destroy textures and pipelines
    
    g_hzb_ctx.initialized = false;
}

/*
 * Builds the HZB mip chain from the scene depth buffer.
 * Usually done via a compute shader that takes Mip K and produces Mip K+1.
 */
int hzb_builder_build(void* source_depth_buffer) {
    if (!g_hzb_ctx.initialized) return -1;
    
    // 1. Copy Depth Buffer to HZB Mip 0
    // RenderAPI_CopyTexture(source_depth_buffer, g_hzb_ctx.hzb_texture, 0);
    
    // 2. Downsample
    // For each mip level 1..N
    //   Bind input: Mip i-1
    //   Bind output: Mip i
    //   Dispatch Compute (Reduction Min/Max)
    
    for (uint32_t i = 1; i < g_hzb_ctx.mip_levels; i++) {
        // uint32_t mip_width = u_max(1, g_hzb_ctx.width >> i);
        // uint32_t mip_height = u_max(1, g_hzb_ctx.height >> i);
        
        // RenderAPI_SetComputeShader(g_hzb_downsample_shader);
        // RenderAPI_Dispatch(...);
        // RenderAPI_Barrier();
    }
    
    return 0;
}

void* hzb_builder_get_texture(void) {
    return g_hzb_ctx.hzb_texture;
}

void hzb_builder_resize(uint32_t width, uint32_t height) {
    if (width == g_hzb_ctx.width && height == g_hzb_ctx.height) return;
    
    hzb_builder_shutdown();
    hzb_builder_init(width, height);
}
