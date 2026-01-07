#include "platform/fluid_vfx_api_bridge.h"
#include "core/logger.h"
#include "physics/fluids/fluid_system.h"

// Delegates to the actual Fluid System

uint64_t fluid_create_emitter(float x, float y, float z, float emit_rate) {
  return fluid_sys_create_emitter(x, y, z, emit_rate);
}

void fluid_destroy_emitter(uint64_t emitter_id) {
  fluid_sys_destroy_emitter(emitter_id);
}

void fluid_set_viscosity(uint64_t emitter_id, float viscosity) {
  fluid_sys_set_viscosity(emitter_id, viscosity);
}

float fluid_get_viscosity(uint64_t emitter_id) {
  return fluid_sys_get_viscosity(emitter_id);
}

void fluid_set_particle_count(uint64_t emitter_id, uint32_t count) {
  fluid_sys_set_particle_count(emitter_id, count);
}

uint32_t fluid_get_particle_count(uint64_t emitter_id) {
  return fluid_sys_get_particle_count(emitter_id);
}

void fluid_enable_splashing(uint64_t emitter_id, bool enabled) {
  fluid_sys_enable_splashing(emitter_id, enabled);
}

bool fluid_is_splashing_enabled(uint64_t emitter_id) {
  return fluid_sys_is_splashing_enabled(emitter_id);
}

void fluid_set_enabled(bool enabled) { fluid_sys_set_enabled(enabled); }

bool fluid_is_enabled(void) { return fluid_sys_is_enabled(); }
