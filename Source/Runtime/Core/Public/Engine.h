#ifndef ULTIMATE_ENGINE_CORE_ENGINE_H
#define ULTIMATE_ENGINE_CORE_ENGINE_H

#include <stdbool.h>

// Forward declarations
// Types are assumed to be standard or defined in core_types.h if included
// But for a clean header, we can depend on stdbool.

#ifdef __cplusplus
extern "C" {
#endif

// Initialization
bool Engine_Init(const char *config_path);

// Update loop
void Engine_Update(float delta_time);

// Shutdown
void Engine_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // ULTIMATE_ENGINE_CORE_ENGINE_H
