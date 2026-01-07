#include "physics/ballistics/trajectory_sim.h"
#include <include/math/math.h>

void trajectory_sim_init() {}

void trajectory_predict(float start[3], float velocity[3], float gravity, float *time_to_target, float end[3]) {
    // Ballistic trajectory prediction
    *time_to_target = 1.0f;
}
