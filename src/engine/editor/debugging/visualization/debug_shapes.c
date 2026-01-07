#include "editor/debugging/visualization/debug_shapes.h"
#include "editor/debugging/visualization/debug_lines.h"
#include <include/math/math.h>

void debug_shapes_init(void) {
    debug_line_init();
}

void debug_shapes_shutdown(void) {
    debug_line_clear();
}

void debug_draw_line(vec3_t start, vec3_t end, vec4_t color) {
    debug_line_draw(start, end, color);
}

void debug_draw_box(vec3_t center, vec3_t extents, vec4_t color) {
    aabb_t aabb = {
        vec3_sub(center, extents),
        vec3_add(center, extents)
    };
    debug_draw_aabb(aabb, color);
}

void debug_draw_aabb(aabb_t aabb, vec4_t color) {
    vec3_t v[8] = {
        {aabb.min.x, aabb.min.y, aabb.min.z}, {aabb.max.x, aabb.min.y, aabb.min.z},
        {aabb.max.x, aabb.max.y, aabb.min.z}, {aabb.min.x, aabb.max.y, aabb.min.z},
        {aabb.min.x, aabb.min.y, aabb.max.z}, {aabb.max.x, aabb.min.y, aabb.max.z},
        {aabb.max.x, aabb.max.y, aabb.max.z}, {aabb.min.x, aabb.max.y, aabb.max.z}
    };
    
    debug_draw_line(v[0], v[1], color); debug_draw_line(v[1], v[2], color);
    debug_draw_line(v[2], v[3], color); debug_draw_line(v[3], v[0], color);
    debug_draw_line(v[4], v[5], color); debug_draw_line(v[5], v[6], color);
    debug_draw_line(v[6], v[7], color); debug_draw_line(v[7], v[4], color);
    debug_draw_line(v[0], v[4], color); debug_draw_line(v[1], v[5], color);
    debug_draw_line(v[2], v[6], color); debug_draw_line(v[3], v[7], color);
}

void debug_draw_sphere(vec3_t center, float radius, vec4_t color) {
    const int segments = 16;
    for (int i = 0; i < segments; i++) {
        float a1 = (float)i / segments * 2.0f * PI;
        float a2 = (float)(i + 1) / segments * 2.0f * PI;
        
        debug_draw_line(
            (vec3_t){ center.x + cosf(a1) * radius, center.y + sinf(a1) * radius, center.z },
            (vec3_t){ center.x + cosf(a2) * radius, center.y + sinf(a2) * radius, center.z },
            color);
        debug_draw_line(
            (vec3_t){ center.x + cosf(a1) * radius, center.y, center.z + sinf(a1) * radius },
            (vec3_t){ center.x + cosf(a2) * radius, center.y, center.z + sinf(a2) * radius },
            color);
        debug_draw_line(
            (vec3_t){ center.x, center.y + cosf(a1) * radius, center.z + sinf(a1) * radius },
            (vec3_t){ center.x, center.y + cosf(a2) * radius, center.z + sinf(a2) * radius },
            color);
    }
}

void debug_draw_cone(vec3_t base, vec3_t dir, float radius, float height, vec4_t color) {
    vec3_t tip = vec3_add(base, vec3_scale(vec3_normalize(dir), height));
    vec3_t orthog = fabsf(dir.x) < 0.9f ? vec3_set(1, 0, 0) : vec3_set(0, 1, 0);
    vec3_t right = vec3_normalize(vec3_cross(dir, orthog));
    vec3_t up = vec3_normalize(vec3_cross(right, dir));
    
    const int segments = 8;
    for (int i = 0; i < segments; i++) {
        float a1 = (float)i / segments * 2.0f * PI;
        float a2 = (float)(i + 1) / segments * 2.0f * PI;
        
        vec3_t p1 = vec3_add(base, vec3_add(vec3_scale(right, cosf(a1) * radius), vec3_scale(up, sinf(a1) * radius)));
        vec3_t p2 = vec3_add(base, vec3_add(vec3_scale(right, cosf(a2) * radius), vec3_scale(up, sinf(a2) * radius)));
        
        debug_draw_line(p1, p2, color);
        debug_draw_line(p1, tip, color);
    }
}

void debug_draw_axes(mat4_t transform, float size) {
    vec3_t pos = { transform.e[3][0], transform.e[3][1], transform.e[3][2] };
    vec3_t x = { transform.e[0][0], transform.e[0][1], transform.e[0][2] };
    vec3_t y = { transform.e[1][0], transform.e[1][1], transform.e[1][2] };
    vec3_t z = { transform.e[2][0], transform.e[2][1], transform.e[2][2] };
    
    debug_draw_line(pos, vec3_add(pos, vec3_scale(x, size)), vec4_set(1, 0, 0, 1));
    debug_draw_line(pos, vec3_add(pos, vec3_scale(y, size)), vec4_set(0, 1, 0, 1));
    debug_draw_line(pos, vec3_add(pos, vec3_scale(z, size)), vec4_set(0, 0, 1, 1));
}

void debug_shapes_clear(void) {
    debug_line_clear();
}

void debug_shapes_update(float delta_time) {
    (void)delta_time;
}
