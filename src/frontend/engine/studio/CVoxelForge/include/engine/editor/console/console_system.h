#ifndef CONSOLE_SYSTEM_H
#define CONSOLE_SYSTEM_H

#include <stdbool.h>

void console_sys_init(void);
void console_sys_shutdown(void);

void console_sys_execute_command(const char *command);
void console_sys_set_enabled(bool enabled);
bool console_sys_is_enabled(void);

const char *console_sys_get_last_output(void);
void console_sys_clear_history(void);

#endif // CONSOLE_SYSTEM_H
