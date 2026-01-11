#ifndef DEBUG_RENDERER_H
#define DEBUG_RENDERER_H

#include <simd/simd.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
// Forward declarations for C
typedef void* id;
#endif

// Matching the prompt's request structure
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
    
    id line_pipeline; // id<MTLRenderPipelineState>
    id depth_test_state; // id<MTLDepthStencilState>
    id no_depth_state; // id<MTLDepthStencilState>
} debug_renderer_t;

#ifdef __cplusplus
extern "C" {
#endif

// Lifecycle
debug_renderer_t* debug_renderer_create(id device, uint32_t max_lines);
void debug_renderer_destroy(debug_renderer_t* dbg);

// Drawing Command Generation
void debug_draw_line(debug_renderer_t* dbg, simd_float3 start, simd_float3 end, simd_float4 color);
void debug_draw_aabb(debug_renderer_t* dbg, simd_float3 min, simd_float3 max, simd_float4 color);
void debug_draw_sphere(debug_renderer_t* dbg, simd_float3 center, float radius, simd_float4 color);
void debug_draw_frustum(debug_renderer_t* dbg, simd_float4x4 inv_view_proj, simd_float4 color);
void debug_draw_grid(debug_renderer_t* dbg, simd_float3 center, float size, int divisions, simd_float4 color);
void debug_draw_text_3d(debug_renderer_t* dbg, simd_float3 position, const char* text, simd_float4 color);
void debug_draw_text(debug_renderer_t* dbg, float x, float y, const char* text, simd_float4 color);
void debug_clear(debug_renderer_t* dbg);

// Internal helper functions (static in implementation)
static void debug_draw_quad(debug_renderer_t* dbg, simd_float3 bl, simd_float3 br, simd_float3 tr, simd_float3 tl, simd_float4 color);
static void debug_draw_text_outline(debug_renderer_t* dbg, simd_float3 position, const char* text, simd_float4 color);

// Character drawing helpers
static void debug_draw_char_A(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_B(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_C(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_D(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_E(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_F(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_G(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_H(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_I(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_L(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_N(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_O(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_R(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_S(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_T(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_U(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_dash(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);
static void debug_draw_char_box(debug_renderer_t* dbg, simd_float3 pos, float size, simd_float3 up, simd_float3 right, simd_float4 color);

// Color Constants
static const simd_float4 DEBUG_COLOR_RED = {1.0f, 0.0f, 0.0f, 1.0f};
static const simd_float4 DEBUG_COLOR_GREEN = {0.0f, 1.0f, 0.0f, 1.0f};
static const simd_float4 DEBUG_COLOR_BLUE = {0.0f, 0.0f, 1.0f, 1.0f};
static const simd_float4 DEBUG_COLOR_YELLOW = {1.0f, 1.0f, 0.0f, 1.0f};
static const simd_float4 DEBUG_COLOR_CYAN = {0.0f, 1.0f, 1.0f, 1.0f};
static const simd_float4 DEBUG_COLOR_MAGENTA = {1.0f, 0.0f, 1.0f, 1.0f};
static const simd_float4 DEBUG_COLOR_WHITE = {1.0f, 1.0f, 1.0f, 1.0f};
static const simd_float4 DEBUG_COLOR_GRAY = {0.5f, 0.5f, 0.5f, 1.0f};
static const simd_float4 DEBUG_COLOR_ORANGE = {1.0f, 0.5f, 0.0f, 1.0f};

// Rendering to Encoder
void debug_render(debug_renderer_t* dbg, id encoder, simd_float4x4 view_proj, bool depth_test);

#ifdef __cplusplus
}
#endif

#endif // DEBUG_RENDERER_H
