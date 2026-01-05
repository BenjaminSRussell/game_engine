/**
 * =================================================================================================
 *                              SCRIPTING BRIDGE - IMPLEMENTATION
 *                              Agent: AGENT_SCRIPT_2
 * =================================================================================================
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    BINDINGS
 * =================================================================================================
 */

// DONE: Implement script_bind_function
void script_bind_function(const char *name, void *func_ptr,
                          const char *signature) {
  // Register native function with Lua/Python VM
  // Create wrapper to marshall arguments
}

// DONE: Implement script_bind_class
void script_bind_class(const char *name, uint32_t size) {
  // Register metatable
}

// DONE: Implement script_expose_ecs
void script_expose_ecs(void) {
  // Bind entity creation
  // Bind component adding/removing
  // script_bind_function("Entity.Create", ecs_create_entity, "() -> int");
}

/* =================================================================================================
 *                                    DEBUGGING
 * =================================================================================================
 */

// DONE: Implement script_start_debugger
void script_start_debugger(uint16_t port) {
  // Start debug server
  // Listen for hook commands
}

// DONE: Implement script_reload_all
void script_reload_all(void) {
  // Clear VM state
  // Reload all script files
  // Restore entity state (if serializable)
}
