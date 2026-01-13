#include "common.h"
#include "core/logger.h"
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
} HotReloadModule;

typedef struct {
  HotReloadConfig config;
  HashMap *modules; // <string, HotReloadModule*>
  HashMap *states;  // <string, ReloadState*>
  bool initialized;
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

  g_hot_reload.config = config;
  g_hot_reload.modules = hashmap_create(
      16, sizeof(char *), sizeof(HotReloadModule *), str_hash, str_equals);
  g_hot_reload.states = hashmap_create(
      16, sizeof(char *), sizeof(ReloadState *), str_hash, str_equals);
  g_hot_reload.initialized = true;

  LOG_INFO(LOG_CAT_GENERAL, "Hot Reload System initialized");
  return true;
}

void hot_reload_shutdown(void) {
  if (!g_hot_reload.initialized)
    return;

  // In real implementation: iterate and free resources

  hashmap_destroy(g_hot_reload.modules);
  hashmap_destroy(g_hot_reload.states);
  g_hot_reload.initialized = false;
}

void hot_reload_update(void) {
  if (!g_hot_reload.config.enable_code_hot_reload)
    return;
  // Iterate and check logic
}

HotReloadModule *hot_reload_register_module(const char *module_name,
                                            const char *library_path) {
  time_t mod_time = get_file_modified_time(library_path);
  if (mod_time == 0) {
    LOG_ERROR(LOG_CAT_IO, "Failed to find module library: %s", library_path);
    return NULL;
  }

  void *handle = dlopen(library_path, RTLD_NOW);
  if (!handle) {
    LOG_ERROR(LOG_CAT_IO, "Failed to load module: %s (Error: %s)", library_path, dlerror());
    return NULL;
  }

  HotReloadModule *module = malloc(sizeof(HotReloadModule));
  module->name = strdup(module_name);
  module->path = strdup(library_path);
  module->handle = handle;
  module->last_modified = mod_time;

  hashmap_insert(g_hot_reload.modules, module_name, module);
  LOG_INFO(LOG_CAT_GENERAL, "Registered hot-reload module: %s", module_name);

  return module;
}

bool hot_reload_check_module(HotReloadModule *module) {
  if (!module)
    return false;

  time_t current_mod_time = get_file_modified_time(module->path);
  if (current_mod_time > module->last_modified) {
    LOG_INFO(LOG_CAT_GENERAL, "Detected change in module: %s, reloading...", module->name);

    if (module->handle) {
      dlclose(module->handle);
    }

    usleep(100000);

    void *new_handle = dlopen(module->path, RTLD_NOW);
    if (new_handle) {
      module->handle = new_handle;
      module->last_modified = current_mod_time;
      LOG_INFO(LOG_CAT_GENERAL, "Module %s reloaded successfully", module->name);
      return true;
    } else {
      LOG_ERROR(LOG_CAT_IO, "Failed to reload module %s: %s", module->name, dlerror());
      module->handle = NULL;
    }
  }
  return false;
}

void *hot_reload_get_symbol(HotReloadModule *module, const char *symbol_name) {
  if (!module || !module->handle)
    return NULL;
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
