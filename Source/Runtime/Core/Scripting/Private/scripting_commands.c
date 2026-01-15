#include "scripting_types.h"
#include "unified_logger.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * COMMAND SYSTEM IMPLEMENTATION
 * ============================================================================
 */

// Built-in command implementations
static void cmd_help(int argc, char **argv) {
  (void)argc;
  (void)argv;
  // Will be implemented with console integration
}

static void cmd_print(int argc, char **argv) {
  if (argc < 2)
    return;
  // Print message (would use console_print)
  LOG_INFO(LOG_CAT_GENERAL, "%s", argv[1]);
}

static void cmd_set(int argc, char **argv) {
  if (argc < 3)
    return;
  // Parse variable name and value
  char *name = argv[1];
  float value = (float)atof(argv[2]);
  // Would call script_set_var
  (void)name;
  (void)value;
}

static void cmd_get(int argc, char **argv) {
  if (argc < 2)
    return;
  // Get variable value
  char *name = argv[1];
  // Would call script_get_var
  (void)name;
}

void scripting_register_builtin_commands(ScriptContext *context) {
  if (!context)
    return;

  script_register_command(context, "help", cmd_help, "Show available commands");
  script_register_command(context, "print", cmd_print, "Print a message");
  script_register_command(context, "set", cmd_set,
                          "Set a variable: set <name> <value>");
  script_register_command(context, "get", cmd_get,
                          "Get a variable: get <name>");
}

bool script_register_command(ScriptContext *context, const char *name,
                             ScriptCommandCallback callback,
                             const char *description) {
  if (!context || !name || !callback)
    return false;
  if (context->command_count >= MAX_COMMANDS) {
    LOG_ERROR(LOG_CAT_GENERAL, "Command limit reached");
    return false;
  }

  CommandEntry *cmd = &context->commands[context->command_count++];
  strncpy(cmd->name, name, sizeof(cmd->name) - 1);
  cmd->callback = callback;
  if (description) {
    strncpy(cmd->description, description, sizeof(cmd->description) - 1);
  }

  return true;
}

bool script_execute(ScriptContext *context, const char *script) {
  if (!context || !script)
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

  // Parse arguments
  char *argv[32];
  int argc = 0;
  argv[argc++] = cmd_name;

  // Skip to arguments
  while (*script && isspace(*script))
    script++;

  // Simple argument parsing (space-separated)
  char arg_buffer[1024];
  strncpy(arg_buffer, script, sizeof(arg_buffer) - 1);
  arg_buffer[sizeof(arg_buffer) - 1] = '\0';

  char *token = strtok(arg_buffer, " ");
  while (token && argc < 32) {
    argv[argc++] = token;
    token = strtok(NULL, " ");
  }

  // Find and execute command
  for (uint32_t j = 0; j < context->command_count; j++) {
    if (strcmp(context->commands[j].name, cmd_name) == 0) {
      context->commands[j].callback(argc, argv);
      return true;
    }
  }

  LOG_ERROR(LOG_CAT_GENERAL, "Unknown command: %s", cmd_name);
  return false;
}
