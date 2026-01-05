#include "physics/aerodynamics/lift_drag.h"
#include <math.h>

void aerodynamics_init() {}

void aerodynamics_calculate_forces(float velocity[3], float angle_of_attack, 
                                    float *lift, float *drag) {
    // Calculate aerodynamic forces
    *lift = velocity[1] * angle_of_attack * 0.5f;
    *drag = velocity[0] * velocity[0] * 0.1f;
}
