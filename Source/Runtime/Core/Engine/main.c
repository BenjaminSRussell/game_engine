// Source/Runtime/Core/Engine/main.c
#include "Private/crash_handler.h"
#include "Private/module_discovery.h"
#include <app_config.h>
#include <core/engine.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tools/profiler.h>
#include <unified_logger.h>
#include <unified_memory.h>

int main(int argc, char *argv[]) {
  // 1. Initialize Crash Handler
  crash_handler_init();

  // 2. Load Configuration
  AppConfig config = parse_arguments(argc, argv);
  load_config_file(&config);
  config = parse_arguments(argc, argv); // Command line overrides config file

  // 3. Initialize Logging & Profiling
  printf("Game Engine v1.0.0\n");
  profiler_init();
  LOG_INFO(LOG_CAT_GENERAL, "Engine Starting...");
  LOG_INFO(LOG_CAT_GENERAL, "  Config: %s", config.config_file);
  LOG_INFO(LOG_CAT_GENERAL, "  Module: %s", config.game_module_path);

  // 4. Initialize Engine
  Engine engine = {0};
  if (!engine_init(&engine, &config.engine_config)) {
    handle_engine_error(ENGINE_ERROR_INIT_FAILED);
  }

  // 5. Module Discovery
  ModuleRegistry *registry = module_registry_create();
  if (registry) {
    discover_modules(registry);
    print_available_modules(registry);
  }

  // 6. Load Game Module
  DynamicModule *dynamic_module = NULL;
  GameModule *game_module = NULL;

  // Try to find/load module (simplified logic from original main.c)
  DiscoveredModuleInfo *info = NULL;
  if (registry)
    info = find_module_by_path(registry, config.game_module_path);
  if (!info && registry)
    info = find_module_by_name(registry, config.game_module_path);

  if (info) {
    LOG_INFO(LOG_CAT_GENERAL, "Loading module: %s", info->name);
    dynamic_module = dynamic_module_load(info->path);
    if (dynamic_module) {
      if (dynamic_module_initialize(dynamic_module, &engine)) {
        game_module = dynamic_module->module;
      } else {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to init module");
      }
    }
  } else {
    LOG_WARN(LOG_CAT_GENERAL, "Module not found: %s", config.game_module_path);
  }

  // 7. Run Engine
  if (engine.state.initialized) {
    engine_run(&engine, game_module);
  }

  // 8. Shutdown
  if (dynamic_module) {
    dynamic_module_unload(dynamic_module);
  }
  if (registry) {
    module_registry_destroy(registry);
  }

  engine_shutdown(&engine);

  return 0;
}
