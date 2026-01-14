#include "platform/input/input_system.h"
#include <string.h>

static int g_keys[512] = {0};
static float g_mouse_x = 0.0f;
static float g_mouse_y = 0.0f;

void input_init() {
    memset(g_keys, 0, sizeof(g_keys));
}

void input_update() {}

int input_is_key_down(int key) {
    return g_keys[key];
}

int input_is_key_pressed(int key) {
    return g_keys[key];
}

void input_get_mouse_position(float *x, float *y) {
    *x = g_mouse_x;
    *y = g_mouse_y;
}

void input_set_mouse_position(float x, float y) {
    g_mouse_x = x;
    g_mouse_y = y;
}

void input_show_cursor(int show) {}

void input_lock_cursor(int lock) {}
