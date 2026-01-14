// include/tools/hot_reload.h
//
// Purpose: Runtime hot-reloading system for modules, shaders, and assets.
//
#ifndef TOOLS_HOT_RELOAD_H
#define TOOLS_HOT_RELOAD_H

#include <core/types.h>
#include <stdbool.h>
#include <stddef.h>

// Constants
#define MAX_HOT_RELOAD_MODULES 64
#define MAX_PENDING_EVENTS 128
#define MAX_ZONE_NAME_LENGTH 64

// Event types
typedef enum {
    HOT_RELOAD_CREATED,
    HOT_RELOAD_MODIFIED,
    HOT_RELOAD_DELETED
} HotReloadEventType;

// Legacy callback type for compatibility
typedef void (*ReloadCallback)(const char *path, void *user_data);

// MARK: - System Management

bool hot_reload_init(void);
void hot_reload_shutdown(void);
void hot_reload_update(void);

// MARK: - File Watching

bool hot_reload_init_watcher(const char* watch_path, void (*callback)(const char*, HotReloadEventType));
void hot_reload_shutdown_watcher(void);

// MARK: - Module Management

bool hot_reload_load_module(const char* module_path);
void hot_reload_unload_module(const char* module_path);
void* hot_reload_get_function(const char* module_path, const char* function_name);
bool hot_reload_reload_module(const char* module_path);
void hot_reload_module(void); // Legacy compatibility

// MARK: - State Management

bool hot_reload_save_state(const char* module_path, void* state_data, size_t state_size);
bool hot_reload_restore_state(const char* module_path, void* state_data, size_t state_size);

// MARK: - Asset Reloading

void hot_reload_process_file_change(const char* file_path, HotReloadEventType event_type);
void hot_reload_reload_shader(const char* shader_path);
void hot_reload_reload_texture(const char* texture_path);
void hot_reload_reload_script(const char* script_path);

// MARK: - Utility Functions

void hot_reload_register_callback(const char *extension, ReloadCallback callback, void *user_data);
void hot_reload_set_callback(void (*callback)(const char*, HotReloadEventType));
bool hot_reload_is_file_supported(const char* file_path);

#endif // TOOLS_HOT_RELOAD_H
