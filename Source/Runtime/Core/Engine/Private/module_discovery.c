// Source/Runtime/Core/Engine/Private/module_discovery.c
#include "module_discovery.h"
#include <core/logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

ModuleRegistry *module_registry_create(void) {
  ModuleRegistry *registry = malloc(sizeof(ModuleRegistry));
  if (!registry)
    return NULL;
  registry->capacity = 16;
  registry->modules = malloc(sizeof(DiscoveredModuleInfo) * registry->capacity);
  registry->count = 0;
  if (!registry->modules) {
    free(registry);
    return NULL;
  }
  return registry;
}

void module_registry_destroy(ModuleRegistry *registry) {
  if (!registry)
    return;
  for (unsigned int i = 0; i < registry->count; i++) {
    free(registry->modules[i].name);
    free(registry->modules[i].path);
    free(registry->modules[i].description);
  }
  free(registry->modules);
  free(registry);
}

static bool module_registry_add(ModuleRegistry *registry, const char *name,
                                const char *path, const char *description) {
  if (!registry || !name || !path)
    return false;
  if (registry->count >= registry->capacity) {
    registry->capacity *= 2;
    DiscoveredModuleInfo *new_modules = realloc(
        registry->modules, sizeof(DiscoveredModuleInfo) * registry->capacity);
    if (!new_modules)
      return false;
    registry->modules = new_modules;
  }
  DiscoveredModuleInfo *module = &registry->modules[registry->count++];
  module->name = strdup(name);
  module->path = strdup(path);
  module->description =
      description ? strdup(description) : strdup("No description");
  return true;
}

bool discover_modules(ModuleRegistry *registry) {
  if (!registry)
    return false;
  module_registry_add(registry, "minecraft_v2",
                      "games/minecraft_v2/minecraft_v2_module",
                      "Minecraft v2 - Sandbox building game");
  // ... more scanning ...
  LOG_INFO("Discovered %u game modules", registry->count);
  return true;
}

DiscoveredModuleInfo *find_module_by_name(ModuleRegistry *registry,
                                          const char *name) {
  if (!registry || !name)
    return NULL;
  for (unsigned int i = 0; i < registry->count; i++) {
    if (strcmp(registry->modules[i].name, name) == 0)
      return &registry->modules[i];
  }
  return NULL;
}

DiscoveredModuleInfo *find_module_by_path(ModuleRegistry *registry,
                                          const char *path) {
  if (!registry || !path)
    return NULL;
  for (unsigned int i = 0; i < registry->count; i++) {
    if (strcmp(registry->modules[i].path, path) == 0)
      return &registry->modules[i];
  }
  return NULL;
}

void print_available_modules(ModuleRegistry *registry) {
  if (!registry || registry->count == 0) {
    LOG_INFO("No game modules available");
    return;
  }
  LOG_INFO("Available game modules:");
  for (unsigned int i = 0; i < registry->count; i++) {
    DiscoveredModuleInfo *module = &registry->modules[i];
    LOG_INFO("  %s - %s (%s)", module->name, module->description, module->path);
  }
}

// Dynamic loading implementation
DynamicModule *dynamic_module_load(const char *module_path) {
  DynamicModule *dyn_mod = malloc(sizeof(DynamicModule));
  if (!dyn_mod)
    return NULL;
  memset(dyn_mod, 0, sizeof(DynamicModule));
  dyn_mod->path = strdup(module_path);

#ifdef _WIN32
  dyn_mod->handle = LoadLibrary(module_path);
  // ... win32 load logic ...
  // Simplified for now
#else
  dyn_mod->handle = dlopen(module_path, RTLD_LAZY);
  if (!dyn_mod->handle) {
    LOG_ERROR("Failed to load module: %s (%s)", module_path, dlerror());
    free(dyn_mod->path);
    free(dyn_mod);
    return NULL;
  }
  typedef GameModule *(*CreateModuleFunc)(void);
  CreateModuleFunc create_module =
      (CreateModuleFunc)dlsym(dyn_mod->handle, "create_game_module");
  if (!create_module) {
    LOG_ERROR("Module does not export create_game_module");
    dlclose(dyn_mod->handle);
    free(dyn_mod->path);
    free(dyn_mod);
    return NULL;
  }
  dyn_mod->module = create_module();
#endif

  if (!dyn_mod->module) {
    // cleanup
    return NULL;
  }
  dyn_mod->loaded = true;
  return dyn_mod;
}

void dynamic_module_unload(DynamicModule *dyn_mod) {
  if (!dyn_mod)
    return;
  if (dyn_mod->module && dyn_mod->module->shutdown)
    dyn_mod->module->shutdown(dyn_mod->module);

#ifdef _WIN32
  if (dyn_mod->handle)
    FreeLibrary(dyn_mod->handle);
#else
  if (dyn_mod->handle)
    dlclose(dyn_mod->handle);
#endif

  free(dyn_mod->path);
  free(dyn_mod);
}

bool dynamic_module_initialize(DynamicModule *dyn_mod, Engine *engine) {
  if (!dyn_mod || !dyn_mod->module || !engine)
    return false;
  if (!dyn_mod->module->initialize)
    return false;
  return dyn_mod->module->initialize(dyn_mod->module, engine);
}
