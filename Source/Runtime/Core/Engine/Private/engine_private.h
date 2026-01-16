#ifndef ENGINE_PRIVATE_H
#define ENGINE_PRIVATE_H

#include "../Public/engine.h"
#include <core/game_loop.h>
#include <core/window.h>

// Internal state wrapper
typedef struct {
  Window window;
  GameLoop loop;
} PlatformData;

// Forward declarations for internal use
// Phased initialization
bool engine_init_core_systems(Engine *engine);
bool engine_init_engine_systems(Engine *engine);
bool engine_init_game_systems(Engine *engine);

// Phased shutdown
void engine_shutdown_core_systems(Engine *engine);
void engine_shutdown_engine_systems(Engine *engine);
void engine_shutdown_game_systems(Engine *engine);

void engine_update_callback(void *user_data, f32 delta_time);
void engine_render_callback(void *user_data, f32 interpolation);

#endif // ENGINE_PRIVATE_H
