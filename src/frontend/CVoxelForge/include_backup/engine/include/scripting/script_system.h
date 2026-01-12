#ifndef SCRIPT_SYSTEM_H
#define SCRIPT_SYSTEM_H

#include <stdbool.h>

// Forward declaration of lua_State to avoid including lua.h here if possible,
// though typically it's needed for API. For now, opaque handle.
struct lua_State;

typedef struct ScriptSystem {
  struct lua_State *state;
} ScriptSystem;

bool ScriptSystem_Init(ScriptSystem *system);
void ScriptSystem_Shutdown(ScriptSystem *system);
void ScriptSystem_Update(ScriptSystem *system, float deltaTime);

// Helper to run a script string directly (good for testing)
bool ScriptSystem_RunString(ScriptSystem *system, const char *script);

#endif // SCRIPT_SYSTEM_H
