/**
 * @file config_system.c
 * @brief Core Implementation
 * @description Core system implementation
 * @date 2026-01-13
 */

/*
 * config_system.c
 * Configuration system implementation
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 * 
 * Provides configuration management with file monitoring and hot-reload
 */

#include "engine/include/core/config_system.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

static f64 get_file_modified_time(const char* path) {
#ifdef _WIN32
    struct _stat64 stat_buf;
    if (_stat64(path, &stat_buf) == 0) {
        return (f64)stat_buf.st_mtime;
    }
#else
    struct stat stat_buf;
    if (stat(path, &stat_buf) == 0) {
        return (f64)stat_buf.st_mtime;
    }
#endif
    return 0.0;
}

bool config_system_init(ConfigSystem* config, const char* config_path) {
    if (!config || !config_path) {
        LOG_ERROR("Config system init failed: null parameters");
        return false;
    }
    
    memset(config, 0, sizeof(ConfigSystem));
    strncpy(config->config_path, config_path, sizeof(config->config_path) - 1);
    
    config->document = config_document_create(32);
    if (!config->document) {
        LOG_ERROR("Failed to create config document");
        return false;
    }
    
    // Try to load existing config
    if (!config_system_load(config)) {
        LOG_WARN("Failed to load config file, using defaults");
    }
    
    config->last_modified_time = get_file_modified_time(config_path);
    config->hot_reload_enabled = false;
    
    return true;
}

void config_system_shutdown(ConfigSystem* config) {
    if (!config) return;
    
    if (config->document) {
        config_document_destroy(config->document);
        config->document = NULL;
    }
}

bool config_system_load(ConfigSystem* config) {
    if (!config || !config->document) return false;
    
    if (!config_document_load(config->document, config->config_path)) {
        return false;
    }
    
    config->last_modified_time = get_file_modified_time(config->config_path);
    return true;
}

bool config_system_save(ConfigSystem* config) {
    if (!config || !config->document) return false;
    
    return config_document_save(config->document);
}

bool config_system_reload(ConfigSystem* config) {
    if (!config) return false;
    
    return config_system_load(config);
}

void config_system_enable_hot_reload(ConfigSystem* config, bool enabled) {
    if (config) {
        config->hot_reload_enabled = enabled;
    }
}

void config_system_check_for_changes(ConfigSystem* config) {
    if (!config || !config->hot_reload_enabled) return;
    
    f64 current_time = get_file_modified_time(config->config_path);
    if (current_time > config->last_modified_time) {
        LOG_INFO("Config file changed, reloading...");
        if (config_system_reload(config)) {
            config->last_modified_time = current_time;
            if (config->on_config_changed) {
                config->on_config_changed("", "");
            }
        }
    }
}

i32 config_system_get_int(ConfigSystem* config, const char* section, const char* key, i32 default_value) {
    if (!config || !config->document) return default_value;
    return config_get_int(config->document, section, key, default_value);
}

f32 config_system_get_float(ConfigSystem* config, const char* section, const char* key, f32 default_value) {
    if (!config || !config->document) return default_value;
    return config_get_float(config->document, section, key, default_value);
}

bool config_system_get_bool(ConfigSystem* config, const char* section, const char* key, bool default_value) {
    if (!config || !config->document) return default_value;
    return config_get_bool(config->document, section, key, default_value);
}

const char* config_system_get_string(ConfigSystem* config, const char* section, const char* key, const char* default_value) {
    if (!config || !config->document) return default_value;
    return config_get_string(config->document, section, key, default_value);
}

bool config_system_set_int(ConfigSystem* config, const char* section, const char* key, i32 value) {
    if (!config || !config->document) return false;
    return config_set_int(config->document, section, key, value);
}

bool config_system_set_float(ConfigSystem* config, const char* section, const char* key, f32 value) {
    if (!config || !config->document) return false;
    return config_set_float(config->document, section, key, value);
}

bool config_system_set_bool(ConfigSystem* config, const char* section, const char* key, bool value) {
    if (!config || !config->document) return false;
    return config_set_bool(config->document, section, key, value);
}

bool config_system_set_string(ConfigSystem* config, const char* section, const char* key, const char* value) {
    if (!config || !config->document) return false;
    return config_set_string(config->document, section, key, value);
}

void config_system_set_change_callback(ConfigSystem* config, void (*callback)(const char* section, const char* key)) {
    if (config) {
        config->on_config_changed = callback;
    }
}


