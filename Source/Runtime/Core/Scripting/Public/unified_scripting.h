#ifndef UNIFIED_SCRIPTING_H
#define UNIFIED_SCRIPTING_H

#include "core/types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * SCRIPTING SYSTEM - PUBLIC API
 * ============================================================================
 */

// Forward declarations (opaque types)
typedef struct ScriptContext ScriptContext;
typedef struct VSGraph VSGraph;

/* ============================================================================
 * COMMAND SYSTEM
 * ============================================================================
 */

/**
 * Command callback function signature
 * @param argc Number of arguments
 * @param argv Array of argument strings
 */
typedef void (*ScriptCommandCallback)(int argc, char **argv);

/**
 * Initialize the scripting system
 * @return Script context on success, NULL on failure
 */
ScriptContext *script_init(void);

/**
 * Shutdown the scripting system
 * @param context Script context to shutdown
 */
void script_shutdown(ScriptContext *context);

/**
 * Update the scripting system (call each frame)
 * @param context Script context
 * @param delta_time Time since last update in seconds
 */
void script_update(ScriptContext *context, float delta_time);

/**
 * Register a command
 * @param context Script context
 * @param name Command name
 * @param callback Command callback function
 * @param description Command description for help
 * @return true on success, false on failure
 */
bool script_register_command(ScriptContext *context, const char *name,
                             ScriptCommandCallback callback,
                             const char *description);

/**
 * Execute a script string
 * @param context Script context
 * @param script Script string to execute
 * @return true on success, false on failure
 */
bool script_execute(ScriptContext *context, const char *script);

/* ============================================================================
 * VARIABLE SYSTEM
 * ============================================================================
 */

/**
 * Get a script variable value
 * @param context Script context
 * @param name Variable name
 * @param default_value Default value if variable doesn't exist
 * @return Variable value or default
 */
float script_get_var(ScriptContext *context, const char *name,
                     float default_value);

/**
 * Set a script variable value
 * @param context Script context
 * @param name Variable name
 * @param value Value to set
 */
void script_set_var(ScriptContext *context, const char *name, float value);

/* ============================================================================
 * CONSOLE SYSTEM
 * ============================================================================
 */

/**
 * Initialize the console system
 * @param context Script context
 */
void console_init(ScriptContext *context);

/**
 * Shutdown the console system
 * @param context Script context
 */
void console_shutdown(ScriptContext *context);

/**
 * Update console (call each frame)
 * @param context Script context
 */
void console_update(ScriptContext *context);

/**
 * Render console (requires ImGui)
 * @param context Script context
 */
void console_render(ScriptContext *context);

/**
 * Print message to console
 * @param context Script context
 * @param text Message text
 */
void console_print(ScriptContext *context, const char *text);

/**
 * Print error to console
 * @param context Script context
 * @param text Error text
 */
void console_print_error(ScriptContext *context, const char *text);

/**
 * Clear console output
 * @param context Script context
 */
void console_clear(ScriptContext *context);

/**
 * Toggle console visibility
 * @param context Script context
 */
void console_toggle(ScriptContext *context);

/**
 * Check if console is visible
 * @param context Script context
 * @return true if visible, false otherwise
 */
bool console_is_visible(ScriptContext *context);

/* ============================================================================
 * VISUAL SCRIPTING SYSTEM
 * ============================================================================
 */

// Pin types
typedef enum {
  VS_PIN_FLOW,
  VS_PIN_INT,
  VS_PIN_FLOAT,
  VS_PIN_STRING,
  VS_PIN_BOOL,
  VS_PIN_OBJECT
} VSPinType;

// Pin direction
typedef enum { VS_PIN_INPUT, VS_PIN_OUTPUT } VSPinDirection;

// Node types
typedef enum {
  VS_NODE_ENTRY,
  VS_NODE_PRINT,
  VS_NODE_BRANCH,
  VS_NODE_ADD,
  VS_NODE_MULTIPLY,
  VS_NODE_GET_VAR,
  VS_NODE_SET_VAR
} VSNodeType;

typedef uint32_t VSNodeID;
typedef uint32_t VSPinID;

/**
 * Create a visual scripting graph
 * @return VSGraph on success, NULL on failure
 */
VSGraph *vs_graph_create(void);

/**
 * Destroy a visual scripting graph
 * @param graph Graph to destroy
 */
void vs_graph_destroy(VSGraph *graph);

/**
 * Add a node to the graph
 * @param graph Visual scripting graph
 * @param title Node title
 * @param type Node type
 * @return Node ID
 */
VSNodeID vs_graph_add_node(VSGraph *graph, const char *title, VSNodeType type);

/**
 * Connect two pins in the graph
 * @param graph Visual scripting graph
 * @param from_pin Output pin ID
 * @param to_pin Input pin ID
 */
void vs_graph_connect(VSGraph *graph, VSPinID from_pin, VSPinID to_pin);

/**
 * Execute a visual scripting graph
 * @param graph Visual scripting graph
 * @param entry_node_id Starting node ID
 */
void vs_graph_execute(VSGraph *graph, VSNodeID entry_node_id);

#ifdef __cplusplus
}
#endif

#endif // UNIFIED_SCRIPTING_H
