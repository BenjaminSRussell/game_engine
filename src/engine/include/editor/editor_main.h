#ifndef EDITOR_MAIN_H
#define EDITOR_MAIN_H

#include <core/types.h>

typedef struct Engine Engine;

void editor_init(Engine *engine);
void editor_shutdown(Engine *engine);
void editor_update(Engine *engine, f32 delta_time);
void editor_render(Engine *engine);

void editor_set_mode(bool enabled);

#endif // EDITOR_MAIN_H
