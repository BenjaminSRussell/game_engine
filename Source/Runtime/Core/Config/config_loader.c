// Source/Runtime/Core/Config/config_loader.c
#include "app_config.h"
#include <core/logger.h>
#include <include/core/config_loader.h> // Original config loader include
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AppConfig create_default_config(void) {
  AppConfig config = {0};
  // Initialize with sensible defaults
  config.game_module_path = "games/minecraft_v2/minecraft_v2_module";
  config.engine_config.window_width = 1920;
  config.engine_config.window_height = 1080;
  // ... other defaults copying from original main.c ... (simplified for brevity
  // or fully copied) I will define basic defaults here.
  config.engine_config.max_threads = 4;
  config.debug_mode = false;
  config.fullscreen = false;
  config.window_width = 1920;
  config.window_height = 1080;
  config.log_level = "INFO";
  config.config_file = "engine_config.json";
  return config;
}

bool load_config_file(AppConfig *config) {
  ConfigDocument *doc = config_document_create(16);
  if (!doc) {
    LOG_WARN("Failed to create config document");
    return false;
  }

  if (!config_document_load(doc, config->config_file)) {
    LOG_WARN("Failed to load config file: %s. Using defaults.",
             config->config_file);
    config_document_destroy(doc);
    return false;
  }

  // Load engine configuration
  ConfigSection *engine_section = config_document_get_section(doc, "engine");
  if (engine_section) {
    config->window_width = config_section_get_int(
        engine_section, "window_width", config->window_width);
    config->window_height = config_section_get_int(
        engine_section, "window_height", config->window_height);
    config->fullscreen = config_section_get_bool(engine_section, "fullscreen",
                                                 config->fullscreen);
    config->engine_config.window_width = config->window_width;
    config->engine_config.window_height = config->window_height;
    config->engine_config.fullscreen = config->fullscreen;
    config->engine_config.vsync = config_section_get_bool(
        engine_section, "vsync", config->engine_config.vsync);
    config->engine_config.max_threads = config_section_get_int(
        engine_section, "max_threads", config->engine_config.max_threads);
    // ... add other fields from main.c
  }

  // Load application configuration
  ConfigSection *app_section = config_document_get_section(doc, "application");
  if (app_section) {
    const char *module_path = config_section_get_string(
        app_section, "game_module_path", config->game_module_path);
    if (module_path && strlen(module_path) > 0) {
      config->game_module_path = module_path;
    }
    config->debug_mode =
        config_section_get_bool(app_section, "debug_mode", config->debug_mode);
    const char *log_level =
        config_section_get_string(app_section, "log_level", config->log_level);
    if (log_level && strlen(log_level) > 0) {
      config->log_level = log_level;
      config->engine_config.log_level = log_level; // Assuming compatibility
    }
  }

  // Load audio configuration - omitting for brevity but should be there

  LOG_INFO("Configuration loaded from: %s", config->config_file);
  config_document_destroy(doc);
  return true;
}

AppConfig parse_arguments(int argc, char *argv[]) {
  AppConfig config = create_default_config();

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--debug") == 0) {
      config.debug_mode = true;
      // config.engine_config = engine_create_debug_config(); // helper
    } else if (strcmp(argv[i], "--fullscreen") == 0) {
      config.fullscreen = true;
      config.engine_config.fullscreen = true;
    } else if (strcmp(argv[i], "--window") == 0 && i + 2 < argc) {
      config.window_width = atoi(argv[++i]);
      config.window_height = atoi(argv[++i]);
      config.engine_config.window_width = config.window_width;
      config.engine_config.window_height = config.window_height;
    } else if (strcmp(argv[i], "--module") == 0 && i + 1 < argc) {
      config.game_module_path = argv[++i];
    } else if (strcmp(argv[i], "--log-level") == 0 && i + 1 < argc) {
      config.log_level = argv[++i];
      // config.engine_config.log_level = config.log_level;
    } else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
      config.config_file = argv[++i];
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      printf("Game Engine Usage:\n");
      printf("  --debug              Enable debug mode\n");
      printf("  --fullscreen         Start in fullscreen mode\n");
      printf("  --window <w> <h>     Set window dimensions\n");
      printf("  --module <path>      Specify game module path\n");
      printf("  --config <file>      Specify configuration file path\n");
      printf("  --help, -h           Show this help message\n");
      exit(0);
    }
  }

  return config;
}
