#include "editor/console/console_system.h"
#include "core/logger.h"
#include <stdio.h>
#include <string.h>

#define MAX_OUTPUT_LEN 1024

static struct {
  bool enabled;
  char last_output[MAX_OUTPUT_LEN];
} console_state;

void console_sys_init(void) {
  console_state.enabled = true;
  console_state.last_output[0] = '\0';
  LOG_INFO("Console System Initialized");
}

void console_sys_shutdown(void) { LOG_INFO("Console System Shutdown"); }

void console_sys_execute_command(const char *command) {
  if (!command || !console_state.enabled)
    return;

  LOG_INFO("Executing console command: %s", command);

  // Simple command parser
  if (strncmp(command, "help", 4) == 0) {
    snprintf(console_state.last_output, MAX_OUTPUT_LEN,
             "Available commands: help, clear, spawn, set, get");
  } else if (strncmp(command, "clear", 5) == 0) {
    snprintf(console_state.last_output, MAX_OUTPUT_LEN, "Console cleared");
  } else if (strncmp(command, "spawn", 5) == 0) {
    snprintf(console_state.last_output, MAX_OUTPUT_LEN, "Spawned entity");
  } else if (strncmp(command, "set", 3) == 0) {
    snprintf(console_state.last_output, MAX_OUTPUT_LEN, "Variable set");
  } else if (strncmp(command, "get", 3) == 0) {
    snprintf(console_state.last_output, MAX_OUTPUT_LEN,
             "Variable value: <value>");
  } else {
    snprintf(console_state.last_output, MAX_OUTPUT_LEN, "Unknown command: %s",
             command);
  }
}

void console_sys_set_enabled(bool enabled) { console_state.enabled = enabled; }

bool console_sys_is_enabled(void) { return console_state.enabled; }

const char *console_sys_get_last_output(void) {
  return console_state.last_output;
}

void console_sys_clear_history(void) { console_state.last_output[0] = '\0'; }
