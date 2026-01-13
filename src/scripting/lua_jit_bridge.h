#pragma once

#include <stdbool.h>

typedef struct lua_State lua_State;

typedef struct {
    lua_State *L;
    bool is_initialized;
    
    // Hot reload support
    char **watched_files;
    int watch_count;
} LuaBridge;

void lua_bridge_init(LuaBridge *bridge);
void lua_bridge_destroy(LuaBridge *bridge);

// Binding
void lua_bind_function(LuaBridge *bridge, const char *name, void *func);
void lua_bind_class(LuaBridge *bridge, const char *class_name);
void lua_add_class_method(LuaBridge *bridge, const char *class_name, const char *method_name, void *func);

// Script execution
bool lua_execute_file(LuaBridge *bridge, const char *filepath);
bool lua_execute_string(LuaBridge *bridge, const char *code);

// Hot reload
void lua_watch_file(LuaBridge *bridge, const char *filepath);
void lua_check_hot_reload(LuaBridge *bridge);

// Debugger integration
void lua_attach_debugger(LuaBridge *bridge, int port);
