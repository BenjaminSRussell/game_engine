// Survival system updates (health/hunger/air).
// Roadmap: docs/SURVIVAL_WEATHER_ROADMAP.md.
#include "engine/include/core/logger.h"
#include <game/survival_system.h>
#include <player/status_effects.h>
#include <stdlib.h>
#include <string.h>
#include <weather/weather.h>

static void survival_update_difficulty_scale(SurvivalManager *mgr) {
  if (!mgr)
    return;

  f32 days = (f32)mgr->day_count;
  f32 hours = (f32)mgr->settings.play_time_seconds / 3600.0f;
  f32 scale = 1.0f + days * 0.02f + hours * 0.03f;

  if (mgr->settings.challenge_mode) {
    scale *= 1.2f;
  }

  mgr->difficulty_scale = CLAMP(scale, 1.0f, 2.0f);

  mgr->settings.damage_scale = mgr->base_damage_scale * mgr->difficulty_scale;
  mgr->settings.hunger_drain_rate =
      mgr->base_hunger_drain_rate * mgr->difficulty_scale;
  mgr->settings.mob_spawn_rate =
      mgr->base_mob_spawn_rate * (0.8f + mgr->difficulty_scale * 0.2f);

  mgr->settings.scarcity_factor =
      CLAMP(0.9f + mgr->difficulty_scale * 0.15f, 0.9f, 1.5f);
}

static void survival_apply_respawn_penalty(SurvivalManager *mgr,
                                           PlayerComponent *player) {
  if (!mgr || !player)
    return;

  player->hunger = player->max_hunger * 0.5f;
  player->saturation = 0.0f;
  player->oxygen = player->max_oxygen;

  status_effect_add(&player->status_effects, STATUS_EFFECT_WEAKNESS, 10.0f,
                    1.0f);
  status_effect_add(&player->status_effects, STATUS_EFFECT_SLOWNESS, 6.0f,
                    1.0f);
}

static void survival_update_temperature(SurvivalManager *mgr, f32 delta_time) {
  if (!mgr || !mgr->player_system || !mgr->player_system->player)
    return;

  PlayerComponent *player = mgr->player_system->player;
  extern WeatherSystem *g_weather_system;

  f32 weather_temp = 20.0f;
  if (g_weather_system && g_weather_system->initialized) {
    weather_temp = weather_get_temperature(g_weather_system);
  }

  f32 day_factor = survival_is_day(mgr) ? 1.0f : -1.0f;
  f32 target_temp = weather_temp + day_factor * 2.0f;

  mgr->temperature +=
      (target_temp - mgr->temperature) * fminf(1.0f, delta_time);

  if (mgr->temperature < 0.0f) {
    status_effect_add(&player->status_effects, STATUS_EFFECT_SLOWNESS, 2.0f,
                      1.0f);
    player->hunger = MAX(0.0f, player->hunger - 0.2f * delta_time);
    if (!mgr->tutorial.temperature_tip_shown) {
      LOG_INFO("Survival tip: Cold weather drains hunger faster.");
      mgr->tutorial.temperature_tip_shown = true;
    }
  } else if (mgr->temperature > 35.0f) {
    player->hunger = MAX(0.0f, player->hunger - 0.3f * delta_time);
    if (!mgr->tutorial.temperature_tip_shown) {
      LOG_INFO("Survival tip: Heat increases thirst and hunger drain.");
      mgr->tutorial.temperature_tip_shown = true;
    }
  }
}

static void survival_update_weather_impact(SurvivalManager *mgr,
                                           f32 delta_time) {
  if (!mgr || !mgr->player_system || !mgr->player_system->player)
    return;

  extern WeatherSystem *g_weather_system;
  if (!g_weather_system || !g_weather_system->initialized)
    return;

  WeatherType weather = weather_get_current_type(g_weather_system);
  PlayerComponent *player = mgr->player_system->player;

  if (weather_type_has_precipitation(weather)) {
    player->hunger = MAX(0.0f, player->hunger - 0.1f * delta_time);
    if (!mgr->tutorial.weather_tip_shown) {
      LOG_INFO("Survival tip: Bad weather slows you down and costs stamina.");
      mgr->tutorial.weather_tip_shown = true;
    }
  }
}

static void survival_check_milestones(SurvivalManager *mgr) {
  if (!mgr || !mgr->player_system || !mgr->player_system->player)
    return;

  static const f32 milestone_times[] = {60.0f, 300.0f, 900.0f, 1800.0f};
  const u32 milestone_count =
      sizeof(milestone_times) / sizeof(milestone_times[0]);

  if (mgr->stats.milestones_completed >= milestone_count)
    return;

  f32 elapsed = mgr->stats.time_survived_seconds;
  if (elapsed >= milestone_times[mgr->stats.milestones_completed]) {
    mgr->stats.milestones_completed++;
    PlayerComponent *player = mgr->player_system->player;
    player->hunger = MIN(player->max_hunger, player->hunger + 4.0f);
    player->saturation = MIN(player->hunger, player->saturation + 2.0f);
    LOG_INFO("Survival milestone reached! Restored hunger and saturation.");
  }
}

static void survival_update_infection(SurvivalManager *mgr, f32 delta_time) {
  if (!mgr || !mgr->player_system || !mgr->player_system->player)
    return;

  PlayerComponent *player = mgr->player_system->player;

  if (!mgr->infected) {
    if (player->hunger < 3.0f) {
      f32 roll = (f32)rand() / (f32)RAND_MAX;
      if (roll < 0.05f * delta_time) {
        mgr->infected = true;
        mgr->infection_timer = 30.0f;
        mgr->infection_severity = 1.0f;
        mgr->stats.disease_cases++;
        status_effect_add(&player->status_effects, STATUS_EFFECT_POISON, 5.0f,
                          1.0f);
        if (!mgr->tutorial.disease_tip_shown) {
          LOG_INFO("Survival tip: Low hunger can lead to infection.");
          mgr->tutorial.disease_tip_shown = true;
        }
      }
    }
    return;
  }

  mgr->infection_timer -= delta_time;
  if (mgr->infection_timer <= 0.0f) {
    mgr->infected = false;
    mgr->infection_timer = 0.0f;
    mgr->infection_severity = 0.0f;
    return;
  }

  if (status_effects_can_take_damage(&player->status_effects)) {
    f32 health = player_get_health(mgr->player_system);
    health -= (0.4f * mgr->infection_severity) * delta_time;
    if (health < 1.0f) {
      health = 1.0f;
    }
    player_set_health(mgr->player_system, health);
  }
}

static void survival_handle_respawn(SurvivalManager *mgr, f32 delta_time) {
  if (!mgr || !mgr->player_system || !mgr->player_system->player)
    return;

  PlayerComponent *player = mgr->player_system->player;
  f32 health = player_get_health(mgr->player_system);

  if (!mgr->respawn_pending && health <= 0.0f) {
    mgr->stats.deaths++;
    mgr->respawn_pending = true;
    mgr->respawn_timer = 3.0f;
    LOG_INFO("Player downed. Respawn in %.1fs", mgr->respawn_timer);
    if (!mgr->tutorial.respawn_tip_shown) {
      LOG_INFO("Survival tip: Dying reduces hunger and applies a penalty.");
      mgr->tutorial.respawn_tip_shown = true;
    }
    return;
  }

  if (mgr->respawn_pending) {
    mgr->respawn_timer -= delta_time;
    if (mgr->respawn_timer <= 0.0f) {
      mgr->respawn_pending = false;
      mgr->respawn_timer = 0.0f;
      player_set_health(mgr->player_system, 10.0f);
      survival_apply_respawn_penalty(mgr, player);
      mgr->stats.respawns++;
    }
  }
}

void survival_manager_init(SurvivalManager *mgr, PlayerSystem *player_system) {
  if (!mgr)
    return;

  mgr->player_system = player_system;
  mgr->settings.difficulty = DIFFICULTY_NORMAL;
  mgr->settings.damage_scale = 1.0f;
  mgr->settings.hunger_drain_rate = 0.1f;
  mgr->settings.mob_spawn_rate = 1.0f;
  mgr->settings.allow_pvp = true;
  mgr->settings.mob_griefing_enabled = true;
  mgr->settings.play_time_seconds = 0;
  mgr->settings.day_time = 6000;
  mgr->settings.challenge_mode = false;
  mgr->settings.scarcity_factor = 1.0f;

  mgr->day_night_cycle_time = 1200.0f;
  mgr->is_raining = false;
  mgr->hunger_timer = 0.0f;
  mgr->day_count = 0;
  mgr->difficulty_scale = 1.0f;
  mgr->base_damage_scale = 1.0f;
  mgr->base_hunger_drain_rate = 0.1f;
  mgr->base_mob_spawn_rate = 1.0f;
  mgr->last_health = 0.0f;
  mgr->respawn_pending = false;
  mgr->respawn_timer = 0.0f;
  mgr->temperature = 20.0f;
  mgr->infection_timer = 0.0f;
  mgr->infection_severity = 0.0f;
  mgr->infected = false;
  memset(&mgr->stats, 0, sizeof(mgr->stats));
  memset(&mgr->tutorial, 0, sizeof(mgr->tutorial));

  survival_apply_difficulty(mgr, mgr->settings.difficulty);
}

void survival_manager_free(SurvivalManager *mgr) {
  if (!mgr)
    return;
  memset(mgr, 0, sizeof(SurvivalManager));
}

void survival_manager_update(SurvivalManager *mgr, f32 delta_time) {
  if (!mgr || !mgr->player_system)
    return;

  PlayerComponent *player = mgr->player_system->player;
  if (!player) {
    return;
  }

  f32 health = player_get_health(mgr->player_system);
  if (mgr->last_health <= 0.0f) {
    mgr->last_health = health;
  }

  status_effects_update(&player->status_effects, delta_time);

  if (status_effect_has(&player->status_effects, STATUS_EFFECT_POISON)) {
    f32 strength = status_effect_get_strength(&player->status_effects,
                                              STATUS_EFFECT_POISON);
    if (status_effects_can_take_damage(&player->status_effects)) {
      health -= (2.0f * strength) * delta_time;
      if (health < 1.0f) {
        health = 1.0f;
      }
    }
  }

  if (status_effect_has(&player->status_effects, STATUS_EFFECT_REGENERATION)) {
    f32 strength = status_effect_get_strength(&player->status_effects,
                                              STATUS_EFFECT_REGENERATION);
    health += (0.4f * (1.0f + strength)) * delta_time;
  }

  mgr->settings.play_time_seconds += (u32)delta_time;
  mgr->stats.time_survived_seconds += delta_time;
  if (mgr->stats.time_survived_seconds > mgr->stats.longest_survival_seconds) {
    mgr->stats.longest_survival_seconds = mgr->stats.time_survived_seconds;
  }

  f32 drain = mgr->settings.hunger_drain_rate * mgr->settings.scarcity_factor;
  if (player->is_sprinting) {
    drain *= 1.5f;
  }
  if (player->is_swimming) {
    drain *= 1.2f;
  }
  player->hunger -= drain * delta_time;
  if (player->hunger < 0.0f) {
    player->hunger = 0.0f;
  }

  if (player->hunger <= 3.0f) {
    player->is_sprinting = false;
    if (!mgr->tutorial.hunger_tip_shown) {
      LOG_INFO("Survival tip: Low hunger disables sprinting.");
      mgr->tutorial.hunger_tip_shown = true;
    }
  }

  if (player->hunger >= 18.0f && player->saturation > 0.0f) {
    health += 0.5f * delta_time;
    player->saturation = MAX(0.0f, player->saturation - 0.25f * delta_time);
  }

  if (player->is_swimming) {
    player->oxygen -= 1.0f * delta_time;
    if (player->oxygen <= 0.0f) {
      player->oxygen = 0.0f;
      health -= 2.0f * delta_time;
    }
  } else {
    player->oxygen += 2.0f * delta_time;
    if (player->oxygen > player->max_oxygen) {
      player->oxygen = player->max_oxygen;
    }
  }

  if (health > 0.0f) {
    player_set_health(mgr->player_system, health);
  }

  mgr->settings.day_time += (u32)(delta_time * 20.0f);
  if (mgr->settings.day_time >= 24000) {
    mgr->settings.day_time = 0;
    mgr->day_count++;
    survival_update_difficulty_scale(mgr);
  }

  survival_update_weather_impact(mgr, delta_time);
  survival_update_temperature(mgr, delta_time);
  survival_update_infection(mgr, delta_time);
  survival_check_milestones(mgr);
  survival_handle_respawn(mgr, delta_time);

  if (mgr->settings.challenge_mode && !mgr->tutorial.challenge_tip_shown) {
    LOG_INFO("Challenge mode active: harsher penalties and faster scaling.");
    mgr->tutorial.challenge_tip_shown = true;
  }

  if (player_get_health(mgr->player_system) < 5.0f) {
    mgr->stats.low_health_warnings++;
  }

  mgr->last_health = player_get_health(mgr->player_system);
}

void survival_apply_difficulty(SurvivalManager *mgr,
                               Difficulty difficulty) {
  if (!mgr)
    return;

  mgr->settings.difficulty = difficulty;

  switch (difficulty) {
  case DIFFICULTY_PEACEFUL:
    mgr->base_damage_scale = 0.0f;
    mgr->base_hunger_drain_rate = 0.0f;
    mgr->base_mob_spawn_rate = 0.0f;
    break;
  case DIFFICULTY_EASY:
    mgr->base_damage_scale = 0.5f;
    mgr->base_hunger_drain_rate = 0.5f;
    mgr->base_mob_spawn_rate = 0.5f;
    break;
  case DIFFICULTY_NORMAL:
    mgr->base_damage_scale = 1.0f;
    mgr->base_hunger_drain_rate = 1.0f;
    mgr->base_mob_spawn_rate = 1.0f;
    break;
  case DIFFICULTY_HARD:
    mgr->base_damage_scale = 1.5f;
    mgr->base_hunger_drain_rate = 1.5f;
    mgr->base_mob_spawn_rate = 1.5f;
    break;
  case DIFFICULTY_EXTREME:
    mgr->base_damage_scale = 2.0f;
    mgr->base_hunger_drain_rate = 2.0f;
    mgr->base_mob_spawn_rate = 2.0f;
    break;
  case DIFFICULTY_COUNT:
    // Fallthrough or handle default
    break;
  }

  survival_update_difficulty_scale(mgr);
}

void survival_damage_player(SurvivalManager *mgr, f32 damage) {
  if (!mgr || !mgr->player_system || !mgr->player_system->player)
    return;

  f32 scaled_damage = damage * mgr->settings.damage_scale;
  f32 health = player_get_health(mgr->player_system);
  health -= scaled_damage;
  if (health < 0.0f) {
    health = 0.0f;
  }
  player_set_health(mgr->player_system, health);
}

void survival_hunger_deplete(SurvivalManager *mgr, f32 amount) {
  if (!mgr || !mgr->player_system || !mgr->player_system->player)
    return;

  mgr->player_system->player->hunger -= amount;
  if (mgr->player_system->player->hunger < 0.0f) {
    mgr->player_system->player->hunger = 0.0f;
  }
}

void survival_restore_hunger(SurvivalManager *mgr, f32 amount) {
  if (!mgr || !mgr->player_system || !mgr->player_system->player)
    return;

  mgr->player_system->player->hunger += amount;
  if (mgr->player_system->player->hunger >
      mgr->player_system->player->max_hunger) {
    mgr->player_system->player->hunger = mgr->player_system->player->max_hunger;
  }
}

u32 survival_get_day_time(SurvivalManager *mgr) {
  if (!mgr)
    return 0;
  return mgr->settings.day_time;
}

void survival_set_day_time(SurvivalManager *mgr, u32 time) {
  if (!mgr)
    return;
  mgr->settings.day_time = time % 24000;
}

bool survival_is_day(SurvivalManager *mgr) {
  if (!mgr)
    return true;
  return mgr->settings.day_time >= 0 && mgr->settings.day_time < 12000;
}

bool survival_is_night(SurvivalManager *mgr) {
  if (!mgr)
    return false;
  return mgr->settings.day_time >= 12000 && mgr->settings.day_time < 24000;
}
