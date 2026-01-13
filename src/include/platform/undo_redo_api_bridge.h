// Undo/Redo API Bridge
// Exposes command history system to VoxelForgeStudio

#ifndef UNDO_REDO_API_BRIDGE_H
#define UNDO_REDO_API_BRIDGE_H

#include "include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Undo/Redo System API
// ============================================================================

/// Check if undo is available
bool undo_can_undo(void);

/// Check if redo is available
bool undo_can_redo(void);

/// Execute undo operation
void undo_execute(void);

/// Execute redo operation
void redo_execute(void);

/// Get description of next undo operation
const char *undo_get_description(void);

/// Get description of next redo operation
const char *redo_get_description(void);

/// Begin a command group (multiple commands treated as one)
void undo_begin_group(const char *description);

/// End current command group
void undo_end_group(void);

/// Get number of commands in undo stack
uint32_t undo_get_stack_size(void);

/// Get number of commands in redo stack
uint32_t redo_get_stack_size(void);

/// Clear all undo/redo history
void undo_clear_history(void);

/// Set maximum undo stack size
void undo_set_max_stack_size(uint32_t size);

/// Get maximum undo stack size
uint32_t undo_get_max_stack_size(void);

/// Command history entry
typedef struct {
  char description[256];
  uint64_t timestamp;
  bool is_group;
  uint32_t group_size;
} UndoHistoryEntry;

/// Get undo history entries
uint32_t undo_get_history(UndoHistoryEntry *entries, uint32_t max_count);

/// Get redo history entries
uint32_t redo_get_history(UndoHistoryEntry *entries, uint32_t max_count);

#ifdef __cplusplus
}
#endif

#endif // UNDO_REDO_API_BRIDGE_H
