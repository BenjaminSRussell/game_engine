#ifndef ENGINE_EDITOR_PLACEHOLDER_TYPES_H
#define ENGINE_EDITOR_PLACEHOLDER_TYPES_H

// PHASE 1: Placeholder types for existing editor code
// These will be properly implemented in Phase 7 (Command System)
// For now, this allows compilation to succeed

#ifdef WITH_EDITOR

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Viewport constants
typedef enum {
    VIEWPORT_PERSPECTIVE,
    VIEWPORT_TOP,
    VIEWPORT_FRONT,
    VIEWPORT_SIDE,
    VIEWPORT_COUNT
} ViewportType;

#define MAX_UNDO_COMMANDS 256

// Forward declarations for command system (Phase 7 implementation)
typedef struct Command Command;
typedef struct UndoRedoSystem UndoRedoSystem;

// Minimal Command interface (placeholder)
struct Command {
    void (*execute)(Command *self);
    void (*undo)(Command *self);
    void *data;  // Command-specific data
    const char *description;
};

// Minimal UndoRedoSystem (placeholder)
struct UndoRedoSystem {
    Command *commands[MAX_UNDO_COMMANDS];
    size_t command_count;
    int current_index;
};

#endif // WITH_EDITOR

#endif // ENGINE_EDITOR_PLACEHOLDER_TYPES_H
