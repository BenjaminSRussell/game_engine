#include "../include/editor/undo/command_history.h"
#include <core/logger.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_COMMAND_STACK 128

typedef struct {
    char name[64];
    char description[128];
    void *(*execute)(void *data);
    void (*undo)(void *data);
    void *data;
    size_t data_size;
    uint64_t timestamp;
    bool is_group;
    uint32_t group_size;
} Command;

typedef struct {
    Command undo_stack[MAX_COMMAND_STACK];
    Command redo_stack[MAX_COMMAND_STACK];
    int undo_top;
    int redo_top;
    uint32_t max_size;
    
    // Grouping
    bool grouping_active;
    char group_description[128];
    uint32_t current_group_size;
} CommandHistory;

static CommandHistory g_history = { 
    .undo_top = -1, 
    .redo_top = -1,
    .max_size = MAX_COMMAND_STACK
};

// Internal helper for timestamp
static uint64_t get_timestamp(void) {
    return (uint64_t)time(NULL);
}

void command_history_push(const char *name, void *(*execute)(void*), void (*undo)(void*), void *data, size_t data_size) {
    if (g_history.undo_top >= (int)g_history.max_size - 1) {
        // Simple shift logic could be added here, for now just replace oldest if full (circular buffer implementation ideal but keeping simple)
        // Actually, just warn if full for now to match previous behavior
        LOG_WARN("Command history full");
        return;
    }
    
    // If we're not grouping, clear redo stack
    // If grouping, we might be pushing simplified commands, but usually redo is cleared on new action
    if (!g_history.grouping_active) {
        g_history.redo_top = -1;
    }
    
    g_history.undo_top++;
    Command *cmd = &g_history.undo_stack[g_history.undo_top];
    
    strncpy(cmd->name, name, sizeof(cmd->name) - 1);
    if (g_history.grouping_active) {
        strncpy(cmd->description, g_history.group_description, sizeof(cmd->description) - 1);
        cmd->is_group = true; // Part of a group
        g_history.current_group_size++;
    } else {
        strncpy(cmd->description, name, sizeof(cmd->description) - 1);
        cmd->is_group = false;
        cmd->group_size = 0;
    }
    
    cmd->execute = execute;
    cmd->undo = undo;
    cmd->data_size = data_size;
    cmd->timestamp = get_timestamp();
    
    if (data_size > 0 && data) {
        cmd->data = malloc(data_size);
        if (cmd->data) {
            memcpy(cmd->data, data, data_size);
        }
    } else {
        cmd->data = NULL;
    }
    
    LOG_INFO("Command Pushed: %s", name);
}

void command_history_undo(void) {
    if (g_history.undo_top < 0) return;
    
    // Handle grouping: if current command is part of a group, undo until group start
    // Note: This basic implementation doesn't fully track groups yet, just single commands
    
    Command *cmd = &g_history.undo_stack[g_history.undo_top];
    if (cmd->undo) {
        cmd->undo(cmd->data);
    }
    
    // Move to redo
    g_history.redo_top++;
    if (g_history.redo_top < MAX_COMMAND_STACK) {
        g_history.redo_stack[g_history.redo_top] = *cmd; 
        // Transfer data ownership
        // cmd in undo stack is now "empty" regarding data ownership, but we don't free it yet to avoid double free if we push back
        // But physically moving data pointer is safer.
    }
    
    g_history.undo_top--;
    LOG_INFO("Undo: %s", cmd->name);
}

void command_history_redo(void) {
    if (g_history.redo_top < 0) return;
    
    Command *cmd = &g_history.redo_stack[g_history.redo_top];
    if (cmd->execute) {
        cmd->execute(cmd->data);
    }
    
    // Move back to undo
    g_history.undo_top++;
    if (g_history.undo_top < MAX_COMMAND_STACK) {
        g_history.undo_stack[g_history.undo_top] = *cmd;
    }
    
    g_history.redo_top--;
    LOG_INFO("Redo: %s", cmd->name);
}

bool command_history_can_undo(void) {
    return g_history.undo_top >= 0;
}

bool command_history_can_redo(void) {
    return g_history.redo_top >= 0;
}

const char* command_history_get_undo_description(void) {
    if (g_history.undo_top >= 0) {
        return g_history.undo_stack[g_history.undo_top].description;
    }
    return NULL;
}

const char* command_history_get_redo_description(void) {
    if (g_history.redo_top >= 0) {
        return g_history.redo_stack[g_history.redo_top].description;
    }
    return NULL;
}

uint32_t command_history_get_undo_count(void) {
    return (uint32_t)(g_history.undo_top + 1);
}

uint32_t command_history_get_redo_count(void) {
    return (uint32_t)(g_history.redo_top + 1);
}

void command_history_clear(void) {
    // Free data
    for (int i = 0; i <= g_history.undo_top; i++) {
        if (g_history.undo_stack[i].data) free(g_history.undo_stack[i].data);
    }
    for (int i = 0; i <= g_history.redo_top; i++) {
        if (g_history.redo_stack[i].data) free(g_history.redo_stack[i].data);
    }
    
    g_history.undo_top = -1;
    g_history.redo_top = -1;
}

void command_history_set_max_size(uint32_t size) {
    if (size > MAX_COMMAND_STACK) size = MAX_COMMAND_STACK;
    g_history.max_size = size;
}

// These return static buffers for bridge usage - not thread safe but okay for now
static CommandHistoryInfo s_undo_list[MAX_COMMAND_STACK];
static CommandHistoryInfo s_redo_list[MAX_COMMAND_STACK];

CommandHistoryInfo* command_history_get_undo_list(void) {
    for (int i = 0; i <= g_history.undo_top; i++) {
        strncpy(s_undo_list[i].description, g_history.undo_stack[i].description, 127);
        s_undo_list[i].timestamp = g_history.undo_stack[i].timestamp;
        s_undo_list[i].is_group = g_history.undo_stack[i].is_group;
        s_undo_list[i].group_size = g_history.undo_stack[i].group_size;
    }
    // Null terminate list logic (by empty description or count check by caller)
    if (g_history.undo_top + 1 < MAX_COMMAND_STACK) {
        s_undo_list[g_history.undo_top + 1].description[0] = '\0';
    }
    return s_undo_list;
}

CommandHistoryInfo* command_history_get_redo_list(void) {
    for (int i = 0; i <= g_history.redo_top; i++) {
        strncpy(s_redo_list[i].description, g_history.redo_stack[i].description, 127);
        s_redo_list[i].timestamp = g_history.redo_stack[i].timestamp;
        s_redo_list[i].is_group = g_history.redo_stack[i].is_group;
        s_redo_list[i].group_size = g_history.redo_stack[i].group_size;
    }
    if (g_history.redo_top + 1 < MAX_COMMAND_STACK) {
        s_redo_list[g_history.redo_top + 1].description[0] = '\0';
    }
    return s_redo_list;
}

void command_history_begin_group(const char* description) {
    g_history.grouping_active = true;
    if (description) {
        strncpy(g_history.group_description, description, sizeof(g_history.group_description) - 1);
    } else {
        strcpy(g_history.group_description, "Group");
    }
    g_history.current_group_size = 0;
}

void command_history_end_group(void) {
    g_history.grouping_active = false;
    // Here we would ideally coalesce commands or mark the last one as end of group
    // For now, simpler implementation
}
