#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include "include/common.h"
#include <math/vec2.h>
#include <math/vec4.h>

// Forward declarations
struct VulkanRenderer;
typedef struct UIRenderer UIRenderer;

// UI Vertex structure
typedef struct {
  Vec2 position;
  Vec2 uv;
  Vec4 color;
} UIVertex;

// Initialize the UI renderer
void ui_renderer_init(UIRenderer **renderer,
                      struct VulkanRenderer *vk_renderer);

// Cleanup the UI renderer
void ui_renderer_cleanup(UIRenderer *renderer);

// Begin a new UI frame
void ui_renderer_begin_frame(UIRenderer *renderer);

// Draw a textured quad
// position: Screen coordinates (pixels)
// size: Size in pixels
// uv_start: Top-left UV coordinate
// uv_end: Bottom-right UV coordinate
// color: Tint color
void ui_renderer_draw_quad(UIRenderer *renderer, Vec2 position, Vec2 size,
                           Vec2 uv_start, Vec2 uv_end, Vec4 color);

// End the UI frame and submit draw commands
void ui_renderer_end_frame(UIRenderer *renderer);

#endif // UI_RENDERER_H
