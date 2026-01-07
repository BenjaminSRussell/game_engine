#ifndef DEBUG_SHAPES_H
#define DEBUG_SHAPES_H

#include "debug_utils.h"

void debug_draw_sphere(debug_renderer_t* dbg, simd_float3 center, float radius, simd_float4 color);
void debug_draw_cone(debug_renderer_t* dbg, simd_float3 base, simd_float3 tip, float radius, simd_float4 color);
void debug_draw_frustum(debug_renderer_t* dbg, simd_float4x4 inv_view_proj, simd_float4 color);

#endif // DEBUG_SHAPES_H
