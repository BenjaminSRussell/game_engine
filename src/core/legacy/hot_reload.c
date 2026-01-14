#include <common.h>
#include "engine/include/core/logger.h"
#include <core/hashmap.h>
#include <core/hot_reload.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

typedef struct HotReloadModule {
  char *name;
  char *path;
  void *handle;
  time_t last_modified;
  uint64_t reload_count;
  uint64_t error_count;
  bool validation_enabled;
} HotReloadModule;

typedef struct {
  HotReloadConfig config;
  HashMap *modules; // <string, HotReloadModule*>
  HashMap *states;  // <string, ReloadState*>
  bool initialized;
  
  // Validation state
  bool validation_enabled;
  uint64_t total_reload_attempts;
  uint64_t successful_reloads;
  uint64_t failed_reloads;
  uint64_t validation_errors;
  uint64_t last_validation_time;
} HotReloadSystem;

static HotReloadSystem g_hot_reload = {0};

static u32 str_hash(const void *key) {
  const char *str = (const char *)key;
  u32 hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;
  return hash;
}

static bool str_equals(const void *a, const void *b) {
  return strcmp((const char *)a, (const char *)b) == 0;
}

// Forward declarations for validation functions
static bool hot_reload_validate_module_path(const char *path);
static bool hot_reload_validate_module_handle(void *handle);
static void hot_reload_log_validation_error(const char *error);
static uint64_t hot_reload_get_timestamp(void);
static bool hot_reload_validate_symbol_name(const char *symbol_name);

static time_t get_file_modified_time(const char *path) {
  struct stat attr;
  if (stat(path, &attr) == 0) {
    return attr.st_mtime;
  }
  return 0;
}

bool hot_reload_init(HotReloadConfig config) {
  if (g_hot_reload.initialized)
    return true;

  // Validate configuration
  if (!config.enable_code_hot_reload && !config.enable_asset_hot_reload) {
    LOG_ERROR("Hot reload: Both code and asset reloading disabled");
    return false;
  }

  if (!config.watch_path || strlen(config.watch_path) == 0) {
    LOG_ERROR("Hot reload: Invalid watch path");
    return false;
  }

  if (!hot_reload_validate_module_path(config.watch_path)) {
    LOG_ERROR("Hot reload: Invalid watch path: %s", config.watch_path);
    return false;
  }

  g_hot_reload.config = config;
  g_hot_reload.modules = hashmap_create(
      16, sizeof(char *), sizeof(HotReloadModule *), str_hash, str_equals);
  g_hot_reload.states = hashmap_create(
      16, sizeof(char *), sizeof(ReloadState *), str_hash, str_equals);
  g_hot_reload.initialized = true;
  
  // Initialize validation
  g_hot_reload.validation_enabled = true;
  g_hot_reload.total_reload_attempts = 0;
  g_hot_reload.successful_reloads = 0;
  g_hot_reload.failed_reloads = 0;
  g_hot_reload.validation_errors = 0;
  g_hot_reload.last_validation_time = hot_reload_get_timestamp();

  LOG_INFO("Hot Reload System initialized with validation enabled");
  return true;
}

void hot_reload_shutdown(void) {
  if (!g_hot_reload.initialized)
    return;

  // Report statistics before shutdown
  if (g_hot_reload.validation_enabled) {
    LOG_INFO("Hot Reload Statistics:");
    LOG_INFO("  Total reload attempts: %lu", g_hot_reload.total_reload_attempts);
    LOG_INFO("  Successful reloads: %lu", g_hot_reload.successful_reloads);
    LOG_INFO("  Failed reloads: %lu", g_hot_reload.failed_reloads);
    LOG_INFO("  Validation errors: %lu", g_hot_reload.validation_errors);
    
    if (g_hot_reload.total_reload_attempts > 0) {
      double success_rate = (double)g_hot_reload.successful_reloads / 
                           g_hot_reload.total_reload_attempts * 100.0;
      LOG_INFO("  Success rate: %.2f%%", success_rate);
    }
  }

  // Free all modules
  const char **keys = hashmap_keys(g_hot_reload.modules);
  for (size_t i = 0; i < hashmap_size(g_hot_reload.modules); i++) {
    HotReloadModule *module = hashmap_get(g_hot_reload.modules, keys[i]);
    if (module) {
      if (module->handle) {
        dlclose(module->handle);
      }
      free(module->name);
      free(module->path);
      free(module);
    }
  }
  free(keys);

  // Free all states
  keys = hashmap_keys(g_hot_reload.states);
  for (size_t i = 0; i < hashmap_size(g_hot_reload.states); i++) {
    ReloadState *state = hashmap_get(g_hot_reload.states, keys[i]);
    if (state) {
      free(state->data);
      free((void *)state->id);
      free(state);
    }
  }
  free(keys);

  hashmap_destroy(g_hot_reload.modules);
  hashmap_destroy(g_hot_reload.states);
  g_hot_reload.initialized = false;
  
  LOG_INFO("Hot Reload System shutdown complete");
}

void hot_reload_update(void) {
  if (!g_hot_reload.config.enable_code_hot_reload)
    return;
  // Iterate and check logic
}

HotReloadModule *hot_reload_register_module(const char *module_name,
                                            const char *library_path) {
  if (!module_name || !library_path) {
    hot_reload_log_validation_error("Invalid module name or library path");
    return NULL;
  }

  if (!hot_reload_validate_module_path(library_path)) {
    hot_reload_log_validation_error("Invalid library path for module registration");
    return NULL;
  }

  time_t mod_time = get_file_modified_time(library_path);
  if (mod_time == 0) {
    LOG_ERROR("Failed to find module library: %s", library_path);
    return NULL;
  }

  void *handle = dlopen(library_path, RTLD_NOW);
  if (!handle) {
    LOG_ERROR("Failed to load module: %s (Error: %s)", library_path, dlerror());
    return NULL;
  }

  if (!hot_reload_validate_module_handle(handle)) {
    hot_reload_log_validation_error("Invalid module handle after loading");
    dlclose(handle);
    return NULL;
  }

  HotReloadModule *module = malloc(sizeof(HotReloadModule));
  if (!module) {
    LOG_ERROR("Failed to allocate memory for hot reload module");
    dlclose(handle);
    return NULL;
  }

  module->name = strdup(module_name);
  module->path = strdup(library_path);
  module->handle = handle;
  module->last_modified = mod_time;
  module->reload_count = 0;
  module->error_count = 0;
  module->validation_enabled = true;

  hashmap_insert(g_hot_reload.modules, module_name, module);
  LOG_INFO("Registered hot-reload module: %s", module_name);

  return module;
}

bool hot_reload_check_module(HotReloadModule *module) {
  if (!module)
    return false;

  if (!module->validation_enabled)
    return false;

  time_t current_mod_time = get_file_modified_time(module->path);
  if (current_mod_time > module->last_modified) {
    LOG_INFO("Detected change in module: %s, reloading...", module->name);
    
    // Update statistics
    g_hot_reload.total_reload_attempts++;

    if (module->handle) {
      dlclose(module->handle);
    }

    usleep(100000);

    void *new_handle = dlopen(module->path, RTLD_NOW);
    if (new_handle) {
      if (!hot_reload_validate_module_handle(new_handle)) {
        hot_reload_log_validation_error("Invalid module handle after reload");
        dlclose(new_handle);
        module->handle = NULL;
        module->error_count++;
        g_hot_reload.failed_reloads++;
        return false;
      }
      
      module->handle = new_handle;
      module->last_modified = current_mod_time;
      module->reload_count++;
      g_hot_reload.successful_reloads++;
      LOG_INFO("Module %s reloaded successfully (reload #%lu)", 
               module->name, module->reload_count);
      return true;
    } else {
      LOG_ERROR("Failed to reload module %s: %s", module->name, dlerror());
      module->handle = NULL;
      module->error_count++;
      g_hot_reload.failed_reloads++;
    }
  }
  return false;
}

void *hot_reload_get_symbol(HotReloadModule *module, const char *symbol_name) {
  if (!module || !module->handle || !symbol_name) {
    return NULL;
  }

  if (!hot_reload_validate_symbol_name(symbol_name)) {
    hot_reload_log_validation_error("Invalid symbol name requested");
    return NULL;
  }

  return dlsym(module->handle, symbol_name);
}

bool hot_reload_save_state(const char *module_path, void *state_data,
                           size_t state_size) {
  ReloadState *old = hashmap_get(g_hot_reload.states, module_path);
  if (old) {
    free(old->data);
    free((void *)old->id);
    free(old);
  }

  ReloadState *state = malloc(sizeof(ReloadState));
  state->id = strdup(module_path);
  state->size = state_size;
  state->data = malloc(state_size);
  memcpy(state->data, state_data, state_size);

  hashmap_insert(g_hot_reload.states, module_path, state);
  return true;
}

bool hot_reload_restore_state(const char *module_path, void *state_data,
                              size_t state_size) {
  ReloadState *state = hashmap_get(g_hot_reload.states, module_path);
  if (state) {
    if (state_data && state->data) {
      memcpy(state_data, state->data,
             state_size < state->size ? state_size : state->size);
      return true;
    }
  }
  return false;
}

// -----------------------------------------------------------------------------
// Hot Reload Validation Implementation
// -----------------------------------------------------------------------------

static bool hot_reload_validate_module_path(const char *path) {
  if (!path) return false;
  
  // Check path length
  if (strlen(path) == 0 || strlen(path) > 1024) {
    return false;
  }
  
  // Check if path exists
  struct stat st;
  if (stat(path, &st) != 0) {
    return false;
  }
  
  // Check if it's a regular file
  if (!S_ISREG(st.st_mode)) {
    return false;
  }
  
  // Check file extension (should be .so, .dylib, or .dll)
  const char *ext = strrchr(path, '.');
  if (!ext) {
    return false;
  }
  
  if (strcmp(ext, ".so") != 0 && strcmp(ext, ".dylib") != 0 && strcmp(ext, ".dll") != 0) {
    return false;
  }
  
  return true;
}

static bool hot_reload_validate_module_handle(void *handle) {
  if (!handle) return false;
  
  // Try to get a known symbol to test the handle
  // This is a basic validation - in a real implementation,
  // you might check for specific symbols that should exist
  Dl_info info;
  if (dladdr(handle, &info) == 0) {
    return false;
  }
  
  return true;
}

static void hot_reload_log_validation_error(const char *error) {
  if (!error) return;
  
  g_hot_reload.validation_errors++;
  LOG_ERROR("Hot Reload Validation Error [%lu]: %s", 
            g_hot_reload.validation_errors, error);
}

static uint64_t hot_reload_get_timestamp(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static bool hot_reload_validate_symbol_name(const char *symbol_name) {
  if (!symbol_name) return false;
  
  // Check for valid symbol name format
  if (strlen(symbol_name) == 0 || strlen(symbol_name) > 256) {
    return false;
  }
  
  // Check for valid characters (alphanumeric, underscore)
  for (const char *p = symbol_name; *p; p++) {
    if (!((*p >= 'a' && *p <= 'z') || 
          (*p >= 'A' && *p <= 'Z') || 
          (*p >= '0' && *p <= '9') || 
          *p == '_')) {
      return false;
    }
  }
  
  return true;
}

// Public validation API
bool hot_reload_validate_state(void) {
  if (!g_hot_reload.initialized || !g_hot_reload.validation_enabled) {
    return true;
  }
  
  bool valid = true;
  uint64_t current_time = hot_reload_get_timestamp();
  
  // Check for excessive error rates
  if (g_hot_reload.total_reload_attempts > 0) {
    double error_rate = (double)g_hot_reload.failed_reloads / g_hot_reload.total_reload_attempts;
    if (error_rate > 0.5) { // More than 50% failure rate
      hot_reload_log_validation_error("High reload failure rate detected");
      valid = false;
    }
  }
  
  // Check for modules with too many errors
  const char **keys = hashmap_keys(g_hot_reload.modules);
  for (size_t i = 0; i < hashmap_size(g_hot_reload.modules); i++) {
    HotReloadModule *module = hashmap_get(g_hot_reload.modules, keys[i]);
    if (module && module->error_count > 10) {
      hot_reload_log_validation_error("Module has excessive error count");
      valid = false;
    }
  }
  free(keys);
  
  // Check for stale state (no activity for extended period)
  uint64_t time_since_last = current_time - g_hot_reload.last_validation_time;
  if (time_since_last > 300000000000ULL) { // 5 minutes
    LOG_INFO("Hot reload validation: No activity for %lu seconds", 
             time_since_last / 1000000000ULL);
  }
  
  g_hot_reload.last_validation_time = current_time;
  return valid;
}

void hot_reload_enable_validation(bool enabled) {
  g_hot_reload.validation_enabled = enabled;
  g_hot_reload.last_validation_time = hot_reload_get_timestamp();
  LOG_INFO("Hot reload validation %s", enabled ? "enabled" : "disabled");
}

uint64_t hot_reload_get_validation_errors(void) {
  return g_hot_reload.validation_errors;
}

void hot_reload_get_statistics(uint64_t *total_attempts, uint64_t *successful_reloads,
                              uint64_t *failed_reloads, uint64_t *validation_errors) {
  if (total_attempts) *total_attempts = g_hot_reload.total_reload_attempts;
  if (successful_reloads) *successful_reloads = g_hot_reload.successful_reloads;
  if (failed_reloads) *failed_reloads = g_hot_reload.failed_reloads;
  if (validation_errors) *validation_errors = g_hot_reload.validation_errors;
}

void hot_reload_reset_validation_stats(void) {
  g_hot_reload.total_reload_attempts = 0;
  g_hot_reload.successful_reloads = 0;
  g_hot_reload.failed_reloads = 0;
  g_hot_reload.validation_errors = 0;
  g_hot_reload.last_validation_time = hot_reload_get_timestamp();
  
  // Reset module statistics
  const char **keys = hashmap_keys(g_hot_reload.modules);
  for (size_t i = 0; i < hashmap_size(g_hot_reload.modules); i++) {
    HotReloadModule *module = hashmap_get(g_hot_reload.modules, keys[i]);
    if (module) {
      module->reload_count = 0;
      module->error_count = 0;
    }
  }
  free(keys);
  
  LOG_INFO("Hot reload validation statistics reset");
}
