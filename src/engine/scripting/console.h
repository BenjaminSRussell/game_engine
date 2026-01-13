#ifndef CONSOLE_H
#define CONSOLE_H

#include <core/types.h>

// Command callback
typedef void (*ConsoleCommandCallback)(int argc, char** argv);

void console_init(void);
void console_shutdown(void);
void console_update(void);
void console_render(void);

// API
void console_register_command(const char* name, ConsoleCommandCallback callback, const char* help);
void console_execute(const char* command_line);
void console_print(const char* text);
void console_print_error(const char* text);
void console_clear(void);
bool console_is_visible(void);
void console_toggle(void);

#endif // CONSOLE_H
