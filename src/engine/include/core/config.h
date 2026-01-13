#ifndef CONFIG_H
#define CONFIG_H

#include "engine/include/common.h"

// Unified game configuration structure
typedef struct GameConfig {
  // Rendering
  int window_width;
  int window_height;
  bool vsync;
  int max_fps;
  float fov;
  bool fullscreen;

  // Graphics
  int render_distance;
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
  u64 world_seed;
  bool generate_structures;
  bool generate_caves;
  bool generate_ores;

  // Audio
  float master_volume;
  float music_volume;
  float sfx_volume;

  // Controls
  float mouse_sensitivity;
  float mouse_smoothing;
  bool invert_mouse_y;
  bool toggle_sprint;
  bool toggle_crouch;
  float movement_speed;
  float sprint_multiplier;
  float crouch_multiplier;
  float jump_force;
  float fly_speed;
  float air_control;
  float gravity;
  float ground_check_distance;
  float reach_distance_survival;
  float reach_distance_creative;
  float controller_deadzone;
  float controller_sensitivity;
  float controller_vibration;
} GameConfig;

#endif // CONFIG_H
