#include "editor/viewport_tools/gizmos.h"

void gizmos_init() {}

void gizmos_render_translate(void *selection, void *camera) {}

void gizmos_render_rotate(void *selection, void *camera) {}

void gizmos_render_scale(void *selection, void *camera) {}

int gizmos_pick(float mouse_x, float mouse_y) {
    // Return which gizmo axis was clicked
    return -1;
}
