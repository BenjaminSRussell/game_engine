#include "editor/debugging/visualization/debug_lines.h"

#define MAX_DEBUG_LINES 5000
typedef struct debug_line {
    vec3_t start;
    vec3_t end;
    vec4_t color;
} debug_line_t;

static debug_line_t g_debug_lines[MAX_DEBUG_LINES];
static u32 g_debug_line_count = 0;

void debug_line_init(void) {
    g_debug_line_count = 0;
}

void debug_line_draw(vec3_t start, vec3_t end, vec4_t color) {
    if (g_debug_line_count >= MAX_DEBUG_LINES) return;
    g_debug_lines[g_debug_line_count++] = (debug_line_t){ start, end, color };
}

void debug_line_draw_ray(vec3_t start, vec3_t dir, float length, vec4_t color) {
    vec3_t end = vec3_add(start, vec3_scale(vec3_normalize(dir), length));
    debug_line_draw(start, end, color);
}

void debug_line_draw_axes(mat4_t transform, float size) {
    vec3_t pos = { transform.e[3][0], transform.e[3][1], transform.e[3][2] };
    vec3_t x = { transform.e[0][0], transform.e[0][1], transform.e[0][2] };
    vec3_t y = { transform.e[1][0], transform.e[1][1], transform.e[1][2] };
    vec3_t z = { transform.e[2][0], transform.e[2][1], transform.e[2][2] };
    
    debug_line_draw(pos, vec3_add(pos, vec3_scale(x, size)), vec4_set(1, 0, 0, 1));
    debug_line_draw(pos, vec3_add(pos, vec3_scale(y, size)), vec4_set(0, 1, 0, 1));
    debug_line_draw(pos, vec3_add(pos, vec3_scale(z, size)), vec4_set(0, 0, 1, 1));
}

void debug_line_clear(void) {
    g_debug_line_count = 0;
}
