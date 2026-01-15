#ifndef SCRIPTING_TYPES_H
#define SCRIPTING_TYPES_H

#include "../Public/unified_scripting.h"

/* ============================================================================
 * INTERNAL TYPES - NOT EXPOSED TO PUBLIC API
 * ============================================================================
 */

#define MAX_COMMANDS 128
#define MAX_SCRIPT_VARS 256
#define MAX_CONSOLE_HISTORY 50
#define MAX_CONSOLE_BUFFER 1024
#define MAX_VS_NODES 256
#define MAX_VS_LINKS 512
#define MAX_VS_PINS_PER_NODE 8

/* ============================================================================
 * COMMAND REGISTRY
 * ============================================================================
 */

typedef struct {
  char name[64];
  ScriptCommandCallback callback;
  char description[128];
} CommandEntry;

/* ============================================================================
 * VARIABLE STORAGE
 * ============================================================================
 */

typedef struct {
  char name[64];
  float value;
  float min_val;
  float max_val;
} ScriptVar;

/* ============================================================================
 * CONSOLE SYSTEM
 * ============================================================================
 */

typedef struct {
  char text[MAX_CONSOLE_BUFFER];
  bool is_error;
} ConsoleLogEntry;

typedef struct {
  ConsoleLogEntry history[MAX_CONSOLE_HISTORY];
  uint32_t log_count;
  uint32_t log_head;
  bool is_visible;
  char input_buffer[MAX_CONSOLE_BUFFER];
} ConsoleState;

/* ============================================================================
 * VISUAL SCRIPTING
 * ============================================================================
 */

typedef struct {
  VSPinID id;
  VSPinType type;
  VSPinDirection direction;
  VSNodeID node_id;
  bool is_connected;
} VSPin;

typedef struct {
  VSNodeID id;
  VSNodeType type_id;
  char title[64];
  VSPin inputs[MAX_VS_PINS_PER_NODE];
  VSPin outputs[MAX_VS_PINS_PER_NODE];
  uint32_t input_count;
  uint32_t output_count;
} VSNode;

typedef struct {
  VSPinID from_pin;
  VSPinID to_pin;
} VSLink;

struct VSGraph {
  VSNode *nodes;
  uint32_t node_count;
  uint32_t node_capacity;

  VSLink *links;
  uint32_t link_count;
  uint32_t link_capacity;
};

/* ============================================================================
 * MAIN CONTEXT
 * ============================================================================
 */

struct ScriptContext {
  // Command system
  CommandEntry commands[MAX_COMMANDS];
  uint32_t command_count;

  // Variable system
  ScriptVar variables[MAX_SCRIPT_VARS];
  uint32_t variable_count;

  // Console state
  ConsoleState console;

  // Initialization flags
  bool is_initialized;
};

#endif // SCRIPTING_TYPES_H
