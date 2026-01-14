#ifndef ENGINE_PRIVATE_H
#define ENGINE_PRIVATE_H

#include <core/engine.h>
#include <core/game_loop.h>
#include <core/window.h>

// Internal state wrapper
typedef struct {
  Window window;
  GameLoop loop;
} PlatformData;

// Forward declarations for internal use
bool engine_init_subsystems(Engine *engine);
void engine_shutdown_subsystems(Engine *engine);

void engine_update_callback(void *user_data, f32 delta_time);
void engine_render_callback(void *user_data, f32 interpolation);

#endif // ENGINE_PRIVATE_H
