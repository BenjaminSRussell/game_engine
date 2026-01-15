#include "console.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define MAX_COMMANDS 128
#define MAX_CMD_NAME 32

typedef struct {
    char name[MAX_CMD_NAME];
    ConsoleCommandFunc func;
    char help[128];
} ConsoleCommand;

static ConsoleCommand g_Commands[MAX_COMMANDS];
static int g_CommandCount = 0;
static bool g_ConsoleVisible = false;

void Console_Init(void) {
    printf("[Console] Initialized\n");
    g_CommandCount = 0;
}

void Console_Shutdown(void) {
    printf("[Console] Shutdown\n");
}

void Console_RegisterCommand(const char* name, ConsoleCommandFunc func, const char* help) {
    if (g_CommandCount >= MAX_COMMANDS) {
        printf("[Console] Error: Max commands reached\n");
        return;
    }

    strncpy(g_Commands[g_CommandCount].name, name, MAX_CMD_NAME - 1);
    g_Commands[g_CommandCount].func = func;
    strncpy(g_Commands[g_CommandCount].help, help ? help : "", 127);
    g_CommandCount++;

    // printf("[Console] Registered command: %s\n", name);
}

void Console_Execute(const char* command_line) {
    if (!command_line || strlen(command_line) == 0) return;

    printf("[Console] Executing: %s\n", command_line);

    // Parse arguments (simple implementation)
    char buffer[256];
    strncpy(buffer, command_line, 255);
    buffer[255] = '\0';

    char* argv[16];
    int argc = 0;

    char* token = strtok(buffer, " ");
    while (token != NULL && argc < 16) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }

    if (argc == 0) return;

    // Find command
    for (int i = 0; i < g_CommandCount; ++i) {
        if (strcmp(g_Commands[i].name, argv[0]) == 0) {
            g_Commands[i].func(argc, argv);
            return;
        }
    }

    printf("[Console] Unknown command: %s\n", argv[0]);
}

void Console_Log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("[Console] LOG: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

void Console_Toggle(void) {
    g_ConsoleVisible = !g_ConsoleVisible;
    printf("[Console] Visible: %s\n", g_ConsoleVisible ? "true" : "false");
}

bool Console_IsVisible(void) {
    return g_ConsoleVisible;
}
