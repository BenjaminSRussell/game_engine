#ifndef VFX_OPTIMIZER_H
#define VFX_OPTIMIZER_H

#include <stdbool.h>
#include <stdint.h>

void vfx_sys_init(void);
void vfx_sys_update(float delta_time);

void vfx_sys_set_culling_enabled(bool enabled);
bool vfx_sys_is_culling_enabled(void);

void vfx_sys_set_culling_distance(float distance);
float vfx_sys_get_culling_distance(void);

void vfx_sys_set_lod_bias(float bias);
float vfx_sys_get_lod_bias(void);

void vfx_sys_set_instancing_enabled(bool enabled);
bool vfx_sys_is_instancing_enabled(void);

void vfx_sys_set_max_particles(uint32_t count);
uint32_t vfx_sys_get_max_particles(void);

uint32_t vfx_sys_get_active_particles(void);
uint32_t vfx_sys_get_culled_count(void); // Extra metric for UI

#endif // VFX_OPTIMIZER_H
