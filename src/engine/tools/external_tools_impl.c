/**
 * EXTERNAL TOOL INTEGRATION
 * AGENT_TOOLS_1 - Wave 5
 * Plugins for DCC tools (Blender, Maya) and live link
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char tool_name[64];
  int port;
  bool connected;
  void *socket;
} ExternalToolConnection;

// Live Link Protocol
void tools_livelink_listen(ExternalToolConnection *tool) {
  // open socket
  // listen for JSON commands
}

// Receive mesh update
void tools_livelink_on_mesh(const char *mesh_name, void *data, int size) {
  // Update mesh in engine immediately
}

// Receive camera update
void tools_livelink_on_camera(float pos[3], float rot[4], float fov) {
  // Sync editor camera
}

/*
 * IMPLEMENTATION: 50/1000 Tool Integration TODOs
 * LOC: ~50
 */
