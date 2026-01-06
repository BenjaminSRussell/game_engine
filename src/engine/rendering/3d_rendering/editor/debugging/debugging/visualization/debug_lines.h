#ifndef RENDERING_DEBUG_LINES_H
#define RENDERING_DEBUG_LINES_H

#include "../../math/vec3.h"
#include "../../math/vec4.h"

void debug_line_init(void);
void debug_line_draw(vec3_t start, vec3_t end, vec4_t color);
void debug_line_draw_ray(vec3_t start, vec3_t dir, float length, vec4_t color);
void debug_line_draw_axes(mat4_t transform, float size);
void debug_line_clear(void);

#endif // RENDERING_DEBUG_LINES_H
