#include "debug_draw.h"
#include <stdio.h>

static bool g_DebugVisible = true;
static int g_LineCount = 0;
static int g_BoxCount = 0;
static int g_SphereCount = 0;

void DebugDraw_Init(void) {
    printf("[DebugDraw] Initialized\n");
}

void DebugDraw_Shutdown(void) {
    printf("[DebugDraw] Shutdown\n");
}

void DebugDraw_Clear(void) {
    g_LineCount = 0;
    g_BoxCount = 0;
    g_SphereCount = 0;
}

void DebugDraw_Render(void) {
    if (!g_DebugVisible) return;

    if (g_LineCount > 0 || g_BoxCount > 0 || g_SphereCount > 0) {
        printf("[DebugDraw] Rendering: %d lines, %d boxes, %d spheres\n",
               g_LineCount, g_BoxCount, g_SphereCount);
    }
}

void DebugDraw_Line(DebugVec3 start, DebugVec3 end, DebugColor color) {
    if (!g_DebugVisible) return;
    g_LineCount++;
    // Store line data for rendering...
}

void DebugDraw_Box(DebugVec3 center, DebugVec3 extent, DebugColor color) {
    if (!g_DebugVisible) return;
    g_BoxCount++;
    // Store box data...
}

void DebugDraw_Sphere(DebugVec3 center, float radius, DebugColor color) {
    if (!g_DebugVisible) return;
    g_SphereCount++;
    // Store sphere data...
}

void DebugDraw_Grid(int size, float spacing, DebugColor color) {
    if (!g_DebugVisible) return;
    // A grid is made of many lines
    for (int i = -size; i <= size; ++i) {
        g_LineCount += 2; // one horizontal, one vertical line roughly
    }
}

void DebugDraw_SetVisible(bool visible) {
    g_DebugVisible = visible;
    printf("[DebugDraw] Visible: %s\n", visible ? "true" : "false");
}

bool DebugDraw_IsVisible(void) {
    return g_DebugVisible;
}
