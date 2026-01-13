// src/engine/core/engine_integration.c
//
// Purpose: Integration layer that connects all engine implementations
// and provides a unified interface. This file bridges GameState, EngineCore,
// and Engine implementations.
//
#include <core/engine.h>
#include <core/engine_core.h>
#include <core/game_module.h>
#include <core/logger.h>
// core/engine_integration.c
// High-level Subsystem Integration and Lifecycle management.
//
// TODO: Implement a robust Scripting Bridge (Lua/Python) for game-logic
// extension.
// TODO: Add support for live-code hot-reloading (shared-library swapping).
// TODO: Implement a cross-subsystem messaging bus (Event Bus).
// TODO: Add support for distributed engine services (Headless-Server mode).
// TODO: Implement a global engine-profiler with visual-frame-pacing analysis.
// TODO: Add support for multi-viewport rendering (Split-screen / Editor-view).
// TODO: Implement a robust startup-dependency graph for subsystem
// initialization.
// TODO: Add support for engine-side crash-reporting and telemetry.
// TODO: Implement a unified asset-database for rapid cross-subsystem querying.
// TODO: Research and implement AI-driven performance optimization (Dynamic
// Budgeting).

#include <common.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations from gamestate_main.c
extern void game_init(void);
extern void game_update(void);
extern void game_render(void);
extern void game_shutdown(void);
extern bool g_game_running;

// Integration: Create Engine from GameState
Engine *engine_create_from_gamestate(void) {
  Engine *engine = (Engine *)calloc(1, sizeof(Engine));
  if (!engine) {
    LOG_ERROR("Failed to allocate engine");
    return NULL;
  }

  // Initialize with default config
  EngineConfig config = engine_create_default_config();
  if (!engine_init(engine, &config)) {
    LOG_ERROR("Failed to initialize engine from GameState");
    free(engine);
    return NULL;
  }

  return engine;
}

// Integration: Create EngineCore from Engine
bool engine_core_from_engine(EngineCore *core, Engine *engine) {
  if (!core || !engine) {
    return false;
  }

  // Map Engine subsystems to EngineCore
  core->ecs_world = *(World *)engine_get_entities(engine); // If available
  core->physics_world = engine_get_physics(engine);
  core->audio_system = engine_get_audio(engine);
  core->renderer = (IRenderer *)engine_get_renderer(engine);

  core->config.window_width = engine->config.window_width;
  core->config.window_height = engine->config.window_height;
  core->config.enable_physics = (engine->subsystems.physics != NULL);
  core->config.enable_audio = (engine->subsystems.audio != NULL);

  core->initialized = engine->state.initialized;
  core->running = engine->state.running;
  core->delta_time = engine->state.delta_time;

  return true;
}

// Integration: Run GameState as Engine module
static bool gamestate_module_initialize(GameModule *module, Engine *engine) {
  (void)module;
  // Initialize GameState systems
  game_init();
  return true;
}

static void gamestate_module_shutdown(GameModule *module) {
  (void)module;
  game_shutdown();
}

static void gamestate_module_update(GameModule *module, Engine *engine,
                                    f32 delta_time) {
  (void)module;
  (void)engine;
  (void)delta_time;
  game_update();
}

static void gamestate_module_render(GameModule *module, Engine *engine) {
  (void)module;
  (void)engine;
  game_render();
}

static void gamestate_module_handle_input(GameModule *module, Engine *engine) {
  (void)module;
  (void)engine;
  // Input handled in game_update
}

// Create GameModule for GameState
GameModule *engine_create_gamestate_module(void) {
  GameModule *module = (GameModule *)calloc(1, sizeof(GameModule));
  if (!module) {
    return NULL;
  }

  // Initialize module info
  module->info.name = "GameState";
  module->info.version = "1.0.0";
  module->info.author = "Engine Team";
  module->info.description = "Main game state module";
  module->info.state = GAME_MODULE_STATE_UNLOADED;

  // Set function pointers
  module->initialize = gamestate_module_initialize;
  module->shutdown = gamestate_module_shutdown;
  module->update = gamestate_module_update;
  module->render = gamestate_module_render;
  module->handle_input = gamestate_module_handle_input;

  return module;
}

// Unified engine entry point
int engine_unified_main(int argc, char *argv[]) {
  // Parse arguments
  EngineConfig config = engine_create_default_config();

  // Initialize engine
  Engine engine;
  if (!engine_init(&engine, &config)) {
    LOG_ERROR("Failed to initialize engine");
    return 1;
  }

  // Create GameState module
  GameModule *game_module = engine_create_gamestate_module();
  if (!game_module) {
    LOG_ERROR("Failed to create GameState module");
    engine_shutdown(&engine);
    return 1;
  }

  // Run engine with GameState module
  engine_run(&engine, game_module);

  // Cleanup
  free(game_module);
  engine_shutdown(&engine);

  return 0;
}
