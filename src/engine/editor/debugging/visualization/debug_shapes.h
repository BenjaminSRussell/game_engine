/**
 * @file debug_shapes.h
 * @brief Debug shape rendering
 */

#ifndef RENDER_DEBUG_SHAPES_H
#define RENDER_DEBUG_SHAPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "include/math/vec3.h"
#include "include/math/vec4.h"
#include "include/math/aabb.h"
#include "include/math/mat4.h"

/* Type definitions */
typedef struct debug_shape_desc {
    vec4_t color;
    float duration;
    bool depth_test;
} debug_shape_desc_t;

/* Function declarations */
void debug_shapes_init(void);
void debug_shapes_shutdown(void);

void debug_draw_line(vec3_t start, vec3_t end, vec4_t color);
void debug_draw_box(vec3_t center, vec3_t extents, vec4_t color);
void debug_draw_aabb(aabb_t aabb, vec4_t color);
void debug_draw_sphere(vec3_t center, float radius, vec4_t color);
void debug_draw_cone(vec3_t base, vec3_t dir, float radius, float height, vec4_t color);
void debug_draw_axes(mat4_t transform, float size);

void debug_shapes_clear(void);
void debug_shapes_update(float delta_time);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_DEBUG_SHAPES_H */
