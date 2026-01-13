#include "../public/engine_init.h"
#include "../public/subsystem_registry.h"
#include <core/logger.h>
#include <stddef.h>

// Extern declaration for platform initialization
// This would be implemented in platform/desktop/desktop_init.c
extern bool platform_init(const EngineConfig *config);
extern void platform_shutdown(void);

// Extern for error recovery
extern void error_recovery_init(void);

static bool g_engine_running = false;

EngineInitResult engine_init(const EngineConfig *config) {
  if (!config)
    return ENGINE_INIT_FAILURE_UNKNOWN;

  // Initialize Logger (if not already done)
  // For now assume logging is functional via macros

  LOG_INFO(LOG_CAT_GENERAL, "Initializing Engine...");

  // Initialize Error Recovery
  error_recovery_init();

  // Platform Initialization (Window, Context, Input)
  if (!platform_init(config)) {
    LOG_FATAL(LOG_CAT_GENERAL, "Platform initialization failed");
    return ENGINE_INIT_FAILURE_WINDOW;
  }

  // Subsystem Initialization
  if (!subsystem_registry_initialize_all(config)) {
    LOG_FATAL(LOG_CAT_GENERAL, "Subsystem initialization failed");
    platform_shutdown();
    return ENGINE_INIT_FAILURE_SUBSYSTEM;
  }

  g_engine_running = true;
  LOG_INFO(LOG_CAT_GENERAL, "Engine Initialization Complete.");
  return ENGINE_INIT_SUCCESS;
}

void engine_shutdown(void) {
  if (!g_engine_running)
    return;

  LOG_INFO(LOG_CAT_GENERAL, "Shutting down Engine...");
  g_engine_running = false;

  subsystem_registry_shutdown_all();
  platform_shutdown();
}

bool engine_is_running(void) { return g_engine_running; }

void engine_register_error_handler(void (*handler)(const char *message)) {
  // Stub
}
