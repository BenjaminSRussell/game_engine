#include "editor/debug/perf_overlay.h"
#include "editor/debug/debug_renderer.h"
#include <stdio.h>

static float g_frame_time = 0.0f;

void debug_perf_overlay_update(float delta_time) { g_frame_time = delta_time; }

void debug_perf_overlay_draw(debug_renderer_t *dbg) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "Frame: %.2f ms (%.1f FPS)",
           g_frame_time * 1000.0f, 1.0f / (g_frame_time + 0.0001f));

  // Assuming debug_draw_text takes: dbg, x, y, text, color
  // If debug_draw_text is not 2D, this might fail.
  // For now keeping it simple.
  // Note: debug_renderer.c had debug_draw_text_3d.
  // If 2D text is not supported, I might need to use that or add 2D support.

  // Using a placeholder call for now to check if signature matches
  // debug_draw_text(dbg, 10.0f, 10.0f, buffer, simd_make_float4(1, 1, 0, 1));
}
