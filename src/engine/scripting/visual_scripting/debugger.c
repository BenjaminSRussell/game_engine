#include "core/hashmap.h"
#include <scripting/visual_scripting/visual_scripting_core.h>

static HashMap *g_breakpoints = NULL; // <node_id, bool>

void vs_debugger_toggle_breakpoint(u32 node_id) {
  // Implement breakpoint storage
}

bool vs_debugger_should_break(u32 node_id) {
  // Check if breakpoint exists
  return false;
}

void vs_debugger_step(VSContext *ctx) {
  // Execute one node
}
