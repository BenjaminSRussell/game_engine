// include/core/config_system.h
//
// Purpose: Configuration system with hot-reload support
//
#ifndef CONFIG_SYSTEM_H
#define CONFIG_SYSTEM_H

#include "common.h"
#include "core/config_loader.h"

// Configuration system
typedef struct {
    ConfigDocument* document;
    char config_path[256];
    bool hot_reload_enabled;
    f64 last_modified_time;
    void (*on_config_changed)(const char* section, const char* key);
} ConfigSystem;

// Initialize configuration system
bool config_system_init(ConfigSystem* config, const char* config_path);
void config_system_shutdown(ConfigSystem* config);

// Load and save
bool config_system_load(ConfigSystem* config);
bool config_system_save(ConfigSystem* config);
bool config_system_reload(ConfigSystem* config);

// Hot-reload support
void config_system_enable_hot_reload(ConfigSystem* config, bool enabled);
void config_system_check_for_changes(ConfigSystem* config);

// Value access
i32 config_system_get_int(ConfigSystem* config, const char* section, const char* key, i32 default_value);
f32 config_system_get_float(ConfigSystem* config, const char* section, const char* key, f32 default_value);
bool config_system_get_bool(ConfigSystem* config, const char* section, const char* key, bool default_value);
const char* config_system_get_string(ConfigSystem* config, const char* section, const char* key, const char* default_value);

// Value setting
bool config_system_set_int(ConfigSystem* config, const char* section, const char* key, i32 value);
bool config_system_set_float(ConfigSystem* config, const char* section, const char* key, f32 value);
bool config_system_set_bool(ConfigSystem* config, const char* section, const char* key, bool value);
bool config_system_set_string(ConfigSystem* config, const char* section, const char* key, const char* value);

// Callbacks
void config_system_set_change_callback(ConfigSystem* config, void (*callback)(const char* section, const char* key));

#endif // CONFIG_SYSTEM_H



