#include "editor/debugging/visualization/debug_shapes.h"
#include <include/math/math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void debug_draw_sphere(debug_renderer_t* dbg, simd_float3 center, float radius, simd_float4 color) {
    // Draw 3 circles
    const int segments = 16;
    const float step = 2.0f * M_PI / segments;
    
    for (int i = 0; i < segments; i++) {
        float theta = i * step;
        float next_theta = (i + 1) * step;
        
        // Circle in XY
        simd_float3 p1_xy = center + simd_make_float3(cosf(theta) * radius, sinf(theta) * radius, 0.0f);
        simd_float3 p2_xy = center + simd_make_float3(cosf(next_theta) * radius, sinf(next_theta) * radius, 0.0f);
        debug_draw_line(dbg, p1_xy, p2_xy, color);
        
        // Circle in XZ
        simd_float3 p1_xz = center + simd_make_float3(cosf(theta) * radius, 0.0f, sinf(theta) * radius);
        simd_float3 p2_xz = center + simd_make_float3(cosf(next_theta) * radius, 0.0f, sinf(next_theta) * radius);
        debug_draw_line(dbg, p1_xz, p2_xz, color);
        
        // Circle in YZ
        simd_float3 p1_yz = center + simd_make_float3(0.0f, cosf(theta) * radius, sinf(theta) * radius);
        simd_float3 p2_yz = center + simd_make_float3(0.0f, cosf(next_theta) * radius, sinf(next_theta) * radius);
        debug_draw_line(dbg, p1_yz, p2_yz, color);
    }
}

void debug_draw_cone(debug_renderer_t* dbg, simd_float3 base, simd_float3 tip, float radius, simd_float4 color) {
    const int segments = 16;
    const float step = 2.0f * M_PI / segments;
    
    // Calculate basis vectors for the base circle
    simd_float3 axis = tip - base;
    float len = simd_length(axis);
    if (len < 0.0001f) return;
    
    simd_float3 up = simd_make_float3(0, 1, 0);
    if (fabsf(simd_dot(simd_normalize(axis), up)) > 0.99f) {
        up = simd_make_float3(1, 0, 0);
    }
    
    simd_float3 right = simd_normalize(simd_cross(axis, up));
    simd_float3 forward = simd_normalize(simd_cross(right, axis));
    
    simd_float3 prev_p = base + (right * cosf(0) + forward * sinf(0)) * radius;
    
    for (int i = 1; i <= segments; i++) {
        float theta = i * step;
        simd_float3 curr_p = base + (right * cosf(theta) + forward * sinf(theta)) * radius;
        
        // Draw base circle segment
        debug_draw_line(dbg, prev_p, curr_p, color);
        
        // Draw line to tip
        debug_draw_line(dbg, curr_p, tip, color);
        
        prev_p = curr_p;
    }
}

void debug_draw_frustum(debug_renderer_t* dbg, simd_float4x4 inv_view_proj, simd_float4 color) {
    // NDC corners
    simd_float4 corners[8] = {
        {-1, -1, 0, 1}, {1, -1, 0, 1}, {1, 1, 0, 1}, {-1, 1, 0, 1}, // Near
        {-1, -1, 1, 1}, {1, -1, 1, 1}, {1, 1, 1, 1}, {-1, 1, 1, 1}  // Far (Metal depth 0..1)
        // If OpenGL clip space (-1..1), change Z to -1 for near
    };
    
    simd_float3 world_corners[8];
    
    for (int i = 0; i < 8; i++) {
        simd_float4 world = simd_mul(inv_view_proj, corners[i]);
        world_corners[i] = simd_make_float3(world.x / world.w, world.y / world.w, world.z / world.w);
    }
    
    // Draw edges
    // Near plane
    for (int i = 0; i < 4; i++) debug_draw_line(dbg, world_corners[i], world_corners[(i+1)%4], color);
    // Far plane
    for (int i = 4; i < 8; i++) debug_draw_line(dbg, world_corners[i], world_corners[4 + (i+1)%4], color);
    // Connecting lines
    for (int i = 0; i < 4; i++) debug_draw_line(dbg, world_corners[i], world_corners[i+4], color);
}
