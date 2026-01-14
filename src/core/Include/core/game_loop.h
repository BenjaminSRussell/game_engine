// src/engine/core/game_loop.h
//
// Purpose: Fixed timestep game loop with variable rendering
//
#ifndef ENGINE_GAME_LOOP_H
#define ENGINE_GAME_LOOP_H

#include <common.h>

typedef struct GameLoop GameLoop;

typedef void (*GameLoopUpdateCallback)(void *user_data, f32 delta_time);
typedef void (*GameLoopRenderCallback)(void *user_data, f32 interpolation);

struct GameLoop {
  // Timing
  f64 current_time;
  f64 accumulator;
  f32 fixed_timestep; // Fixed update timestep (e.g., 1/60)
  f32 max_frame_time; // Maximum frame time to prevent spiral of death

  // Callbacks
  GameLoopUpdateCallback update_callback;
  GameLoopRenderCallback render_callback;
  void *user_data;

  // State
  bool running;
  bool paused;
  f32 time_scale;

  // Statistics
  u64 frame_count;
  f32 fps;
  f32 average_frame_time;
};

// Game loop lifecycle
void game_loop_init(GameLoop *loop, f32 fixed_timestep);
void game_loop_shutdown(GameLoop *loop);

// Game loop control
void game_loop_run(GameLoop *loop);
void game_loop_stop(GameLoop *loop);
void game_loop_pause(GameLoop *loop);
void game_loop_resume(GameLoop *loop);

// Callbacks
void game_loop_set_update_callback(GameLoop *loop,
                                   GameLoopUpdateCallback callback);
void game_loop_set_render_callback(GameLoop *loop,
                                   GameLoopRenderCallback callback);
void game_loop_set_user_data(GameLoop *loop, void *data);

// Time control
void game_loop_set_time_scale(GameLoop *loop, f32 scale);
f32 game_loop_get_time_scale(const GameLoop *loop);

// Statistics
u64 game_loop_get_frame_count(const GameLoop *loop);
f32 game_loop_get_fps(const GameLoop *loop);

#endif // ENGINE_GAME_LOOP_H
