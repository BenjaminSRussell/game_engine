#ifndef EDITOR_DEBUG_RENDERER_H
#define EDITOR_DEBUG_RENDERER_H

#include <simd/simd.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

typedef struct debug_renderer debug_renderer_t;

// Lifecycle
debug_renderer_t* debug_renderer_create(id device, uint32_t max_lines);
void debug_renderer_destroy(debug_renderer_t* dbg);

// Drawing
void debug_draw_line(debug_renderer_t* dbg, simd_float3 start, simd_float3 end, simd_float4 color);
void debug_draw_box(debug_renderer_t* dbg, simd_float3 min, simd_float3 max, simd_float4 color);
void debug_draw_sphere(debug_renderer_t* dbg, simd_float3 center, float radius, simd_float4 color);
void debug_draw_text_3d(debug_renderer_t* dbg, simd_float3 position, const char* text, simd_float4 color);

// Rendering to Encoder
void debug_render(id encoder, debug_renderer_t* dbg, const simd_float4x4* view_proj);
void debug_clear(debug_renderer_t* dbg);

#endif // EDITOR_DEBUG_RENDERER_H
