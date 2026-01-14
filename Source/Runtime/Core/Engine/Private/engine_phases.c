// Source/Runtime/Core/Engine/Private/engine_phases.c
#include "engine_init_internal.h"
#include <stdio.h>
#include <string.h>

const char *get_init_phase_string(EngineInitPhase phase) {
  switch (phase) {
  case ENGINE_INIT_PHASE_PRE_INIT:
    return "Pre-Initialization";
  case ENGINE_INIT_PHASE_CORE_SYSTEMS:
    return "Core Systems";
  case ENGINE_INIT_PHASE_RENDERING_BACKEND:
    return "Rendering Backend";
  case ENGINE_INIT_PHASE_AUDIO_SYSTEM:
    return "Audio System";
  case ENGINE_INIT_PHASE_INPUT_SYSTEM:
    return "Input System";
  case ENGINE_INIT_PHASE_ASSET_SYSTEM:
    return "Asset System";
  case ENGINE_INIT_PHASE_SCRIPT_SYSTEM:
    return "Script System";
  case ENGINE_INIT_PHASE_NETWORK_SYSTEM:
    return "Network System";
  case ENGINE_INIT_PHASE_PHYSICS_SYSTEM:
    return "Physics System";
  case ENGINE_INIT_PHASE_UI_SYSTEM:
    return "UI System";
  case ENGINE_INIT_PHASE_GAME_SYSTEMS:
    return "Game Systems";
  case ENGINE_INIT_PHASE_POST_INIT:
    return "Post-Initialization";
  case ENGINE_INIT_PHASE_READY:
    return "Ready";
  case ENGINE_INIT_PHASE_ERROR:
    return "Error";
  default:
    return "Unknown";
  }
}

const char *get_status_string(EngineInitStatus status) {
  switch (status) {
  case ENGINE_INIT_STATUS_NOT_STARTED:
    return "Not Started";
  case ENGINE_INIT_STATUS_IN_PROGRESS:
    return "In Progress";
  case ENGINE_INIT_STATUS_SUCCESS:
    return "Success";
  case ENGINE_INIT_STATUS_WARNING:
    return "Warning";
  case ENGINE_INIT_STATUS_ERROR:
    return "Error";
  default:
    return "Unknown";
  }
}

bool check_dependencies(const EngineModule *module) {
  if (module->dependencies == 0) {
    return true; // No dependencies
  }

  for (uint32_t i = 0; i < g_moduleCount; i++) {
    if (g_moduleRegistry[i].initialized && (module->dependencies & (1 << i))) {
      return true;
    }
  }

  return false;
}

bool initialize_module(EngineModule *module) {
  if (!module || !module->init) {
    return false;
  }

  printf("Initializing module: %s\n", module->name);

  double startTime = (double)clock() / CLOCKS_PER_SEC * 1000.0;

  bool success = module->init(module->config);

  double endTime = (double)clock() / CLOCKS_PER_SEC * 1000.0;
  module->init_time = endTime - startTime;

  module->status =
      success ? ENGINE_INIT_STATUS_SUCCESS : ENGINE_INIT_STATUS_ERROR;
  module->initialized = success;

  if (success) {
    g_engineInit.successful_inits++;
    printf("Module %s initialized successfully in %.2f ms\n", module->name,
           module->init_time);

    // Trigger success event
    EngineEventData event = {.type = ENGINE_EVENT_MODULE_INITIALIZED,
                             .module_name = module->name,
                             .status = ENGINE_INIT_STATUS_SUCCESS,
                             .timestamp = endTime,
                             .message = "Module initialized successfully"};
    engine_trigger_event(&event);
  } else {
    g_engineInit.failed_inits++;
    snprintf(g_engineInit.error_buffer, sizeof(g_engineInit.error_buffer),
             "Failed to initialize module: %s", module->name);
    printf("ERROR: Module %s failed to initialize\n", module->name);

    // Trigger failure event
    EngineEventData event = {.type = ENGINE_EVENT_MODULE_FAILED,
                             .module_name = module->name,
                             .status = ENGINE_INIT_STATUS_ERROR,
                             .timestamp = endTime,
                             .message = g_engineInit.error_buffer};
    engine_trigger_event(&event);
  }

  return success;
}

bool initialize_phase(EngineInitPhase phase) {
  printf("Starting phase: %s\n", get_init_phase_string(phase));

  g_engineInit.current_phase = phase;
  g_engineInit.overall_status = ENGINE_INIT_STATUS_IN_PROGRESS;

  // Trigger phase start event
  EngineEventData event = {.type = ENGINE_EVENT_PHASE_STARTED,
                           .module_name = "",
                           .status = ENGINE_INIT_STATUS_IN_PROGRESS,
                           .timestamp =
                               (double)clock() / CLOCKS_PER_SEC * 1000.0,
                           .message = get_init_phase_string(phase)};
  engine_trigger_event(&event);

  // Initialize all modules for this phase
  bool phaseSuccess = true;
  uint32_t phaseModuleCount = 0;

  for (uint32_t i = 0; i < g_moduleCount; i++) {
    EngineModule *module = &g_moduleRegistry[i];

    // Check if module belongs to this phase
    bool belongsToPhase = false;
    switch (phase) {
    case ENGINE_INIT_PHASE_CORE_SYSTEMS:
      belongsToPhase = (module->dependencies & ENGINE_DEP_CORE_SYSTEMS) != 0;
      break;
    case ENGINE_INIT_PHASE_RENDERING_BACKEND:
      belongsToPhase =
          (module->dependencies & ENGINE_DEP_RENDERING_BACKEND) != 0;
      break;
    case ENGINE_INIT_PHASE_AUDIO_SYSTEM:
      belongsToPhase = (module->dependencies & ENGINE_DEP_AUDIO_SYSTEM) != 0;
      break;
    case ENGINE_INIT_PHASE_INPUT_SYSTEM:
      belongsToPhase = (module->dependencies & ENGINE_DEP_INPUT_SYSTEM) != 0;
      break;
    case ENGINE_INIT_PHASE_ASSET_SYSTEM:
      belongsToPhase = (module->dependencies & ENGINE_DEP_ASSET_SYSTEM) != 0;
      break;
    case ENGINE_INIT_PHASE_SCRIPT_SYSTEM:
      belongsToPhase = (module->dependencies & ENGINE_DEP_SCRIPT_SYSTEM) != 0;
      break;
    case ENGINE_INIT_PHASE_NETWORK_SYSTEM:
      belongsToPhase = (module->dependencies & ENGINE_DEP_NETWORK_SYSTEM) != 0;
      break;
    case ENGINE_INIT_PHASE_PHYSICS_SYSTEM:
      belongsToPhase = (module->dependencies & ENGINE_DEP_PHYSICS_SYSTEM) != 0;
      break;
    case ENGINE_INIT_PHASE_UI_SYSTEM:
      belongsToPhase = (module->dependencies & ENGINE_DEP_UI_SYSTEM) != 0;
      break;
    case ENGINE_INIT_PHASE_GAME_SYSTEMS:
      belongsToPhase = (module->dependencies & ENGINE_DEP_GAME_SYSTEMS) != 0;
      break;
    default:
      belongsToPhase = false;
      break;
    }

    if (belongsToPhase && !module->initialized) {
      if (check_dependencies(module)) {
        if (initialize_module(module)) {
          phaseModuleCount++;
        } else if (module->required) {
          phaseSuccess = false;
          snprintf(g_engineInit.error_buffer, sizeof(g_engineInit.error_buffer),
                   "Required module %s failed to initialize", module->name);
        }
      } else {
        printf("Module %s waiting for dependencies\n", module->name);
      }
    }
  }

  if (phaseSuccess && phaseModuleCount > 0) {
    g_engineInit.overall_status = ENGINE_INIT_STATUS_SUCCESS;

    // Trigger phase completion event
    EngineEventData event = {.type = ENGINE_EVENT_PHASE_COMPLETED,
                             .module_name = "",
                             .status = ENGINE_INIT_STATUS_SUCCESS,
                             .timestamp =
                                 (double)clock() / CLOCKS_PER_SEC * 1000.0,
                             .message = get_init_phase_string(phase)};
    engine_trigger_event(&event);

    printf("Phase %s completed successfully (%d modules)\n",
           get_init_phase_string(phase), phaseModuleCount);
  } else {
    g_engineInit.overall_status = ENGINE_INIT_STATUS_ERROR;

    // Trigger phase error event
    EngineEventData event = {.type = ENGINE_EVENT_PHASE_COMPLETED,
                             .module_name = "",
                             .status = ENGINE_INIT_STATUS_ERROR,
                             .timestamp =
                                 (double)clock() / CLOCKS_PER_SEC * 1000.0,
                             .message = get_init_phase_string(phase)};
    engine_trigger_event(&event);

    printf("ERROR: Phase %s failed\n", get_init_phase_string(phase));
  }

  return phaseSuccess;
}

void shutdown_all_modules(void) {
  printf("Shutting down all modules...\n");

  // Shutdown in reverse dependency order
  for (int32_t i = (int32_t)g_moduleCount - 1; i >= 0; i--) {
    EngineModule *module = &g_moduleRegistry[i];
    if (module->initialized && module->shutdown) {
      printf("Shutting down module: %s\n", module->name);
      module->shutdown();
      module->initialized = false;
    }
  }
}
