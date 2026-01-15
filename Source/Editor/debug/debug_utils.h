#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <simd/simd.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __OBJC__
#include <Metal/Metal.h>
#else
typedef void *id;
#endif

// Forward declare struct
typedef struct debug_renderer debug_renderer_t;

typedef struct debug_line {
  simd_float3 start;
  simd_float4 start_color;
  simd_float3 end;
  simd_float4 end_color;
} debug_line_t;

// API
debug_renderer_t *debug_renderer_create(id device, uint32_t max_lines,
                                        uint32_t color_format,
                                        uint32_t depth_format);
void debug_renderer_destroy(debug_renderer_t *dbg);

void debug_draw_line(debug_renderer_t *dbg, simd_float3 start, simd_float3 end,
                     simd_float4 color);
void debug_draw_aabb(debug_renderer_t *dbg, simd_float3 min, simd_float3 max,
                     simd_float4 color);
void debug_draw_text(debug_renderer_t *dbg, float x, float y, const char *text,
                     simd_float4 color);

void debug_render(debug_renderer_t *dbg, id encoder, simd_float4x4 view_proj,
                  bool depth_test);
void debug_clear(debug_renderer_t *dbg);

#endif // DEBUG_UTILS_H
