/**
 * @file lua_script_system.c
 * @brief Lua Scripting Integration.
 *
 * Embeds Lua VM for gameplay scripting. Hndles state management and reloading.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <scripting/lua/lua_script_system.h>
#include <include/vendor/lua/lauxlib.h>
#include <include/vendor/lua/lua.h>
#include <include/vendor/lua/lualib.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

typedef struct ScriptComponent {
  char script_path[256];
  lua_State *L;
  int update_ref; // Lua reference to Update() function
  int start_ref;
  bool is_initialized;
} ScriptComponent;

typedef struct ScriptSystem {
  lua_State *global_vm;
} ScriptSystem;

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

ScriptSystem *script_system_init() {
  ScriptSystem *sys = malloc(sizeof(ScriptSystem));
  sys->global_vm = luaL_newstate();
  luaL_openlibs(sys->global_vm);

  // Bind Engine API
  // script_bind_api(sys->global_vm);

  return sys;
}

void script_load(ScriptSystem *sys, ScriptComponent *script, const char *path) {
  // Each script could run in global VM (shared) or separate threads/coroutines
  // For simplicity, assuming shared VM here
  lua_State *L = sys->global_vm;

  // Load file
  if (luaL_dofile(L, path) != LUA_OK) {
    // Log Error: lua_tostring(L, -1);
    return;
  }

  // Logic: Scripts might return a table (class-like)
  // script = require("player_controller")

  // Get Update function
  lua_getglobal(L, "Update");
  if (lua_isfunction(L, -1)) {
    script->update_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  } else {
    lua_pop(L, 1);
  }
}

void script_update(ScriptSystem *sys, ScriptComponent *script, float dt) {
  if (script->update_ref != LUA_NOREF) {
    lua_State *L = sys->global_vm;

    // Push function
    lua_rawgeti(L, LUA_REGISTRYINDEX, script->update_ref);

    // Push dt
    lua_pushnumber(L, dt);

    // Call: Update(dt)
    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
      // Log Error
    }
  }
}
