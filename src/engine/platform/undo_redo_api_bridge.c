// Undo/Redo API Bridge Implementation
// Stubs to unblock build

#include <platform/undo_redo_api_bridge.h>
#include <string.h>

bool undo_can_undo(void) { return false; }

bool undo_can_redo(void) { return false; }

void undo_execute(void) {}

void redo_execute(void) {}

const char *undo_get_description(void) { return ""; }

const char *redo_get_description(void) { return ""; }

void undo_begin_group(const char *description) {
  (void)description;
}

void undo_end_group(void) {}

uint32_t undo_get_stack_size(void) { return 0; }

uint32_t redo_get_stack_size(void) { return 0; }

void undo_clear_history(void) {}

void undo_set_max_stack_size(uint32_t size) { (void)size; }

uint32_t undo_get_max_stack_size(void) { return 0; }

uint32_t undo_get_history(UndoHistoryEntry *entries, uint32_t max_count) {
  (void)entries;
  (void)max_count;
  return 0;
}

uint32_t redo_get_history(UndoHistoryEntry *entries, uint32_t max_count) {
  (void)entries;
  (void)max_count;
  return 0;
}
