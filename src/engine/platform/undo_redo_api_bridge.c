// Undo/Redo API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/editor/undo/command_history.h"
#include "../include/platform/undo_redo_api_bridge.h"
#include <string.h>
#include <time.h>

// Global state
static uint32_t g_max_stack_size = 100;

bool undo_can_undo(void) { return command_history_can_undo(); }

bool undo_can_redo(void) { return command_history_can_redo(); }

void undo_execute(void) {
  if (undo_can_undo()) {
    const char *desc = undo_get_description();
    command_history_undo();
    LOG_INFO("Undo: %s", desc ? desc : "Unknown");
  }
}

void redo_execute(void) {
  if (undo_can_redo()) {
    const char *desc = redo_get_description();
    command_history_redo();
    LOG_INFO("Redo: %s", desc ? desc : "Unknown");
  }
}

const char *undo_get_description(void) {
  return command_history_get_undo_description();
}

const char *redo_get_description(void) {
  return command_history_get_redo_description();
}

void undo_begin_group(const char *description) {
  command_history_begin_group(description);
  LOG_INFO("Begin command group: %s", description);
}

void undo_end_group(void) {
  command_history_end_group();
  LOG_INFO("End command group");
}

uint32_t undo_get_stack_size(void) { return command_history_get_undo_count(); }

uint32_t redo_get_stack_size(void) { return command_history_get_redo_count(); }

void undo_clear_history(void) {
  command_history_clear();
  LOG_INFO("Cleared undo/redo history");
}

void undo_set_max_stack_size(uint32_t size) {
  g_max_stack_size = size;
  command_history_set_max_size(size);
  LOG_INFO("Max undo stack size: %u", size);
}

uint32_t undo_get_max_stack_size(void) { return g_max_stack_size; }

uint32_t undo_get_history(UndoHistoryEntry *entries, uint32_t max_count) {
  if (!entries || max_count == 0)
    return 0;

  CommandHistoryInfo *history = command_history_get_undo_list();
  if (!history)
    return 0;

  uint32_t count = 0;
  for (uint32_t i = 0; i < max_count && history[i].description[0] != '\0';
       i++) {
    strncpy(entries[i].description, history[i].description,
            sizeof(entries[i].description) - 1);
    entries[i].timestamp = history[i].timestamp;
    entries[i].is_group = history[i].is_group;
    entries[i].group_size = history[i].group_size;
    count++;
  }

  return count;
}

uint32_t redo_get_history(UndoHistoryEntry *entries, uint32_t max_count) {
  if (!entries || max_count == 0)
    return 0;

  CommandHistoryInfo *history = command_history_get_redo_list();
  if (!history)
    return 0;

  uint32_t count = 0;
  for (uint32_t i = 0; i < max_count && history[i].description[0] != '\0';
       i++) {
    strncpy(entries[i].description, history[i].description,
            sizeof(entries[i].description) - 1);
    entries[i].timestamp = history[i].timestamp;
    entries[i].is_group = history[i].is_group;
    entries[i].group_size = history[i].group_size;
    count++;
  }

  return count;
}
