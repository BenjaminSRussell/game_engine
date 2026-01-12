#ifndef DEBUG_DRAW_H
#define DEBUG_DRAW_H

#include <stdbool.h>

// Simple vector/color types
typedef struct { float x, y, z; } DebugVec3;
typedef struct { float r, g, b, a; } DebugColor;

void DebugDraw_Init(void);
void DebugDraw_Shutdown(void);

// Clear all debug primitives (call at start of frame)
void DebugDraw_Clear(void);

// Render all queued primitives (call at end of frame)
void DebugDraw_Render(void);

// Primitives
void DebugDraw_Line(DebugVec3 start, DebugVec3 end, DebugColor color);
void DebugDraw_Box(DebugVec3 center, DebugVec3 extent, DebugColor color);
void DebugDraw_Sphere(DebugVec3 center, float radius, DebugColor color);
void DebugDraw_Grid(int size, float spacing, DebugColor color);

// Toggle visibility
void DebugDraw_SetVisible(bool visible);
bool DebugDraw_IsVisible(void);

#endif // DEBUG_DRAW_H
