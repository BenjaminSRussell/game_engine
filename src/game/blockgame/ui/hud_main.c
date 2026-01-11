#include "../include/player/player.h"
#include "../include/ui/hud.h"
#include "../include/ui/hud_spell_cooldown.h"
#include <stdio.h>
#include <string.h>

void hud_init(HUDSystem *hud, Vec2 screen_size) {
  if (!hud)
    return;
  memset(hud, 0, sizeof(HUDSystem));
  hud->screen_size = screen_size;

  // Default configs
  hud->health_bar.max = 20.0f;
  hud->health_bar.current = 20.0f;
  hud->health_bar.animation_speed = 5.0f;

  hud->hunger_bar.max = 20.0f;
  hud->hunger_bar.current = 20.0f;

  hud->stamina_bar.max = 100.0f;
  hud->stamina_bar.current = 100.0f;

  hud->hotbar.slot_count = 9;

  hud->crosshair.style = CROSSHAIR_STYLE_DEFAULT;
  hud->crosshair.size = 16.0f;
  hud->crosshair.color = (Vec4){1, 1, 1, 1}; // Fix for union/array alignment

  for (int i = 0; i < HUD_ELEMENT_COUNT; i++) {
    hud->elements_visible[i] = true;
  }
}

void hud_update(HUDSystem *hud, Player *player, f32 delta_time) {
  if (!hud || !player)
    return;

  void hud_update_bars(HUDSystem * hud, Player * player,
                       f32 delta_time);
  void hud_update_overlay(HUDSystem * hud, Player * player,
                          f32 delta_time);
  void hud_update_logging(HUDSystem * hud, f32 delta_time);
  void hud_update_debug(HUDSystem * hud, f32 delta_time);

  hud_update_bars(hud, player, delta_time);
  hud_update_overlay(hud, player, delta_time);
  hud_update_logging(hud, delta_time);
  hud_update_debug(hud, delta_time);
  hud_update_spell_cooldowns(hud, player, delta_time);
}

void hud_resize(HUDSystem *hud, Vec2 new_size) {
  if (!hud)
    return;
  hud->screen_size = new_size;
}

void hud_set_element_visible(HUDSystem *hud, HUDElementType type,
                             bool visible) {
  if (!hud || type >= HUD_ELEMENT_COUNT)
    return;
  hud->elements_visible[type] = visible;
}
