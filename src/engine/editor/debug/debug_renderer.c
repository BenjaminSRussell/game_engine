#include "debug_renderer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

// Internal Structures
typedef struct {
    simd_float3 start;
    simd_float4 start_color;
    simd_float3 end;
    simd_float4 end_color;
} debug_line_vertex_t;

struct debug_renderer {
    id line_buffer;      // id<MTLBuffer>
    id text_buffer;      // id<MTLBuffer>
    id font_atlas;       // id<MTLTexture>
    uint32_t line_count;
    uint32_t max_lines;
    
    // Pipelines
    id line_pipeline;    // id<MTLRenderPipelineState>
    id depth_state;      // id<MTLDepthStencilState>
};

debug_renderer_t* debug_renderer_create(id device_ptr, uint32_t max_lines) {
#ifdef __OBJC__
    if (!device_ptr) return NULL;
    id<MTLDevice> device = (id<MTLDevice>)device_ptr;
    
    debug_renderer_t* dbg = (debug_renderer_t*)calloc(1, sizeof(debug_renderer_t));
    if (!dbg) return NULL;
    
    dbg->max_lines = max_lines;
    dbg->line_count = 0;
    
    dbg->line_buffer = [device newBufferWithLength:sizeof(debug_line_vertex_t) * max_lines options:MTLResourceStorageModeShared];
    
    // Create Pipeline (mock setup, assuming library exists or handled elsewhere)
    // In a real implementation we would load shaders here
    NSError* error = nil;
    MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];
    desc.label = @"Debug Lines";
    // Setup generic defaults, would need actual shader functions
    // desc.vertexFunction = ...
    // desc.fragmentFunction = ...
    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    
    // dbg->line_pipeline = [device newRenderPipelineStateWithDescriptor:desc error:&error];
    
    return dbg;
#else
    return NULL;
#endif
}

void debug_renderer_destroy(debug_renderer_t* dbg) {
    if (dbg) {
        // Release Metal objects if not using ARC, or just free struct
        free(dbg);
    }
}

void debug_draw_line(debug_renderer_t* dbg, simd_float3 start, simd_float3 end, simd_float4 color) {
#ifdef __OBJC__
    if (!dbg || dbg->line_count >= dbg->max_lines) return;
    
    debug_line_vertex_t* lines = (debug_line_vertex_t*)[(id<MTLBuffer>)dbg->line_buffer contents];
    lines[dbg->line_count++] = (debug_line_vertex_t){
        .start = start,
        .start_color = color,
        .end = end,
        .end_color = color
    };
#endif
}

void debug_draw_box(debug_renderer_t* dbg, simd_float3 min, simd_float3 max, simd_float4 color) {
    // 12 Lines
    simd_float3 p0 = {min.x, min.y, min.z};
    simd_float3 p1 = {max.x, min.y, min.z};
    simd_float3 p2 = {max.x, max.y, min.z};
    simd_float3 p3 = {min.x, max.y, min.z};
    
    simd_float3 p4 = {min.x, min.y, max.z};
    simd_float3 p5 = {max.x, min.y, max.z};
    simd_float3 p6 = {max.x, max.y, max.z};
    simd_float3 p7 = {min.x, max.y, max.z};
    
    debug_draw_line(dbg, p0, p1, color);
    debug_draw_line(dbg, p1, p2, color);
    debug_draw_line(dbg, p2, p3, color);
    debug_draw_line(dbg, p3, p0, color);
    
    debug_draw_line(dbg, p4, p5, color);
    debug_draw_line(dbg, p5, p6, color);
    debug_draw_line(dbg, p6, p7, color);
    debug_draw_line(dbg, p7, p4, color);
    
    debug_draw_line(dbg, p0, p4, color);
    debug_draw_line(dbg, p1, p5, color);
    debug_draw_line(dbg, p2, p6, color);
    debug_draw_line(dbg, p3, p7, color);
}

void debug_draw_sphere(debug_renderer_t* dbg, simd_float3 center, float radius, simd_float4 color) {
    const int segments = 16;
    const float step = 2.0f * M_PI / (float)segments;
    
    for (int i = 0; i < segments; i++) {
        float theta1 = i * step;
        float theta2 = (i + 1) * step;
        
        float c1 = cosf(theta1) * radius;
        float s1 = sinf(theta1) * radius;
        float c2 = cosf(theta2) * radius;
        float s2 = sinf(theta2) * radius;
        
        // XY
        debug_draw_line(dbg, 
            simd_make_float3(center.x + c1, center.y + s1, center.z), 
            simd_make_float3(center.x + c2, center.y + s2, center.z), color);
            
        // XZ
        debug_draw_line(dbg, 
            simd_make_float3(center.x + c1, center.y, center.z + s1), 
            simd_make_float3(center.x + c2, center.y, center.z + s2), color);
            
        // YZ
        debug_draw_line(dbg, 
            simd_make_float3(center.x, center.y + c1, center.z + s1), 
            simd_make_float3(center.x, center.y + c2, center.z + s2), color);
    }
}

void debug_draw_text_3d(debug_renderer_t* dbg, simd_float3 position, const char* text, simd_float4 color) {
    // Stub implementation for text rendering
    // Would typically batch quads into a text buffer
}

void debug_render(id encoder_ptr, debug_renderer_t* dbg, const simd_float4x4* view_proj) {
#ifdef __OBJC__
    if (!dbg || dbg->line_count == 0) return;
    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>)encoder_ptr;
    
    // Bind pipeline and buffers here
    // [encoder setRenderPipelineState:dbg->line_pipeline];
    // [encoder setVertexBuffer:dbg->line_buffer offset:0 atIndex:0];
    // [encoder setVertexBytes:view_proj length:sizeof(simd_float4x4) atIndex:1];
    // [encoder drawPrimitives:MTLPrimitiveTypeLine vertexStart:0 vertexCount:dbg->line_count * 2];
#endif
}

void debug_clear(debug_renderer_t* dbg) {
    if (dbg) {
        dbg->line_count = 0;
    }
}
