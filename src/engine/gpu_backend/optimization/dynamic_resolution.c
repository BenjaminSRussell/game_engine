#include "renderer/optimization/dynamic_resolution.h"

static float g_current_scale = 1.0f;
static float g_target_fps = 60.0f;

void dynamic_resolution_init() {
    g_current_scale = 1.0f;
}

void dynamic_resolution_update(float current_fps) {
    if (current_fps < g_target_fps - 5.0f) {
        g_current_scale *= 0.95f; // Reduce resolution
    } else if (current_fps > g_target_fps + 5.0f) {
        g_current_scale *= 1.05f; // Increase resolution
    }
    
    if (g_current_scale < 0.5f) g_current_scale = 0.5f;
    if (g_current_scale > 1.0f) g_current_scale = 1.0f;
}

float dynamic_resolution_get_scale() {
    return g_current_scale;
}

void dynamic_resolution_set_target_fps(float fps) {
    g_target_fps = fps;
}
