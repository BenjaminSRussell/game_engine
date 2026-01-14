/**
 * @file game_loop.c
 * @brief Core Implementation
 * @description Core system implementation
 * @date 2026-01-13
 */

/*
 * game_loop.c
 * Fixed timestep game loop implementation
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 * 
 * Provides high-resolution timing and fixed timestep game loop functionality
 */

#include "engine/include/core/logger.h"
#include "engine/include/core/game_loop.h"
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

// High-resolution timer
static f64 get_time(void) {
#ifdef _WIN32
  static LARGE_INTEGER frequency = {0};
  if (frequency.QuadPart == 0) {
    QueryPerformanceFrequency(&frequency);
  }
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return (f64)counter.QuadPart / (f64)frequency.QuadPart;
#elif defined(__APPLE__)
  static mach_timebase_info_data_t timebase = {0};
  if (timebase.denom == 0) {
    kern_return_t kr = mach_timebase_info(&timebase);
    if (kr != KERN_SUCCESS) {
            return 0.0;
    }
  }
  if (timebase.denom == 0)
    return 0.0; // Prevent div by zero

  return (f64)mach_absolute_time() * (f64)timebase.numer / (f64)timebase.denom /
         1e9;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (f64)ts.tv_sec + (f64)ts.tv_nsec / 1e9;
#endif
}

// Game loop lifecycle
void game_loop_init(GameLoop *loop, f32 fixed_timestep) {
  if (!loop)
    return;

  memset(loop, 0, sizeof(GameLoop));
  loop->fixed_timestep = fixed_timestep;
  loop->max_frame_time = 0.25f; // 250ms max
  loop->time_scale = 1.0f;
  loop->running = false;
  loop->paused = false;

  LOG_INFO("Game loop initialized (fixed timestep: %.3f ms)",
           fixed_timestep * 1000.0f);
}

void game_loop_shutdown(GameLoop *loop) {
  if (!loop)
    return;

  loop->running = false;
  LOG_INFO("Game loop shutdown");
}

// Game loop control

void game_loop_run(GameLoop *loop) {
  if (!loop)
    return;

  loop->running = true;
  loop->current_time = get_time();
  loop->accumulator = 0.0;

  LOG_INFO("Starting game loop");

  f64 last_time = loop->current_time;
  f32 frame_time_accumulator = 0.0f;
  u32 fps_frame_count = 0;

  while (loop->running) {
    // Calculate frame time
    f64 new_time = get_time();
    f64 frame_time = new_time - last_time;
    last_time = new_time;

    // Clamp frame time to prevent spiral of death
    if (frame_time > loop->max_frame_time) {
      frame_time = loop->max_frame_time;
    }

    // Apply time scale
    frame_time *= loop->time_scale;

    // Update FPS counter
    frame_time_accumulator += (f32)frame_time;
    fps_frame_count++;
    if (frame_time_accumulator >= 1.0f) {
      loop->fps = (f32)fps_frame_count / frame_time_accumulator;
      loop->average_frame_time =
          frame_time_accumulator / (f32)fps_frame_count * 1000.0f;
      frame_time_accumulator = 0.0f;
      fps_frame_count = 0;
    }

    if (!loop->paused) {
      // Fixed timestep updates
      loop->accumulator += frame_time;

      // Update
      while (loop->accumulator >= loop->fixed_timestep) {
        // LOG_INFO("Triggering Update. Accum >= Fixed");

        if (loop->update_callback) {
          // LOG_INFO("Calling update callback");
          loop->update_callback(loop->user_data, (f32)loop->fixed_timestep);
        }

        loop->accumulator -= loop->fixed_timestep;
      }

      if (loop->render_callback) {
        f32 interpolation = (f32)(loop->accumulator / loop->fixed_timestep);
        // LOG_INFO("Calling render callback. Interp: %.2f", interpolation);
        loop->render_callback(loop->user_data, interpolation);
      }
    }
  }
}

void game_loop_stop(GameLoop *loop) { loop->running = false; }

void game_loop_pause(GameLoop *loop) {
  if (loop) {
    loop->paused = true;
    LOG_INFO("Game loop paused");
  }
}

void game_loop_resume(GameLoop *loop) {
  if (loop) {
    loop->paused = false;
    LOG_INFO("Game loop resumed");
  }
}

// Callbacks
void game_loop_set_update_callback(GameLoop *loop,
                                   GameLoopUpdateCallback callback) {
  if (loop) {
    loop->update_callback = callback;
  }
}

void game_loop_set_render_callback(GameLoop *loop,
                                   GameLoopRenderCallback callback) {
  if (loop) {
    loop->render_callback = callback;
  }
}

void game_loop_set_user_data(GameLoop *loop, void *data) {
  if (loop) {
    loop->user_data = data;
  }
}

// Time control
void game_loop_set_time_scale(GameLoop *loop, f32 scale) {
  if (loop) {
    loop->time_scale = scale;
  }
}

f32 game_loop_get_time_scale(const GameLoop *loop) {
  return loop ? loop->time_scale : 1.0f;
}

// Statistics
u64 game_loop_get_frame_count(const GameLoop *loop) {
  return loop ? loop->frame_count : 0;
}

f32 game_loop_get_fps(const GameLoop *loop) { return loop ? loop->fps : 0.0f; }
