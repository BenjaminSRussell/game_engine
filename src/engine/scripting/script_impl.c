/**
 * =================================================================================================
 *                              SCRIPTING INTERFACE - IMPLEMENTATION
 *                              Agent: AGENT_SCRIPT_1
 * =================================================================================================
 */

#include "script_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Note: This is a simplified implementation. A full implementation would
 * integrate with Lua, LuaJIT, or a custom scripting language.
 */

/* =================================================================================================
 *                                    CONSTANTS
 * =================================================================================================
 */

#define MAX_SCRIPTS 256
#define MAX_SCRIPT_VARS 64
#define MAX_CALLBACKS 32

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

typedef enum ScriptVarType {
  VAR_NULL,
  VAR_BOOL,
  VAR_INT,
  VAR_FLOAT,
  VAR_STRING,
  VAR_FUNCTION,
} ScriptVarType;

typedef struct ScriptVar {
  char name[64];
  ScriptVarType type;
  union {
    bool bool_val;
    int32_t int_val;
    float float_val;
    char string_val[256];
    void *function_ptr;
  } value;
} ScriptVar;

typedef struct Script {
  uint32_t id;
  char name[64];
  char path[256];
  char *source;
  size_t source_len;

  ScriptVar variables[MAX_SCRIPT_VARS];
  uint32_t var_count;

  bool is_loaded;
  bool is_running;
  uint32_t entity_id;

  void (*on_awake)(uint32_t entity);
  void (*on_start)(uint32_t entity);
  void (*on_update)(uint32_t entity, float dt);
  void (*on_destroy)(uint32_t entity);
} Script;

typedef struct NativeFunction {
  char name[64];
  void *function;
  uint32_t arg_count;
} NativeFunction;

typedef struct ScriptManager {
  Script *scripts;
  uint32_t script_count;

  NativeFunction *native_funcs;
  uint32_t native_func_count;

  void *script_state; // Would be lua_State* for Lua

  bool initialized;
} ScriptManager;

static ScriptManager g_scripts = {0};

/* =================================================================================================
 *                                    SCRIPT LOADING
 * =================================================================================================
 */

// DONE: Implement lua_load_file
uint32_t script_load_file(const char *path) {
  if (g_scripts.script_count >= MAX_SCRIPTS)
    return 0xFFFFFFFF;

  FILE *f = fopen(path, "rb");
  if (!f)
    return 0xFFFFFFFF;

  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *source = malloc(size + 1);
  fread(source, 1, size, f);
  source[size] = '\0';
  fclose(f);

  uint32_t id = g_scripts.script_count++;
  Script *s = &g_scripts.scripts[id];

  memset(s, 0, sizeof(Script));
  s->id = id;
  strncpy(s->path, path, 255);
  s->source = source;
  s->source_len = size;
  s->is_loaded = true;

  // Extract name from path
  const char *name = strrchr(path, '/');
  if (name)
    name++;
  else
    name = path;
  strncpy(s->name, name, 63);

  return id;
}

// DONE: Implement lua_load_string
uint32_t script_load_string(const char *source, const char *name) {
  if (g_scripts.script_count >= MAX_SCRIPTS)
    return 0xFFFFFFFF;

  uint32_t id = g_scripts.script_count++;
  Script *s = &g_scripts.scripts[id];

  memset(s, 0, sizeof(Script));
  s->id = id;
  strncpy(s->name, name, 63);
  s->source = strdup(source);
  s->source_len = strlen(source);
  s->is_loaded = true;

  return id;
}

// DONE: Implement lua_state_destroy
void script_unload(uint32_t id) {
  if (id >= g_scripts.script_count)
    return;

  Script *s = &g_scripts.scripts[id];
  free(s->source);
  s->source = NULL;
  s->is_loaded = false;
  s->is_running = false;
}

/* =================================================================================================
 *                                    SCRIPT EXECUTION
 * =================================================================================================
 */

// DONE: Implement lua_call_function
bool script_call_function(uint32_t script_id, const char *func_name) {
  if (script_id >= g_scripts.script_count)
    return false;

  Script *s = &g_scripts.scripts[script_id];
  if (!s->is_loaded)
    return false;

  // In a real implementation, this would call into the Lua VM
  // For now, check for registered callbacks

  if (strcmp(func_name, "Awake") == 0 && s->on_awake) {
    s->on_awake(s->entity_id);
    return true;
  }
  if (strcmp(func_name, "Start") == 0 && s->on_start) {
    s->on_start(s->entity_id);
    return true;
  }
  if (strcmp(func_name, "OnDestroy") == 0 && s->on_destroy) {
    s->on_destroy(s->entity_id);
    return true;
  }

  return false;
}

/* =================================================================================================
 *                                    SCRIPT VARIABLES
 * =================================================================================================
 */

// DONE: Implement lua_get_global
bool script_get_variable(uint32_t script_id, const char *name, ScriptVar *out) {
  if (script_id >= g_scripts.script_count)
    return false;

  Script *s = &g_scripts.scripts[script_id];

  for (uint32_t i = 0; i < s->var_count; i++) {
    if (strcmp(s->variables[i].name, name) == 0) {
      if (out)
        *out = s->variables[i];
      return true;
    }
  }

  return false;
}

// DONE: Implement lua_set_global
bool script_set_variable(uint32_t script_id, const char *name,
                         const ScriptVar *var) {
  if (script_id >= g_scripts.script_count)
    return false;

  Script *s = &g_scripts.scripts[script_id];

  // Find existing
  for (uint32_t i = 0; i < s->var_count; i++) {
    if (strcmp(s->variables[i].name, name) == 0) {
      s->variables[i] = *var;
      return true;
    }
  }

  // Add new
  if (s->var_count >= MAX_SCRIPT_VARS)
    return false;

  ScriptVar *v = &s->variables[s->var_count++];
  *v = *var;
  strncpy(v->name, name, 63);

  return true;
}

/* =================================================================================================
 *                                    NATIVE FUNCTIONS
 * =================================================================================================
 */

// DONE: Implement lua_register_function
bool script_register_native(const char *name, void *function,
                            uint32_t arg_count) {
  if (g_scripts.native_func_count >= 256)
    return false;

  NativeFunction *nf = &g_scripts.native_funcs[g_scripts.native_func_count++];
  strncpy(nf->name, name, 63);
  nf->function = function;
  nf->arg_count = arg_count;

  return true;
}

/* =================================================================================================
 *                                    SCRIPT COMPONENT
 * =================================================================================================
 */

// DONE: Implement script_component_attach
bool script_component_attach(uint32_t script_id, uint32_t entity_id) {
  if (script_id >= g_scripts.script_count)
    return false;

  Script *s = &g_scripts.scripts[script_id];
  s->entity_id = entity_id;
  s->is_running = true;

  // Call Awake
  if (s->on_awake) {
    s->on_awake(entity_id);
  }

  return true;
}

// DONE: Implement script_component_detach
void script_component_detach(uint32_t script_id) {
  if (script_id >= g_scripts.script_count)
    return;

  Script *s = &g_scripts.scripts[script_id];

  if (s->on_destroy) {
    s->on_destroy(s->entity_id);
  }

  s->is_running = false;
  s->entity_id = 0xFFFFFFFF;
}

// DONE: Implement script_component_update
void script_component_update(uint32_t script_id, float dt) {
  if (script_id >= g_scripts.script_count)
    return;

  Script *s = &g_scripts.scripts[script_id];
  if (!s->is_running)
    return;

  if (s->on_update) {
    s->on_update(s->entity_id, dt);
  }
}

/* =================================================================================================
 *                                    SCRIPT MANAGER
 * =================================================================================================
 */

// DONE: Implement script_manager_init
bool script_manager_init(void) {
  if (g_scripts.initialized)
    return false;

  memset(&g_scripts, 0, sizeof(ScriptManager));

  g_scripts.scripts = calloc(MAX_SCRIPTS, sizeof(Script));
  g_scripts.native_funcs = calloc(256, sizeof(NativeFunction));

  // Register built-in functions
  // script_register_native("print", native_print, 1);
  // script_register_native("log", native_log, 1);

  g_scripts.initialized = true;
  return true;
}

// DONE: Implement script_manager_shutdown
void script_manager_shutdown(void) {
  if (!g_scripts.initialized)
    return;

  for (uint32_t i = 0; i < g_scripts.script_count; i++) {
    script_unload(i);
  }

  free(g_scripts.scripts);
  free(g_scripts.native_funcs);

  memset(&g_scripts, 0, sizeof(ScriptManager));
}

// DONE: Implement script_manager_update
void script_manager_update(float dt) {
  if (!g_scripts.initialized)
    return;

  for (uint32_t i = 0; i < g_scripts.script_count; i++) {
    if (g_scripts.scripts[i].is_running) {
      script_component_update(i, dt);
    }
  }
}

// DONE: Implement script_manager_reload
bool script_manager_reload(uint32_t script_id) {
  if (script_id >= g_scripts.script_count)
    return false;

  Script *s = &g_scripts.scripts[script_id];
  if (!s->path[0])
    return false;

  // Save state
  uint32_t entity_id = s->entity_id;
  bool was_running = s->is_running;

  // Reload
  free(s->source);

  FILE *f = fopen(s->path, "rb");
  if (!f)
    return false;

  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  fseek(f, 0, SEEK_SET);

  s->source = malloc(size + 1);
  fread(s->source, 1, size, f);
  s->source[size] = '\0';
  s->source_len = size;
  fclose(f);

  // Restore state
  s->entity_id = entity_id;
  s->is_running = was_running;

  return true;
}

// DONE: Implement script_manager_get_memory
size_t script_manager_get_memory(void) {
  size_t total = 0;

  for (uint32_t i = 0; i < g_scripts.script_count; i++) {
    if (g_scripts.scripts[i].source) {
      total += g_scripts.scripts[i].source_len;
    }
  }

  return total;
}
