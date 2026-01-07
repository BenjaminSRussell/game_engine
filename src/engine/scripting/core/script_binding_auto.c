/**
 * @file script_binding_auto.c
 * @brief Auto-generated Script Bindings.
 *
 * Wrapper functions to expose Engine C API to Lua/Visual Script.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <scripting/core/script_binding_auto.h>
#include <scene/transform.h>

// =================================================================================================
//                                      BINDINGS
// =================================================================================================

// Internal wrapper for Transform_SetPosition
int lua_Transform_SetPosition(lua_State *L) {
  // Argument 1: Entity ID (or userdata)
  // Argument 2,3,4: x, y, z

  // Transform* t = ... get transform ...
  float x = lua_tonumber(L, 2);
  float y = lua_tonumber(L, 3);
  float z = lua_tonumber(L, 4);

  // transform_set_position(t, (vec3){x,y,z});

  return 0; // 0 return values
}

// Internal wrapper for Audio_PlaySound
int lua_Audio_PlaySound(lua_State *L) {
  const char *name = lua_tostring(L, 1);
  // audio_play_sound(name);
  return 0;
}

void script_register_bindings(lua_State *L) {
  lua_register(L, "SetPosition", lua_Transform_SetPosition);
  lua_register(L, "PlaySound", lua_Audio_PlaySound);
  // ... 1000s more ...
}
