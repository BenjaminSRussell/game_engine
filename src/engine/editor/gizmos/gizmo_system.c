#include "gizmo_system.h"
#include <stdio.h>
#include <math.h>

static GizmoMode g_CurrentMode = GIZMO_MODE_TRANSLATE;
static GizmoAxis g_SelectedAxis = GIZMO_AXIS_NONE;
static bool g_IsInteracting = false;

// Mock math functions
static GizmoVec3 Vec3_Add(GizmoVec3 a, GizmoVec3 b) { return (GizmoVec3){a.x + b.x, a.y + b.y, a.z + b.z}; }
static GizmoVec3 Vec3_Sub(GizmoVec3 a, GizmoVec3 b) { return (GizmoVec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
// static GizmoVec3 Vec3_Scale(GizmoVec3 a, float s) { return (GizmoVec3){a.x * s, a.y * s, a.z * s}; }

void Gizmo_Init(void) {
    printf("[Gizmo] Initialized\n");
    g_CurrentMode = GIZMO_MODE_TRANSLATE;
}

void Gizmo_Shutdown(void) {
    printf("[Gizmo] Shutdown\n");
}

void Gizmo_SetMode(GizmoMode mode) {
    g_CurrentMode = mode;
    printf("[Gizmo] Mode set to %d\n", mode);
}

GizmoMode Gizmo_GetMode(void) {
    return g_CurrentMode;
}

bool Gizmo_BeginInteraction(GizmoVec3 ray_origin, GizmoVec3 ray_dir) {
    // Simplified ray casting logic for demonstration
    // In a real implementation, we would check intersection with gizmo geometry (cylinders, cones)
    // Here we just pretend we hit the X axis if looking roughly along X

    if (fabsf(ray_dir.x) > fabsf(ray_dir.y) && fabsf(ray_dir.x) > fabsf(ray_dir.z)) {
        g_SelectedAxis = GIZMO_AXIS_X;
    } else if (fabsf(ray_dir.y) > fabsf(ray_dir.x) && fabsf(ray_dir.y) > fabsf(ray_dir.z)) {
        g_SelectedAxis = GIZMO_AXIS_Y;
    } else {
        g_SelectedAxis = GIZMO_AXIS_Z;
    }

    g_IsInteracting = true;
    printf("[Gizmo] Interaction started on axis %d\n", g_SelectedAxis);
    return true;
}

GizmoVec3 Gizmo_UpdateInteraction(GizmoVec3 ray_origin, GizmoVec3 ray_dir) {
    if (!g_IsInteracting) return (GizmoVec3){0,0,0};

    // Calculate delta based on mouse movement relative to axis
    // This is highly simplified
    GizmoVec3 delta = {0,0,0};

    switch (g_SelectedAxis) {
        case GIZMO_AXIS_X: delta.x = 0.1f; break;
        case GIZMO_AXIS_Y: delta.y = 0.1f; break;
        case GIZMO_AXIS_Z: delta.z = 0.1f; break;
        default: break;
    }

    return delta;
}

void Gizmo_EndInteraction(void) {
    g_IsInteracting = false;
    g_SelectedAxis = GIZMO_AXIS_NONE;
    printf("[Gizmo] Interaction ended\n");
}

void Gizmo_Render(GizmoVec3 position, float scale) {
    // In a real engine, this would push lines/meshes to the debug renderer
    // printf("[Gizmo] Rendering at (%.2f, %.2f, %.2f) scale %.2f\n", position.x, position.y, position.z, scale);
}
