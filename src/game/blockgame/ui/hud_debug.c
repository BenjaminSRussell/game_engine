#include "../include/ui/hud.h"

void hud_update_debug(HUDSystem *hud, f32 delta_time) {
  if (!hud)
    return;

  hud->fps_timer += delta_time;
  hud->frame_count++;

  if (hud->fps_timer >= 1.0f) {
    hud->last_fps = hud->frame_count;
    hud->frame_count = 0;
    hud->fps_timer = 0.0f;
  }
}

void hud_toggle_debug(HUDSystem *hud) {
  if (!hud)
    return;
  hud->elements_visible[HUD_ELEMENT_DEBUG_INFO] =
      !hud->elements_visible[HUD_ELEMENT_DEBUG_INFO];
}
