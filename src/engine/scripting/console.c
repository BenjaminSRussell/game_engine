#include "console.h"
#include "engine/include/core/logger.h"
#include <core/memory.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// If ImGui is enabled, include headers
#ifdef ENABLE_IMGUI
#include <imgui.h>
#endif

#define MAX_COMMANDS 256
#define MAX_HISTORY 50
#define MAX_BUFFER_SIZE 1024

typedef struct {
    char name[64];
    ConsoleCommandCallback callback;
    char help[128];
} ConsoleCommand;

typedef struct {
    char text[MAX_BUFFER_SIZE];
    bool is_error;
} ConsoleLogEntry;

typedef struct {
    ConsoleCommand commands[MAX_COMMANDS];
    u32 command_count;

    ConsoleLogEntry log_history[MAX_HISTORY];
    u32 log_count;
    u32 log_head; // Ring buffer head

    bool is_visible;
    char input_buffer[MAX_BUFFER_SIZE];
} ConsoleState;

static ConsoleState g_console = {0};

// Forward declaration
static void console_help_command(int argc, char** argv);

void console_init(void) {
    memset(&g_console, 0, sizeof(ConsoleState));
    g_console.is_visible = false; // Hidden by default

    // Register built-in commands
    console_register_command("help", console_help_command, "List all commands");
    console_register_command("clear", (ConsoleCommandCallback)console_clear, "Clear console output");

    LOG_INFO("Developer Console initialized");
}

void console_shutdown(void) {
    // Cleanup if needed
}

void console_update(void) {
    // Logic to toggle console visibility would typically go here
}

void console_render(void) {
    if (!g_console.is_visible) return;

#ifdef ENABLE_IMGUI
    if (ImGui::Begin("Console", &g_console.is_visible)) {
        // Output Region
        if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -30), false, ImGuiWindowFlags_HorizontalScrollbar)) {
            for (u32 i = 0; i < g_console.log_count; i++) {
                // Handle ring buffer logic if fully implemented,
                // for simplicity assume linear history up to MAX_HISTORY for display
                if (g_console.log_history[i].text[0]) {
                     if (g_console.log_history[i].is_error)
                         ImGui::TextColored(ImVec4(1,0,0,1), "%s", g_console.log_history[i].text);
                     else
                         ImGui::Text("%s", g_console.log_history[i].text);
                }
            }
        }
        ImGui::EndChild();

        // Input Region
        if (ImGui::InputText("Input", g_console.input_buffer, MAX_BUFFER_SIZE, ImGuiInputTextFlags_EnterReturnsTrue)) {
            console_execute(g_console.input_buffer);
            g_console.input_buffer[0] = '\0';
            ImGui::SetKeyboardFocusHere(-1); // Auto focus back
        }
    }
    ImGui::End();
#endif
}

void console_register_command(const char* name, ConsoleCommandCallback callback, const char* help) {
    if (g_console.command_count >= MAX_COMMANDS) {
        LOG_ERROR("Console command limit reached");
        return;
    }

    ConsoleCommand* cmd = &g_console.commands[g_console.command_count++];
    strncpy(cmd->name, name, sizeof(cmd->name) - 1);
    cmd->callback = callback;
    if (help) strncpy(cmd->help, help, sizeof(cmd->help) - 1);
}

void console_execute(const char* command_line) {
    if (!command_line || strlen(command_line) == 0) return;

    console_print(command_line); // Echo command

    // Parse arguments
    char buffer[MAX_BUFFER_SIZE];
    strncpy(buffer, command_line, MAX_BUFFER_SIZE - 1);

    char* argv[32];
    int argc = 0;

    char* token = strtok(buffer, " ");
    while (token && argc < 32) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }

    if (argc == 0) return;

    // Find command
    for (u32 i = 0; i < g_console.command_count; i++) {
        if (strcmp(g_console.commands[i].name, argv[0]) == 0) {
            if (g_console.commands[i].callback) {
                g_console.commands[i].callback(argc, argv);
            }
            return;
        }
    }

    console_print_error("Unknown command");
}

void console_print(const char* text) {
    // Add to log history
    if (g_console.log_count < MAX_HISTORY) {
        strncpy(g_console.log_history[g_console.log_count].text, text, MAX_BUFFER_SIZE - 1);
        g_console.log_history[g_console.log_count].is_error = false;
        g_console.log_count++;
    } else {
        // Shift history (simple implementation)
        for(u32 i=0; i<MAX_HISTORY-1; ++i) {
            g_console.log_history[i] = g_console.log_history[i+1];
        }
        strncpy(g_console.log_history[MAX_HISTORY-1].text, text, MAX_BUFFER_SIZE - 1);
        g_console.log_history[MAX_HISTORY-1].is_error = false;
    }

    LOG_INFO("[Console] %s", text);
}

void console_print_error(const char* text) {
    if (g_console.log_count < MAX_HISTORY) {
        strncpy(g_console.log_history[g_console.log_count].text, text, MAX_BUFFER_SIZE - 1);
        g_console.log_history[g_console.log_count].is_error = true;
        g_console.log_count++;
    } else {
        for(u32 i=0; i<MAX_HISTORY-1; ++i) {
            g_console.log_history[i] = g_console.log_history[i+1];
        }
        strncpy(g_console.log_history[MAX_HISTORY-1].text, text, MAX_BUFFER_SIZE - 1);
        g_console.log_history[MAX_HISTORY-1].is_error = true;
    }

    LOG_ERROR("[Console] %s", text);
}

void console_clear(void) {
    memset(g_console.log_history, 0, sizeof(g_console.log_history));
    g_console.log_count = 0;
}

bool console_is_visible(void) {
    return g_console.is_visible;
}

void console_toggle(void) {
    g_console.is_visible = !g_console.is_visible;
}

static void console_help_command(int argc, char** argv) {
    console_print("Available commands:");
    for (u32 i = 0; i < g_console.command_count; i++) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "  %s - %s", g_console.commands[i].name, g_console.commands[i].help);
        console_print(buffer);
    }
}
