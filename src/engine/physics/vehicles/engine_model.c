#include "physics/vehicles/engine_model.h"

void engine_model_init() {}

void engine_calculate_torque(float rpm, float throttle, float *torque) {
    *torque = throttle * 300.0f; // Simplified
}

void engine_update(void *engine, float dt) {}
