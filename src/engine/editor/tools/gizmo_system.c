// #include "gizmo_system.h"

/**
 * =================================================================================================
 *                                   EDITOR GIZMO SYSTEM - COMPLETE
 * =================================================================================================
 */

// SELECTION & INTERACTION
// TASK_1100: Implement "Ray-Gizmo Intersection" (AABB or Capsule check)
// TASK_1101: Track "Selected Axis" (X, Y, Z, XY, YZ, ZX, or Center)
// TASK_1102: Handle Mouse Drag state and delta calculation
// TASK_1103: Support "Multi-Selection" gizmo (centered between items)

// TRANSFORM TYPES
// TASK_1110: Implement Translation Gizmo (Arrows)
// TASK_1111: Implement Rotation Gizmo (Arcs/Circles)
// TASK_1112: Implement Scale Gizmo (Boxes/Uniform handle)
// TASK_1113: Add "Screen Space" move/rotate (view-aligned)

// COORDINATE SPACES
// TASK_1120: Implement "World Space" mode
// TASK_1121: Implement "Local Space" mode (gizmo rotates with object)
// TASK_1122: Implement "View Space" mode
// TASK_1123: Add "Surface Snapping" (stick object to ground on move)

// SNAPPING & GRID
// TASK_1130: Implement Grid Snapping (Move in units of 0.1, 1, 10)
// TASK_1131: Implement Angle Snapping (Rotate in increments of 5, 15, 45 deg)
// TASK_1132: Add "Vertex Snapping" (press V to snap to mesh vertices)

// RENDERING
// TASK_1140: Draw Gizmo Mesh (Arrow, Ring, Box) with Unlit color
// TASK_1141: Highlight specific axis on hover
// TASK_1142: Draw "Ghost" transform during drag
// TASK_1143: Ensure Gizmo is rendered on top of everything (No depth test or
// Offset)

// UX & FEEDBACK
// TASK_1150: Implement "Hotkeys" (W: Move, E: Rotate, R: Scale)
// TASK_1151: Add "Undo/Redo" entries on drag end
// TASK_1152: Visualizer: Draw "Numerical Delta" over gizmo during drag
// TASK_1153: Handle Camera movement while dragging (Auto-pan)

// ADVANCED TOOLS
// TASK_1160: Implement "Spline Node" gizmo
// TASK_1161: Implement "Physics Shape" gizmo (resize box/sphere colliders)
// TASK_1162: Add "Terrain Brush" gizmo integration
