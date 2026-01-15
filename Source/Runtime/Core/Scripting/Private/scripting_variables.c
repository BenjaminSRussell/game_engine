#include "scripting_types.h"
#include <string.h>

/* ============================================================================
 * VARIABLE SYSTEM IMPLEMENTATION
 * ============================================================================
 */

float script_get_var(ScriptContext *context, const char *name,
                     float default_value) {
  if (!context || !name)
    return default_value;

  for (uint32_t i = 0; i < context->variable_count; i++) {
    if (strcmp(context->variables[i].name, name) == 0) {
      return context->variables[i].value;
    }
  }

  return default_value;
}

void script_set_var(ScriptContext *context, const char *name, float value) {
  if (!context || !name)
    return;

  // Check if variable exists
  for (uint32_t i = 0; i < context->variable_count; i++) {
    if (strcmp(context->variables[i].name, name) == 0) {
      context->variables[i].value = value;
      return;
    }
  }

  // Create new variable
  if (context->variable_count < MAX_SCRIPT_VARS) {
    ScriptVar *var = &context->variables[context->variable_count++];
    strncpy(var->name, name, sizeof(var->name) - 1);
    var->value = value;
    var->min_val = -1e9f;
    var->max_val = 1e9f;
  }
}
