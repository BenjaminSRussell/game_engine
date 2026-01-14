#ifndef ENGINE_INIT_INTERNAL_H
#define ENGINE_INIT_INTERNAL_H

#include <core/engine_init.h>
#include <stdio.h>
#include <time.h>

// Global module registry shared between init files
extern EngineModule g_moduleRegistry[64];
extern uint32_t g_moduleCount;

// Event callbacks shared
extern EngineEventCallback g_eventCallbacks[16];
extern uint32_t g_eventCallbackCount;

// Shared internal functions
const char *get_init_phase_string(EngineInitPhase phase);
const char *get_status_string(EngineInitStatus status);
bool check_dependencies(const EngineModule *module);
bool initialize_module(EngineModule *module);
bool initialize_phase(EngineInitPhase phase);
void shutdown_all_modules(void);

#endif // ENGINE_INIT_INTERNAL_H
