/*
 * forward_lighting.c
 * Forward lighting loop
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "forward_lighting.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

/* ============================================================================
 * GLOBALS
 * ============================================================================ */

#ifdef __OBJC__
static forward_renderer_t g_forward_renderer = {0};
#else
// Placeholder for non-ObjC builds, though this file should be compiled as ObjC
static char g_forward_renderer_placeholder[sizeof(void*) * 6]; 
#endif

static bool g_initialized = false;

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

forward_renderer_t* rendering_forward_lighting_get_renderer(void) {
#ifdef __OBJC__
    return &g_forward_renderer;
#else
    return (forward_renderer_t*)&g_forward_renderer_placeholder;
#endif
}

int rendering_forward_lighting_init(void) {
    if (g_initialized) return 0;

#ifdef __OBJC__
    id<MTLDevice> device = MTLCreateSystemDefaultDevice(); // In real engine, get from subsystem
    if (!device) return -1;

    // 1. Create Buffers
    // ------------------------------------------------------------------------
    // Light grid buffer (Clustered Lighting)
    // Size depends on screen resolution and tile size (e.g., 16x16 tiles)
    // For now, allocating a reasonable fixed size or based on a max config
    NSUInteger grid_size = 16 * 16 * 256 * sizeof(uint32_t); // Example size
    g_forward_renderer.light_grid_buffer = [device newBufferWithLength:grid_size options:MTLResourceStorageModePrivate];
    
    // Light data buffer
    NSUInteger light_data_size = 4096 * 64; // Max 4096 lights * 64 bytes per light
    g_forward_renderer.light_data_buffer = [device newBufferWithLength:light_data_size options:MTLResourceStorageModeShared];

    // 2. Load Shaders and Create Pipelines
    // ------------------------------------------------------------------------
    id<MTLLibrary> library = [device newDefaultLibrary];
    if (!library) {
        // Fallback or error if no default library found (might happen in test env)
        // Proceeding with warning - pipelines will be nil and draws will be skipped/crash if not handled
    } else {
        // Opaque Pipeline
        MTLRenderPipelineDescriptor* opaqueDesc = [[MTLRenderPipelineDescriptor alloc] init];
        opaqueDesc.vertexFunction = [library newFunctionWithName:@"vertex_forward"];
        opaqueDesc.fragmentFunction = [library newFunctionWithName:@"fragment_forward_opaque"];
        opaqueDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm; // Assumption
        opaqueDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        
        NSError* error = nil;
        g_forward_renderer.opaque_pipeline = [device newRenderPipelineStateWithDescriptor:opaqueDesc error:&error];
        if (error) {
            // Log error
        }

        // Transparent Pipeline
        MTLRenderPipelineDescriptor* transpDesc = [[MTLRenderPipelineDescriptor alloc] init];
        transpDesc.vertexFunction = [library newFunctionWithName:@"vertex_forward"];
        transpDesc.fragmentFunction = [library newFunctionWithName:@"fragment_forward_transparent"];
        transpDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        transpDesc.colorAttachments[0].blendingEnabled = YES;
        transpDesc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        transpDesc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        transpDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        transpDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        transpDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        transpDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        transpDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        g_forward_renderer.transparent_pipeline = [device newRenderPipelineStateWithDescriptor:transpDesc error:&error];
    }
    
    // 3. Create Depth Stencil States
    // ------------------------------------------------------------------------
    // Opaque: Write On, Test Less
    MTLDepthStencilDescriptor* dsOpaque = [[MTLDepthStencilDescriptor alloc] init];
    dsOpaque.depthCompareFunction = MTLCompareFunctionLess;
    dsOpaque.depthWriteEnabled = YES;
    g_forward_renderer.depth_state_opaque = [device newDepthStencilStateWithDescriptor:dsOpaque];

    // Transparent: Write Off, Test Less
    MTLDepthStencilDescriptor* dsTransp = [[MTLDepthStencilDescriptor alloc] init];
    dsTransp.depthCompareFunction = MTLCompareFunctionLess;
    dsTransp.depthWriteEnabled = NO;
    g_forward_renderer.depth_state_transparent = [device newDepthStencilStateWithDescriptor:dsTransp];

#endif

    g_initialized = true;
    return 0;
}

void rendering_forward_lighting_shutdown(void) {
    if (!g_initialized) return;

#ifdef __OBJC__
    g_forward_renderer.opaque_pipeline = nil;
    g_forward_renderer.transparent_pipeline = nil;
    g_forward_renderer.depth_state_opaque = nil;
    g_forward_renderer.depth_state_transparent = nil;
    g_forward_renderer.light_grid_buffer = nil;
    g_forward_renderer.light_data_buffer = nil;
#endif

    g_initialized = false;
}

/* ============================================================================
 * OTHER API IMPLEMENTATIONS (STUBS OR SIMPLE)
 * ============================================================================ */

int rendering_forward_lighting_create(rendering_forward_lighting_handle_t* out_handle, const rendering_forward_lighting_desc_t* desc) {
    // Placeholder - handle creation logic
    if (out_handle) out_handle->id = 0;
    return 0;
}

void rendering_forward_lighting_destroy(rendering_forward_lighting_handle_t handle) {
}

int rendering_forward_lighting_update(rendering_forward_lighting_handle_t handle, const void* data, size_t size) {
#ifdef __OBJC__
    // Update light data buffer with new lights
    // memcpy to buffer.contents
    if (g_forward_renderer.light_data_buffer && data && size <= g_forward_renderer.light_data_buffer.length) {
        memcpy(g_forward_renderer.light_data_buffer.contents, data, size);
        return 0;
    }
#endif
    return -1;
}

bool rendering_forward_lighting_is_valid(rendering_forward_lighting_handle_t handle) {
    return true;
}

int rendering_forward_lighting_get_info(rendering_forward_lighting_handle_t handle, rendering_forward_lighting_info_t* out_info) {
    return 0;
}

void rendering_forward_lighting_mark_dirty(rendering_forward_lighting_handle_t handle) {
}

int rendering_forward_lighting_process_pending(void) {
    return 0;
}

uint32_t rendering_forward_lighting_get_count(void) {
    return 0;
}

size_t rendering_forward_lighting_get_memory_usage(void) {
#ifdef __OBJC__
    size_t total = 0;
    if (g_forward_renderer.light_grid_buffer) total += g_forward_renderer.light_grid_buffer.length;
    if (g_forward_renderer.light_data_buffer) total += g_forward_renderer.light_data_buffer.length;
    return total;
#else
    return 0;
#endif
}

void rendering_forward_lighting_debug_print(void) {
}
