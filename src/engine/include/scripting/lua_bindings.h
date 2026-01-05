#ifndef LUA_BINDINGS_H
#define LUA_BINDINGS_H

#include <vendor/lua/lua.h>

// Register all engine bindings with Lua state
void lua_bindings_register_all(lua_State *L);

// Individual registration functions
void lua_bindings_register_logging(lua_State *L);

#endif // LUA_BINDINGS_H
