#pragma once

#include <stdbool.h>

typedef void (*CommandFunc)(int argc, const char **argv);

typedef struct {
    const char *name;
    const char *help_text;
    CommandFunc func;
} ConsoleCommand;

typedef struct {
    ConsoleCommand *commands;
    int command_count;
    int capacity;
    
    // History
    char **history;
    int history_count;
    int history_capacity;
    
    // Aliases
    const char **alias_from;
    const char **alias_to;
    int alias_count;
} CommandProcessor;

void command_processor_init(CommandProcessor *proc);
void command_processor_destroy(CommandProcessor *proc);

// Registration
void command_register(CommandProcessor *proc, const char *name, CommandFunc func, const char *help);
void command_alias(CommandProcessor *proc, const char *alias, const char *target);

// Execution
bool command_execute(CommandProcessor *proc, const char *command_line);

// Autocomplete
int command_get_suggestions(CommandProcessor *proc, const char *partial, const char **out_suggestions, int max_suggestions);

// Help
void command_print_help(CommandProcessor *proc, const char *command_name);
