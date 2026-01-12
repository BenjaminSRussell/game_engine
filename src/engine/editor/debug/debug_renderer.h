#ifndef DEBUG_RENDERER_H
#define DEBUG_RENDERER_H

#include <simd/simd.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
// Forward declarations for C
typedef void *id;
#endif

typedef struct debug_line {
  simd_float3 start;
  simd_float4 start_color;
  simd_float3 end;
  simd_float4 end_color;
} debug_line_t;

typedef struct debug_renderer {
  // Metal objects (void* in C, id in ObjC)
  id line_buffer; // id<MTLBuffer>
  uint32_t line_count;
  uint32_t max_lines;

  id line_pipeline;    // id<MTLRenderPipelineState>
  id depth_test_state; // id<MTLDepthStencilState>
  id no_depth_state;   // id<MTLDepthStencilState>
} debug_renderer_t;

#ifdef __cplusplus
extern "C" {
#endif

// Lifecycle
debug_renderer_t *debug_renderer_create(id device, uint32_t max_lines);
void debug_renderer_destroy(debug_renderer_t *dbg);

// Drawing Command Generation
void debug_draw_line(debug_renderer_t *dbg, simd_float3 start, simd_float3 end,
                     simd_float4 color);
void debug_draw_aabb(debug_renderer_t *dbg, simd_float3 min, simd_float3 max,
                     simd_float4 color);
void debug_draw_sphere(debug_renderer_t *dbg, simd_float3 center, float radius,
                       simd_float4 color);
void debug_draw_frustum(debug_renderer_t *dbg, simd_float4x4 inv_view_proj,
                        simd_float4 color);
void debug_draw_grid(debug_renderer_t *dbg, simd_float3 center, float size,
                     int divisions, simd_float4 color);
void debug_draw_text_3d(debug_renderer_t *dbg, simd_float3 position,
                        const char *text, simd_float4 color);
void debug_draw_text(debug_renderer_t *dbg, float x, float y, const char *text,
                     simd_float4 color);
void debug_clear(debug_renderer_t *dbg);

// Rendering to Encoder
void debug_render(debug_renderer_t *dbg, id encoder, simd_float4x4 view_proj,
                  bool depth_test);

#ifdef __cplusplus
}
#endif

#endif // DEBUG_RENDERER_H
