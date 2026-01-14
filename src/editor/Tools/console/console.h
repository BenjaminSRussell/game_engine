#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdbool.h>

typedef void (*ConsoleCommandFunc)(int argc, char** argv);

void Console_Init(void);
void Console_Shutdown(void);

// Execute a command string
void Console_Execute(const char* command_line);

// Register a new command
void Console_RegisterCommand(const char* name, ConsoleCommandFunc func, const char* help);

// Log output to console
void Console_Log(const char* fmt, ...);

// Toggle console visibility
void Console_Toggle(void);
bool Console_IsVisible(void);

#endif // CONSOLE_H
