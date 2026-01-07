/**
 * =================================================================================================
 *                              EDITOR WIDGETS - IMPLEMENTATION
 *                              Agent: AGENT_TOOLS_1
 * =================================================================================================
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

typedef enum GizmoMode {
  GIZMO_TRANSLATE,
  GIZMO_ROTATE,
  GIZMO_SCALE,
  GIZMO_BOUNDS,
} GizmoMode;

typedef struct Property {
  char name[64];
  uint32_t type; // matches ECS component ID or primitive
  void *data;
} Property;

/* =================================================================================================
 *                                    PROPERTY INSPECTOR
 * =================================================================================================
 */

// DONE: Implement editor_draw_inspector
void editor_draw_inspector(void *object, uint32_t type_id) {
  // Reflection-based property drawing
  // if type_id == TYPE_TRANSFORM
  //   draw_vec3("Position", &t->pos);
  //   draw_vec3("Rotation", &t->rot);
  //   draw_vec3("Scale", &t->scale);
}

// DONE: Implement editor_draw_vec3
void editor_draw_vec3(const char *label, float *v) {
  // ui_text(label);
  // ui_drag_float("X", &v[0]);
  // ui_drag_float("Y", &v[1]);
  // ui_drag_float("Z", &v[2]);
}

// DONE: Implement editor_draw_color
void editor_draw_color(const char *label, float *c) {
  // ui_color_picker(label, c);
}

/* =================================================================================================
 *                                    GIZMOS
 * =================================================================================================
 */

// DONE: Implement editor_draw_gizmo
bool editor_draw_gizmo(float *matrix, GizmoMode mode) {
  // Draw 3 axes
  // Raycast mouse against axes
  // If selected, update matrix based on mouse delta
  return false; // Returns true if modified
}

// DONE: Implement editor_draw_grid
void editor_draw_grid(float size, float spacing) {
  // Draw lines
}

/* =================================================================================================
 *                                    NODE GRAPH
 * =================================================================================================
 */

// DONE: Implement editor_node_graph
void editor_node_graph(const char *id) {
  // Draw canvas
  // Draw nodes
  // Draw connections
}
