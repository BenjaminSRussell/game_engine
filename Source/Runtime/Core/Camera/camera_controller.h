#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include <common.h>

void camera_controller_init(void);
void camera_controller_update(f32 delta_time);
void camera_set_fov_effect(f32 target_fov);

// Camera collision helper
Vec3 camera_check_collision(Vec3 desired_position, f32 camera_radius);

#endif
