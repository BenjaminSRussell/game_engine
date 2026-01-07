#include "../include/player/player.h"
#include "../include/ui/hud.h"

void hud_update_overlay(HUDSystem *hud, struct PlayerSystem *player,
                        f32 delta_time) {
  if (!player->player)
    return;
  PlayerComponent *p = player->player;

  // Hotbar
  hud->hotbar.selected_slot = (u32)p->hotbar.config.selected_slot;

  // Crosshair hit timers
  if (hud->crosshair.hit_time > 0.0f) {
    hud->crosshair.hit_time -= delta_time;
  }
  if (hud->crosshair.crit_time > 0.0f) {
    hud->crosshair.crit_time -= delta_time;
  }
}

void hud_trigger_crosshair_hit(HUDSystem *hud, bool is_critical) {
  if (!hud)
    return;
  hud->crosshair.hit_time = 0.2f;
  if (is_critical) {
    hud->crosshair.crit_time = 0.4f;
  }
}
