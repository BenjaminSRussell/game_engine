// Source/Runtime/Core/Engine/Private/engine_modules.c
#include "engine_init_internal.h"
#include <stdio.h>
#include <string.h>

// Static module registry definitions
EngineModule g_moduleRegistry[64];
uint32_t g_moduleCount = 0;

bool engine_register_module(const EngineModule *module) {
  if (g_moduleCount >= 64) {
    printf("ERROR: Module registry full\n");
    return false;
  }

  // Check for duplicate module names
  for (uint32_t i = 0; i < g_moduleCount; i++) {
    if (strcmp(g_moduleRegistry[i].name, module->name) == 0) {
      printf("ERROR: Module %s already registered\n", module->name);
      return false;
    }
  }

  // Add module to registry
  g_moduleRegistry[g_moduleCount] = *module;
  g_moduleCount++;

  printf("Registered module: %s (priority: %u)\n", module->name,
         module->priority);

  // Trigger registration event
  EngineEventData event = {.type = ENGINE_EVENT_MODULE_REGISTERED,
                           .module_name = module->name,
                           .status = ENGINE_INIT_STATUS_SUCCESS,
                           .timestamp =
                               (double)clock() / CLOCKS_PER_SEC * 1000.0,
                           .message = "Module registered"};
  engine_trigger_event(&event);

  return true;
}

bool engine_unregister_module(const char *module_name) {
  for (uint32_t i = 0; i < g_moduleCount; i++) {
    if (strcmp(g_moduleRegistry[i].name, module_name) == 0) {
      if (g_moduleRegistry[i].initialized && g_moduleRegistry[i].shutdown) {
        g_moduleRegistry[i].shutdown();
      }

      // Remove module from registry
      for (uint32_t j = i; j < g_moduleCount - 1; j++) {
        g_moduleRegistry[j] = g_moduleRegistry[j + 1];
      }
      g_moduleCount--;

      printf("Unregistered module: %s\n", module_name);
      return true;
    }
  }

  printf("WARNING: Module %s not found\n", module_name);
  return false;
}

EngineModule *engine_get_module(const char *module_name) {
  for (uint32_t i = 0; i < g_moduleCount; i++) {
    if (strcmp(g_moduleRegistry[i].name, module_name) == 0) {
      return &g_moduleRegistry[i];
    }
  }
  return NULL;
}

EngineModule *engine_get_all_modules(uint32_t *count) {
  *count = g_moduleCount;
  return g_moduleRegistry;
}

void engine_print_module_info(void) {
  printf("\n=== Engine Module Registry ===\n");
  printf("Total modules: %u\n", g_moduleCount);
  printf("Successful inits: %u\n", g_engineInit.successful_inits);
  printf("Failed inits: %u\n", g_engineInit.failed_inits);
  printf("Warnings: %u\n", g_engineInit.warning_count);

  for (uint32_t i = 0; i < g_moduleCount; i++) {
    const EngineModule *module = &g_moduleRegistry[i];
    printf("  %s v%s - %s - %s\n", module->name,
           module->version ? module->version : "unknown",
           module->initialized ? "initialized" : "not initialized",
           get_status_string(module->status));
    if (module->init_time > 0) {
      printf("    Init time: %.2f ms\n", module->init_time);
    }
  }
  printf("================================\n\n");
}
