#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <simd/simd.h>
#include <Metal/Metal.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct debug_line {
    simd_float3 start;
    simd_float4 start_color;
    simd_float3 end;
    simd_float4 end_color;
} debug_line_t;

typedef struct debug_renderer {
    id<MTLBuffer> line_buffer;
    uint32_t line_count;
    uint32_t max_lines;
    id<MTLRenderPipelineState> line_pipeline;
    id<MTLDepthStencilState> depth_test_state;
    id<MTLDepthStencilState> no_depth_state;
} debug_renderer_t;

// API
debug_renderer_t* debug_renderer_create(id<MTLDevice> device, uint32_t max_lines, MTLPixelFormat color_format, MTLPixelFormat depth_format);
void debug_renderer_destroy(debug_renderer_t* dbg);

void debug_draw_line(debug_renderer_t* dbg, simd_float3 start, simd_float3 end, simd_float4 color);
void debug_draw_aabb(debug_renderer_t* dbg, simd_float3 min, simd_float3 max, simd_float4 color);
void debug_draw_text(debug_renderer_t* dbg, float x, float y, const char* text, simd_float4 color);

void debug_render(debug_renderer_t* dbg, id<MTLRenderCommandEncoder> encoder, simd_float4x4 view_proj, bool depth_test);
void debug_clear(debug_renderer_t* dbg);

#endif // DEBUG_UTILS_H
