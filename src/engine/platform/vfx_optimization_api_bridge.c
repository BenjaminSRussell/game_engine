#include "platform/vfx_optimization_api_bridge.h"
#include "core/logger.h"
#include "renderer/systems/vfx_optimizer.h"

void vfx_opt_set_culling_enabled(bool enabled) {
  vfx_sys_set_culling_enabled(enabled);
}

bool vfx_opt_is_culling_enabled(void) { return vfx_sys_is_culling_enabled(); }

void vfx_opt_set_culling_distance(float distance) {
  vfx_sys_set_culling_distance(distance);
}

float vfx_opt_get_culling_distance(void) {
  return vfx_sys_get_culling_distance();
}

void vfx_opt_set_lod_bias(float bias) { vfx_sys_set_lod_bias(bias); }

float vfx_opt_get_lod_bias(void) { return vfx_sys_get_lod_bias(); }

void vfx_opt_set_instancing_enabled(bool enabled) {
  vfx_sys_set_instancing_enabled(enabled);
}

bool vfx_opt_is_instancing_enabled(void) {
  return vfx_sys_is_instancing_enabled();
}

void vfx_opt_set_max_particles(uint32_t count) {
  vfx_sys_set_max_particles(count);
}

uint32_t vfx_opt_get_max_particles(void) { return vfx_sys_get_max_particles(); }

uint32_t vfx_opt_get_active_particles(void) {
  return vfx_sys_get_active_particles();
}
