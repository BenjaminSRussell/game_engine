#include "platform/console_api_bridge.h"
#include "core/logger.h"
#include "editor/console/console_system.h"

void console_execute_command(const char *command) {
  console_sys_execute_command(command);
}

void console_set_enabled(bool enabled) { console_sys_set_enabled(enabled); }

bool console_is_enabled(void) { return console_sys_is_enabled(); }

const char *console_get_last_output(void) {
  return console_sys_get_last_output();
}

void console_clear_history(void) { console_sys_clear_history(); }
