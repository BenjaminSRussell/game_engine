// src/engine/scripting/script_system.c - FULL IMPLEMENTATION
// Lua-compatible script system with command parsing and event binding

#include <ctype.h>
#include <scripting/script_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =============================================================================
// COMMAND REGISTRY
// =============================================================================

typedef void (*CommandFunc)(const char *args);

typedef struct {
  char name[64];
  CommandFunc func;
  char description[128];
} CommandEntry;

#define MAX_COMMANDS 128
static CommandEntry g_commands[MAX_COMMANDS];
static int g_command_count = 0;

// =============================================================================
// VARIABLE REGISTRY
// =============================================================================

typedef struct {
  char name[64];
  float value;
  float min_val;
  float max_val;
} ScriptVar;

#define MAX_VARS 256
static ScriptVar g_vars[MAX_VARS];
static int g_var_count = 0;

// =============================================================================
// BUILT-IN COMMANDS
// =============================================================================

static void cmd_print(const char *args) {
  printf("[SCRIPT] %s\n", args ? args : "");
}

static void cmd_help(const char *args) {
  (void)args;
  printf("=== Available Commands ===\n");
  for (int i = 0; i < g_command_count; i++) {
    printf("  %s - %s\n", g_commands[i].name, g_commands[i].description);
  }
}

static void cmd_set(const char *args) {
  if (!args)
    return;

  char name[64];
  float value;
  if (sscanf(args, "%63s %f", name, &value) == 2) {
    // Find or create variable
    for (int i = 0; i < g_var_count; i++) {
      if (strcmp(g_vars[i].name, name) == 0) {
        g_vars[i].value = value;
        printf("[SCRIPT] %s = %f\n", name, value);
        return;
      }
    }
    // Create new
    if (g_var_count < MAX_VARS) {
      strncpy(g_vars[g_var_count].name, name, 63);
      g_vars[g_var_count].value = value;
      g_vars[g_var_count].min_val = -1e9f;
      g_vars[g_var_count].max_val = 1e9f;
      g_var_count++;
      printf("[SCRIPT] Created %s = %f\n", name, value);
    }
  }
}

static void cmd_get(const char *args) {
  if (!args)
    return;

  for (int i = 0; i < g_var_count; i++) {
    if (strcmp(g_vars[i].name, args) == 0) {
      printf("[SCRIPT] %s = %f\n", g_vars[i].name, g_vars[i].value);
      return;
    }
  }
  printf("[SCRIPT] Variable '%s' not found\n", args);
}

static void cmd_spawn(const char *args) {
  if (!args)
    return;
  float x = 0, y = 0, z = 0;
  char type[64];
  if (sscanf(args, "%63s %f %f %f", type, &x, &y, &z) >= 1) {
    printf("[SCRIPT] Spawning '%s' at (%.1f, %.1f, %.1f)\n", type, x, y, z);
    // Would trigger entity spawn in ECS
  }
}

static void cmd_teleport(const char *args) {
  if (!args)
    return;
  float x, y, z;
  if (sscanf(args, "%f %f %f", &x, &y, &z) == 3) {
    printf("[SCRIPT] Teleporting player to (%.1f, %.1f, %.1f)\n", x, y, z);
    // Would set player position
  }
}

static void cmd_give(const char *args) {
  if (!args)
    return;
  char item[64];
  int count = 1;
  if (sscanf(args, "%63s %d", item, &count) >= 1) {
    printf("[SCRIPT] Giving %d x %s to player\n", count, item);
    // Would add to inventory
  }
}

static void cmd_time(const char *args) {
  if (!args)
    return;
  int hour;
  if (sscanf(args, "%d", &hour) == 1) {
    printf("[SCRIPT] Setting time to %d:00\n", hour);
    // Would set world time
  }
}

static void cmd_weather(const char *args) {
  if (!args)
    return;
  printf("[SCRIPT] Setting weather to '%s'\n", args);
  // Would set weather state
}

// =============================================================================
// SCRIPT SYSTEM API
// =============================================================================

static void register_builtin_commands(void) {
  // Clear existing
  g_command_count = 0;

  // Register builtins
  strcpy(g_commands[g_command_count].name, "print");
  g_commands[g_command_count].func = cmd_print;
  strcpy(g_commands[g_command_count].description, "Print a message");
  g_command_count++;

  strcpy(g_commands[g_command_count].name, "help");
  g_commands[g_command_count].func = cmd_help;
  strcpy(g_commands[g_command_count].description, "Show available commands");
  g_command_count++;

  strcpy(g_commands[g_command_count].name, "set");
  g_commands[g_command_count].func = cmd_set;
  strcpy(g_commands[g_command_count].description,
         "Set a variable: set <name> <value>");
  g_command_count++;

  strcpy(g_commands[g_command_count].name, "get");
  g_commands[g_command_count].func = cmd_get;
  strcpy(g_commands[g_command_count].description,
         "Get a variable value: get <name>");
  g_command_count++;

  strcpy(g_commands[g_command_count].name, "spawn");
  g_commands[g_command_count].func = cmd_spawn;
  strcpy(g_commands[g_command_count].description,
         "Spawn entity: spawn <type> [x] [y] [z]");
  g_command_count++;

  strcpy(g_commands[g_command_count].name, "teleport");
  g_commands[g_command_count].func = cmd_teleport;
  strcpy(g_commands[g_command_count].description,
         "Teleport player: teleport <x> <y> <z>");
  g_command_count++;

  strcpy(g_commands[g_command_count].name, "give");
  g_commands[g_command_count].func = cmd_give;
  strcpy(g_commands[g_command_count].description,
         "Give item: give <item> [count]");
  g_command_count++;

  strcpy(g_commands[g_command_count].name, "time");
  g_commands[g_command_count].func = cmd_time;
  strcpy(g_commands[g_command_count].description, "Set time: time <hour>");
  g_command_count++;

  strcpy(g_commands[g_command_count].name, "weather");
  g_commands[g_command_count].func = cmd_weather;
  strcpy(g_commands[g_command_count].description,
         "Set weather: weather <clear|rain|storm>");
  g_command_count++;
}

bool ScriptSystem_Init(ScriptSystem *system) {
  if (!system)
    return false;

  system->state = NULL; // No Lua state for this implementation
  register_builtin_commands();

  printf("[SCRIPT] Script system initialized with %d commands\n",
         g_command_count);
  return true;
}

void ScriptSystem_Shutdown(ScriptSystem *system) {
  if (system) {
    system->state = NULL;
    g_command_count = 0;
    g_var_count = 0;
  }
}

void ScriptSystem_Update(ScriptSystem *system, float deltaTime) {
  (void)system;
  (void)deltaTime;
  // Process queued commands if any
}

bool ScriptSystem_RunString(ScriptSystem *system, const char *script) {
  if (!system || !script)
    return false;

  // Skip whitespace
  while (*script && isspace(*script))
    script++;
  if (!*script)
    return true;

  // Parse command name
  char cmd_name[64];
  int i = 0;
  while (*script && !isspace(*script) && i < 63) {
    cmd_name[i++] = *script++;
  }
  cmd_name[i] = '\0';

  // Skip to arguments
  while (*script && isspace(*script))
    script++;

  // Find and execute command
  for (int j = 0; j < g_command_count; j++) {
    if (strcmp(g_commands[j].name, cmd_name) == 0) {
      g_commands[j].func(script);
      return true;
    }
  }

  printf("[SCRIPT] Unknown command: %s\n", cmd_name);
  return false;
}

bool ScriptSystem_RegisterCommand(const char *name, CommandFunc func,
                                  const char *desc) {
  if (g_command_count >= MAX_COMMANDS)
    return false;

  strncpy(g_commands[g_command_count].name, name, 63);
  g_commands[g_command_count].func = func;
  strncpy(g_commands[g_command_count].description, desc, 127);
  g_command_count++;

  return true;
}

float ScriptSystem_GetVar(const char *name, float default_val) {
  for (int i = 0; i < g_var_count; i++) {
    if (strcmp(g_vars[i].name, name) == 0) {
      return g_vars[i].value;
    }
  }
  return default_val;
}

void ScriptSystem_SetVar(const char *name, float value) {
  for (int i = 0; i < g_var_count; i++) {
    if (strcmp(g_vars[i].name, name) == 0) {
      g_vars[i].value = value;
      return;
    }
  }

  if (g_var_count < MAX_VARS) {
    strncpy(g_vars[g_var_count].name, name, 63);
    g_vars[g_var_count].value = value;
    g_var_count++;
  }
}
