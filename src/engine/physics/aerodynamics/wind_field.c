#include "physics/aerodynamics/wind_field.h"

void wind_field_init() {}

void wind_field_get_velocity(float position[3], float *velocity_out) {
    velocity_out[0] = 5.0f;
    velocity_out[1] = 0.0f;
    velocity_out[2] = 0.0f;
}

void wind_field_set_direction(float direction[3]) {}
