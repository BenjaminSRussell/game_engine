#include <scripting/script_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Include Lua headers
#include <scripting/lua_bindings.h>
#include <vendor/lua/lauxlib.h>
#include <vendor/lua/lua.h>
#include <vendor/lua/lualib.h>

// Include Next-Gen System Headers (assumed paths based on project)
// #include <physics/pbd/pbd_solver.h>
// #include <audio/synthesis/impact_synth.h>

// ==============================================================================
// Lua Bindings: Physics
// ==============================================================================

static int lua_Physics_SpawnBox(lua_State *L) {
    double x = luaL_checknumber(L, 1);
    double y = luaL_checknumber(L, 2);
    double z = luaL_checknumber(L, 3);
    
    // Call into C physics engine
    // pbd_create_box((float)x, (float)y, (float)z, 1.0f);
    
    printf("[LUA] Physics.SpawnBox(%.2f, %.2f, %.2f)\n", x, y, z);
    return 0;
}

// ==============================================================================
// Lua Bindings: Audio
// ==============================================================================

static int lua_Audio_PlayImpact(lua_State *L) {
    const char* material_a = luaL_checkstring(L, 1);
    const char* material_b = luaL_checkstring(L, 2);
    double velocity = luaL_checknumber(L, 3);
    
    // Call into C audio engine
    // synthesis_trigger_impact(g_audio_synth, get_mat_id(material_a), get_mat_id(material_b), (float)velocity);
    
    printf("[LUA] Audio.PlayImpact('%s', '%s', %.2f)\n", material_a, material_b, velocity);
    return 0;
}

// ==============================================================================
// Script System Implementation
// ==============================================================================

bool ScriptSystem_Init(ScriptSystem *system) {
  if (!system)
    return false;

  // Create new Lua state
  system->state = luaL_newstate();
  if (!system->state) {
    printf("Failed to create Lua state\n");
    return false;
  }

  // Open standard libraries
  luaL_openlibs(system->state);

  // Register Global "Physics" Table
  lua_newtable(system->state);
  lua_pushcfunction(system->state, lua_Physics_SpawnBox);
  lua_setfield(system->state, -2, "SpawnBox");
  lua_setglobal(system->state, "Physics");

  // Register Global "Audio" Table
  lua_newtable(system->state);
  lua_pushcfunction(system->state, lua_Audio_PlayImpact);
  lua_setfield(system->state, -2, "PlayImpact");
  lua_setglobal(system->state, "Audio");


  // Register engine bindings
  // lua_bindings_register_all(system->state); // Commented out until implemented

  printf("Script System Initialized (Lua %s)\n", LUA_RELEASE);
  printf(" - Exposed 'Physics' API\n");
  printf(" - Exposed 'Audio' API\n");

  // Load test script for verification
  if (luaL_dofile(system->state, "assets/scripts/test_physics.lua") != LUA_OK) {
      const char *errorMsg = lua_tostring(system->state, -1);
      printf("Lua Error loading test script: %s\n", errorMsg);
      lua_pop(system->state, 1);
  } else {
      printf("Lua: Loaded assets/scripts/test_physics.lua\n");
  }
  
  return true;
}

void ScriptSystem_Shutdown(ScriptSystem *system) {
  if (system && system->state) {
    lua_close(system->state);
    system->state = NULL;
    printf("Script System Shutdown\n");
  }
}

void ScriptSystem_Update(ScriptSystem *system, float deltaTime) {
  // Process scheduled script events?
}

bool ScriptSystem_RunString(ScriptSystem *system, const char *script) {
  if (!system || !system->state)
    return false;

  if (luaL_dostring(system->state, script) != LUA_OK) {
    const char *errorMsg = lua_tostring(system->state, -1);
    printf("Lua Error: %s\n", errorMsg);
    lua_pop(system->state, 1); // Pop error
    return false;
  }
  return true;
}
