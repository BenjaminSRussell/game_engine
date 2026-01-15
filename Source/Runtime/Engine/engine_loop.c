#include "engine_loop.h"
#include "../../Core/Camera/camera_controller.h"
#include "../../Core/Config/config_presets.h"
#include "../../Core/Game/game_context.h"
#include "../../Core/Initialization/init_pipeline.h"
#include "../../Core/Platform/platform_bootstrap.h"
#include "../../Input/input_system.h"
#include "../../World/Generation/chunk_scheduler.h"
#include "../../World/Generation/spawn_manager.h"

// Forward declarations for systems not yet fully modularized but used
// These would eventually be headers
void audio_system_update(AudioSystem *sys, f32 dt);
void weather_system_update(WeatherSystem *sys, f32 dt);
void combat_system_update(CombatSystem *sys, f32 dt);
// ... others as needed, simplified for this extraction

// Render function wrapper
static void game_render_impl(void);

void engine_loop_init(void) {
  camera_controller_init();
  // derived from game_init logic not covered by init_pipeline if any
}

void engine_update(void) {
  // Calculate delta time
  f32 current_time = (f32)platform_get_time();
  g_game.delta_time =
      current_time - (f32)(g_game.last_frame_time / 1000.0); // Simplified
  // ideally use proper timer
  static double last_time = 0;
  if (last_time == 0)
    last_time = platform_get_time();
  double now = platform_get_time();
  g_game.delta_time = (f32)(now - last_time);
  last_time = now;

  // Platform events
  input_system_update(&g_game.input_state, g_game.window);

  // Camera
  camera_controller_update(g_game.delta_time);

  // Systems
  // (Here we would call all the systems extracted logic)
  // For now, in a real refactor, we would include headers for them.
  // I am using the new modules I created:

  chunk_scheduler_update(g_game.delta_time);
  async_spawn_update();
  spawn_marker_update(g_game.delta_time);

  // Trigger quit if needed
  if (input_is_key_pressed(&g_game.input_state, INPUT_KEY_ESCAPE)) {
    g_game.running = false;
  }
}

void engine_render(void) {
  // Call the monolithic render logic or refactored one
  // Since game_render was huge and monolithic, we ideally refactor it here
  // using the renderer API For this step, I'll assume we invoke the renderer
  // system

  // renderer_begin_frame(g_game.renderer);
  // ... render scene ...
  // renderer_end_frame(g_game.renderer);
}

void engine_shutdown(void) {
  // Calls shutdown on all systems
  // platform_shutdown(); provided by platform_bootstrap
  // ...
}

void engine_run(void) {
  g_game.running = true;

  while (g_game.running && !platform_window_should_close()) {
    engine_update();
    engine_render();
  }

  engine_shutdown();
}
