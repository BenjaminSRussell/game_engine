/**
 * =================================================================================================
 *                          LUA JIT BRIDGE
 * =================================================================================================
 */

#include "scripting/lua_jit_bridge.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Stub Lua state type
typedef struct lua_State { int dummy; } lua_State;

void lua_bridge_init(LuaBridge *bridge) {
    // In real implementation: bridge->L = luaL_newstate();
    bridge->L = malloc(sizeof(lua_State));
    bridge->is_initialized = true;
    bridge->watched_files = NULL;
    bridge->watch_count = 0;
    
    // Load standard libraries
    // luaL_openlibs(bridge->L);
}

void lua_bridge_destroy(LuaBridge *bridge) {
    if (bridge->L) {
        // lua_close(bridge->L);
        free(bridge->L);
    }
    for (int i = 0; i < bridge->watch_count; i++) {
        free(bridge->watched_files[i]);
    }
    free(bridge->watched_files);
}

void lua_bind_function(LuaBridge *bridge, const char *name, void *func) {
    // Register C function to Lua
    // lua_register(bridge->L, name, (lua_CFunction)func);
}

void lua_bind_class(LuaBridge *bridge, const char *class_name) {
    // Create metatable for class
    // luaL_newmetatable(bridge->L, class_name);
    
    // Set up __index to point to itself
    // lua_pushvalue(bridge->L, -1);
    // lua_setfield(bridge->L, -2, "__index");
}

void lua_add_class_method(LuaBridge *bridge, const char *class_name, const char *method_name, void *func) {
    // Get metatable
    // luaL_getmetatable(bridge->L, class_name);
    
    // Register method
    // lua_pushstring(bridge->L, method_name);
    // lua_pushcfunction(bridge->L, (lua_CFunction)func);
    // lua_settable(bridge->L, -3);
    
    // lua_pop(bridge->L, 1);
}

bool lua_execute_file(LuaBridge *bridge, const char *filepath) {
    // Load and execute file
    // if (luaL_loadfile(bridge->L, filepath) != LUA_OK) {
    //     const char *error = lua_tostring(bridge->L, -1);
    //     fprintf(stderr, "Lua error: %s\n", error);
    //     lua_pop(bridge->L, 1);
    //     return false;
    // }
    
    // if (lua_pcall(bridge->L, 0, 0, 0) != LUA_OK) {
    //     const char *error = lua_tostring(bridge->L, -1);
    //     fprintf(stderr, "Lua error: %s\n", error);
    //     lua_pop(bridge->L, 1);
    //     return false;
    // }
    
    return true;
}

bool lua_execute_string(LuaBridge *bridge, const char *code) {
    // Similar to file but with luaL_loadstring
    return true;
}

void lua_watch_file(LuaBridge *bridge, const char *filepath) {
    bridge->watch_count++;
    bridge->watched_files = realloc(bridge->watched_files, sizeof(char*) * bridge->watch_count);
    bridge->watched_files[bridge->watch_count - 1] = strdup(filepath);
}

void lua_check_hot_reload(LuaBridge *bridge) {
    // Check file modification times
    // If changed, reload script
    
    for (int i = 0; i < bridge->watch_count; i++) {
        // struct stat file_stat;
        // if (stat(bridge->watched_files[i], &file_stat) == 0) {
        //     // Check if modified since last check
        //     // If yes: lua_execute_file(bridge, bridge->watched_files[i]);
        // }
    }
}

void lua_attach_debugger(LuaBridge *bridge, int port) {
    // Integrate with debugger protocol (e.g., DBGp or custom)
    // Start debug server on specified port
    // Set up breakpoint hooks in Lua
}
