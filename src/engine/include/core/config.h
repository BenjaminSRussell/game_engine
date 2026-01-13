#ifndef CORE_CONFIG_H
#define CORE_CONFIG_H

#include "engine/include/common.h"

// Configuration settings for the game
typedef struct GameConfig {
  // Rendering
  u32 window_width;
  u32 window_height;
  bool vsync;
  u32 max_fps;
  f32 fov;
  bool fullscreen;

  // Graphics
  u32 render_distance;
  bool fancy_graphics;
  bool smooth_lighting;
  bool ambient_occlusion;
  bool shadows;
  bool ray_tracing;

  // Performance
  u32 max_chunks_loaded;
  u32 chunk_generation_threads;
  u32 mesh_generation_threads;
  bool multithreading;

  // World
  u32 world_seed;
  bool generate_structures;
  bool generate_caves;
  bool generate_ores;

  // Audio
  f32 master_volume;
  f32 music_volume;
  f32 sfx_volume;

  // Controls
  f32 mouse_sensitivity;
  f32 mouse_smoothing;
  bool invert_mouse_y;
  bool toggle_sprint;
  bool toggle_crouch;
  f32 movement_speed;
  f32 sprint_multiplier;
  f32 crouch_multiplier;
  f32 jump_force;
  f32 fly_speed;
  f32 air_control;
  f32 gravity;
  f32 ground_check_distance;
  f32 reach_distance_survival;
  f32 reach_distance_creative;
  f32 controller_deadzone;
  f32 controller_sensitivity;
  f32 controller_vibration;
} GameConfig;

// Configuration functions
void config_set_defaults(GameConfig *config);
void config_validate(GameConfig *config);
void config_load(GameConfig *config, const char *filename);
void config_save(const GameConfig *config, const char *filename);
bool config_reload_if_modified(GameConfig *config, const char *filename,
                               u64 *last_modified);
void config_merge(GameConfig *base, const GameConfig *overrides);

#endif // CORE_CONFIG_H
