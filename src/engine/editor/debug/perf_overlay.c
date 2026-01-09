#include "debug_renderer.h"
#include "editor/debugging/perf_overlay.h"
#include <stdio.h>

static float g_frame_time = 0.0f;

void debug_perf_overlay_update(float delta_time) {
    g_frame_time = delta_time;
}

void debug_perf_overlay_draw(debug_renderer_t* dbg) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Frame: %.2f ms (%.1f FPS)", g_frame_time * 1000.0f, 1.0f / (g_frame_time + 0.0001f));
    debug_draw_text(dbg, 10.0f, 10.0f, buffer, simd_make_float4(1, 1, 0, 1));
}
