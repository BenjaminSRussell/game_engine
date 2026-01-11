#ifndef HUD_SPELL_COOLDOWN_H
#define HUD_SPELL_COOLDOWN_H

#include <ui/hud.h>
#include <player/player.h>

void hud_update_spell_cooldowns(HUDSystem* hud, Player* player, f32 delta_time);
void hud_render_spell_cooldowns(HUDSystem* hud, Player* player, void* renderer);

#endif // HUD_SPELL_COOLDOWN_H
