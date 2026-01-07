#include "vr/vr_system.h"

void vr_init() {}

int vr_is_available() {
    return 0;
}

void vr_get_hmd_pose(float position[3], float rotation[4]) {}

void vr_get_controller_pose(int hand, float position[3], float rotation[4]) {}

int vr_is_button_pressed(int hand, int button) {
    return 0;
}

void vr_trigger_haptic(int hand, float intensity) {}

void vr_submit_frame(void *left_eye, void *right_eye) {}
