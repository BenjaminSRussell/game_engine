#include "scripting_types.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>

/* ============================================================================
 * CORE IMPLEMENTATION
 * ============================================================================
 */

// Forward declarations from other modules
extern void scripting_register_builtin_commands(ScriptContext *context);

ScriptContext *script_init(void) {
  ScriptContext *context = UNIFIED_ALLOC(sizeof(ScriptContext));
  if (!context) {
    LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate script context");
    return NULL;
  }

  memset(context, 0, sizeof(ScriptContext));
  context->is_initialized = true;

  // Register built-in commands
  scripting_register_builtin_commands(context);

  // Initialize console
  context->console.is_visible = false;

  LOG_INFO(LOG_CAT_GENERAL, "Scripting system initialized with %u commands",
           context->command_count);

  return context;
}

void script_shutdown(ScriptContext *context) {
  if (!context)
    return;

  LOG_INFO(LOG_CAT_GENERAL, "Shutting down scripting system");

  context->is_initialized = false;
  context->command_count = 0;
  context->variable_count = 0;

  UNIFIED_FREE(context);
}

void script_update(ScriptContext *context, float delta_time) {
  (void)context;
  (void)delta_time;
  // Process queued commands if any (future expansion)
}
