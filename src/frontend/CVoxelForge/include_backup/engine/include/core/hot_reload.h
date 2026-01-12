#ifndef ENGINE_HOT_RELOAD_H
#define ENGINE_HOT_RELOAD_H

#include "common.h"
#include <stdbool.h>

typedef enum {
  HOT_RELOAD_CREATED,
  HOT_RELOAD_MODIFIED,
  HOT_RELOAD_DELETED
} HotReloadEventType;

// Config
typedef struct HotReloadConfig {
  bool enable_code_hot_reload;
  bool enable_asset_hot_reload;
  const char *watch_path;
} HotReloadConfig;

// State preservation across reloads
typedef struct ReloadState {
  const char *id;
  void *data;
  size_t size;
} ReloadState;

// Opaque handle for a module
typedef struct HotReloadModule HotReloadModule;

// Initialization
bool hot_reload_init(HotReloadConfig config);
void hot_reload_shutdown(void);
void hot_reload_update(void);

// Watcher
bool hot_reload_init_watcher(const char *watch_path,
                             void (*callback)(const char *,
                                              HotReloadEventType));
void hot_reload_shutdown_watcher(void);
void hot_reload_process_file_change(const char *file_path,
                                    HotReloadEventType event_type);

// Module Management
bool hot_reload_load_module(const char *module_path);
void hot_reload_unload_module(const char *module_path);
bool hot_reload_reload_module(const char *module_path);
void *hot_reload_get_function(const char *module_path,
                              const char *function_name);

// State Management
bool hot_reload_save_state(const char *module_path, void *state_data,
                           size_t state_size);
bool hot_reload_restore_state(const char *module_path, void *state_data,
                              size_t state_size);

// Specific Asset Reloads
void hot_reload_reload_shader(const char *shader_path);
void hot_reload_reload_texture(const char *texture_path);
void hot_reload_reload_script(const char *script_path);

// Utilities
void hot_reload_set_callback(void (*callback)(const char *,
                                              HotReloadEventType));
bool hot_reload_is_file_supported(const char *file_path);

#endif // ENGINE_HOT_RELOAD_H
