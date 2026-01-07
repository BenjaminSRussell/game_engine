#include "rendering/systems/vfx_optimizer.h"
#include "core/logger.h"

static struct {
  bool culling_enabled;
  float culling_distance;
  float lod_bias;
  bool instancing_enabled;
  uint32_t max_particles;

  // Stats
  uint32_t active_particles;
  uint32_t culled_count;
} vfx_context;

void vfx_sys_init(void) {
  vfx_context.culling_enabled = true;
  vfx_context.culling_distance = 100.0f;
  vfx_context.lod_bias = 0.0f;
  vfx_context.instancing_enabled = true;
  vfx_context.max_particles = 100000;
  vfx_context.active_particles = 0;
  vfx_context.culled_count = 0;
  LOG_INFO("VFX Optimization System Initialized");
}

void vfx_sys_update(float delta_time) {
  // In real engine: Update the underlying Particle System parameters
  // e.g., set GPU buffers, update simulation shader uniforms

  // Mock simulation of stats for UI
  // vfx_context.active_particles = particle_system_get_count();
}

void vfx_sys_set_culling_enabled(bool enabled) {
  vfx_context.culling_enabled = enabled;
}

bool vfx_sys_is_culling_enabled(void) { return vfx_context.culling_enabled; }

void vfx_sys_set_culling_distance(float distance) {
  vfx_context.culling_distance = distance;
}

float vfx_sys_get_culling_distance(void) {
  return vfx_context.culling_distance;
}

void vfx_sys_set_lod_bias(float bias) { vfx_context.lod_bias = bias; }

float vfx_sys_get_lod_bias(void) { return vfx_context.lod_bias; }

void vfx_sys_set_instancing_enabled(bool enabled) {
  vfx_context.instancing_enabled = enabled;
}

bool vfx_sys_is_instancing_enabled(void) {
  return vfx_context.instancing_enabled;
}

void vfx_sys_set_max_particles(uint32_t count) {
  vfx_context.max_particles = count;
}

uint32_t vfx_sys_get_max_particles(void) { return vfx_context.max_particles; }

uint32_t vfx_sys_get_active_particles(void) {
  return vfx_context.active_particles;
}

uint32_t vfx_sys_get_culled_count(void) { return vfx_context.culled_count; }
