#ifndef FLUID_SYSTEM_H
#define FLUID_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>

void fluid_sys_init(void);
void fluid_sys_shutdown(void);
void fluid_sys_update(float delta_time);

uint64_t fluid_sys_create_emitter(float x, float y, float z, float emit_rate);
void fluid_sys_destroy_emitter(uint64_t emitter_id);

void fluid_sys_set_viscosity(uint64_t emitter_id, float viscosity);
float fluid_sys_get_viscosity(uint64_t emitter_id);

void fluid_sys_set_particle_count(uint64_t emitter_id, uint32_t count);
uint32_t fluid_sys_get_particle_count(uint64_t emitter_id);

void fluid_sys_enable_splashing(uint64_t emitter_id, bool enabled);
bool fluid_sys_is_splashing_enabled(uint64_t emitter_id);

void fluid_sys_set_enabled(bool enabled);
bool fluid_sys_is_enabled(void);

#endif // FLUID_SYSTEM_H
