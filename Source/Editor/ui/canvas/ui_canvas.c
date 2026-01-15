/*
 * ui_canvas.c
 * UI Canvas System
 * Manages UI elements and batch rendering
 */

#include "editor/ui/canvas/ui_canvas.h"
#include "editor/ui/widgets/ui_rect.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>

typedef struct ui_rendering_ui_canvas_context {
  // List of active rects managed by this canvas
  // In a full implementation, we'd have a hierarchical tree or graph
  ui_rendering_ui_rect_handle_t *rect_handles;
  uint32_t rect_capacity;
  uint32_t rect_count;

  bool initialized;
} ui_rendering_ui_canvas_context_t;

static ui_rendering_ui_canvas_context_t g_ui_canvas = {0};

int ui_rendering_ui_canvas_init(void) {
  if (g_ui_canvas.initialized)
    return 0;

  if (ui_rendering_ui_rect_init() != 0) {
    LOG_ERROR("Failed to init UI Rect system dependency");
    return -1;
  }

  g_ui_canvas.rect_capacity = 1024;
  g_ui_canvas.rect_handles = (ui_rendering_ui_rect_handle_t *)calloc(
      g_ui_canvas.rect_capacity, sizeof(ui_rendering_ui_rect_handle_t));
  g_ui_canvas.rect_count = 0;
  g_ui_canvas.initialized = true;

  LOG_INFO("UI Canvas initialized");
  return 0;
}

void ui_rendering_ui_canvas_shutdown(void) {
  if (g_ui_canvas.rect_handles) {
    free(g_ui_canvas.rect_handles);
    g_ui_canvas.rect_handles = NULL;
  }
  ui_rendering_ui_rect_shutdown();
  g_ui_canvas.initialized = false;
}

// Draw a filled rectangle
void ui_canvas_draw_rect(float x, float y, float w, float h, float r, float g,
                         float b, float a) {
  if (!g_ui_canvas.initialized)
    return;

  if (g_ui_canvas.rect_count >= g_ui_canvas.rect_capacity) {
    // Simple cap for now
    return;
  }

  // Allocate a new rect from the pool for this frame (immediate mode style)
  // For retained mode, we would track IDs. Here we just create new ones for
  // simplicity of the prompt task But since `ui_rect` is retained, we'll try to
  // reuse or just create persistent ones for the editor. NOTE: For a "Dragon
  // Drop" editor, retained mode is better. Let's assume this function creates a
  // NEW persistent rect for now and returns a handle implicitly managed. Wait,
  // the signature I gave doesn't return a handle. This suggests immediate mode
  // usage.

  // We'll implement a simple immediate-mode-over-retained-layer adapter here
  // But wait, the previous `ui_rect.c` was purely retained.
  // Let's create a temporary rect handle, update it, and store it.

  ui_rendering_ui_rect_desc_t desc = {.x = x,
                                      .y = y,
                                      .width = w,
                                      .height = h,
                                      .color = {r, g, b, a},
                                      .flags = 0};

  ui_rendering_ui_rect_handle_t handle;
  if (ui_rendering_ui_rect_create(&handle, &desc) == 0) {
    g_ui_canvas.rect_handles[g_ui_canvas.rect_count++] = handle;
  }
}

// Clear the canvas (reset for next frame)
void ui_canvas_begin(void) {
  // Reset counters for immediate mode style usage
  // We destroy all previous rects to simulate immediate mode clear
  for (uint32_t i = 0; i < g_ui_canvas.rect_count; i++) {
    ui_rendering_ui_rect_destroy(g_ui_canvas.rect_handles[i]);
  }
  g_ui_canvas.rect_count = 0;
}

void ui_canvas_end(void) {
  // Submit draw calls to renderer
  // This would iterate over `rect_handles`, get render data, and submit to
  // `voxel_renderer` or similar. Since `voxel_renderer` has
  // `voxel_renderer_render_sprite` (or similar UI quad support from my plan),
  // we connect here.

  // For now, we just leave the data in `ui_rect` system ready for a render pass
  // to read.
}

uint32_t ui_rendering_ui_canvas_get_count(void) {
  return g_ui_canvas.rect_count;
}
