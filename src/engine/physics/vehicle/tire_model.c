#include "physics/vehicle/tire_model.h"

void tire_model_init() {}

void tire_calculate_forces(void *tire, float slip_angle, float slip_ratio,
                            float *lateral_force, float *longitudinal_force) {
    // Pacejka Magic Formula (simplified)
    *lateral_force = slip_angle * 1000.0f;
    *longitudinal_force = slip_ratio * 2000.0f;
}

void tire_set_load(void *tire, float normal_load) {}

void tire_update_temperature(void *tire, float dt) {}
