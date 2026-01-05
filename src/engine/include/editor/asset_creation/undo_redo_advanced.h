#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct Command Command;

typedef struct Command {
    void (*execute)(Command *cmd);
    void (*undo)(Command *cmd);
    void (*destroy)(Command *cmd);
    void *data; // Command-specific data
    char *description;
} Command;

typedef struct {
    Command **stack;
    int capacity;
    int current_index; // Points to next available slot
    int saved_index;   // Index when last saved
    
    bool in_transaction;
    Command **transaction_buffer;
    int transaction_count;
} UndoRedoSystem;

void undo_system_init(UndoRedoSystem *system, int max_history);
void undo_system_destroy(UndoRedoSystem *system);

// Command execution
void undo_system_execute(UndoRedoSystem *system, Command *cmd);
void undo_system_undo(UndoRedoSystem *system);
void undo_system_redo(UndoRedoSystem *system);

bool undo_system_can_undo(UndoRedoSystem *system);
bool undo_system_can_redo(UndoRedoSystem *system);

// Transaction grouping
void undo_system_begin_transaction(UndoRedoSystem *system);
void undo_system_end_transaction(UndoRedoSystem *system);

// State
bool undo_system_is_dirty(UndoRedoSystem *system);
void undo_system_mark_saved(UndoRedoSystem *system);
