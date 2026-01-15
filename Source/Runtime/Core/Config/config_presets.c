#include "config_presets.h"
#include "../../Game/game_context.h"

void config_apply_preset(ConfigPreset preset) {
  switch (preset) {
  case CONFIG_PRESET_LOW:
    g_game.config.window_width = 1024;
    g_game.config.window_height = 576;
    g_game.config.render_distance = 4;
    g_game.config.max_chunks_loaded = 256;
    g_game.config.fancy_graphics = false;
    g_game.config.smooth_lighting = false;
    g_game.config.ambient_occlusion = false;
    g_game.config.shadows = false;
    g_game.config.ray_tracing = false;
    g_game.config.multithreading = true;
    break;
  case CONFIG_PRESET_MEDIUM:
    g_game.config.window_width = 1280;
    g_game.config.window_height = 720;
    g_game.config.render_distance = 8;
    g_game.config.max_chunks_loaded = 512;
    g_game.config.fancy_graphics = true;
    g_game.config.smooth_lighting = true;
    g_game.config.ambient_occlusion = false;
    g_game.config.shadows = false;
    g_game.config.ray_tracing = false;
    g_game.config.multithreading = true;
    break;
  case CONFIG_PRESET_HIGH:
    g_game.config.window_width = 1920;
    g_game.config.window_height = 1080;
    g_game.config.render_distance = 12;
    g_game.config.max_chunks_loaded = 1024;
    g_game.config.fancy_graphics = true;
    g_game.config.smooth_lighting = true;
    g_game.config.ambient_occlusion = true;
    g_game.config.shadows = true;
    g_game.config.ray_tracing = false;
    g_game.config.multithreading = true;
    break;
  case CONFIG_PRESET_ULTRA:
    g_game.config.window_width = 2560;
    g_game.config.window_height = 1440;
    g_game.config.render_distance = 16;
    g_game.config.max_chunks_loaded = 2048;
    g_game.config.fancy_graphics = true;
    g_game.config.smooth_lighting = true;
    g_game.config.ambient_occlusion = true;
    g_game.config.shadows = true;
    g_game.config.ray_tracing = true;
    g_game.config.multithreading = true;
    break;
  }
  // config_validate(&g_game.config); // Assuming declared elsewhere or included
}
