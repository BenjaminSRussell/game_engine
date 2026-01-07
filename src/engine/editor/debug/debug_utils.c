#include "editor/debug/debug_utils.h"
#include <stdlib.h>

debug_renderer_t* debug_renderer_create(id<MTLDevice> device, uint32_t max_lines, MTLPixelFormat color_format, MTLPixelFormat depth_format) {
    debug_renderer_t* dbg = (debug_renderer_t*)calloc(1, sizeof(debug_renderer_t));
    if (!dbg) return NULL;
    
    dbg->max_lines = max_lines;
    dbg->line_count = 0;
    
    dbg->line_buffer = [device newBufferWithLength:sizeof(debug_line_t) * max_lines options:MTLResourceStorageModeShared];
    
    // Create Pipeline
    id<MTLLibrary> library = [device newDefaultLibrary]; // Assuming debug shaders are in default lib
    if (!library) {
        free(dbg);
        return NULL;
    }
    
    id<MTLFunction> vertex_fn = [library newFunctionWithName:@"debug_line_vertex"];
    id<MTLFunction> fragment_fn = [library newFunctionWithName:@"debug_line_fragment"];
    
    MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];
    desc.vertexFunction = vertex_fn;
    desc.fragmentFunction = fragment_fn;
    desc.colorAttachments[0].pixelFormat = color_format;
    desc.depthAttachmentPixelFormat = depth_format;
    
    // Enable simple alpha blending
    desc.colorAttachments[0].blendingEnabled = YES;
    desc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    desc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    desc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    desc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    desc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
    desc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    
    NSError* error = nil;
    dbg->line_pipeline = [device newRenderPipelineStateWithDescriptor:desc error:&error];
    
    // Depth states
    MTLDepthStencilDescriptor* depth_desc = [[MTLDepthStencilDescriptor alloc] init];
    depth_desc.depthCompareFunction = MTLCompareFunctionLess;
    depth_desc.depthWriteEnabled = YES; // Or NO if we don't want lines to occlude
    dbg->depth_test_state = [device newDepthStencilStateWithDescriptor:depth_desc];
    
    depth_desc.depthCompareFunction = MTLCompareFunctionAlways;
    depth_desc.depthWriteEnabled = NO;
    dbg->no_depth_state = [device newDepthStencilStateWithDescriptor:depth_desc];
    
    return dbg;
}

void debug_renderer_destroy(debug_renderer_t* dbg) {
    if (dbg) {
        free(dbg);
    }
}

void debug_draw_line(debug_renderer_t* dbg, simd_float3 start, simd_float3 end, simd_float4 color) {
    if (!dbg || dbg->line_count >= dbg->max_lines) return;

    debug_line_t* lines = (debug_line_t*)[dbg->line_buffer contents];
    lines[dbg->line_count++] = (debug_line_t){
        .start = start,
        .start_color = color,
        .end = end,
        .end_color = color
    };
}

void debug_draw_aabb(debug_renderer_t* dbg, simd_float3 min, simd_float3 max, simd_float4 color) {
    // 12 edges
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

void debug_draw_text(debug_renderer_t* dbg, float x, float y, const char* text, simd_float4 color) {
    // TODO: Implement text rendering using font atlas
    // This requires a separate pipeline and vertex buffer for quads/glyphs
}

void debug_render(debug_renderer_t* dbg, id<MTLRenderCommandEncoder> encoder,
                  simd_float4x4 view_proj, bool depth_test) {
    if (!dbg || dbg->line_count == 0) return;

    [encoder setRenderPipelineState:dbg->line_pipeline];
    [encoder setDepthStencilState:depth_test ? dbg->depth_test_state : dbg->no_depth_state];
    [encoder setVertexBuffer:dbg->line_buffer offset:0 atIndex:0];
    [encoder setVertexBytes:&view_proj length:sizeof(simd_float4x4) atIndex:1];

    [encoder drawPrimitives:MTLPrimitiveTypeLine vertexStart:0 vertexCount:dbg->line_count * 2];
}

void debug_clear(debug_renderer_t* dbg) {
    if (dbg) {
        dbg->line_count = 0;
    }
}
