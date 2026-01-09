#include "debug_renderer.h"
#include <stdlib.h>
#include <stdio.h>

// Stubs for debug renderer (C-compatible)

debug_renderer_t* debug_renderer_create(id device, uint32_t max_lines) {
    debug_renderer_t* dbg = (debug_renderer_t*)calloc(1, sizeof(debug_renderer_t));
    if (dbg) {
        dbg->max_lines = max_lines;
        dbg->line_count = 0;
    }
    return dbg;
}

void debug_renderer_destroy(debug_renderer_t* dbg) {
    if (dbg) free(dbg);
}

void debug_draw_line(debug_renderer_t* dbg, simd_float3 start, simd_float3 end, simd_float4 color) {
    // No-op stub
}

void debug_draw_aabb(debug_renderer_t* dbg, simd_float3 min, simd_float3 max, simd_float4 color) {
    // No-op stub
}

void debug_draw_sphere(debug_renderer_t* dbg, simd_float3 center, float radius, simd_float4 color) {
    // No-op stub
}

void debug_draw_frustum(debug_renderer_t* dbg, simd_float4x4 inv_view_proj, simd_float4 color) {
    // No-op stub
}

void debug_draw_grid(debug_renderer_t* dbg, simd_float3 center, float size, int divisions, simd_float4 color) {
    // No-op stub
}

void debug_draw_text_3d(debug_renderer_t* dbg, simd_float3 position, const char* text, simd_float4 color) {
    // No-op stub
}

void debug_draw_text(debug_renderer_t* dbg, float x, float y, const char* text, simd_float4 color) {
    // No-op stub
}

void debug_clear(debug_renderer_t* dbg) {
    if (dbg) dbg->line_count = 0;
}

void debug_render(debug_renderer_t* dbg, id encoder, simd_float4x4 view_proj, bool depth_test) {
    // No-op stub
}
