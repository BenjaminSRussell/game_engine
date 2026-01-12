#ifndef PROPERTY_EDITOR_H
#define PROPERTY_EDITOR_H

#include <core/types.h>
#include <ecs/ecs.h>

void PropertyEditor_Init(void);
void PropertyEditor_Update(void);
void PropertyEditor_Render(void);

// Inspect an entity
void PropertyEditor_SetSelection(Entity entity);
void PropertyEditor_ClearSelection(void);

#endif // PROPERTY_EDITOR_H
