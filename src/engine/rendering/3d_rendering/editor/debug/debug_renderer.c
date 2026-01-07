#include "debug_renderer.h"
#include <stdlib.h>
#include <string.h>

// Force ObjC syntax if compiling as C but needing Metal
// In a real project, build rules would handle this.
#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

debug_renderer_t* debug_renderer_create(id device_ptr, uint32_t max_lines) {
#ifdef __OBJC__
    id<MTLDevice> device = (id<MTLDevice>)device_ptr;
    debug_renderer_t* dbg = (debug_renderer_t*)calloc(1, sizeof(debug_renderer_t));
    if (!dbg) return NULL;
    
    dbg->max_lines = max_lines;
    dbg->line_count = 0;
    
    // Create Buffers
    dbg->line_buffer = [device newBufferWithLength:sizeof(debug_line_t) * max_lines options:MTLResourceStorageModeShared];
    
    // Create Pipeline
    id<MTLLibrary> library = [device newDefaultLibrary];
    id<MTLFunction> vertex_fn = [library newFunctionWithName:@"vertex_debug"];
    id<MTLFunction> fragment_fn = [library newFunctionWithName:@"fragment_debug"];
    
    if (vertex_fn && fragment_fn) {
        MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];
        desc.vertexFunction = vertex_fn;
        desc.fragmentFunction = fragment_fn;
        desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm; // Assumed swapchain format
        desc.colorAttachments[0].blendingEnabled = YES;
        desc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        desc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float; // Assumed depth format
        
        NSError* error = nil;
        dbg->line_pipeline = [device newRenderPipelineStateWithDescriptor:desc error:&error];
    }
    
    // Depth States
    MTLDepthStencilDescriptor* depth_desc = [[MTLDepthStencilDescriptor alloc] init];
    depth_desc.depthCompareFunction = MTLCompareFunctionLessEqual;
    depth_desc.depthWriteEnabled = NO; // Debug lines usually don't write depth
    dbg->depth_test_state = [device newDepthStencilStateWithDescriptor:depth_desc];
    
    depth_desc.depthCompareFunction = MTLCompareFunctionAlways;
    dbg->no_depth_state = [device newDepthStencilStateWithDescriptor:depth_desc];
    
    return dbg;
#else
    return NULL;
#endif
}

void debug_renderer_destroy(debug_renderer_t* dbg) {
    if (dbg) free(dbg);
}

void debug_draw_line(debug_renderer_t* dbg, simd_float3 start, simd_float3 end, simd_float4 color) {
#ifdef __OBJC__
    if (!dbg || dbg->line_count >= dbg->max_lines) return;

    debug_line_t* lines = (debug_line_t*)[(id<MTLBuffer>)dbg->line_buffer contents];
    lines[dbg->line_count++] = (debug_line_t){
        .start = start,
        .start_color = color,
        .end = end,
        .end_color = color
    };
#endif
}

void debug_draw_aabb(debug_renderer_t* dbg, simd_float3 min, simd_float3 max, simd_float4 color) {
    simd_float3 corners[8] = {
        simd_make_float3(min.x, min.y, min.z),
        simd_make_float3(max.x, min.y, min.z),
        simd_make_float3(max.x, max.y, min.z),
        simd_make_float3(min.x, max.y, min.z),
        simd_make_float3(min.x, min.y, max.z),
        simd_make_float3(max.x, min.y, max.z),
        simd_make_float3(max.x, max.y, max.z),
        simd_make_float3(min.x, max.y, max.z),
    };

    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7}
    };

    for (int i = 0; i < 12; i++) {
        debug_draw_line(dbg, corners[edges[i][0]], corners[edges[i][1]], color);
    }
}

void debug_draw_sphere(debug_renderer_t* dbg, simd_float3 center, float radius, simd_float4 color) {
#ifdef __OBJC__
    const int segments = 16;
    const float step = 2.0f * 3.14159f / (float)segments;
    
    // Draw 3 circles
    for (int i = 0; i < segments; i++) {
        float theta1 = i * step;
        float theta2 = (i + 1) * step;
        
        float c1 = cos(theta1) * radius;
        float s1 = sin(theta1) * radius;
        float c2 = cos(theta2) * radius;
        float s2 = sin(theta2) * radius;
        
        // XY Plane
        debug_draw_line(dbg, center + simd_make_float3(c1, s1, 0), center + simd_make_float3(c2, s2, 0), color);
        // XZ Plane
        debug_draw_line(dbg, center + simd_make_float3(c1, 0, s1), center + simd_make_float3(c2, 0, s2), color);
        // YZ Plane
        debug_draw_line(dbg, center + simd_make_float3(0, c1, s1), center + simd_make_float3(0, c2, s2), color);
    }
#endif
}

void debug_draw_frustum(debug_renderer_t* dbg, simd_float4x4 inv_view_proj, simd_float4 color) {
#ifdef __OBJC__
    simd_float4 corners[8] = {
        simd_make_float4(-1, -1, 0, 1), simd_make_float4( 1, -1, 0, 1),
        simd_make_float4( 1,  1, 0, 1), simd_make_float4(-1,  1, 0, 1),
        simd_make_float4(-1, -1, 1, 1), simd_make_float4( 1, -1, 1, 1),
        simd_make_float4( 1,  1, 1, 1), simd_make_float4(-1,  1, 1, 1)
    };
    
    simd_float3 world_corners[8];
    for(int i=0; i<8; i++) {
        simd_float4 world = simd_mul(inv_view_proj, corners[i]);
        world_corners[i] = world.xyz / world.w;
    }
    
    // Near plane
    debug_draw_line(dbg, world_corners[0], world_corners[1], color);
    debug_draw_line(dbg, world_corners[1], world_corners[2], color);
    debug_draw_line(dbg, world_corners[2], world_corners[3], color);
    debug_draw_line(dbg, world_corners[3], world_corners[0], color);
    
    // Far plane
    debug_draw_line(dbg, world_corners[4], world_corners[5], color);
    debug_draw_line(dbg, world_corners[5], world_corners[6], color);
    debug_draw_line(dbg, world_corners[6], world_corners[7], color);
    debug_draw_line(dbg, world_corners[7], world_corners[4], color);
    
    // Connections
    debug_draw_line(dbg, world_corners[0], world_corners[4], color);
    debug_draw_line(dbg, world_corners[1], world_corners[5], color);
    debug_draw_line(dbg, world_corners[2], world_corners[6], color);
    debug_draw_line(dbg, world_corners[3], world_corners[7], color);
#endif
}

void debug_draw_grid(debug_renderer_t* dbg, simd_float3 center, float size, int divisions, simd_float4 color) {
#ifdef __OBJC__
    if (!dbg || divisions <= 0) return;
    
    float step = size / (float)divisions;
    float half_size = size * 0.5f;
    
    // Draw grid on XZ plane (assume Y is up)
    for (int i = 0; i <= divisions; i++) {
        float offset = -half_size + (i * step);
        
        // Lines parallel to X axis
        simd_float3 start_x = center + simd_make_float3(-half_size, 0, offset);
        simd_float3 end_x = center + simd_make_float3(half_size, 0, offset);
        
        // Lines parallel to Z axis
        simd_float3 start_z = center + simd_make_float3(offset, 0, -half_size);
        simd_float3 end_z = center + simd_make_float3(offset, 0, half_size);
        
        // Dim the color for non-center lines
        simd_float4 line_color = color;
        if (i == divisions / 2) {
            line_color.w = 1.0f; // Full alpha for center lines
        } else {
            line_color.w = 0.3f; // Dimmer for other lines
        }
        
        debug_draw_line(dbg, start_x, end_x, line_color);
        debug_draw_line(dbg, start_z, end_z, line_color);
    }
#endif
}

void debug_draw_text_3d(debug_renderer_t* dbg, simd_float3 position, const char* text, simd_float4 color) {
#ifdef __OBJC__
    // TODO: Implement 3D text rendering using billboards or SDF text
    // For now, draw a small sphere as a placeholder marker
    if (!dbg || !text) return;
    debug_draw_sphere(dbg, position, 0.1f, color);
#endif
}

void debug_render(debug_renderer_t* dbg, id encoder_ptr, simd_float4x4 view_proj, bool depth_test) {
#ifdef __OBJC__
    if (!dbg || dbg->line_count == 0 || !dbg->line_pipeline) return;

    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>)encoder_ptr;

    [encoder setRenderPipelineState:dbg->line_pipeline];
    [encoder setDepthStencilState:depth_test ? dbg->depth_test_state : dbg->no_depth_state];
    [encoder setVertexBuffer:dbg->line_buffer offset:0 atIndex:0];
    [encoder setVertexBytes:&view_proj length:sizeof(simd_float4x4) atIndex:1];

    [encoder drawPrimitives:MTLPrimitiveTypeLine vertexStart:0 vertexCount:dbg->line_count * 2];
#endif
}

void debug_clear(debug_renderer_t* dbg) {
    if (dbg) dbg->line_count = 0;
}
