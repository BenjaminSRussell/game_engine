#include "../include/editor/asset_creation/undo_redo_advanced.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Internal structure for Composite Commands (Transactions)
typedef struct {
  Command **commands;
  int count;
} CompositeCommandData;

static void composite_execute(Command *cmd) {
  CompositeCommandData *data = (CompositeCommandData *)cmd->data;
  for (int i = 0; i < data->count; ++i) {
    if (data->commands[i]->execute) {
      data->commands[i]->execute(data->commands[i]);
    }
  }
}

static void composite_undo(Command *cmd) {
  CompositeCommandData *data = (CompositeCommandData *)cmd->data;
  // Undo in reverse order
  for (int i = data->count - 1; i >= 0; --i) {
    if (data->commands[i]->undo) {
      data->commands[i]->undo(data->commands[i]);
    }
  }
}

static void composite_destroy(Command *cmd) {
  CompositeCommandData *data = (CompositeCommandData *)cmd->data;
  for (int i = 0; i < data->count; ++i) {
    if (data->commands[i]->destroy) {
      data->commands[i]->destroy(data->commands[i]);
    }
    free(data->commands[i]);
  }
  free(data->commands);
  free(data);
}

// -------------------------------------------------------------------------------------------------
// INITIALIZATION & DESTRUCTION
// -------------------------------------------------------------------------------------------------

void undo_system_init(UndoRedoSystem *system, int max_history) {
  if (!system)
    return;

  system->capacity = max_history > 0 ? max_history : 100;
  system->stack = (Command **)calloc(system->capacity, sizeof(Command *));
  system->current_index = 0;
  system->saved_index = 0;

  // Transaction support
  system->in_transaction = false;
  system->transaction_count = 0;
  // Buffer size equal to history capacity is usually sufficient for a single
  // transaction, but we can resize if needed.
  system->transaction_buffer = (Command **)calloc(64, sizeof(Command *));
}

void undo_system_destroy(UndoRedoSystem *system) {
  if (!system)
    return;

  // Free stack
  for (int i = 0; i < system->current_index; ++i) {
    if (system->stack[i]) {
      if (system->stack[i]->destroy) {
        system->stack[i]->destroy(system->stack[i]);
      }
      free(system->stack[i]);
    }
  }
  free(system->stack);

  // Free transaction buffer (if any pending)
  for (int i = 0; i < system->transaction_count; ++i) {
    if (system->transaction_buffer[i]) {
      if (system->transaction_buffer[i]->destroy) {
        system->transaction_buffer[i]->destroy(system->transaction_buffer[i]);
      }
      free(system->transaction_buffer[i]);
    }
  }
  free(system->transaction_buffer);

  system->capacity = 0;
  system->current_index = 0;
}

// -------------------------------------------------------------------------------------------------
// COMMAND EXECUTION
// -------------------------------------------------------------------------------------------------

void undo_system_execute(UndoRedoSystem *system, Command *cmd) {
  if (!system || !cmd)
    return;

  if (system->in_transaction) {
    // Buffer the command
    if (system->transaction_count < 64) { // Fixed buffer for now
      Command *cmd_copy = (Command *)malloc(sizeof(Command));
      *cmd_copy = *cmd;
      system->transaction_buffer[system->transaction_count++] = cmd_copy;

      // Execute immediately for visual feedback
      if (cmd->execute) {
        cmd->execute(cmd);
      }
    } else {
      printf("UndoSystem Error: Transaction buffer overflow\n");
    }
    return;
  }

  // Normal execution

  // 1. Clear redo history (commands after current index)
  // Note: Since current_index usually points to the "next slot", valid commands
  // are 0..current_index-1. However, if we undid, current_index moved back. We
  // need to clear everything from current_index onwards. Wait, typical stack:
  // [0] [1] [2] ... [current_index] ...
  // If we are at [2] and push new, we overwrite [2] (or insert) and effectively
  // lose [3..N]

  // Actually, usually current_index represents "number of items in undo stack".
  // Or "position of head".
  // Implementation: stack[0...current_index-1] are valid undoable commands.

  /*
     Wait, if we are in the middle of the stack, say capacity 10, used 5, index
     at 3. Indices 0, 1, 2 are active actions. 3, 4 are redoable actions. If we
     execute a NEW command, we must free 3 and 4.
  */

  // NOTE: We don't track "top" separate from "current". In linear undo/redo
  // without branching, executing a new command destroys the redo path.

  // Current implementation assumes standard linear undo/redo.
  // We assume system->capacity is sufficient.
  // If current_index < max count (which is not stored in structs above
  // explicitly other than implicitly via array content? header doesn't have a
  // separate 'count' vs 'index'. Actually header has `current_index`. Let's
  // assume there is no separate `count`. Wait, we need to know how many valid
  // items are there for REDO. The struct in header: `stack`, `capacity`,
  // `current_index`. It implies we might just null out future items? Or we
  // simply say: If I push, I overwrite stack[current_index]. But what about
  // free? We typically valid items are up to `count`. Ah, header definition is
  // minimal. I should probably treat it as: Any pointers in
  // stack[current_index...capacity-1] are redo items if they are not NULL. BUT,
  // usually we need a separate `count` or `top`. Let's rely on NULL check or
  // just track a local 'count' if needed. Actually, simpler: destroy anything
  // from current_index upwards that is not NULL.

  for (int i = system->current_index; i < system->capacity; ++i) {
    if (system->stack[i]) {
      if (system->stack[i]->destroy) {
        system->stack[i]->destroy(system->stack[i]);
      }
      free(system->stack[i]);
      system->stack[i] = NULL;
    }
  }

  // 2. Execute
  if (cmd->execute) {
    cmd->execute(cmd);
  }

  // 3. Store in stack
  if (system->current_index < system->capacity) {
    Command *cmd_copy = (Command *)malloc(sizeof(Command));
    *cmd_copy = *cmd;
    system->stack[system->current_index] = cmd_copy;
    system->current_index++;
  } else {
    // Shift stack (discard oldest)
    if (system->stack[0] && system->stack[0]->destroy) {
      system->stack[0]->destroy(system->stack[0]);
    }
    free(system->stack[0]);

    for (int i = 0; i < system->capacity - 1; ++i) {
      system->stack[i] = system->stack[i + 1];
    }

    Command *cmd_copy = (Command *)malloc(sizeof(Command));
    *cmd_copy = *cmd;
    system->stack[system->capacity - 1] = cmd_copy;

    // current_index remains at capacity
    // Adjust saved_index if needed
    if (system->saved_index >= 0) {
      system->saved_index--;
    }
  }
}

void undo_system_undo(UndoRedoSystem *system) {
  if (!system || !undo_system_can_undo(system))
    return;

  system->current_index--;
  Command *cmd = system->stack[system->current_index];
  if (cmd && cmd->undo) {
    cmd->undo(cmd);
  }
}

void undo_system_redo(UndoRedoSystem *system) {
  if (!system || !undo_system_can_redo(system))
    return;

  Command *cmd = system->stack[system->current_index];
  if (cmd && cmd->execute) {
    cmd->execute(cmd);
  }
  system->current_index++;
}

bool undo_system_can_undo(UndoRedoSystem *system) {
  return system && system->current_index > 0;
}

bool undo_system_can_redo(UndoRedoSystem *system) {
  if (!system || system->current_index >= system->capacity)
    return false;
  // Check if next slot has a command
  return system->stack[system->current_index] != NULL;
}

// -------------------------------------------------------------------------------------------------
// TRANSACTIONS
// -------------------------------------------------------------------------------------------------

void undo_system_begin_transaction(UndoRedoSystem *system) {
  if (!system)
    return;
  if (system->in_transaction) {
    printf("UndoSystem Warning: Nested transactions not fully supported, "
           "merging.\n");
    return;
  }
  system->in_transaction = true;
  system->transaction_count = 0;
}

void undo_system_end_transaction(UndoRedoSystem *system) {
  if (!system || !system->in_transaction)
    return;

  system->in_transaction = false;

  if (system->transaction_count == 0)
    return;

  // Create a Composite Command
  CompositeCommandData *data =
      (CompositeCommandData *)malloc(sizeof(CompositeCommandData));
  data->count = system->transaction_count;
  data->commands = (Command **)malloc(sizeof(Command *) * data->count);

  // Move commands from buffer to composite data
  for (int i = 0; i < data->count; ++i) {
    data->commands[i] = system->transaction_buffer[i];
  }

  // Create wrapper command
  Command composite = {0};
  composite.execute = composite_execute;
  composite.undo = composite_undo;
  composite.destroy = composite_destroy;
  composite.data = data;
  composite.description =
      strdup("Transaction"); // Could be improved to take name

  // Push the composite command to the main stack
  // NOTE: logic is tricky because internal commands ALREADY executed.
  // 'undo_system_execute' calls execute. We don't want to execute twice.
  // We need a version of push that doesn't execute.

  // Let's Inline the push logic here but skip execution

  // 1. Clear redo
  for (int i = system->current_index; i < system->capacity; ++i) {
    if (system->stack[i]) {
      if (system->stack[i]->destroy) {
        system->stack[i]->destroy(system->stack[i]);
      }
      free(system->stack[i]);
      system->stack[i] = NULL;
    }
  }

  // 2. Push without execute
  if (system->current_index < system->capacity) {
    Command *cmd_copy = (Command *)malloc(sizeof(Command));
    *cmd_copy = composite;
    system->stack[system->current_index] = cmd_copy;
    system->current_index++;
  } else {
    // Shift
    if (system->stack[0] && system->stack[0]->destroy) {
      system->stack[0]->destroy(system->stack[0]);
    }
    free(system->stack[0]);

    for (int i = 0; i < system->capacity - 1; ++i) {
      system->stack[i] = system->stack[i + 1];
    }

    Command *cmd_copy = (Command *)malloc(sizeof(Command));
    *cmd_copy = composite;
    system->stack[system->capacity - 1] = cmd_copy;

    if (system->saved_index >= 0)
      system->saved_index--;
  }

  system->transaction_count = 0;
}

// -------------------------------------------------------------------------------------------------
// STATE
// -------------------------------------------------------------------------------------------------

bool undo_system_is_dirty(UndoRedoSystem *system) {
  if (!system)
    return false;
  return system->current_index != system->saved_index;
}

void undo_system_mark_saved(UndoRedoSystem *system) {
  if (!system)
    return;
  system->saved_index = system->current_index;
}
