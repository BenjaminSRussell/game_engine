// Source/Runtime/Core/Engine/Private/engine_init.c
#include "engine_init_internal.h"
#include <stdlib.h>
#include <string.h>

// Global context definition
EngineInitContext g_engineInit = {0};
EngineEventCallback g_eventCallbacks[16];
uint32_t g_eventCallbackCount = 0;

// Internal helpers
bool engine_register_event_callback(EngineEventCallback callback) {
  if (g_eventCallbackCount >= 16) {
    printf("ERROR: Event callback registry full\n");
    return false;
  }

  g_eventCallbacks[g_eventCallbackCount++] = callback;
  return true;
}

void engine_unregister_event_callback(EngineEventCallback callback) {
  for (uint32_t i = 0; i < g_eventCallbackCount; i++) {
    if (g_eventCallbacks[i] == callback) {
      for (uint32_t j = i; j < g_eventCallbackCount - 1; j++) {
        g_eventCallbacks[j] = g_eventCallbacks[j + 1];
      }
      g_eventCallbackCount--;
      break;
    }
  }
}

void engine_trigger_event(const EngineEventData *event) {
  for (uint32_t i = 0; i < g_eventCallbackCount; i++) {
    g_eventCallbacks[i](event);
  }
}

void engine_process_events(void) {
  // Process queued events (simplified)
}

// Public API
bool engine_init(const EngineConfig *config) {
  if (g_engineInit.overall_status != ENGINE_INIT_STATUS_NOT_STARTED &&
      g_engineInit.overall_status != ENGINE_INIT_STATUS_ERROR) {
    printf("Engine already initialized or initialization in progress\n");
    return false;
  }

  printf("Starting engine initialization...\n");

  memset(&g_engineInit, 0, sizeof(g_engineInit));
  g_engineInit.config = *config;
  g_engineInit.start_time = (double)clock() / CLOCKS_PER_SEC * 1000.0;
  g_engineInit.current_phase = ENGINE_INIT_PHASE_PRE_INIT;
  g_engineInit.overall_status = ENGINE_INIT_STATUS_IN_PROGRESS;

  EngineEventData event = {.type = ENGINE_EVENT_SYSTEM_READY,
                           .module_name = "engine",
                           .status = ENGINE_INIT_STATUS_IN_PROGRESS,
                           .timestamp = g_engineInit.start_time,
                           .message = "Engine initialization started"};
  engine_trigger_event(&event);

  if (!engine_validate_configuration(config)) {
    g_engineInit.overall_status = ENGINE_INIT_STATUS_ERROR;
    return false;
  }

  // Initialize phases in order
  EngineInitPhase phases[] = {
      ENGINE_INIT_PHASE_CORE_SYSTEMS,   ENGINE_INIT_PHASE_RENDERING_BACKEND,
      ENGINE_INIT_PHASE_AUDIO_SYSTEM,   ENGINE_INIT_PHASE_INPUT_SYSTEM,
      ENGINE_INIT_PHASE_ASSET_SYSTEM,   ENGINE_INIT_PHASE_SCRIPT_SYSTEM,
      ENGINE_INIT_PHASE_NETWORK_SYSTEM, ENGINE_INIT_PHASE_PHYSICS_SYSTEM,
      ENGINE_INIT_PHASE_UI_SYSTEM,      ENGINE_INIT_PHASE_GAME_SYSTEMS};

  for (size_t i = 0; i < sizeof(phases) / sizeof(phases[0]); i++) {
    if (!initialize_phase(phases[i])) {
      printf("ERROR: Phase %s failed, aborting initialization\n",
             get_init_phase_string(phases[i]));
      g_engineInit.current_phase = ENGINE_INIT_PHASE_ERROR;
      return false;
    }

    if (g_engineInit.abort_requested) {
      printf("Initialization aborted by user\n");
      break;
    }
  }

  g_engineInit.current_phase = ENGINE_INIT_PHASE_POST_INIT;
  g_engineInit.total_init_time =
      (double)clock() / CLOCKS_PER_SEC * 1000.0 - g_engineInit.start_time;
  g_engineInit.overall_status = ENGINE_INIT_STATUS_SUCCESS;

  EngineEventData readyEvent = {.type = ENGINE_EVENT_SYSTEM_READY,
                                .module_name = "engine",
                                .status = ENGINE_INIT_STATUS_SUCCESS,
                                .timestamp =
                                    (double)clock() / CLOCKS_PER_SEC * 1000.0,
                                .message = "Engine initialization completed"};
  engine_trigger_event(&readyEvent);

  printf("Engine initialization completed successfully in %.2f ms\n",
         g_engineInit.total_init_time);
  engine_print_init_summary();

  return true;
}

void engine_shutdown(void) {
  if (g_engineInit.overall_status == ENGINE_INIT_STATUS_NOT_STARTED) {
    return;
  }

  printf("Starting engine shutdown...\n");

  EngineEventData event = {.type = ENGINE_EVENT_SHUTDOWN_STARTED,
                           .module_name = "engine",
                           .status = ENGINE_INIT_STATUS_IN_PROGRESS,
                           .timestamp =
                               (double)clock() / CLOCKS_PER_SEC * 1000.0,
                           .message = "Engine shutdown started"};
  engine_trigger_event(&event);

  shutdown_all_modules();

  memset(&g_engineInit, 0, sizeof(g_engineInit));
  g_engineInit.current_phase = ENGINE_INIT_PHASE_PRE_INIT;
  g_engineInit.overall_status = ENGINE_INIT_STATUS_NOT_STARTED;

  EngineEventData completedEvent = {.type = ENGINE_EVENT_SHUTDOWN_COMPLETED,
                                    .module_name = "engine",
                                    .status = ENGINE_INIT_STATUS_SUCCESS,
                                    .timestamp = (double)clock() /
                                                 CLOCKS_PER_SEC * 1000.0,
                                    .message = "Engine shutdown completed"};
  engine_trigger_event(&completedEvent);

  printf("Engine shutdown completed\n");
}

bool engine_is_initialized(void) {
  return g_engineInit.overall_status == ENGINE_INIT_STATUS_SUCCESS &&
         g_engineInit.current_phase == ENGINE_INIT_PHASE_READY;
}

bool engine_is_ready(void) {
  return g_engineInit.overall_status == ENGINE_INIT_STATUS_SUCCESS;
}

EngineInitPhase engine_get_current_phase(void) {
  return g_engineInit.current_phase;
}

EngineInitStatus engine_get_phase_status(EngineInitPhase phase) {
  return g_engineInit.overall_status;
}

bool engine_advance_phase(void) {
  // Logic for stepping phases manually if needed
  // Simplified: similar to engine_init loop logic but single step
  return true;
}

bool engine_wait_for_phase(EngineInitPhase phase, uint32_t timeout_ms) {
  uint32_t startTime = (uint32_t)(clock() / CLOCKS_PER_SEC * 1000);

  while (g_engineInit.current_phase != phase &&
         g_engineInit.overall_status != ENGINE_INIT_STATUS_ERROR &&
         g_engineInit.overall_status != ENGINE_INIT_STATUS_NOT_STARTED) {

    uint32_t currentTime = (uint32_t)(clock() / CLOCKS_PER_SEC * 1000);
    if (currentTime - startTime > timeout_ms) {
      printf("Timeout waiting for phase %s\n", get_init_phase_string(phase));
      return false;
    }
    // sleep...
  }
  return g_engineInit.current_phase == phase;
}

const EngineConfig *engine_get_config(void) { return &g_engineInit.config; }

bool engine_update_config(const EngineConfig *config) {
  g_engineInit.config = *config;
  return engine_validate_configuration(config);
}

// Config loading helpers...
bool engine_validate_configuration(const EngineConfig *config) {
  if (!config) {
    engine_set_error("Configuration is null");
    return false;
  }
  if (!config->app_name || strlen(config->app_name) == 0) {
    engine_set_error("Application name is required");
    return false;
  }
  return true;
}

void engine_set_error(const char *error) {
  strncpy(g_engineInit.error_buffer, error,
          sizeof(g_engineInit.error_buffer) - 1);
  g_engineInit.error_buffer[sizeof(g_engineInit.error_buffer) - 1] = '\0';
}

const char *engine_get_last_error(void) { return g_engineInit.error_buffer; }

void engine_clear_error(void) {
  memset(g_engineInit.error_buffer, 0, sizeof(g_engineInit.error_buffer));
}

void engine_print_init_summary(void) {
  printf("\n=== Engine Initialization Summary ===\n");
  printf("Total init time: %.2f ms\n", g_engineInit.total_init_time);
  printf("Final status: %s\n", get_status_string(g_engineInit.overall_status));
  printf("Modules initialized: %u/%u\n", g_engineInit.successful_inits,
         g_moduleCount); // g_moduleCount extern from modules
                         // ... rest of print ...
}

bool engine_run_diagnostics(void) {
  printf("Running engine diagnostics...\n");
  // ...
  return true;
}

// Stub for now or needs to be implemented fully if used
bool engine_load_config_from_file(const char *config_path) { return false; }
bool engine_save_config_to_file(const char *config_path) { return false; }
bool engine_validate_module_dependencies(void) { return true; }
void engine_get_metrics(EngineMetrics *metrics) {}
void engine_reset_metrics(void) {}
bool engine_enable_profiling(bool enable) { return true; }
bool engine_is_profiling_enabled(void) { return false; }
