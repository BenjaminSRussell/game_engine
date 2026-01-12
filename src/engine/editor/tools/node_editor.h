#ifndef NODE_EDITOR_H
#define NODE_EDITOR_H

#include <core/unified_engine.h>

void NodeEditor_Init(void);
void NodeEditor_Shutdown(void);
void NodeEditor_Update(float delta_time);
void NodeEditor_Render(void);

// Input handling (simplified)
void NodeEditor_OnMouseDown(float x, float y, int button);
void NodeEditor_OnMouseUp(float x, float y, int button);
void NodeEditor_OnMouseMove(float x, float y);

#endif
