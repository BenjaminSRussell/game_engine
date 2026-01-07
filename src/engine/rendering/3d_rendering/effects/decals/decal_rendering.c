/*
 * decal_rendering.c
 * Deferred decal rendering
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement GPU particle system
 * TODO: Add particle collision
 * TODO: Implement ribbon/trail rendering
 * TODO: Add VFX graph system
 * TODO: Implement decal rendering
 * TODO: Add weather effects
 * TODO: Implement particle sorting
 * TODO: Add particle LOD
 * TODO: Implement force fields
 * TODO: Add particle events
 * TODO: Implement decal rendering initialization
 * TODO: Add decal rendering cleanup/shutdown
 * TODO: Implement decal rendering validation
 * TODO: Add decal rendering error handling
 * TODO: Implement decal rendering serialization
 * TODO: Add decal rendering debug output
 * TODO: Implement decal rendering unit tests
 * TODO: Add decal rendering performance counters
 * TODO: Implement decal rendering hot-reload
 * TODO: Add decal rendering thread safety
 * TODO: Implement decal rendering memory pooling
 * TODO: Add decal rendering caching layer
 * TODO: Implement decal rendering async operations
 * TODO: Add decal rendering GPU integration
 * TODO: Implement decal rendering SIMD optimization
 * TODO: Add decal rendering batch processing
 * TODO: Implement decal rendering streaming support
 * TODO: Add decal rendering LOD support
 * TODO: Implement decal rendering culling integration
 * TODO: Add decal rendering render graph node
 */

#include "decal_rendering.h"
#include "../../core/math/math/mat4.h"
#include <Metal/Metal.h>
#include <simd/simd.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "../rendering/deferred/gbuffer_layout.h"

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define EFFECTS_DECAL_RENDERING_MAX_COUNT 4096
#define EFFECTS_DECAL_RENDERING_DEFAULT_CAPACITY 256

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

typedef struct {
    matrix_float4x4 world_to_decal;
    matrix_float4x4 model_matrix;
    vector_float4 color_tint;
    float normal_threshold;
    uint32_t albedo_texture_index;
    uint32_t normal_texture_index;
    uint32_t material_texture_index;
    float roughness_override;
    float metallic_override;
} decal_instance_t;

typedef struct {
    matrix_float4x4 view_proj;
    matrix_float4x4 inv_view_proj;
    vector_float3 camera_pos;
    vector_float2 screen_size;
} scene_uniforms_t;

typedef struct effects_decal_rendering_internal {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    bool dirty;
    // Decal specific data
    matrix_float4x4 transform;
    uint32_t albedo_tex;
    uint32_t normal_tex;
    float opacity;
} effects_decal_rendering_internal_t;

typedef struct effects_decal_rendering_context {
    effects_decal_rendering_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
    
    // Metal objects
    id<MTLDevice> device;
    id<MTLRenderPipelineState> pipeline_state;
    id<MTLDepthStencilState> depth_stencil_state;
    id<MTLBuffer> instance_buffer;
    id<MTLBuffer> uniform_buffer;
} effects_decal_rendering_context_t;

static effects_decal_rendering_context_t g_decal_rendering_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void create_pipeline_state(void) {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice(); // Or pass usage
    g_decal_rendering_ctx.device = device;
    
    id<MTLLibrary> library = [device newDefaultLibrary];
    if (!library) return;
    
    id<MTLFunction> vertex_fn = [library newFunctionWithName:@"decal_vertex"];
    id<MTLFunction> fragment_fn = [library newFunctionWithName:@"decal_fragment"];
    
    MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];
    desc.vertexFunction = vertex_fn;
    desc.fragmentFunction = fragment_fn;
    
    // G-Buffer formats (Should match gbuffer_layout.h or be passed in)
    // Assuming standard formats:
    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm; // Albedo
    desc.colorAttachments[0].blendingEnabled = YES;
    desc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    desc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    desc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    desc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    desc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
    desc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorZero;
    
    desc.colorAttachments[1].pixelFormat = MTLPixelFormatRG16Float; // Normal
    desc.colorAttachments[1].blendingEnabled = YES;
    desc.colorAttachments[1].rgbBlendOperation = MTLBlendOperationAdd;
    desc.colorAttachments[1].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    desc.colorAttachments[1].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    desc.colorAttachments[1].alphaBlendOperation = MTLBlendOperationAdd;
    desc.colorAttachments[1].sourceAlphaBlendFactor = MTLBlendFactorOne;
    desc.colorAttachments[1].destinationAlphaBlendFactor = MTLBlendFactorZero;
    
    desc.colorAttachments[2].pixelFormat = MTLPixelFormatRGBA8Unorm; // Material
    desc.colorAttachments[2].blendingEnabled = NO;

    desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float; // G-Buffer Depth
    
    NSError* error = nil;
    g_decal_rendering_ctx.pipeline_state = [device newRenderPipelineStateWithDescriptor:desc error:&error];
    
    // Depth State: Disable depth write, but we might want depth test?
    // We actually just want to draw the box. Often we draw back-faces of the cube to avoid
    // camera clipping issues, and disable depth test or use Greater/Always.
    // Since we reconstruct depth in shader, rasterization depth test is less critical
    // BUT we don't want to draw decals occluded by other geometry potentially?
    // Actually, decal projection should handle "on top of".
    // Let's use Always pass, no write.
    MTLDepthStencilDescriptor* depth_desc = [[MTLDepthStencilDescriptor alloc] init];
    depth_desc.depthCompareFunction = MTLCompareFunctionAlways;
    depth_desc.depthWriteEnabled = NO;
    g_decal_rendering_ctx.depth_stencil_state = [device newDepthStencilStateWithDescriptor:depth_desc];
    
    // Create buffers
    g_decal_rendering_ctx.instance_buffer = [device newBufferWithLength:sizeof(decal_instance_t) * EFFECTS_DECAL_RENDERING_MAX_COUNT options:MTLResourceStorageModeShared];
    g_decal_rendering_ctx.uniform_buffer = [device newBufferWithLength:sizeof(scene_uniforms_t) options:MTLResourceStorageModeShared];
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int effects_decal_rendering_init(void) {
    if (g_decal_rendering_ctx.initialized) return 0;

    g_decal_rendering_ctx.capacity = EFFECTS_DECAL_RENDERING_DEFAULT_CAPACITY;
    g_decal_rendering_ctx.items = calloc(g_decal_rendering_ctx.capacity, sizeof(effects_decal_rendering_internal_t));
    
    create_pipeline_state();

    g_decal_rendering_ctx.count = 0;
    g_decal_rendering_ctx.initialized = true;
    return 0;
}

void effects_decal_rendering_shutdown(void) {
    if (!g_decal_rendering_ctx.initialized) return;
    free(g_decal_rendering_ctx.items);
    g_decal_rendering_ctx.items = NULL;
    g_decal_rendering_ctx.initialized = false;
}

int effects_decal_rendering_create(effects_decal_rendering_handle_t* out_handle, const effects_decal_rendering_desc_t* desc) {
    if (!out_handle || !desc || !g_decal_rendering_ctx.initialized) return -1;
    if (g_decal_rendering_ctx.count >= g_decal_rendering_ctx.capacity) return -3;

    uint32_t index = g_decal_rendering_ctx.count++;
    effects_decal_rendering_internal_t* item = &g_decal_rendering_ctx.items[index];
    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    
    // Set default transform
    item->transform = matrix_identity_float4x4;
    item->opacity = 1.0f;

    out_handle->id = index;
    return 0;
}

void effects_decal_rendering_destroy(effects_decal_rendering_handle_t handle) {
    if (handle.id < g_decal_rendering_ctx.count) {
        g_decal_rendering_ctx.items[handle.id].initialized = false;
    }
}

int effects_decal_rendering_update(effects_decal_rendering_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_decal_rendering_ctx.count) return -1;
    // Update transform or properties from data
    // Assuming data is matrix_float4x4 for now
    if (size == sizeof(matrix_float4x4)) {
        g_decal_rendering_ctx.items[handle.id].transform = *(const matrix_float4x4*)data;
    }
    return 0;
}

bool effects_decal_rendering_is_valid(effects_decal_rendering_handle_t handle) {
    return (handle.id < g_decal_rendering_ctx.count) && g_decal_rendering_ctx.items[handle.id].initialized;
}

int effects_decal_rendering_get_info(effects_decal_rendering_handle_t handle, effects_decal_rendering_info_t* out_info) {
    if (!out_info || handle.id >= g_decal_rendering_ctx.count) return -1;
    out_info->id = g_decal_rendering_ctx.items[handle.id].id;
    return 0;
}

void effects_decal_rendering_mark_dirty(effects_decal_rendering_handle_t handle) {}

int effects_decal_rendering_process_pending(void) { return 0; }

uint32_t effects_decal_rendering_get_count(void) { return g_decal_rendering_ctx.count; }

size_t effects_decal_rendering_get_memory_usage(void) {
    return sizeof(g_decal_rendering_ctx) + g_decal_rendering_ctx.capacity * sizeof(effects_decal_rendering_internal_t);
}

void effects_decal_rendering_debug_print(void) {}

void effects_decal_rendering_render(effects_decal_rendering_handle_t handle, void* encoder_ptr, void* uniforms_ptr) {
    // Note: 'handle' in this context is likely unused if we render ALL decals.
    // The previous design seemed to imply stateful management.
    // We will render ALL active decals.
    
    id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)encoder_ptr;
    // Uniforms passed from higher level or reconstructed.
    // Ideally uniforms_ptr points to scene_uniforms_t struct.
    
    if (!g_decal_rendering_ctx.pipeline_state) return;

    [encoder setRenderPipelineState:g_decal_rendering_ctx.pipeline_state];
    [encoder setDepthStencilState:g_decal_rendering_ctx.depth_stencil_state];
    // Set Cull Mode: Front to draw back faces (camera inside volume support)
    [encoder setCullMode:MTLCullModeFront]; 
    
    // Update Instance Buffer
    decal_instance_t* instances = (decal_instance_t*)[g_decal_rendering_ctx.instance_buffer contents];
    uint32_t instance_count = 0;
    
    for (uint32_t i = 0; i < g_decal_rendering_ctx.count; i++) {
        if (g_decal_rendering_ctx.items[i].initialized) {
            instances[instance_count].model_matrix = g_decal_rendering_ctx.items[i].transform;
            instances[instance_count].world_to_decal = simd_inverse(g_decal_rendering_ctx.items[i].transform);
            instances[instance_count].color_tint = (vector_float4){1, 1, 1, g_decal_rendering_ctx.items[i].opacity};
            // Set texture indices if we had a bindless array, for now 0
            instance_count++;
        }
    }
    
    if (instance_count == 0) return;
    
    // Update Uniform Buffer
    if (uniforms_ptr) {
        memcpy([g_decal_rendering_ctx.uniform_buffer contents], uniforms_ptr, sizeof(scene_uniforms_t));
    }
    
    [encoder setVertexBuffer:g_decal_rendering_ctx.instance_buffer offset:0 atIndex:0];
    [encoder setVertexBuffer:g_decal_rendering_ctx.uniform_buffer offset:0 atIndex:1];
    
    [encoder setFragmentBuffer:g_decal_rendering_ctx.instance_buffer offset:0 atIndex:0];
    [encoder setFragmentBuffer:g_decal_rendering_ctx.uniform_buffer offset:0 atIndex:1];
    
    // Bind G-Buffer Textures (from Global or passed in?)
    // This part is tricky without the GBuffer handles.
    // We rely on gbuffer_layout.h getter
    void *albedo, *normal, *material, *depth;
    rendering_gbuffer_get_targets(&albedo, &normal, &material, &depth);
    
    [encoder setFragmentTexture:(__bridge id<MTLTexture>)depth atIndex:0];
    // [encoder setFragmentTexture:albedo_texture_array atIndex:1]; // TODO: Bind decal atlas/array
    
    // Draw Cube (36 vertices)
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:36 instanceCount:instance_count];
}
