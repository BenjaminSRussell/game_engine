// src/game/game_hooks.c
//
// Purpose: Implementation of mod API hooks for game-specific events.
// Provides registration and trigger mechanisms for game events.
//

#include <core/logger.h>
#include <game/game_hooks.h>
#include <modding/mod_api.h>
#include <stdlib.h>

static struct ModRegistry *s_game_mod_registry = NULL;

void game_hook_init(struct ModRegistry *registry) {
  if (!registry) {
    LOG_ERROR("Invalid mod registry for game hooks");
    return;
  }

  s_game_mod_registry = registry;
  LOG_INFO("Game hooks initialized with mod registry");
}

void game_hook_trigger_world_init(struct GameState *state,
                                  struct GameModeState *mode) {
  if (!s_game_mod_registry)
    return;

  GameInitEventData event = {
      .game_state = state,
      .mode_state = mode,
  };

  mod_registry_trigger_hook(s_game_mod_registry, MOD_HOOK_INIT, &event);
  LOG_DEBUG("Game world init hook triggered");
}

void game_hook_trigger_game_mode_changed(int old_mode, int new_mode,
                                         int difficulty) {
  if (!s_game_mod_registry)
    return;

  GameModeChangeEventData event = {
      .old_mode = old_mode,
      .new_mode = new_mode,
      .difficulty = difficulty,
  };

  // Create a custom hook type for this (would need to be in mod_api.h)
  mod_registry_trigger_hook(s_game_mod_registry, MOD_HOOK_UPDATE, &event);
  LOG_DEBUG("Game mode changed hook triggered: %d -> %d", old_mode, new_mode);
}

void game_hook_trigger_world_tick(float dt, uint64_t tick_count) {
  if (!s_game_mod_registry)
    return;

  GameWorldTickEventData event = {
      .delta_time = dt,
      .tick_count = tick_count,
  };

  mod_registry_trigger_hook(s_game_mod_registry, MOD_HOOK_WORLD_TICK, &event);
}

void game_hook_trigger_player_action(int action_type, float x, float y,
                                     float z) {
  if (!s_game_mod_registry)
    return;

  PlayerActionEventData event = {
      .action_type = action_type,
      .x = x,
      .y = y,
      .z = z,
  };

  // Block placed/broken hooks
  if (action_type == 0) {
    mod_registry_trigger_hook(s_game_mod_registry, MOD_HOOK_BLOCK_PLACED,
                              &event);
  } else if (action_type == 1) {
    mod_registry_trigger_hook(s_game_mod_registry, MOD_HOOK_BLOCK_BROKEN,
                              &event);
  }
}

void game_hook_trigger_world_loaded(const char *world_name) {
  if (!s_game_mod_registry || !world_name)
    return;

  mod_registry_trigger_hook(s_game_mod_registry, MOD_HOOK_CHUNK_LOADED,
                            (void *)world_name);
  LOG_DEBUG("World loaded hook triggered: %s", world_name);
}

void game_hook_trigger_world_saved(const char *world_name) {
  if (!s_game_mod_registry || !world_name)
    return;

  mod_registry_trigger_hook(s_game_mod_registry, MOD_HOOK_CHUNK_UNLOADED,
                            (void *)world_name);
  LOG_DEBUG("World saved hook triggered: %s", world_name);
}
