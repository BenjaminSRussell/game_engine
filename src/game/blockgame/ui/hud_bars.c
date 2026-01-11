#include "../../../engine/include/ecs/component_ids.h"
#include "../../../engine/include/ecs/components/health.h"
#include "../../../engine/include/ecs/ecs.h"
#include "../include/player/player.h"
#include "../include/ui/hud.h"
#include <math.h>

static void animate_bar(f32 *current, f32 target, f32 speed, f32 dt) {
  f32 diff = target - *current;
  if (fabsf(diff) > 0.01f) {
    *current += diff * speed * dt;
  } else {
    *current = target;
  }
}

void hud_update_bars(HUDSystem *hud, struct PlayerSystem *player,
                     f32 delta_time) {
  if (!player || !player->player)
    return;
  PlayerComponent *p = player->player;

  // Health (Fetched from ECS HealthComponent)
  if (player->ecs_world) {
    HealthComponent *hc = (HealthComponent *)ecs_get_component(
        player->ecs_world, (Entity){.id = p->entity_id, .generation = 0},
        HEALTH_COMPONENT_ID);
    if (hc) {
      hud->health_bar.target = hc->health;
      hud->health_bar.max = hc->max_health;
    }
  }
  animate_bar(&hud->health_bar.current, hud->health_bar.target,
              hud->health_bar.animation_speed, delta_time);

  // Hunger
  hud->hunger_bar.target = p->hunger;
  hud->hunger_bar.max = p->max_hunger;
  animate_bar(&hud->hunger_bar.current, hud->hunger_bar.target, 3.0f,
              delta_time);

  // Stamina
  hud->stamina_bar.target = p->stamina;
  hud->stamina_bar.max = p->max_stamina;
  animate_bar(&hud->stamina_bar.current, hud->stamina_bar.target, 6.0f,
              delta_time);

  // Oxygen
  hud->oxygen_bar.target = p->oxygen;
  hud->oxygen_bar.max = p->max_oxygen;
  hud->is_underwater = p->is_swimming;
  animate_bar(&hud->oxygen_bar.current, hud->oxygen_bar.target, 8.0f,
              delta_time);
}
