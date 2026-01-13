#include "engine/include/core/logger.h"
#include <rendering/ui_renderer.h>
#include <ui/hud_renderer.h>

static UIRenderer *g_ui_renderer = NULL;
static Vec2 g_screen_size = {0};

void hud_renderer_init(Vec2 screen_size) {
  g_screen_size = screen_size;
  LOG_INFO("HUD renderer initialized for screen size: %.0fx%.0f", screen_size.x,
           screen_size.y);
}

void hud_renderer_free(void) {
  if (g_ui_renderer) {
    ui_renderer_cleanup(g_ui_renderer);
    g_ui_renderer = NULL;
  }
}

void hud_render(HUDSystem *hud, struct VulkanRenderer *renderer) {
  hud_render_impl(hud, renderer);
}

void hud_render_impl(HUDSystem *hud, struct VulkanRenderer *vulkan_renderer) {
  if (!hud || !vulkan_renderer)
    return;

  if (!g_ui_renderer) {
    ui_renderer_init(&g_ui_renderer, vulkan_renderer);
  }

  ui_renderer_begin_frame(g_ui_renderer);

  // Health Bar
  if (hud->health_bar.current > 0.0f &&
      hud->elements_visible[HUD_ELEMENT_HEALTH_BAR]) {
    Vec2 pos = {20.0f, g_screen_size.y - 60.0f};
    Vec2 size = {200.0f, 20.0f};
    ui_renderer_draw_quad(g_ui_renderer, pos, size, (Vec2){0, 0}, (Vec2){1, 1},
                          (Vec4){0.2f, 0.2f, 0.2f, 0.8f});
    Vec2 fill = {size.x * (hud->health_bar.current / hud->health_bar.max),
                 size.y};
    ui_renderer_draw_quad(g_ui_renderer, pos, fill, (Vec2){0, 0}, (Vec2){1, 1},
                          (Vec4){0.8f, 0.2f, 0.2f, 1.0f});
  }

  // Hunger Bar
  if (hud->hunger_bar.current > 0.0f &&
      hud->elements_visible[HUD_ELEMENT_HUNGER_BAR]) {
    Vec2 pos = {20.0f, g_screen_size.y - 35.0f};
    Vec2 size = {200.0f, 15.0f};
    ui_renderer_draw_quad(g_ui_renderer, pos, size, (Vec2){0, 0}, (Vec2){1, 1},
                          (Vec4){0.2f, 0.2f, 0.2f, 0.8f});
    Vec2 fill = {size.x * (hud->hunger_bar.current / hud->hunger_bar.max),
                 size.y};
    ui_renderer_draw_quad(g_ui_renderer, pos, fill, (Vec2){0, 0}, (Vec2){1, 1},
                          (Vec4){0.8f, 0.6f, 0.2f, 1.0f});
  }

  // Crosshair
  if (hud->elements_visible[HUD_ELEMENT_CROSSHAIR]) {
    Vec2 ch_center = {g_screen_size.x / 2.0f, g_screen_size.y / 2.0f};
    Vec4 color = hud->crosshair.color;
    if (hud->crosshair.hit_time > 0.0f)
      color = (Vec4){1, 0, 0, 1}; // Red on hit

    ui_renderer_draw_quad(g_ui_renderer,
                          (Vec2){ch_center.x - 1, ch_center.y - 7},
                          (Vec2){2, 14}, (Vec2){0, 0}, (Vec2){1, 1}, color);
    ui_renderer_draw_quad(g_ui_renderer,
                          (Vec2){ch_center.x - 7, ch_center.y - 1},
                          (Vec2){14, 2}, (Vec2){0, 0}, (Vec2){1, 1}, color);
  }

  ui_renderer_end_frame(g_ui_renderer);
}
