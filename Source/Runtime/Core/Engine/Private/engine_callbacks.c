// Source/Runtime/Core/Engine/Private/engine_callbacks.c
#include "engine_private.h"
#include <ai/npc/perception_system.h>
#include <ai/npc_advanced/memory_system.h>
#include <core/game_loop.h>
#include <core/hot_reload.h>
#include <core/logger.h>
#include <core/window.h>
#include <ecs/ecs.h>
#include <rendering/renderer.h>

void engine_update_callback(void *user_data, f32 delta_time) {
  // Update Hot Reload
  hot_reload_update();

  Engine *engine = (Engine *)user_data;
  if (!engine) {
    LOG_FATAL("Engine is NULL in update!");
    return;
  }

  PlatformData *pdata = (PlatformData *)engine->platform_data;
  if (!pdata) {
    LOG_FATAL("PlatformData is NULL in update!");
    return;
  }

  // Poll window events
  window_poll_events();

  if (!engine->state.running) {
    game_loop_stop(&pdata->loop);
    return;
  }

  // Update internal state
  engine->state.delta_time = delta_time;
  engine->state.total_time += delta_time;
  engine->state.frame_count++;

  // Update Subsystems
  if (engine->subsystems.input) {
    engine->subsystems.input->update(engine->subsystems.input, delta_time);
  }

  if (engine->subsystems.entities) {
    World *world = (World *)engine->subsystems.entities;
    ecs_world_update(world, delta_time);
  }

  // Update AI Subsystems
  if (engine->subsystems.perception) {
    perception_system_process_frame(engine->subsystems.perception, delta_time);
  }

  if (engine->subsystems.memory) {
    memory_system_update(engine->subsystems.memory, delta_time);
  }

  // Game Module Input & Update
  if (engine->game_module) {
    if (engine->game_module->handle_input) {
      engine->game_module->handle_input(engine->game_module, engine);
    }
    if (engine->game_module->update) {
      engine->game_module->update(engine->game_module, engine, delta_time);
    }
  }
}

void engine_render_callback(void *user_data, f32 interpolation) {
  Engine *engine = (Engine *)user_data;
  PlatformData *pdata = (PlatformData *)engine->platform_data;
  (void)interpolation;

  if (engine->subsystems.renderer) {
    u32 image_index;
    if (engine->subsystems.renderer->begin_frame(engine->subsystems.renderer,
                                                 &image_index)) {
      // Game Module Render
      if (engine->game_module && engine->game_module->render) {
        engine->game_module->render(engine->game_module, engine);
      }
      engine->subsystems.renderer->end_frame(engine->subsystems.renderer,
                                             image_index);
    }
  }

  window_swap_buffers(&pdata->window);
}
