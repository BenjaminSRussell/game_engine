#ifndef CONFIG_H
#define CONFIG_H

#include "engine/include/common.h"

// Unified game configuration structure
typedef struct GameConfig {
  int window_width;
  int window_height;
  int chunk_render_distance;
  bool vsync;
  bool fullscreen;
  float fov;
  float mouse_sensitivity;
} GameConfig;

#endif // CONFIG_H
