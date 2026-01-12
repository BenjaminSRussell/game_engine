#include "node_editor.h"
#include <core/logger.h>
#include <editor/ui/canvas/ui_canvas.h>
#include <math/math.h>
#include <scripting/visual_scripting.h>
#include <stdlib.h>

// Mock state for the editor
typedef struct {
  VSGraph *graph;
  bool is_open;
  float pan_x, pan_y;
  float zoom;

  // Drag/Drop state
  VSNodeID dragging_node;
  bool is_dragging;
  float drag_start_x;
  float drag_start_y;
} NodeEditorState;

static NodeEditorState g_node_editor = {0};

void NodeEditor_Init(void) {
  g_node_editor.graph = vs_graph_create();
  g_node_editor.is_open = true;
  g_node_editor.zoom = 1.0f;
  g_node_editor.dragging_node = 0xFFFFFFFF; // Invalid ID

  // Add some test nodes
  vs_graph_add_node(g_node_editor.graph, "Event: Start", VS_NODE_ENTRY);
  VSNodeID print_node =
      vs_graph_add_node(g_node_editor.graph, "Print String", VS_NODE_PRINT);

  // Position nodes manually for now (since we don't have layout yet)
  // We need to access the node data. Since VSNode is opaque or struct, let's
  // assume we can modify it. Wait, VSNode definition is in 'visual_scripting.h'
  // or internal? Based on previous file read, it was internal to the
  // 'visual_scripting.c' logic or shared header? Let's check visual_scripting.h
  // later. For now, we just initialize.

  LOG_INFO("Node Editor Initialized");
}

void NodeEditor_Shutdown(void) {
  if (g_node_editor.graph) {
    vs_graph_destroy(g_node_editor.graph);
    g_node_editor.graph = NULL;
  }
}

void NodeEditor_Update(float delta_time) {
  // Handle animations or logic updates
}

static void draw_node(VSNode *node, float x, float y) {
  float node_w = 150.0f;
  float node_h = 100.0f;

  // Node Body
  ui_canvas_draw_rect(x, y, node_w, node_h, 0.2f, 0.2f, 0.2f, 1.0f);

  // Node Header
  ui_canvas_draw_rect(x, y, node_w, 24.0f, 0.3f, 0.4f, 0.8f, 1.0f);

  // Pins (simplified)
  for (uint32_t i = 0; i < node->input_count; ++i) {
    ui_canvas_draw_rect(x - 8, y + 30 + i * 20, 16, 16, 0.5f, 0.8f, 0.5f, 1.0f);
  }

  for (uint32_t i = 0; i < node->output_count; ++i) {
    ui_canvas_draw_rect(x + node_w - 8, y + 30 + i * 20, 16, 16, 0.8f, 0.5f,
                        0.5f, 1.0f);
  }
}

void NodeEditor_Render(void) {
  if (!g_node_editor.is_open || !g_node_editor.graph)
    return;

  ui_canvas_begin();

  // Draw Background Grid (Placeholder)
  ui_canvas_draw_rect(0, 0, 1920, 1080, 0.1f, 0.1f, 0.1f, 1.0f);

  // Draw Nodes
  // Simple layout for demo purpose: stagger them
  for (uint32_t i = 0; i < g_node_editor.graph->node_count; ++i) {
    VSNode *node = &g_node_editor.graph->nodes[i];

    // We really need position data in VSNode.
    // For now, calculate position based on ID to prove rendering works.
    float nx = 100.0f + (i * 200.0f) + g_node_editor.pan_x;
    float ny = 100.0f + (i * 50.0f) + g_node_editor.pan_y;

    draw_node(node, nx, ny);
  }

  ui_canvas_end();
}

void NodeEditor_OnMouseDown(float x, float y, int button) {
  // Simple hit test for drag (very basic)
  g_node_editor.drag_start_x = x;
  g_node_editor.drag_start_y = y;
  g_node_editor.is_dragging = true;
}

void NodeEditor_OnMouseUp(float x, float y, int button) {
  g_node_editor.is_dragging = false;
}

void NodeEditor_OnMouseMove(float x, float y) {
  if (g_node_editor.is_dragging) {
    float dx = x - g_node_editor.drag_start_x;
    float dy = y - g_node_editor.drag_start_y;

    g_node_editor.pan_x += dx;
    g_node_editor.pan_y += dy;

    g_node_editor.drag_start_x = x;
    g_node_editor.drag_start_y = y;
  }
}
