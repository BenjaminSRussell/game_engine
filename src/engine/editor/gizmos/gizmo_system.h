#ifndef GIZMO_SYSTEM_H
#define GIZMO_SYSTEM_H

#include <stdbool.h>

// Simple vector types for standalone compilation if engine math is not linked
typedef struct { float x, y, z; } GizmoVec3;

typedef enum {
    GIZMO_MODE_NONE,
    GIZMO_MODE_TRANSLATE,
    GIZMO_MODE_ROTATE,
    GIZMO_MODE_SCALE
} GizmoMode;

typedef enum {
    GIZMO_AXIS_NONE,
    GIZMO_AXIS_X,
    GIZMO_AXIS_Y,
    GIZMO_AXIS_Z,
    GIZMO_AXIS_XY, // Planes for translation/scaling
    GIZMO_AXIS_XZ,
    GIZMO_AXIS_YZ,
    GIZMO_AXIS_VIEW // Screen space rotate
} GizmoAxis;

void Gizmo_Init(void);
void Gizmo_Shutdown(void);

// Set the active mode (usually via UI toolbar)
void Gizmo_SetMode(GizmoMode mode);
GizmoMode Gizmo_GetMode(void);

// Start interacting with a gizmo (usually on mouse down)
// Returns true if an axis was selected
bool Gizmo_BeginInteraction(GizmoVec3 ray_origin, GizmoVec3 ray_dir);

// Update interaction (mouse drag)
// Returns the delta transformation
GizmoVec3 Gizmo_UpdateInteraction(GizmoVec3 ray_origin, GizmoVec3 ray_dir);

// End interaction (mouse up)
void Gizmo_EndInteraction(void);

// Render the gizmo at a specific position
void Gizmo_Render(GizmoVec3 position, float scale);

#endif // GIZMO_SYSTEM_H
