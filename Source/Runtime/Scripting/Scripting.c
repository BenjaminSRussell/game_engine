#include "Public/Scripting.h"
#include <stdlib.h>

// Struct Stubs
struct LuaState {
  int id;
};
struct PythonState {
  int id;
};
struct VisualScripting {
  int id;
};

static ScriptingSystem g_scripting_system = {0};

// Implementation Stubs
LuaState *Lua_CreateState(void) { return malloc(sizeof(LuaState)); }
void Lua_Destroy(LuaState *state) { free(state); }
void Lua_Update(LuaState *state, float dt) {
  (void)state;
  (void)dt;
}

PythonState *Python_CreateState(void) { return malloc(sizeof(PythonState)); }
void Python_Destroy(PythonState *state) { free(state); }
void Python_Update(PythonState *state, float dt) {
  (void)state;
  (void)dt;
}

VisualScripting *VisualScripting_Create(void) {
  return malloc(sizeof(VisualScripting));
}
void VisualScripting_Destroy(VisualScripting *vs) { free(vs); }
void VisualScripting_Update(VisualScripting *vs, float dt) {
  (void)vs;
  (void)dt;
}

void Scripting_RegisterEngineBindings(void) {}

bool Scripting_Init(void) {
  g_scripting_system.lua_state = Lua_CreateState();
  g_scripting_system.python_state = Python_CreateState();
  g_scripting_system.visual_scripting = VisualScripting_Create();

  // Register engine bindings
  Scripting_RegisterEngineBindings();

  return true;
}

void Scripting_Update(float delta_time) {
  // Update Lua scripts
  Lua_Update(g_scripting_system.lua_state, delta_time);

  // Update Python scripts
  Python_Update(g_scripting_system.python_state, delta_time);

  // Update visual scripts
  VisualScripting_Update(g_scripting_system.visual_scripting, delta_time);
}

void Scripting_Shutdown(void) {
  VisualScripting_Destroy(g_scripting_system.visual_scripting);
  Python_Destroy(g_scripting_system.python_state);
  Lua_Destroy(g_scripting_system.lua_state);
}
