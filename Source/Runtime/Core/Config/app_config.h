#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <core/engine.h>
#include <stdbool.h>

typedef struct {
  const char *game_module_path;
  EngineConfig engine_config;
  bool debug_mode;
  bool fullscreen;
  uint32_t window_width;
  uint32_t window_height;
  const char *log_level;
  const char *config_file;
} AppConfig;

AppConfig create_default_config(void);
AppConfig parse_arguments(int argc, char *argv[]);
bool load_config_file(AppConfig *config); // Implementation uses ConfigDocument

#endif // APP_CONFIG_H
