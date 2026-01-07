#include "debug_renderer.h"

typedef struct {
    simd_float3 min;
    simd_float3 max;
} aabb_t;

void bounds_render_aabb(debug_renderer_t* dbg, aabb_t bounds, simd_float4 color) {
    debug_draw_box(dbg, bounds.min, bounds.max, color);
}

void bounds_render_obb(debug_renderer_t* dbg, simd_float4x4 transform, aabb_t bounds, simd_float4 color) {
    // Transform 8 corners then draw lines
    simd_float3 corners[8];
    simd_float3 min = bounds.min;
    simd_float3 max = bounds.max;
    
    simd_float3 raw_corners[8] = {
        {min.x, min.y, min.z}, {max.x, min.y, min.z},
        {max.x, max.y, min.z}, {min.x, max.y, min.z},
        {min.x, min.y, max.z}, {max.x, min.y, max.z},
        {max.x, max.y, max.z}, {min.x, max.y, max.z}
    };
    
    for(int i=0; i<8; i++) {
        simd_float4 p = simd_mul(transform, simd_make_float4(raw_corners[i].x, raw_corners[i].y, raw_corners[i].z, 1.0f));
        corners[i] = p.xyz;
    }
    
    // Connect corners
    // Bottom
    debug_draw_line(dbg, corners[0], corners[1], color);
    debug_draw_line(dbg, corners[1], corners[2], color);
    debug_draw_line(dbg, corners[2], corners[3], color);
    debug_draw_line(dbg, corners[3], corners[0], color);
    // Top
    debug_draw_line(dbg, corners[4], corners[5], color);
    debug_draw_line(dbg, corners[5], corners[6], color);
    debug_draw_line(dbg, corners[6], corners[7], color);
    debug_draw_line(dbg, corners[7], corners[4], color);
    // Sides
    debug_draw_line(dbg, corners[0], corners[4], color);
    debug_draw_line(dbg, corners[1], corners[5], color);
    debug_draw_line(dbg, corners[2], corners[6], color);
    debug_draw_line(dbg, corners[3], corners[7], color);
}
