#ifndef MODULE_DISCOVERY_H
#define MODULE_DISCOVERY_H

#include <core/engine.h>
#include <core/game_module.h>

typedef struct {
  char *name;
  char *path;
  char *description;
} DiscoveredModuleInfo;

typedef struct {
  DiscoveredModuleInfo *modules;
  unsigned int count;
  unsigned int capacity;
} ModuleRegistry;

typedef struct {
  void *handle;
  GameModule *module;
  char *path;
  bool loaded;
} DynamicModule;

ModuleRegistry *module_registry_create(void);
void module_registry_destroy(ModuleRegistry *registry);
bool discover_modules(ModuleRegistry *registry);
DiscoveredModuleInfo *find_module_by_name(ModuleRegistry *registry,
                                          const char *name);
DiscoveredModuleInfo *find_module_by_path(ModuleRegistry *registry,
                                          const char *path);
void print_available_modules(ModuleRegistry *registry);

DynamicModule *dynamic_module_load(const char *module_path);
void dynamic_module_unload(DynamicModule *dyn_mod);
bool dynamic_module_initialize(DynamicModule *dyn_mod, Engine *engine);

#endif // MODULE_DISCOVERY_H
