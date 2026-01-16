#ifndef ULTIMATE_ENGINE_SCRIPTING_H
#define ULTIMATE_ENGINE_SCRIPTING_H

#include "../../Core/Public/core_types.h"
#include <stdbool.h>

// Structures
typedef struct LuaState LuaState;
typedef struct PythonState PythonState;
typedef struct VisualScripting VisualScripting;

typedef struct ScriptingSystem {
  LuaState *lua_state;
  PythonState *python_state;
  VisualScripting *visual_scripting;
} ScriptingSystem;

// Lifecycle
bool Scripting_Init(void);
void Scripting_Shutdown(void);
void Scripting_Update(float delta_time);
void Scripting_RegisterEngineBindings(void);

// Subsystems
LuaState *Lua_CreateState(void);
void Lua_Destroy(LuaState *state);
void Lua_Update(LuaState *state, float dt);

PythonState *Python_CreateState(void);
void Python_Destroy(PythonState *state);
void Python_Update(PythonState *state, float dt);

VisualScripting *VisualScripting_Create(void);
void VisualScripting_Destroy(VisualScripting *vs);
void VisualScripting_Update(VisualScripting *vs, float dt);

#endif // ULTIMATE_ENGINE_SCRIPTING_H
